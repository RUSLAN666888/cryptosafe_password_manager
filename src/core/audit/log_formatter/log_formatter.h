#ifndef LOG_FORMATTER_H
#define LOG_FORMATTER_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "../core/audit/log_verifier/log_verifier.h"
#include "../src/database/DB_helper/db_helper.h"


class LogFormatter{
    std::vector<uint8_t> hexToBytes(const std::string& hex) {
        std::vector<uint8_t> bytes;
        for (size_t i = 0; i < hex.length(); i += 2) {
            std::string byteString = hex.substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(strtol(byteString.c_str(), nullptr, 16));
            bytes.push_back(byte);
        }
        return bytes;
    }

    Database* m_db;

public:
    struct ImportResult{
        std::string msg;
        bool isValid;
        int failedSeq;
    };

    static LogFormatter& getInstance(){
        static LogFormatter instance;
        return instance;
    }

    void initDatabase(Database* db){m_db = db;}

    LogFormatter::ImportResult importJSON(std::string file_path);
    void exportCSV(const std::string& file_path);
    void exportJSON(const std::string& file_path);

};

#endif
