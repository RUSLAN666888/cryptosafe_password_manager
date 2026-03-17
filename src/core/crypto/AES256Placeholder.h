#ifndef AES256_PLACEHOLDER_H
#define AES256_PLACEHOLDER_H

#include "abstract.h"
#include <cstring>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace Crypto
{
class AES256Placeholder : public EncryptionService
{
  static constexpr size_t KEY_SIZE = 32; // 256 бит

public:
  std::vector<uint8_t> encrypt(const std::vector<uint8_t> &data,
                               const std::vector<uint8_t> &key) override
  {
    // XOR с циклическим ключом
    std::vector<uint8_t> result(data.size());
    for (size_t i = 0; i < data.size(); ++i)
    {
      result[i] = data[i] ^ key[i % key.size()];
    }

    return result;
  }
  // Для XOR шифрование и дешифрование одинаковы
  std::vector<uint8_t> decrypt(const std::vector<uint8_t> &ciphertext,
                               const std::vector<uint8_t> &key) override
  {
    return encrypt(ciphertext, key); // XOR симметричен
  }
};
} // namespace Crypto

#endif