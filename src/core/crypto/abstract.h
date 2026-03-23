#ifndef ENCRYPTION_SERVICE_H
#define ENCRYPTION_SERVICE_H

#include <cstdint>
#include <../src/core/key_manager.h>
#include <vector>


class EncryptionService
{
public:
  virtual std::vector<uint8_t> encrypt(const std::vector<uint8_t> &data,
                                       const KeyManager::KeyData& key) = 0;

  virtual std::vector<uint8_t> decrypt(const std::vector<uint8_t> &ciphertext,
                                       const KeyManager::KeyData& key) = 0;
};


#endif
