// sharing_service.h
#ifndef SHARING_SERVICE_H
#define SHARING_SERVICE_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "../vault/plaintext_entry.h"
#include "../serialization/Serializer.h"
#include "../crypto/aes_gcm.h"
#include "../crypto/key_derivation.h"
#include "../crypto/rsa_cipher.h"
#include "base64.h"
#include "key_manager.h"

using json = nlohmann::json;

struct ShareMetadata {
    std::string sharer;
    std::string shared_at;
    std::string expires_at;
    std::string permissions;
    std::string entry_title;
    std::string entry_username;
};

struct ImportShareResult {
    bool success;
    std::string errorMessage;
    ShareMetadata metadata;
    PlaintextEntry entry;
    bool isExpired;
    bool signatureValid;
};

class SharingService {
public:
    static SharingService& getInstance() {
        static SharingService instance;
        return instance;
    }

    // Создание share-пакета с паролем
    void shareWithPassword(const PlaintextEntry& entry,
                           const std::string& password,
                           const std::string& sharerName,
                           int expirationDays,
                           const std::string& permissions,
                           const std::string& filepath);

    // Создание share-пакета с публичным ключом (RSA)
    void shareWithPublicKey(const PlaintextEntry& entry,
                            const std::vector<uint8_t>& recipientPublicKey,
                            const std::string& sharerName,
                            int expirationDays,
                            const std::string& permissions,
                            const std::string& filepath);

    // Импорт share-пакета
    ImportShareResult importSharedEntry(const std::string& filepath,
                                        const std::string& password = "");

    // Проверка срока действия
    bool isExpired(const std::string& expires_at);

private:
    SharingService() = default;

    std::string generateExpirationDate(int days);
    void saveToFile(const std::string& filepath, const json& data);
    json loadFromFile(const std::string& filepath);
};

#endif
