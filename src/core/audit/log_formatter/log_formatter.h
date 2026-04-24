#ifndef LOG_FORMATTER_H
#define LOG_FORMATTER_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "../core/audit/log_verifier/log_verifier.h"
#include "../gui/dialogs/audit_dialog//audit_log_model.h"


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

    LogFormatter::ImportResult importJSON(std::string file_path);

};

#endif
