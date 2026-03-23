#ifndef AES256_PLACEHOLDER_H
#define AES256_PLACEHOLDER_H

#include "abstract.h"
#include <cstring>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>
#include <../src/core/key_manager.h>


class AES256Placeholder : public EncryptionService
{
  static constexpr size_t KEY_SIZE = 32; // 256 бит

public:
  std::vector<uint8_t> encrypt(const std::vector<uint8_t>& data,
                               const KeyManager::KeyData& key) override
  {
      std::vector<uint8_t> result(data.size());
      for (size_t i = 0; i < data.size(); ++i)
      {
          result[i] = data[i] ^ key.data[i % key.size];
      }
      return result;
  }

  std::vector<uint8_t> decrypt(const std::vector<uint8_t>& ciphertext,
                               const KeyManager::KeyData& key) override
  {
      return encrypt(ciphertext, key);
  }
};


#endif
