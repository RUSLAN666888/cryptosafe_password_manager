#ifndef AES_GCM_H
#define AES_GCM_H

#include <cstddef>
#include <stdexcept>
#include <vector>
#include <openssl/rand.h>
#include "IEncryptionService.h"
#include "key_manager.h"

template<size_t KeyBits>
class AESGCM : public IEncryptionService{
    static_assert(KeyBits == 128 || KeyBits == 256);

    static constexpr size_t KEY_SIZE = KeyBits / 8;
    static constexpr size_t NONCE_LEN = 12;
    static constexpr size_t TAG_LEN = 16;

public:
    std::vector<uint8_t> encrypt(const KeyData& key, const std::vector<uint8_t>& plaintext) override {
        // Проверка размера ключа
        if (key.size != KEY_SIZE) {
            throw std::runtime_error("Invalid key size");
        }

        // Генерация nonce
        std::vector<uint8_t> nonce(NONCE_LEN);
        if (RAND_bytes(nonce.data(), NONCE_LEN) != 1) {
            throw std::runtime_error("Failed to generate nonce");
        }

        // Создание контекста
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            throw std::runtime_error("Failed to create cipher context");
        }

        // Выбор алгоритма
        const EVP_CIPHER* cipher = (KeyBits == 256) ? EVP_aes_256_gcm() : EVP_aes_128_gcm();

        std::vector<uint8_t> ciphertext(plaintext.size());
        std::vector<uint8_t> tag(TAG_LEN);

        try {
            // Инициализация
            if (EVP_EncryptInit_ex(ctx, cipher, nullptr, nullptr, nullptr) != 1) {
                throw std::runtime_error("Failed to init encryption");
            }

            // Установка ключа и nonce
            if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data, nonce.data()) != 1) {
                throw std::runtime_error("Failed to set key and IV");
            }

            // Шифрование
            int len = 0;
            if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len,
                                  plaintext.data(), static_cast<int>(plaintext.size())) != 1) {
                throw std::runtime_error("Failed to encrypt");
            }

            // Финализация
            int final_len = 0;
            if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &final_len) != 1) {
                throw std::runtime_error("Failed to finalize");
            }
            ciphertext.resize(len + final_len);

            // Получение тега
            if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_LEN, tag.data()) != 1) {
                throw std::runtime_error("Failed to get tag");
            }

        } catch (...) {
            EVP_CIPHER_CTX_free(ctx);
            throw;
        }

        EVP_CIPHER_CTX_free(ctx);

        // Формируем результат: nonce + ciphertext + tag
        std::vector<uint8_t> result;
        result.reserve(nonce.size() + ciphertext.size() + tag.size());
        result.insert(result.end(), nonce.begin(), nonce.end());
        result.insert(result.end(), ciphertext.begin(), ciphertext.end());
        result.insert(result.end(), tag.begin(), tag.end());

        return result;
    }

    std::vector<uint8_t> decrypt(const KeyData& key, const std::vector<uint8_t>& ciphertext_package) override {
        if (key.size != KEY_SIZE) {
            throw std::runtime_error("Invalid key size");
        }

        if (ciphertext_package.size() < NONCE_LEN + TAG_LEN) {
            throw std::runtime_error("Package too small");
        }

        // Разделяем
        std::vector<uint8_t> nonce(ciphertext_package.begin(),
                                   ciphertext_package.begin() + NONCE_LEN);
        std::vector<uint8_t> tag(ciphertext_package.end() - TAG_LEN,
                                 ciphertext_package.end());
        std::vector<uint8_t> encrypted_data(ciphertext_package.begin() + NONCE_LEN,
                                            ciphertext_package.end() - TAG_LEN);

        // Создание контекста
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            throw std::runtime_error("Failed to create cipher context");
        }

        const EVP_CIPHER* cipher = (KeyBits == 256) ? EVP_aes_256_gcm() : EVP_aes_128_gcm();

        std::vector<uint8_t> plaintext(encrypted_data.size());

        try {
            if (EVP_DecryptInit_ex(ctx, cipher, nullptr, nullptr, nullptr) != 1) {
                throw std::runtime_error("Failed to init decryption");
            }

            if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data, nonce.data()) != 1) {
                throw std::runtime_error("Failed to set key and IV");
            }

            int out_len = 0;
            if (EVP_DecryptUpdate(ctx, plaintext.data(), &out_len,
                                  encrypted_data.data(),
                                  static_cast<int>(encrypted_data.size())) != 1) {
                throw std::runtime_error("Failed to decrypt");
            }

            if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_LEN, tag.data()) != 1) {
                throw std::runtime_error("Failed to set tag");
            }

            int final_len = 0;
            if (EVP_DecryptFinal_ex(ctx, plaintext.data() + out_len, &final_len) != 1) {
                throw std::runtime_error("Authentication failed - wrong tag");
            }

            plaintext.resize(out_len + final_len);

        } catch (...) {
            EVP_CIPHER_CTX_free(ctx);
            throw;
        }

        EVP_CIPHER_CTX_free(ctx);

        return plaintext;
    }
};

#endif // AES_GCM_H
