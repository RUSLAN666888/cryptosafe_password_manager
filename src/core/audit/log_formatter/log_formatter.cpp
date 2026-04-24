#include "../src/core/audit/log_formatter/log_formatter.h"
#include "../src/core/audit/log_verifier/log_verifier.h"
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <string>
#include <cstdint>
#include "../src/core/LogEntry.h"


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


        // Восстанавливаем LogEntry
        LogEntry entry;
        entry.user_id = entry_json["user_id"];
        entry.type = static_cast<EventType>(entry_json["event_type"].get<int>());
        entry.source = entry_json["source"];
        entry.timestamp = entry_json["timestamp"];
        entry.severity = static_cast<Severity>(entry_json["severity"].get<int>());
        entry.entry_id = entry_json["entry_id"];
        entry.details = entry_json["details"];

        // Используем ту же функцию, что и при подписи
        std::string signed_data = to_json(entry).dump();

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

