#include "../src/core/import_export/export/export.h"
#include "aes_gcm.h"
#include "key_derivation.h"
#include "Serializer.h"
#include "key_manager.h"
#include "key_storage.h"
#include "LogEntry.h"
#include "base64.h"
#include "Ed25519.h"

#include <fstream>
#include <QCoreApplication>
#include <QProcess>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/hmac.h>
#include <openssl/kdf.h>

#include <iomanip>
#include <sstream>

#include <QDir>
#include <QUuid>
#include <QTemporaryFile>
#include <sys/stat.h>



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

        //KeyManager::getInstance().clearExportKey();
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

void Exporter::exportToBitwardenEncryptedJSON(std::vector<PlaintextEntry>& entries,
                                              const std::string& filepath,
                                              const std::string& password) {
    // Сохраняем CSV файл рядом с выходным файлом
    std::string csvPath = filepath + ".csv";
    std::ofstream csvFile(csvPath);
    if (!csvFile.is_open()) {
        throw std::runtime_error("Failed to create CSV file: " + csvPath);
    }

    // Записываем CSV
    csvFile << "folder,favorite,type,name,notes,fields,login_uri,login_username,login_password,login_totp\n";

    for (const auto& entry : entries) {
        csvFile << ",,";                                    // folder, favorite
        csvFile << "1,";                                    // type (1 = login)
        csvFile << "\"" << entry.title << "\",";
        csvFile << "\"" << entry.notes << "\",";
        csvFile << ",";                                     // fields
        csvFile << "\"" << entry.url << "\",";
        csvFile << "\"" << entry.username << "\",";
        csvFile << "\"" << entry.password << "\",";
        csvFile << "\n";
    }
    csvFile.close();

    // Создаем shell скрипт
    QString scriptPath = QCoreApplication::applicationDirPath() + "/import_export.sh";
    std::ofstream scriptFile(scriptPath.toStdString());
    scriptFile << "#!/bin/bash\n";
    scriptFile << "CSV_FILE=\"" << csvPath << "\"\n";
    scriptFile << "OUTPUT_FILE=\"" << filepath << "\"\n";
    scriptFile << "\n";
    scriptFile << "echo '========================================='\n";
    scriptFile << "echo 'Bitwarden Export'\n";
    scriptFile << "echo '========================================='\n";
    scriptFile << "echo ''\n";
    scriptFile << "echo 'CSV файл: ' $CSV_FILE\n";
    scriptFile << "echo ''\n";
    scriptFile << "echo '1. Импорт CSV в Bitwarden...'\n";
    scriptFile << "bw import bitwardencsv \"$CSV_FILE\"\n";
    scriptFile << "echo ''\n";
    scriptFile << "echo '2. Экспорт в зашифрованный JSON...'\n";
    scriptFile << "bw export --format encrypted_json --output \"$OUTPUT_FILE\"\n";
    scriptFile << "echo ''\n";
    scriptFile << "echo '========================================='\n";
    scriptFile << "echo 'Готово! Зашифрованный файл: ' $OUTPUT_FILE\n";
    scriptFile << "echo 'CSV файл сохранен: ' $CSV_FILE\n";
    scriptFile << "echo '========================================='\n";
    scriptFile << "rm -f \"$CSV_FILE\"\n";
    scriptFile << "read -p 'Нажмите Enter для закрытия окна...'\n";
    scriptFile.close();

    chmod(scriptPath.toStdString().c_str(), 0755);

    // Открываем терминал
    QString command = scriptPath;
#ifdef Q_OS_WIN
    QProcess::startDetached("cmd.exe", QStringList() << "/c" << "start" << "cmd.exe" << "/k" << command);
#else
    if (!QProcess::startDetached("gnome-terminal", QStringList() << "--" << "bash" << "-c" << command + "; exec bash")) {
        if (!QProcess::startDetached("konsole", QStringList() << "-e" << "bash" << "-c" << command + "; exec bash")) {
            if (!QProcess::startDetached("xterm", QStringList() << "-e" << "bash" << "-c" << command + "; exec bash")) {
                QProcess::startDetached("xfce4-terminal", QStringList() << "-e" << "bash" << "-c" << command + "; exec bash");
            }
        }
    }
#endif
}
// Вспомогательная функция для стрейчинга ключа (Bitwarden HKDF)
std::vector<uint8_t> Exporter::stretchBitwardenKey(const std::vector<uint8_t>& key) {
    // Bitwarden использует HKDF, но мы можем реализовать его через HMAC
    // HKDF(PRK, salt = "enc", info = "masterKey", length = 64)

    std::vector<uint8_t> stretchedKey(64);
    std::string salt = "enc";
    std::string info = "masterKey";

    // HKDF-Extract: PRK = HMAC-Hash(salt, key)
    std::vector<uint8_t> prk(32); // SHA256 output is 32 bytes
    unsigned int prkLen = 0;

    HMAC_CTX* ctx = HMAC_CTX_new();

    // Extract stage
    if (HMAC_Init_ex(ctx, salt.c_str(), salt.size(), EVP_sha256(), nullptr) != 1 ||
        HMAC_Update(ctx, key.data(), key.size()) != 1 ||
        HMAC_Final(ctx, prk.data(), &prkLen) != 1) {
        HMAC_CTX_free(ctx);
        throw std::runtime_error("HKDF-Extract failed");
    }
    prk.resize(prkLen);

    // Expand stage
    std::vector<uint8_t> out(64);
    std::vector<uint8_t> t;
    std::vector<uint8_t> infoWithCounter;

    for (unsigned int i = 1; i <= 3; ++i) { // We need 64 bytes (2 blocks of 32)
        infoWithCounter = std::vector<uint8_t>(info.begin(), info.end());
        infoWithCounter.push_back(static_cast<uint8_t>(i));

        if (i == 1) {
            // T(1) = HMAC(PRK, info || 0x01)
            if (HMAC_Init_ex(ctx, prk.data(), prk.size(), EVP_sha256(), nullptr) != 1 ||
                HMAC_Update(ctx, infoWithCounter.data(), infoWithCounter.size()) != 1) {
                HMAC_CTX_free(ctx);
                throw std::runtime_error("HKDF-Expand failed");
            }
        } else {
            // T(n) = HMAC(PRK, T(n-1) || info || n)
            if (HMAC_Init_ex(ctx, prk.data(), prk.size(), EVP_sha256(), nullptr) != 1 ||
                HMAC_Update(ctx, t.data(), t.size()) != 1 ||
                HMAC_Update(ctx, infoWithCounter.data(), infoWithCounter.size()) != 1) {
                HMAC_CTX_free(ctx);
                throw std::runtime_error("HKDF-Expand failed");
            }
        }

        t.resize(32);
        unsigned int tLen = 0;
        if (HMAC_Final(ctx, t.data(), &tLen) != 1) {
            HMAC_CTX_free(ctx);
            throw std::runtime_error("HKDF-Expand final failed");
        }
        t.resize(tLen);

        // Копируем результат в выходной буфер
        size_t offset = (i - 1) * 32;
        size_t copySize = std::min<size_t>(t.size(), out.size() - offset);
        std::copy(t.begin(), t.begin() + copySize, out.begin() + offset);
    }

    HMAC_CTX_free(ctx);
    return out;
}

// Вспомогательная функция для шифрования данных в формате Bitwarden
std::string Exporter::encryptBitwardenData(const std::string& plaintext,
                                           const std::vector<uint8_t>& stretchedKey) {
    // Split stretched key: первые 32 байта - ключ шифрования (AES), следующие 32 - ключ MAC (HMAC)
    std::vector<uint8_t> encKey(stretchedKey.begin(), stretchedKey.begin() + 32);
    std::vector<uint8_t> macKey(stretchedKey.begin() + 32, stretchedKey.begin() + 64);

    // Генерация IV (16 байт для AES-CBC)
    std::vector<uint8_t> iv(16);
    if (RAND_bytes(iv.data(), iv.size()) != 1) {
        throw std::runtime_error("Failed to generate IV");
    }

    // AES-256-CBC шифрование с PKCS7 паддингом
    std::vector<uint8_t> ciphertext = aesCbcEncrypt(plaintext, encKey, iv);

    // Вычисление HMAC-SHA256
    std::vector<uint8_t> mac = computeHmacSha256(iv, ciphertext, macKey);

    // Формат Bitwarden: "2.IV|CIPHERTEXT|MAC"
    std::string ivBase64 = base64Encode(iv);
    std::string ciphertextBase64 = base64Encode(ciphertext);
    std::string macBase64 = base64Encode(mac);

    return "2." + ivBase64 + "|" + ciphertextBase64 + "|" + macBase64;
}

// AES-CBC шифрование с PKCS7 паддингом
std::vector<uint8_t> Exporter::aesCbcEncrypt(const std::string& plaintext,
                                             const std::vector<uint8_t>& key,
                                             const std::vector<uint8_t>& iv) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create cipher context");
    }

    // Используем AES-256-CBC (ключ 32 байта)
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize encryption");
    }

    // Включаем PKCS7 паддинг (по умолчанию включен)

    std::vector<uint8_t> ciphertext(plaintext.length() + EVP_CIPHER_CTX_block_size(ctx));
    int outLen = 0;
    int finalLen = 0;

    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &outLen,
                          reinterpret_cast<const unsigned char*>(plaintext.c_str()),
                          plaintext.length()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Encryption update failed");
    }

    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + outLen, &finalLen) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Encryption final failed");
    }

    ciphertext.resize(outLen + finalLen);
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext;
}

// Вычисление HMAC-SHA256
std::vector<uint8_t> Exporter::computeHmacSha256(const std::vector<uint8_t>& iv,
                                                 const std::vector<uint8_t>& ciphertext,
                                                 const std::vector<uint8_t>& key) {
    HMAC_CTX* ctx = HMAC_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create HMAC context");
    }

    if (HMAC_Init_ex(ctx, key.data(), key.size(), EVP_sha256(), nullptr) != 1) {
        HMAC_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize HMAC");
    }

    // HMAC вычисляется от IV + CIPHERTEXT
    if (HMAC_Update(ctx, iv.data(), iv.size()) != 1) {
        HMAC_CTX_free(ctx);
        throw std::runtime_error("Failed to update HMAC with IV");
    }

    if (HMAC_Update(ctx, ciphertext.data(), ciphertext.size()) != 1) {
        HMAC_CTX_free(ctx);
        throw std::runtime_error("Failed to update HMAC with ciphertext");
    }

    std::vector<uint8_t> mac(EVP_MAX_MD_SIZE);
    unsigned int macLen = 0;

    if (HMAC_Final(ctx, mac.data(), &macLen) != 1) {
        HMAC_CTX_free(ctx);
        throw std::runtime_error("Failed to finalize HMAC");
    }

    mac.resize(macLen);
    HMAC_CTX_free(ctx);

    return mac;
}

// Генерация UUID v4
std::string Exporter::generateUUID() {
    std::vector<uint8_t> uuidBytes(16);
    if (RAND_bytes(uuidBytes.data(), uuidBytes.size()) != 1) {
        throw std::runtime_error("Failed to generate UUID");
    }

    // Set version (4) and variant bits
    uuidBytes[6] = (uuidBytes[6] & 0x0F) | 0x40; // Version 4
    uuidBytes[8] = (uuidBytes[8] & 0x3F) | 0x80; // Variant 1

    char uuidStr[37];
    snprintf(uuidStr, sizeof(uuidStr),
             "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             uuidBytes[0], uuidBytes[1], uuidBytes[2], uuidBytes[3],
             uuidBytes[4], uuidBytes[5], uuidBytes[6], uuidBytes[7],
             uuidBytes[8], uuidBytes[9], uuidBytes[10], uuidBytes[11],
             uuidBytes[12], uuidBytes[13], uuidBytes[14], uuidBytes[15]);

    return std::string(uuidStr);
}

void Exporter::exportToLastPassCSV(const std::vector<PlaintextEntry>& entries, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }

    // Заголовки LastPass CSV
    file << "url,username,password,totp,extra,name,grouping,fav\n";

    for (const auto& entry : entries) {
        // Экранируем поля
        std::string url = escapeCSV(entry.url);
        std::string username = escapeCSV(entry.username);
        std::string password = escapeCSV(entry.password);
        std::string totp = ""; // TOTP секрет (у нас нет, оставляем пустым)
        std::string extra = escapeCSV(entry.notes); // Заметки в поле extra
        std::string name = escapeCSV(entry.title);
        std::string grouping = escapeCSV(entry.category); // Категория как grouping
        std::string fav = "0"; // LastPass fav (0 = не избранное, так как у нас нет этого поля)

        file << url << ","
             << username << ","
             << password << ","
             << totp << ","
             << extra << ","
             << name << ","
             << grouping << ","
             << fav << "\n";
    }

    file.close();
}

