// importer.cpp
#include "import.h"
#include <QTextCodec>
#include <sstream>
#include <algorithm>
#include <cctype>

std::string Importer::sanitize(const std::string& input) {
    if (input.empty()) return "";

    std::string result;
    for (char c : input) {
        // Удаляем потенциально опасные символы
        if (c == '<' || c == '>' || c == '\'' || c == '"' || c == ';' || c == '`') {
            continue;
        }
        // Контрольные символы (кроме табуляции, перевода строки, возврата каретки)
        if (static_cast<unsigned char>(c) < 0x20 && c != '\t' && c != '\n' && c != '\r') {
            continue;
        }
        result += c;
    }
    return result;
}

bool Importer::containsMaliciousContent(const std::string& input) {
    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    // Простые паттерны
    if (lower.find("<script") != std::string::npos) return true;
    if (lower.find("javascript:") != std::string::npos) return true;
    if (lower.find("onload=") != std::string::npos) return true;
    if (lower.find("onerror=") != std::string::npos) return true;
    if (lower.find("alert(") != std::string::npos) return true;
    if (lower.find("' or '1'='1") != std::string::npos) return true;
    if (lower.find("'; drop") != std::string::npos) return true;
    if (lower.find("--") != std::string::npos) return true;

    return false;
}

ImportResult Importer::importFromEncryptedJSON(const QString& filepath, const std::string& password) {
    ImportResult result;
    result.success = false;
    result.totalCount = 0;
    result.duplicateCount = 0;
    result.sanitizedCount = 0;

    // Читаем файл через Qt
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.errorMessage = "Failed to open file: " + filepath.toStdString();
        return result;
    }

    QByteArray fileData = file.readAll();
    file.close();

    if (fileData.isEmpty()) {
        result.errorMessage = "File is empty";
        return result;
    }

    // Парсим JSON
    json exportJson;
    try {
        exportJson = json::parse(fileData.toStdString());
    } catch (const std::exception& e) {
        result.errorMessage = "Invalid JSON: " + std::string(e.what());
        return result;
    }

    // Проверяем что это наш экспорт
    if (!exportJson.contains("cryptosafe_export") || !exportJson["cryptosafe_export"].get<bool>()) {
        result.errorMessage = "Not a CryptoSafe export file";
        return result;
    }

    // Извлекаем публичный ключ из файла
    std::vector<uint8_t> publicKeyFromFile;
    if (exportJson.contains("public_key")) {
        std::string pubKeyB64 = exportJson["public_key"];
        publicKeyFromFile = base64Decode(pubKeyB64);
    } else {
        result.errorMessage = "Missing public key in export file";
        return result;
    }

    // Извлекаем параметры шифрования
    if (!exportJson.contains("encryption")) {
        result.errorMessage = "Missing encryption data";
        return result;
    }

    auto enc = exportJson["encryption"];
    std::string algorithm = enc.value("algorithm", "");
    std::string saltB64 = enc.value("salt", "");
    std::string nonceB64 = enc.value("nonce", "");
    int iterations = enc.value("iterations", 100000);

    if (saltB64.empty() || nonceB64.empty()) {
        result.errorMessage = "Missing salt or nonce";
        return result;
    }

    std::vector<uint8_t> salt = base64Decode(saltB64);
    std::vector<uint8_t> nonce = base64Decode(nonceB64);

    // Выводим ключ
    std::vector<uint8_t> key;
    derive_encryption_key(password, salt, key);

    // Расшифровываем данные
    std::string dataB64 = exportJson.value("data", "");
    if (dataB64.empty()) {
        result.errorMessage = "Missing data";
        return result;
    }

    std::vector<uint8_t> encryptedData = base64Decode(dataB64);

    // Проверяем алгоритм и расшифровываем
    std::vector<uint8_t> decrypted;
    KeyData keyData{key.data(), key.size()};

    if (algorithm == "AES-256-GCM") {
        AESGCM<256> cipher;
        decrypted = cipher.decrypt(keyData, encryptedData);
    } else if (algorithm == "AES-128-GCM") {
        AESGCM<128> cipher;
        decrypted = cipher.decrypt(keyData, encryptedData);
    } else {
        result.errorMessage = "Unknown algorithm: " + algorithm;
        return result;
    }

    // Проверяем подпись с использованием публичного ключа из файла
    if (exportJson.contains("signature")) {
        std::string sigB64 = exportJson["signature"];
        std::vector<uint8_t> signature = base64Decode(sigB64);

        // Используем публичный ключ из файла, а не из KeyManager
        if (!verify(encryptedData, signature, publicKeyFromFile)) {
            result.errorMessage = "Signature verification failed - file may be tampered";
            return result;
        }
    }

    // Десериализуем записи
    result.entries = Serializer::deserializeBatch<PlaintextEntry>(decrypted);

    // Санитизация
    for (auto& entry : result.entries) {
        bool malicious = false;
        if (containsMaliciousContent(entry.title) ||
            containsMaliciousContent(entry.username) ||
            containsMaliciousContent(entry.password) ||
            containsMaliciousContent(entry.url) ||
            containsMaliciousContent(entry.notes)) {
            malicious = true;
            result.sanitizedCount++;
        }

        if (malicious) {
            entry.title = sanitize(entry.title);
            entry.username = sanitize(entry.username);
            entry.password = sanitize(entry.password);
            entry.url = sanitize(entry.url);
            entry.notes = sanitize(entry.notes);
            entry.category = sanitize(entry.category);
            entry.tags = sanitize(entry.tags);
        }
    }

    result.totalCount = result.entries.size();
    result.success = true;

    // Очищаем ключ из памяти
    volatile uint8_t* p = key.data();
    for (size_t i = 0; i < key.size(); ++i) {
        p[i] = 0;
    }

    return result;
}

ImportResult Importer::importFromCSV(const QString& filepath) {
    ImportResult result;
    result.success = false;
    result.totalCount = 0;
    result.duplicateCount = 0;
    result.sanitizedCount = 0;

    // Открываем файл через Qt с UTF-8 кодировкой
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.errorMessage = "Failed to open file: " + filepath.toStdString();
        return result;
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");  // Поддержка русских символов

    // Читаем заголовки (первая строка)
    if (stream.atEnd()) {
        result.errorMessage = "File is empty";
        return result;
    }

    QString headerLine = stream.readLine();  // пропускаем заголовки

    // Читаем записи
    int lineNum = 0;
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        lineNum++;

        if (line.trimmed().isEmpty()) continue;

        // Парсим CSV строку с учётом кавычек
        QStringList fields;
        bool inQuotes = false;
        QString currentField;

        for (int i = 0; i < line.length(); ++i) {
            QChar ch = line[i];

            if (ch == '"') {
                if (i + 1 < line.length() && line[i + 1] == '"') {
                    // Экранированная кавычка
                    currentField += '"';
                    i++;
                } else {
                    inQuotes = !inQuotes;
                }
            } else if (ch == ',' && !inQuotes) {
                fields.append(currentField);
                currentField.clear();
            } else {
                currentField += ch;
            }
        }
        fields.append(currentField);

        if (fields.size() < 3) {
            qWarning() << "Line" << lineNum << "has less than 3 fields, skipping";
            continue;
        }

        PlaintextEntry entry;
        entry.title = fields[0].toStdString();
        entry.username = fields[1].toStdString();
        entry.password = fields[2].toStdString();
        if (fields.size() > 3) entry.url = fields[3].toStdString();
        if (fields.size() > 4) entry.notes = fields[4].toStdString();
        if (fields.size() > 5) entry.category = fields[5].toStdString();
        if (fields.size() > 6) entry.creation_timestamp = fields[6].toStdString();
        if (fields.size() > 7) entry.tags = fields[7].toStdString();

        // Санитизация
        bool malicious = false;
        if (containsMaliciousContent(entry.title) ||
            containsMaliciousContent(entry.username) ||
            containsMaliciousContent(entry.password)) {
            malicious = true;
            result.sanitizedCount++;
        }

        if (malicious) {
            entry.title = sanitize(entry.title);
            entry.username = sanitize(entry.username);
            entry.password = sanitize(entry.password);
            entry.url = sanitize(entry.url);
            entry.notes = sanitize(entry.notes);
            entry.category = sanitize(entry.category);
            entry.tags = sanitize(entry.tags);
        }

        result.entries.push_back(entry);
        result.totalCount++;
    }

    file.close();
    result.success = true;
    return result;
}
