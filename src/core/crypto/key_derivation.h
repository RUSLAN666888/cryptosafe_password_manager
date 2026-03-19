#ifndef KEY_DERIVATION_H
#define KEY_DERIVATION_H

#include <cstdint>
#include <openssl/evp.h>
#include <stdexcept>
#include <string>
#include <vector>

// Выводит ключ из пароля и соли используя PBKDF2-HMAC-SHA256
inline void
derive_encryption_key(const std::string &password,
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

#endif