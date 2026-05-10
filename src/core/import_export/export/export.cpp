#include "../src/core/import_export/export/export.h"
#include "aes_gcm.h"
#include "key_derivation.h"
#include "Serializer.h"
#include "key_manager.h"
#include "key_storage.h"
#include "LogEntry.h"
#include "base64.h"
#include "Ed25519.h"

#include <openssl/rand.h>
#include <fstream>

void Exporter::exportToEncryptedJSON(std::vector<PlaintextEntry>& entries,
                                     const std::string& filepath,
                                     const std::string& password,
                                     EncryptionStrength strength){

    std::vector<uint8_t> plaintext = Serializer::serializeBatch(entries);

    std::vector<uint8_t> salt(16);
    std::vector<uint8_t> nonce(12);

    if (RAND_bytes(salt.data(), salt.size()) != 1) {
        throw std::runtime_error("Failed to generate salt");
    }
    if (RAND_bytes(nonce.data(), nonce.size()) != 1) {
        throw std::runtime_error("Failed to generate nonce");
    }

    std::vector<uint8_t> key;
    derive_encryption_key(password, salt, key);
    KeyManager::getInstance().storeExportKey(key);

    std::vector<uint8_t> ciphertext;

    KeyData d;
    KeyManager::getInstance().getExportKey(d);

    if (strength == EncryptionStrength::AES_256) {
        AESGCM<256> cipher;
        ciphertext = cipher.encrypt(d, plaintext);
    } else {
        AESGCM<128> cipher;
        ciphertext = cipher.encrypt(d, plaintext);
    }

    std::vector<uint8_t> publicKey = derivePublicKey("exportSign");

    json exportJson;
    exportJson["cryptosafe_export"] = true;
    exportJson["timestamp"] = getUTCTimestamp();
    exportJson["encryption"]["algorithm"] = (strength == EncryptionStrength::AES_256) ? "AES-256-GCM" : "AES-128-GCM";
    exportJson["encryption"]["key_derivation"] = "PBKDF2-SHA256";
    exportJson["encryption"]["iterations"] = 100000;
    exportJson["encryption"]["salt"] = base64Encode(salt);
    exportJson["encryption"]["nonce"] = base64Encode(nonce);
    exportJson["data"] = base64Encode(ciphertext);

    exportJson["public_key"] = base64Encode(publicKey);

    std::ofstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }

    std::vector<uint8_t> data_bytes = base64Decode(exportJson["data"].get<std::string>());

    std::vector<uint8_t> sig = sign_data("exportSign", data_bytes);

    exportJson["signature"] = base64Encode(sig);

    file << exportJson.dump();
    file.close();

    KeyManager::getInstance().clearExportKey();
}

void Exporter::exportToCSV(const std::vector<PlaintextEntry>& entries,
                           const std::string& filepath,
                           bool encrypt,
                           const std::string& password) {

    std::string csv_content;

    // Заголовки
    csv_content += "title,username,password,url,notes,category,creation_timestamp,tags\n";

    // Данные
    for (const auto& entry : entries) {
        csv_content += escapeCSV(entry.title) + ",";
        csv_content += escapeCSV(entry.username) + ",";
        csv_content += escapeCSV(entry.password) + ",";
        csv_content += escapeCSV(entry.url) + ",";
        csv_content += escapeCSV(entry.notes) + ",";
        csv_content += escapeCSV(entry.category) + ",";
        csv_content += escapeCSV(entry.creation_timestamp) + ",";
        csv_content += escapeCSV(entry.tags) + "\n";
    }

    if (encrypt) {
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
        KeyManager::getInstance().storeExportKey(key);

        // Шифруем CSV данные
        std::vector<uint8_t> plaintext(csv_content.begin(), csv_content.end());
        std::vector<uint8_t> ciphertext;

        KeyData d;
        KeyManager::getInstance().getExportKey(d);

        AESGCM<256> cipher;
        ciphertext = cipher.encrypt(d, plaintext);

        // Формируем JSON контейнер для зашифрованного CSV
        json exportJson;
        exportJson["cryptosafe_export"] = true;
        exportJson["format"] = "csv_encrypted";
        exportJson["timestamp"] = getUTCTimestamp();
        exportJson["encryption"]["algorithm"] = "AES-256-GCM";
        exportJson["encryption"]["key_derivation"] = "PBKDF2-SHA256";
        exportJson["encryption"]["iterations"] = 100000;
        exportJson["encryption"]["salt"] = base64Encode(salt);
        exportJson["encryption"]["nonce"] = base64Encode(nonce);
        exportJson["data"] = base64Encode(ciphertext);

        std::ofstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filepath);
        }
        file << exportJson.dump();
        file.close();

        KeyManager::getInstance().clearExportKey();
    } else {
        std::ofstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filepath);
        }
        file << csv_content;
        file.close();
    }
}

// Вспомогательная функция для экранирования CSV полей
std::string Exporter::escapeCSV(const std::string& field) {
    if (field.empty()) {
        return "";
    }

    // Если поле содержит запятую, кавычку или перевод строки - оборачиваем в кавычки
    bool needQuotes = field.find(',') != std::string::npos ||
                      field.find('"') != std::string::npos ||
                      field.find('\n') != std::string::npos;

    if (!needQuotes) {
        return field;
    }

    // Экранируем кавычки ("" вместо ")
    std::string escaped = field;
    size_t pos = 0;
    while ((pos = escaped.find('"', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\"\"");
        pos += 2;
    }

    return "\"" + escaped + "\"";
}
