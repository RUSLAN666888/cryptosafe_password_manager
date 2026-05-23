#ifndef AUTHENTICATION_H
#define AUTHENTICATION_H

#include <argon2.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <zxcvbn.h>
#include <openssl/rand.h>

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

inline void secure_zero(void* ptr, size_t len)
{
    if (ptr && len) {
        volatile char* p = static_cast<volatile char*>(ptr);
        for (size_t i = 0; i < len; ++i) {
            p[i] = 0;
        }
    }
}

inline void hash_password(const char* password, size_t password_len, Argon2Data& data)
{
    data.salt.resize(16);
    if (RAND_bytes(data.salt.data(), static_cast<int>(data.salt.size())) != 1)
    {
        throw std::runtime_error("Failed to generate random salt with OpenSSL");
    }

    uint64_t memory_kib = static_cast<uint64_t>(data.memory_cost_mb) * 1024;
    data.hash.resize(data.hash_len);

    int rc = argon2id_hash_raw(data.time_cost, memory_kib, data.parallelism,
                               password, password_len,
                               data.salt.data(), data.salt.size(),
                               data.hash.data(), data.hash.size());

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
inline bool verify_password(const char* password, size_t password_len, const Argon2Data& data)
{
    uint64_t memory_kib = static_cast<uint64_t>(data.memory_cost_mb) * 1024;
    std::vector<uint8_t> computed_hash(data.hash_len);

    int rc = argon2id_hash_raw(data.time_cost, memory_kib, data.parallelism,
                               password, password_len,
                               data.salt.data(), data.salt.size(),
                               computed_hash.data(), computed_hash.size());

    if (rc != ARGON2_OK)
    {
        return false;
    }

    return constant_time_compare(computed_hash, data.hash);
}

inline int check_password_strength(const char* password, size_t password_len)
{
    ZxcMatch_t* info = NULL;
    double entropy = ZxcvbnMatch(password, NULL, &info);

    // Безопасное освобождение памяти zxcvbn
    if (info) {
        ZxcvbnFreeInfo(info);
    }

    if (entropy < 20) return 0;
    if (entropy < 40) return 1;
    if (entropy < 60) return 2;
    if (entropy < 80) return 3;
    return 4;
}

#endif
