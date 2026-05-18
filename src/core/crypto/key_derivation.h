#ifndef KEY_DERIVATION_H
#define KEY_DERIVATION_H

#include <cstdint>
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/params.h>
#include <stdexcept>
#include <string>
#include <vector>

// Выводит ключ из пароля и соли используя PBKDF2-HMAC-SHA256
inline void derive_encryption_key(const std::string &password,
                      const std::vector<uint8_t> &salt,
                      std::vector<uint8_t> &key) // выходной параметр
{
  const uint64_t ITERATIONS = 100000;
  const size_t SALT_LEN = 16;
  const size_t KEY_LEN = 32;

  // Проверка длины соли
  if (salt.size() != SALT_LEN)
  {
    throw std::runtime_error("Salt must be exactly 16 bytes");
  }

  // Изменяем размер выходного вектора
  key.resize(KEY_LEN);

  // PBKDF2-HMAC-SHA256 через OpenSSL
  int result = PKCS5_PBKDF2_HMAC(
      password.c_str(), static_cast<int>(password.length()), salt.data(),
      static_cast<int>(salt.size()), static_cast<int>(ITERATIONS), EVP_sha256(),
      static_cast<int>(key.size()), // используем size() вектора
      key.data()                    // используем data() вектора
  );

  if (result != 1)
  {
    throw std::runtime_error("Key derivation failed");
  }
}

inline void derive_encryption_key_600000(const std::string &password,
                                  const std::vector<uint8_t> &salt,
                                  std::vector<uint8_t> &key) // выходной параметр
{
    const uint64_t ITERATIONS = 600000;
    const size_t SALT_LEN = 16;
    const size_t KEY_LEN = 32;

    // Проверка длины соли
    if (salt.size() != SALT_LEN)
    {
        throw std::runtime_error("Salt must be exactly 16 bytes");
    }

    // Изменяем размер выходного вектора
    key.resize(KEY_LEN);

    // PBKDF2-HMAC-SHA256 через OpenSSL
    int result = PKCS5_PBKDF2_HMAC(
        password.c_str(), static_cast<int>(password.length()), salt.data(),
        static_cast<int>(salt.size()), static_cast<int>(ITERATIONS), EVP_sha256(),
        static_cast<int>(key.size()), // используем size() вектора
        key.data()                    // используем data() вектора
        );

    if (result != 1)
    {
        throw std::runtime_error("Key derivation failed");
    }
}

inline void derive_private_sign_key(const std::string &password, std::vector<uint8_t> &key, std::vector<uint8_t> salt, const std::string info) {

    const int key_length = 32;
    key.resize(key_length);

    // Загружаем алгоритм HKDF
    EVP_KDF *kdf = EVP_KDF_fetch(NULL, "HKDF", NULL);
    if (kdf == NULL) {
        std::fill(key.begin(), key.end(), 0);
        return;
    }

    // Создаём контекст
    EVP_KDF_CTX *kctx = EVP_KDF_CTX_new(kdf);
    EVP_KDF_free(kdf); // kctx сохраняет ссылку

    if (kctx == NULL) {
        std::fill(key.begin(), key.end(), 0);
        return;
    }

    // Параметры для HKDF
    OSSL_PARAM params[6];
    OSSL_PARAM *p = params;

    // Алгоритм хеширования
    *p++ = OSSL_PARAM_construct_utf8_string("digest",
                                            const_cast<char*>("SHA256"),
                                            (size_t)6);

    // Соль
    *p++ = OSSL_PARAM_construct_octet_string("salt",
                                             const_cast<uint8_t*>(salt.data()),
                                             salt.size());

    // мастер-пароль
    *p++ = OSSL_PARAM_construct_octet_string("key",
                                             const_cast<char*>(password.c_str()),
                                             password.size());

    // Информация (контекст)
    *p++ = OSSL_PARAM_construct_octet_string("info",
                                             const_cast<char*>(info.c_str()),
                                             info.size());

    *p = OSSL_PARAM_construct_end();

    // Устанавливаем параметры
    if (EVP_KDF_CTX_set_params(kctx, params) <= 0) {
        EVP_KDF_CTX_free(kctx);
        std::fill(key.begin(), key.end(), 0);
        return;
    }

    // Выполняем derivation
    if (EVP_KDF_derive(kctx, key.data(), key.size(), NULL) <= 0) {
        EVP_KDF_CTX_free(kctx);
        std::fill(key.begin(), key.end(), 0);
        return;
    }

    // Очищаем контекст
    EVP_KDF_CTX_free(kctx);
}

#endif
