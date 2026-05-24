// importer.cpp
#include "import.h"
#include <QTextCodec>
#include <cctype>

#include <fstream>
#include <sstream>
#include <algorithm>

#include <QDir>
#include <QProcess>
#include <sys/stat.h>
#include <QUuid>

#include <QCoreApplication>
#include <QMessageBox>

#include <QThread>

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

ImportResult Importer::importFromLastPassCSV(const std::string& filepath) {
    ImportResult result;
    result.success = false;

    std::ifstream file(filepath);
    if (!file.is_open()) {
        result.errorMessage = "Failed to open file: " + filepath;
        return result;
    }

    std::string line;
    bool isFirstLine = true;
    int lineNum = 0;

    while (std::getline(file, line)) {
        lineNum++;

        if (line.empty()) continue;

        // Парсим заголовок
        if (isFirstLine) {
            isFirstLine = false;
            // Проверяем, что это заголовок LastPass
            if (line.find("url,username,password") == std::string::npos) {
                result.errorMessage = "Invalid LastPass CSV format: missing required headers";
                return result;
            }
            continue;
        }

        // Парсим строку CSV
        std::vector<std::string> fields;
        std::stringstream ss(line);
        std::string field;

        while (std::getline(ss, field, ',')) {
            fields.push_back(field);
        }

        // LastPass CSV имеет 8 полей
        if (fields.size() < 7) {
            result.errorMessage = "Invalid CSV format at line " + std::to_string(lineNum) +
                                  ": expected at least 7 fields, got " + std::to_string(fields.size());
            return result;
        }

        PlaintextEntry entry;

        // url (field 0)
        if (fields.size() > 0) entry.url = unescapeCSV(fields[0]);

        // username (field 1)
        if (fields.size() > 1) entry.username = unescapeCSV(fields[1]);

        // password (field 2)
        if (fields.size() > 2) entry.password = unescapeCSV(fields[2]);

        // totp (field 3) - пропускаем

        // extra (field 4) - заметки
        if (fields.size() > 4) entry.notes = unescapeCSV(fields[4]);

        // name (field 5) - название
        if (fields.size() > 5) entry.title = unescapeCSV(fields[5]);

        // grouping (field 6) - категория
        if (fields.size() > 6) entry.category = unescapeCSV(fields[6]);

        // fav (field 7) - игнорируем, так как у нас нет этого поля

        // Устанавливаем timestamp создания
        entry.creation_timestamp = getUTCTimestamp();
        entry.version = 1;
        entry.tags = ""; // LastPass не имеет тегов, оставляем пустым

        // Если название пустое, используем "Imported from LastPass"
        if (entry.title.empty()) {
            entry.title = "Imported from LastPass";
        }

        result.entries.push_back(entry);
    }

    file.close();

    if (result.entries.empty()) {
        result.errorMessage = "No entries found in CSV file";
        return result;
    }

    result.success = true;
    result.sanitizedCount = 0; // LastPass импорт не требует санитизации

    return result;
}

std::string Importer::unescapeCSV(const std::string& field) {
    std::string result = field;

    // Если поле в двойных кавычках, удаляем их
    if (result.size() >= 2 && result.front() == '"' && result.back() == '"') {
        result = result.substr(1, result.size() - 2);
        // Заменяем двойные кавычки внутри
        size_t pos = 0;
        while ((pos = result.find("\"\"", pos)) != std::string::npos) {
            result.replace(pos, 2, "\"");
            pos++;
        }
    }

    return result;
}
inline QStringList parseCSVLine(const QString& line) {
    QStringList result;
    QString field;
    bool inQuote = false;

    for (int i = 0; i < line.length(); ++i) {
        QChar ch = line[i];

        if (ch == '"') {
            inQuote = !inQuote;
        } else if (ch == ',' && !inQuote) {
            result.append(field);
            field.clear();
        } else {
            field += ch;
        }
    }
    result.append(field);

    // Очищаем кавычки и экранированные кавычки
    for (QString& f : result) {
        if (f.startsWith('"') && f.endsWith('"')) {
            f = f.mid(1, f.length() - 2);
            f.replace("\"\"", "\"");
        }
    }

    return result;
}
ImportResult Importer::importFromBitwardenEncryptedJSON(const QString& filepath) {
    ImportResult result;
    result.success = false;

    // 1. Создаем CSV файл во временной папке (как в экспорте)
    QString tempDir = QDir::temp().absoluteFilePath("bitwarden_import");
    QDir().mkpath(tempDir);
    QString csvPath = "/home/master666/Рабочий стол/export.csv";

    // 2. Создаем скрипт (как в экспорте)
    QString scriptPath = QCoreApplication::applicationDirPath() + "/bitwarden_import.sh";

    std::ofstream scriptFile(scriptPath.toStdString());
    scriptFile << "#!/bin/bash\n";
    scriptFile << "CSV_FILE=\"" << csvPath.toStdString() << "\"\n";
    scriptFile << "INPUT_FILE=\"" << filepath.toStdString() << "\"\n";
    scriptFile << "\n";
    scriptFile << "echo '========================================='\n";
    scriptFile << "echo 'Bitwarden Import'\n";
    scriptFile << "echo '========================================='\n";
    scriptFile << "echo ''\n";
    scriptFile << "echo '1. Importing encrypted JSON to Bitwarden...'\n";
    scriptFile << "bw import bitwardenjson \"$INPUT_FILE\"\n";
    scriptFile << "echo ''\n";
    scriptFile << "echo '2. Exporting to CSV...'\n";
    scriptFile << "bw export --format csv --output \"$CSV_FILE\"\n";
    scriptFile << "echo ''\n";
    scriptFile << "echo '========================================='\n";
    scriptFile << "echo 'SUCCESS! CSV created at: ' $CSV_FILE\n";
    scriptFile << "echo '========================================='\n";
    scriptFile << "read -p 'Press Enter to close...'\n";
    scriptFile.close();

    chmod(scriptPath.toStdString().c_str(), 0755);

    // 3. Открываем терминал (как в экспорте)
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

    // 4. Ждем подтверждения
    QMessageBox::StandardButton reply = QMessageBox::question(nullptr, "Bitwarden Import",
                                                              "В терминале:\n\n"
                                                              "1. Введите мастер-пароль от Bitwarden (2 раза)\n"
                                                              "2. Дождитесь сообщения 'SUCCESS'\n"
                                                              "3. Нажмите Enter\n\n"
                                                              "Затем нажмите OK",
                                                              QMessageBox::Ok | QMessageBox::Cancel);

    if (reply != QMessageBox::Ok) {
        QFile::remove(scriptPath);
        result.errorMessage = "Cancelled";
        return result;
    }

    // 6. Читаем CSV
    QFile csvFile(csvPath);
    if (!csvFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.errorMessage = "Cannot open CSV file";
        QFile::remove(scriptPath);
        return result;
    }

    QTextStream stream(&csvFile);
    stream.setCodec("UTF-8");

    QString headerLine = stream.readLine();
    QStringList headers = parseCSVLine(headerLine);

    // Определяем индексы нужных колонок
    int titleIdx = headers.indexOf("name");
    int usernameIdx = headers.indexOf("login_username");
    int passwordIdx = headers.indexOf("login_password");
    int urlIdx = headers.indexOf("login_uri");
    int notesIdx = headers.indexOf("notes");
    int categoryIdx = headers.indexOf("folder");

    // Если не нашли "name", пробуем другие варианты
    if (titleIdx == -1) titleIdx = headers.indexOf("title");
    if (titleIdx == -1) titleIdx = headers.indexOf("entry_name");

    // Если не нашли "folder", пробуем "category"
    if (categoryIdx == -1) categoryIdx = headers.indexOf("category");

    while (!stream.atEnd()) {
        QString line = stream.readLine();
        if (line.trimmed().isEmpty()) continue;

        QStringList fields = parseCSVLine(line);

        // Проверяем, что у нас достаточно полей
        int maxIdx = std::max({titleIdx, usernameIdx, passwordIdx, urlIdx, notesIdx, categoryIdx});
        if (maxIdx == -1 || fields.size() <= maxIdx) continue;

        PlaintextEntry entry;

        // Заполняем только существующие поля
        if (titleIdx != -1 && titleIdx < fields.size())
            entry.title = fields[titleIdx].toStdString();

        if (usernameIdx != -1 && usernameIdx < fields.size())
            entry.username = fields[usernameIdx].toStdString();

        if (passwordIdx != -1 && passwordIdx < fields.size())
            entry.password = fields[passwordIdx].toStdString();

        if (urlIdx != -1 && urlIdx < fields.size())
            entry.url = fields[urlIdx].toStdString();

        if (notesIdx != -1 && notesIdx < fields.size())
            entry.notes = fields[notesIdx].toStdString();

        if (categoryIdx != -1 && categoryIdx < fields.size())
            entry.category = fields[categoryIdx].toStdString();

        // Генерируем timestamp если его нет
        if (entry.creation_timestamp.empty()) {
            auto now = std::chrono::system_clock::now();
            auto now_c = std::chrono::system_clock::to_time_t(now);
            entry.creation_timestamp = std::ctime(&now_c);
            // Убираем символ новой строки
            entry.creation_timestamp.pop_back();
        }

        result.entries.push_back(entry);
        result.totalCount++;
    }

    csvFile.close();

    // 7. Очистка
    QDir(tempDir).removeRecursively();
    QFile::remove(scriptPath);

    result.success = !result.entries.empty();
    if (!result.success) {
        result.errorMessage = "No entries found in CSV";
    }

    return result;
}
