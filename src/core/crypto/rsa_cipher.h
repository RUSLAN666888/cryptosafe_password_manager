// rsa_cipher.h
#ifndef RSA_CIPHER_H
#define RSA_CIPHER_H

#include <vector>
#include <cstdint>
#include <stdexcept>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/err.h>

class RSACipher {
public:
    // Структура для хранения ключевой пары
    struct KeyPair {
        std::vector<uint8_t> privateKey;  // PEM формат
        std::vector<uint8_t> publicKey;   // PEM формат
    };

    // Генерация пары ключей RSA-2048
    static KeyPair generateKeyPair(int bits = 2048) {
        KeyPair result;

        EVP_PKEY* pkey = EVP_PKEY_new();
        EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);

        if (!ctx) {
            throw std::runtime_error("Failed to create RSA context");
        }

        if (EVP_PKEY_keygen_init(ctx) != 1) {
            EVP_PKEY_CTX_free(ctx);
            throw std::runtime_error("Failed to init key generation");
        }

        if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, bits) != 1) {
            EVP_PKEY_CTX_free(ctx);
            throw std::runtime_error("Failed to set RSA bits");
        }

        if (EVP_PKEY_keygen(ctx, &pkey) != 1) {
            EVP_PKEY_CTX_free(ctx);
            throw std::runtime_error("Failed to generate RSA key pair");
        }

        EVP_PKEY_CTX_free(ctx);

        // Извлекаем приватный ключ в PEM формате
        BIO* bioPrivate = BIO_new(BIO_s_mem());
        if (!bioPrivate) {
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to create BIO for private key");
        }

        if (PEM_write_bio_PrivateKey(bioPrivate, pkey, nullptr, nullptr, 0, nullptr, nullptr) != 1) {
            BIO_free(bioPrivate);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to write private key");
        }

        const char* privData;
        long privLen = BIO_get_mem_data(bioPrivate, &privData);
        result.privateKey.assign(reinterpret_cast<const uint8_t*>(privData),
                                 reinterpret_cast<const uint8_t*>(privData) + privLen);
        BIO_free(bioPrivate);

        // Извлекаем публичный ключ в PEM формате
        BIO* bioPublic = BIO_new(BIO_s_mem());
        if (!bioPublic) {
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to create BIO for public key");
        }

        if (PEM_write_bio_PUBKEY(bioPublic, pkey) != 1) {
            BIO_free(bioPublic);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to write public key");
        }

        const char* pubData;
        long pubLen = BIO_get_mem_data(bioPublic, &pubData);
        result.publicKey.assign(reinterpret_cast<const uint8_t*>(pubData),
                                reinterpret_cast<const uint8_t*>(pubData) + pubLen);
        BIO_free(bioPublic);

        EVP_PKEY_free(pkey);

        return result;
    }

    // Шифрование данных публичным ключом (PEM формат)
    static std::vector<uint8_t> encrypt(const std::vector<uint8_t>& data,
                                        const std::vector<uint8_t>& publicKeyPEM) {
        // Загружаем публичный ключ из PEM
        BIO* bio = BIO_new_mem_buf(publicKeyPEM.data(), static_cast<int>(publicKeyPEM.size()));
        if (!bio) {
            throw std::runtime_error("Failed to create BIO for public key");
        }

        EVP_PKEY* pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
        BIO_free(bio);

        if (!pkey) {
            throw std::runtime_error("Failed to load public key");
        }

        EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
        if (!ctx) {
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to create encryption context");
        }

        if (EVP_PKEY_encrypt_init(ctx) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to init encryption");
        }

        // Устанавливаем OAEP padding (более безопасный, чем PKCS#1 v1.5)
        if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to set RSA padding");
        }

        // Получаем размер зашифрованных данных
        size_t outlen = 0;
        if (EVP_PKEY_encrypt(ctx, nullptr, &outlen, data.data(), data.size()) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to get encrypted size");
        }

        // Шифруем
        std::vector<uint8_t> encrypted(outlen);
        if (EVP_PKEY_encrypt(ctx, encrypted.data(), &outlen, data.data(), data.size()) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to encrypt data");
        }

        encrypted.resize(outlen);

        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);

        return encrypted;
    }

    // Расшифровка данных приватным ключом (PEM формат)
    static std::vector<uint8_t> decrypt(const std::vector<uint8_t>& encryptedData,
                                        const std::vector<uint8_t>& privateKeyPEM) {
        // Загружаем приватный ключ из PEM
        BIO* bio = BIO_new_mem_buf(privateKeyPEM.data(), static_cast<int>(privateKeyPEM.size()));
        if (!bio) {
            throw std::runtime_error("Failed to create BIO for private key");
        }

        EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
        BIO_free(bio);

        if (!pkey) {
            throw std::runtime_error("Failed to load private key");
        }

        EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
        if (!ctx) {
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to create decryption context");
        }

        if (EVP_PKEY_decrypt_init(ctx) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to init decryption");
        }

        // Устанавливаем OAEP padding (должен совпадать с тем, что использовался при шифровании)
        if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to set RSA padding");
        }

        // Получаем размер расшифрованных данных
        size_t outlen = 0;
        if (EVP_PKEY_decrypt(ctx, nullptr, &outlen, encryptedData.data(), encryptedData.size()) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to get decrypted size");
        }

        // Расшифровываем
        std::vector<uint8_t> decrypted(outlen);
        if (EVP_PKEY_decrypt(ctx, decrypted.data(), &outlen, encryptedData.data(), encryptedData.size()) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to decrypt data");
        }

        decrypted.resize(outlen);

        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);

        return decrypted;
    }

    // Получение публичного ключа из приватного (PEM -> PEM)
    static std::vector<uint8_t> extractPublicKey(const std::vector<uint8_t>& privateKeyPEM) {
        BIO* bio = BIO_new_mem_buf(privateKeyPEM.data(), static_cast<int>(privateKeyPEM.size()));
        if (!bio) {
            throw std::runtime_error("Failed to create BIO for private key");
        }

        EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
        BIO_free(bio);

        if (!pkey) {
            throw std::runtime_error("Failed to load private key");
        }

        BIO* bioPublic = BIO_new(BIO_s_mem());
        if (!bioPublic) {
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to create BIO for public key");
        }

        if (PEM_write_bio_PUBKEY(bioPublic, pkey) != 1) {
            BIO_free(bioPublic);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to write public key");
        }

        const char* pubData;
        long pubLen = BIO_get_mem_data(bioPublic, &pubData);
        std::vector<uint8_t> publicKey(reinterpret_cast<const uint8_t*>(pubData),
                                       reinterpret_cast<const uint8_t*>(pubData) + pubLen);

        BIO_free(bioPublic);
        EVP_PKEY_free(pkey);

        return publicKey;
    }

    // Получение размера ключа в битах
    static int getKeySize(const std::vector<uint8_t>& publicKeyPEM) {
        BIO* bio = BIO_new_mem_buf(publicKeyPEM.data(), static_cast<int>(publicKeyPEM.size()));
        if (!bio) {
            return 0;
        }

        EVP_PKEY* pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
        BIO_free(bio);

        if (!pkey) {
            return 0;
        }

        int bits = EVP_PKEY_bits(pkey);
        EVP_PKEY_free(pkey);

        return bits;
    }
};



#endif
