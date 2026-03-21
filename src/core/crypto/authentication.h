#ifndef AUTHENTICATION_H
#define AUTHENTICATION_H

#include <argon2.h>
#include <iostream>
#include <sodium.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <zxcvbn.h>

struct Argon2Data
{
  std::vector<uint8_t> salt;
  std::vector<uint8_t> hash;
  uint32_t time_cost;
  uint32_t memory_cost_mb;
  uint32_t parallelism;
  uint32_t hash_len;

  Argon2Data() = default;

  Argon2Data(uint32_t t, uint32_t m, uint32_t p, uint32_t len)
      : time_cost(t), memory_cost_mb(m), parallelism(p), hash_len(len)
  {
  }
};

inline void hash_password(const std::string &password, Argon2Data &data)
{
  if (sodium_init() < 0)
  {
    throw std::runtime_error("Failed to initialize libsodium");
  }

  // Генерируем соль
  data.salt.resize(crypto_pwhash_SALTBYTES);
  randombytes_buf(data.salt.data(), data.salt.size());

  // Переводим MiB в KiB
  uint64_t memory_kib = static_cast<uint64_t>(data.memory_cost_mb) * 1024;

  // Буфер для хеша
  data.hash.resize(data.hash_len);

  // Вызываем argon2
  int rc = argon2id_hash_raw(data.time_cost, memory_kib, data.parallelism,
                        password.c_str(), password.length(), data.salt.data(),
                        data.salt.size(), data.hash.data(), data.hash.size());

  if (rc != ARGON2_OK)
  {
    throw std::runtime_error(argon2_error_message(rc));
  }
}

// Constant-time сравнение двух векторов
inline bool constant_time_compare(const std::vector<uint8_t> &a,
                                  const std::vector<uint8_t> &b)
{
  if (a.size() != b.size())
  {
    return false;
  }

  // XOR и проверка
  uint8_t result = 0;
  for (size_t i = 0; i < a.size(); i++)
  {
    result |= a[i] ^ b[i];
  }

  // Если result == 0, значит все байты совпали
  return result == 0;
}

// Функция верификации пароля
inline bool verify_password(const std::string &password, const Argon2Data &data)
{
  // Переводим MiB в KiB
  uint64_t memory_kib = static_cast<uint64_t>(data.memory_cost_mb) * 1024;

  // Буфер для вычисленного хеша
  std::vector<uint8_t> computed_hash(data.hash_len);

  // Вычисляем хеш из введенного пароля
  int rc = argon2id_hash_raw(data.time_cost, memory_kib, data.parallelism,
                             password.c_str(), password.length(),
                             data.salt.data(), data.salt.size(),
                             computed_hash.data(), computed_hash.size());

  if (rc != ARGON2_OK)
  {
    return false; // Ошибка вычисления
  }

  // Constant-time сравнение
  return constant_time_compare(computed_hash, data.hash);
}

inline int check_password_strength(const std::string &password)
{
  ZxcMatch_t *info = NULL;
  double entropy = ZxcvbnMatch(password.c_str(), NULL, &info);

  if (entropy < 20)
    return 0;
  if (entropy < 40)
    return 1;
  if (entropy < 60)
    return 2;
  if (entropy < 80)
    return 3;
  if (entropy >= 80)
    return 4;

  return 4;
}

#endif
