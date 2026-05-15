#include "sharing_service.h"
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <openssl/rand.h>
#include "LogEntry.h"
#include "Ed25519.h"

std::string SharingService::generateExpirationDate(int days) {
    auto now = std::chrono::system_clock::now();
    auto expire = now + std::chrono::hours(days * 24);
    auto expire_time_t = std::chrono::system_clock::to_time_t(expire);
    std::tm* utc_time = std::gmtime(&expire_time_t);

    std::ostringstream oss;
    oss << std::put_time(utc_time, "%Y-%m-%dT%H:%M:%S") << "Z";
    return oss.str();
}

bool SharingService::isExpired(const std::string& expires_at) {
    std::tm tm = {};
    std::istringstream ss(expires_at);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");

    auto expire = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    auto now = std::chrono::system_clock::now();

    return now > expire;
}

void SharingService::saveToFile(const std::string& filepath, const json& data) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }
    file << data.dump(2);
    file.close();
}

json SharingService::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }

    std::stringstream ss;
    ss << file.rdbuf();
    file.close();

    return json::parse(ss.str());
}

void SharingService::shareWithPassword(const PlaintextEntry& entry,
                                       const std::string& password,
                                       const std::string& sharerName,
                                       int expirationDays,
                                       const std::string& permissions,
                                       const std::string& filepath) {
    // Сериализуем запись
    std::vector<uint8_t> plaintext = Serializer::serialize(entry);

    // Генерируем соль и nonce
    std::vector<uint8_t> salt(16);
    std::vector<uint8_t> nonce(12);

    if (RAND_bytes(salt.data(), salt.size()) != 1) {
        throw std::runtime_error("Failed to generate salt");
    }
    if (RAND_bytes(nonce.data(), nonce.size()) != 1) {
        throw std::runtime_error("Failed to generate nonce");
    }

    // Выводим ключ из пароля
    std::vector<uint8_t> key;
    derive_encryption_key(password, salt, key);

    // Шифруем данные
    AESGCM<256> cipher;
    KeyData keyData{key.data(), key.size()};
    std::vector<uint8_t> ciphertext = cipher.encrypt(keyData, plaintext);

    // Формируем JSON
    json shareJson;
    shareJson["type"] = "cryptosafe_share";
    shareJson["encryption_method"] = "password";

    shareJson["metadata"]["sharer"] = sharerName;
    shareJson["metadata"]["shared_at"] = getUTCTimestamp();
    shareJson["metadata"]["expires_at"] = generateExpirationDate(expirationDays);
    shareJson["metadata"]["permissions"] = permissions;
    shareJson["metadata"]["entry_title"] = entry.title;
    shareJson["metadata"]["entry_username"] = entry.username;

    shareJson["encryption"]["algorithm"] = "AES-256-GCM";
    shareJson["encryption"]["key_derivation"] = "PBKDF2-SHA256";
    shareJson["encryption"]["iterations"] = 100000;
    shareJson["encryption"]["salt"] = base64Encode(salt);
    shareJson["encryption"]["nonce"] = base64Encode(nonce);

    shareJson["data"] = base64Encode(ciphertext);

    // std::vector<uint8_t> signature = sign_data("exportSign", ciphertext);
    // shareJson["signature"] = base64Encode(signature);

    // std::vector<uint8_t> senderPublicKey = derivePublicKey("exportSign");
    // shareJson["sender_public_key"] = base64Encode(senderPublicKey);

    // Сохраняем в файл
    saveToFile(filepath, shareJson);

    // Очищаем ключ
    volatile uint8_t* p = key.data();
    for (size_t i = 0; i < key.size(); ++i) {
        p[i] = 0;
    }
}

void SharingService::shareWithPublicKey(const PlaintextEntry& entry,
                                        const std::vector<uint8_t>& recipientPublicKey,
                                        const std::string& sharerName,
                                        int expirationDays,
                                        const std::string& permissions,
                                        const std::string& filepath) {
    // Сериализуем запись
    std::vector<uint8_t> plaintext = Serializer::serialize(entry);

    // Генерируем случайный AES-256 ключ
    std::vector<uint8_t> aesKey(32);
    if (RAND_bytes(aesKey.data(), aesKey.size()) != 1) {
        throw std::runtime_error("Failed to generate AES key");
    }

    // Генерируем nonce
    std::vector<uint8_t> nonce(12);
    if (RAND_bytes(nonce.data(), nonce.size()) != 1) {
        throw std::runtime_error("Failed to generate nonce");
    }

    // Шифруем данные AES-256-GCM
    AESGCM<256> cipher;
    KeyData keyData{aesKey.data(), aesKey.size()};
    std::vector<uint8_t> ciphertext = cipher.encrypt(keyData, plaintext);

    // Шифруем AES-ключ публичным ключом получателя (RSA)
    std::vector<uint8_t> encryptedAesKey = RSACipher::encrypt(aesKey, recipientPublicKey);

    // Формируем JSON
    json shareJson;
    shareJson["type"] = "cryptosafe_share";
    shareJson["encryption_method"] = "public_key";

    shareJson["metadata"]["sharer"] = sharerName;
    shareJson["metadata"]["shared_at"] = getUTCTimestamp();
    shareJson["metadata"]["expires_at"] = generateExpirationDate(expirationDays);
    shareJson["metadata"]["permissions"] = permissions;
    shareJson["metadata"]["entry_title"] = entry.title;
    shareJson["metadata"]["entry_username"] = entry.username;

    shareJson["encryption"]["algorithm"] = "RSA-OAEP + AES-256-GCM";
    shareJson["encryption"]["encrypted_key"] = base64Encode(encryptedAesKey);
    shareJson["encryption"]["nonce"] = base64Encode(nonce);

    shareJson["data"] = base64Encode(ciphertext);

    // std::vector<uint8_t> signature = sign_data("exportSign", ciphertext);
    // shareJson["signature"] = base64Encode(signature);

    // std::vector<uint8_t> senderPublicKey = derivePublicKey("exportSign");
    // shareJson["sender_public_key"] = base64Encode(senderPublicKey);

    // Сохраняем в файл
    saveToFile(filepath, shareJson);

    // Очищаем AES ключ
    volatile uint8_t* p = aesKey.data();
    for (size_t i = 0; i < aesKey.size(); ++i) {
        p[i] = 0;
    }
}

ImportShareResult SharingService::importSharedEntry(const std::string& filepath,
                                                    const std::string& password) {
    ImportShareResult result;
    result.success = false;
    result.isExpired = false;

    try {
        // Загружаем JSON
        json shareJson = loadFromFile(filepath);

        // Проверяем тип
        if (!shareJson.contains("type") || shareJson["type"] != "cryptosafe_share") {
            result.errorMessage = "Invalid share file";
            return result;
        }

        // Извлекаем метаданные
        auto meta = shareJson["metadata"];
        result.metadata.sharer = meta.value("sharer", "");
        result.metadata.shared_at = meta.value("shared_at", "");
        result.metadata.expires_at = meta.value("expires_at", "");
        result.metadata.permissions = meta.value("permissions", "read_only");
        result.metadata.entry_title = meta.value("entry_title", "");
        result.metadata.entry_username = meta.value("entry_username", "");

        std::string sigB64 = shareJson["signature"];
        std::string senderPubKeyB64 = shareJson["sender_public_key"];

        std::vector<uint8_t> signature = base64Decode(sigB64);
        std::vector<uint8_t> senderPublicKey = base64Decode(senderPubKeyB64);

        // Получаем зашифрованные данные для проверки подписи
        // std::string dataB64 = shareJson.value("data", "");
        // if (!dataB64.empty()) {
        //     std::vector<uint8_t> encryptedData = base64Decode(dataB64);

        //     // Верифицируем подпись
        //     if (verify(encryptedData, signature, senderPublicKey)) {
        //         result.signatureValid = true;
        //     } else {
        //         result.errorMessage = "Signature verification failed - share may be tampered";
        //     }
        // }

        // Проверяем срок действия
        if (isExpired(result.metadata.expires_at)) {
            result.isExpired = true;
            result.errorMessage = "Share has expired";
            return result;
        }

        std::string method = shareJson.value("encryption_method", "");
        std::vector<uint8_t> decrypted;

        if (method == "password") {
            // Расшифровка с паролем
            if (password.empty()) {
                result.errorMessage = "Password required";
                return result;
            }

            auto enc = shareJson["encryption"];
            std::string saltB64 = enc.value("salt", "");
            std::string nonceB64 = enc.value("nonce", "");
            std::string dataB64 = shareJson.value("data", "");

            if (saltB64.empty() || nonceB64.empty()) {
                result.errorMessage = "Missing encryption parameters";
                return result;
            }

            std::vector<uint8_t> salt = base64Decode(saltB64);
            std::vector<uint8_t> nonce = base64Decode(nonceB64);
            std::vector<uint8_t> encryptedData = base64Decode(dataB64);

            // Выводим ключ из пароля
            std::vector<uint8_t> key;
            derive_encryption_key(password, salt, key);

            // Расшифровываем
            AESGCM<256> cipher;
            KeyData keyData{key.data(), key.size()};
            decrypted = cipher.decrypt(keyData, encryptedData);

            // Очищаем ключ
            volatile uint8_t* p = key.data();
            for (size_t i = 0; i < key.size(); ++i) p[i] = 0;

        } else if (method == "public_key") {
            // Расшифровка с публичным ключом
            auto enc = shareJson["encryption"];
            std::string encryptedKeyB64 = enc.value("encrypted_key", "");
            std::string nonceB64 = enc.value("nonce", "");
            std::string dataB64 = shareJson.value("data", "");

            if (encryptedKeyB64.empty()) {
                result.errorMessage = "Missing encrypted key";
                return result;
            }

            std::vector<uint8_t> encryptedKey = base64Decode(encryptedKeyB64);
            std::vector<uint8_t> nonce = base64Decode(nonceB64);
            std::vector<uint8_t> encryptedData = base64Decode(dataB64);

            KeyData privateKeyData;
            KeyManager::getInstance().getPrivateRSAKey(privateKeyData);

            if (privateKeyData.size == 0) {
                result.errorMessage = "No private RSA key found. Please generate keys first.";
                return result;
            }

            std::vector<uint8_t> privateKeyPEM(privateKeyData.data,
                                               privateKeyData.data + privateKeyData.size);

            // Расшифровываем AES-ключ
            std::vector<uint8_t> aesKey = RSACipher::decrypt(encryptedKey, privateKeyPEM);

            // Расшифровываем данные
            AESGCM<256> cipher;
            KeyData keyData{aesKey.data(), aesKey.size()};
            decrypted = cipher.decrypt(keyData, encryptedData);

            // Очищаем AES ключ
            volatile uint8_t* p = aesKey.data();
            for (size_t i = 0; i < aesKey.size(); ++i) p[i] = 0;

        } else {
            result.errorMessage = "Unknown encryption method: " + method;
            return result;
        }

        // Десериализуем запись
        result.entry = Serializer::deserialize<PlaintextEntry>(decrypted);
        result.success = true;

        // if (!result.signatureValid && shareJson.contains("signature")) {
        //     result.errorMessage = "WARNING: Share signature invalid. File may be tampered.";
        // }

    } catch (const std::exception& e) {
        result.errorMessage = e.what();
    }

    return result;
}
