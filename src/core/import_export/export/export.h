#ifndef EXPORT_H
#define EXPORT_H

#include "../src/core/vault/plaintext_entry.h"

class Exporter{
public:
    std::string escapeCSV(const std::string& field);

public:
    enum class EncryptionStrength {
        AES_128,
        AES_256
    };

    void exportToEncryptedJSON(std::vector<PlaintextEntry>& entries, const std::string& filepath,
                               const std::string& password,
                                EncryptionStrength strength = EncryptionStrength::AES_256);

    void exportToCSV(const std::vector<PlaintextEntry>& entries,
                     const std::string& filepath,
                     bool encrypt = false,
                     const std::string& password = "");


    void exportToBitwardenEncryptedJSON(std::vector<PlaintextEntry>& entries,
                                                  const std::string& filepath,
                                                  const std::string& password);

    std::vector<uint8_t> stretchBitwardenKey(const std::vector<uint8_t>& key);
    std::string encryptBitwardenData(const std::string& plaintext, const std::vector<uint8_t>& stretchedKey);
    std::vector<uint8_t> aesCbcEncrypt(const std::string& plaintext, const std::vector<uint8_t>& key, const std::vector<uint8_t>& iv);
    std::vector<uint8_t> computeHmacSha256(const std::vector<uint8_t>& iv, const std::vector<uint8_t>& ciphertext, const std::vector<uint8_t>& key);
    std::string generateUUID();
    void exportToLastPassCSV(const std::vector<PlaintextEntry>& entries, const std::string& filepath);

};

#endif // EXPORT_H
