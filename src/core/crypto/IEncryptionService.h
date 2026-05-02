#ifndef ENCRYPTION_SERVICE_H
#define ENCRYPTION_SERVICE_H

#include <cstdint>
#include <vector>
#include "key_manager.h"


class IEncryptionService
{
public:
  virtual std::vector<uint8_t> encrypt(const KeyManager::KeyData& key, const std::vector<uint8_t>& plaintext) = 0;

  virtual std::vector<uint8_t> decrypt(const KeyManager::KeyData& key, const std::vector<uint8_t>& ciphertext) = 0;
};


#endif
