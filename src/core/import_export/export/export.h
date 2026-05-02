#ifndef EXPORT_H
#define EXPORT_H

#include "../src/core/vault/plaintext_entry.h"

class Exporter{
public:
    enum class EncryptionStrength {
        AES_128,
        AES_256
    };

    void exportToEncryptedJSON(const std::vector<PlaintextEntry>& entries, const std::string& filepath,
                               const std::string& password,
                                EncryptionStrength strength = EncryptionStrength::AES_256);
};

#endif // EXPORT_H
