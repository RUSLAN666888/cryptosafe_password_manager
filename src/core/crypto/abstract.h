#ifndef ENCRYPTION_SERVICE_H
#define ENCRYPTION_SERVICE_H

#include <cstdint>
#include <../src/core/key_manager.h>
#include "../src/core/vault/plaintext_entry.h"
#include <vector>


class EncryptionService
{
public:
  virtual std::vector<uint8_t> encrypt(const KeyManager::KeyData& key, const PlaintextEntry& entry) = 0;

  virtual PlaintextEntry decrypt(const std::vector<uint8_t> &ciphertext, const KeyManager::KeyData& key) = 0;
};


#endif
