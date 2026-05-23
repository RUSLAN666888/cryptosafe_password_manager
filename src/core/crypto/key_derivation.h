#ifndef KEY_DERIVATION_H
#define KEY_DERIVATION_H

#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <authentication.h>


// Выводит ключ из пароля и соли используя PBKDF2-HMAC-SHA256
// НОВАЯ ВЕРСИЯ: принимает указатель на пароль
inline void derive_encryption_key(const char* password, size_t password_len,
                                  const std::vector<uint8_t>& salt,
                                  std::vector<uint8_t>& key)
{
    const uint64_t ITERATIONS = 100000;
    const size_t SALT_LEN = 16;
    const size_t KEY_LEN = 32;

    if (salt.size() != SALT_LEN) {
        throw std::runtime_error("Salt must be exactly 16 bytes");
    }

    key.resize(KEY_LEN);

    int result = PKCS5_PBKDF2_HMAC(
        password, static_cast<int>(password_len),
        salt.data(), static_cast<int>(salt.size()),
        static_cast<int>(ITERATIONS), EVP_sha256(),
        static_cast<int>(key.size()), key.data()
        );

    if (result != 1) {
        throw std::runtime_error("Key derivation failed");
    }
}

// Overload для совместимости (deprecated)
inline void derive_encryption_key(const std::string& password,
                                  const std::vector<uint8_t>& salt,
                                  std::vector<uint8_t>& key)
{
    derive_encryption_key(password.c_str(), password.length(), salt, key);
}

// НОВАЯ ВЕРСИЯ: derive_private_sign_key с указателем
inline void derive_private_sign_key(const char* password, size_t password_len,
                                    const std::vector<uint8_t>& salt,
                                    const std::string& info,
                                    std::vector<uint8_t>& key)
{
    const int key_length = 32;
    key.resize(key_length);

    EVP_KDF* kdf = EVP_KDF_fetch(NULL, "HKDF", NULL);
    if (kdf == NULL) {
        secure_zero(key.data(), key.size());
        throw std::runtime_error("Failed to fetch HKDF");
    }

    EVP_KDF_CTX* kctx = EVP_KDF_CTX_new(kdf);
    EVP_KDF_free(kdf);

    if (kctx == NULL) {
        secure_zero(key.data(), key.size());
        throw std::runtime_error("Failed to create KDF context");
    }

    OSSL_PARAM params[6];
    OSSL_PARAM* p = params;

    *p++ = OSSL_PARAM_construct_utf8_string("digest",
                                            const_cast<char*>("SHA256"), 6);
    *p++ = OSSL_PARAM_construct_octet_string("salt",
                                             const_cast<uint8_t*>(salt.data()),
                                             salt.size());
    *p++ = OSSL_PARAM_construct_octet_string("key",
                                             const_cast<char*>(password),
                                             password_len);
    *p++ = OSSL_PARAM_construct_octet_string("info",
                                             const_cast<char*>(info.c_str()),
                                             info.size());
    *p = OSSL_PARAM_construct_end();

    if (EVP_KDF_CTX_set_params(kctx, params) <= 0) {
        EVP_KDF_CTX_free(kctx);
        secure_zero(key.data(), key.size());
        throw std::runtime_error("Failed to set KDF parameters");
    }

    if (EVP_KDF_derive(kctx, key.data(), key.size(), NULL) <= 0) {
        EVP_KDF_CTX_free(kctx);
        secure_zero(key.data(), key.size());
        throw std::runtime_error("Failed to derive key");
    }

    EVP_KDF_CTX_free(kctx);
}

// Overload для совместимости (deprecated)
inline void derive_private_sign_key(const std::string& password,
                                    std::vector<uint8_t>& key,
                                    const std::vector<uint8_t>& salt =
                                    {0x43, 0x72, 0x79, 0x70, 0x74, 0x6f, 0x53, 0x61, 0x66, 0x65, 0x5f, 0x41, 0x75, 0x64, 0x69, 0x74},
                                    const std::string& info = "audit-signing")
{
    derive_private_sign_key(password.c_str(), password.length(), salt, info, key);
}

#endif
