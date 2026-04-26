#include "../src/core/audit/log_formatter/log_formatter.h"
#include "../src/core/audit/log_verifier/log_verifier.h"
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <string>
#include <cstdint>
#include "../src/core/LogEntry.h"
#include "../src/database/DB_helper/db_helper.h"


using json = nlohmann::json;




LogFormatter::ImportResult LogFormatter::importJSON(std::string file_path){
    std::ifstream in(file_path);
    if (!in.is_open())
        throw std::runtime_error("Opening file error");

    std::string md = "";
    std::getline(in, md);
    json metadata = json::parse(md);

    json entry_json;
    std::string current_line = "";

    int seq = 0;

    LogFormatter::ImportResult r;
    r.msg = "";
    r.isValid = true;
    r.failedSeq = 0;

    while(std::getline(in, current_line)){
        entry_json = json::parse(current_line);


        // Используем ту же функцию, что и при подписи
        std::string signed_data = entry_json["signed_data"];

        std::string current_hash = entry_json["current_hash"].get<std::string>();
        std::string previous_hash = entry_json["previous_hash"].get<std::string>();
        std::string signature = entry_json["signature"].get<std::string>();
        std::string public_key = entry_json["public_key"].get<std::string>();

        std::vector<uint8_t> sig = hexToBytes(signature);
        std::vector<uint8_t> pk = hexToBytes(public_key);

        //std::string LogVerifier::verifyImportedEntry(const std::string& entry_data, const std::string& previous_hash, const std::string& current_hash,
                                                     //std::vector<uint8_t>& signature, std::vector<uint8_t>& public_key)

        r.msg = LogVerifier::getInstance().verifyImportedEntry(signed_data, previous_hash, current_hash, sig, pk);

        if (!r.msg.empty()){
            r.isValid = false;
            return r;
        }
    }

    return r;
}

void LogFormatter::exportCSV(const std::string& file_path){
    // Открываем файл
    std::ofstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + file_path);
    }

    // Заголовки
    file << "#,Дата/Время,Тип события,Уровень,User ID,Источник,Entry ID,Данные\n";

    // Параметры для пагинации
    int offset = 0;
    int limit = 100;
    std::string emptyFilter = "";
    std::string sortColumn = "sequence_number";
    bool sortOrder = true;  // ASC

    while (true) {
        // Загружаем порцию из 100 записей
        std::vector<AuditEntryDisplay> entries = m_db->getAuditPage(
            offset, limit,
            sortColumn, sortOrder,
            emptyFilter, "", "", ""
            );

        // Если записей нет - выходим
        if (entries.empty()) {
            break;
        }

        // Записываем порцию в файл
        for (const auto& entry : entries) {
            file << entry.sequence_number << ","
                 << entry.created_at << ","
                 << entry.event_type << ","
                 << entry.severity << ","
                 << entry.user_id << ","
                 << entry.source << ","
                 << entry.entry_id << ","
                 << "\"" << entry.entry_data << "\"\n";
        }

        // Переходим к следующей порции
        offset += limit;

        // Если загрузили меньше, чем limit - это последняя порция
        if (entries.size() < static_cast<size_t>(limit)) {
            break;
        }
    }

    file.close();
}

void LogFormatter::exportJSON(const std::string& file_path){
    std::ofstream out(file_path);

    std::stringstream ss;

    int totalCount = m_db->getLogEntryCount();

    json metadata;
    metadata["export_timestamp"] = getUTCTimestamp();
    metadata["exporter"] = "CryptoSafe Manager";
    metadata["entry_count"] = totalCount;
    metadata["algorithm"] = "Ed25519";

    ss << metadata.dump() << "\n";
    out.write(ss.str().c_str(), ss.str().size());

    ss.str("");


    for (int seq = 1; seq <= totalCount; ++seq){
        std::string previous_hash, current_hash, entry_data, created_at, event_type_str;
        std::vector<uint8_t> signature;
        int key_version;

        // Получаем запись из БД по sequence_number
        m_db->getLogEntry(seq, previous_hash, current_hash, entry_data,
                          signature, key_version, created_at, event_type_str);


        // Получаем публичный ключ для этой записи
        std::vector<uint8_t> publicKey;
        m_db->getPublicKeyForSequence(seq, publicKey, key_version);

        // Преобразуем публичный ключ в hex
        std::stringstream pubKeySs;
        for (uint8_t byte : publicKey) {
            pubKeySs << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
        }

        json entryJson = json::parse(entry_data);

        json exportEntry;


        exportEntry["signed_data"] = entry_data;

        exportEntry["previous_hash"] = previous_hash;
        exportEntry["current_hash"] = current_hash;

        std::stringstream sigSs;
        for (uint8_t byte : signature) {
            sigSs << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
        }

        exportEntry["signature"] = sigSs.str();

        exportEntry["public_key"] = pubKeySs.str();

        ss << exportEntry.dump() << "\n";
        out.write(ss.str().c_str(), ss.str().size());

        ss.str("");
    }

    out.close();
}
