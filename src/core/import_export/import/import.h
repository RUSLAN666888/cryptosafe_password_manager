// importer.h
#ifndef IMPORTER_H
#define IMPORTER_H

#include <vector>
#include <string>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QByteArray>
#include <QDebug>

#include <nlohmann/json.hpp>
#include "../vault/plaintext_entry.h"
#include "Serializer.h"
#include "aes_gcm.h"
#include "key_derivation.h"
#include "key_manager.h"
#include "db_helper.h"
#include "base64.h"
#include "Ed25519.h"

using json = nlohmann::json;

struct ImportResult {
    bool success;
    std::string errorMessage;
    std::vector<PlaintextEntry> entries;
    int totalCount;
    int duplicateCount;
    int sanitizedCount;
};

class Importer {
public:
    // Импорт из Encrypted JSON
    ImportResult importFromEncryptedJSON(const QString& filepath, const std::string& password);

    // Импорт из CSV
    ImportResult importFromCSV(const QString& filepath);

private:
    std::string sanitize(const std::string& input);
    bool containsMaliciousContent(const std::string& input);
};

#endif
