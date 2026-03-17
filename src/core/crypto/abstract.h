#ifndef ENCRYPTION_SERVICE_H
#define ENCRYPTION_SERVICE_H

#include <cstdint>
#include <vector>

namespace Crypto
{
class EncryptionService
{
public:
  virtual std::vector<uint8_t> encrypt(const std::vector<uint8_t> &data,
                                       const std::vector<uint8_t> &key) = 0;

  virtual std::vector<uint8_t> decrypt(const std::vector<uint8_t> &ciphertext,
                                       const std::vector<uint8_t> &key) = 0;
};
} // namespace Crypto

#endif