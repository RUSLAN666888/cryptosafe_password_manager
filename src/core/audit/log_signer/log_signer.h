#ifndef LOG_SIGNER_H
#define LOG_SIGNER_H

#include <string>
#include <cstdint>
#include <vector>

#include "../src/core/LogEntry.h"

using json = nlohmann::json;

class LogSigner{
public:
    std::vector<uint8_t> m_public_key;

    void initFromMasterPassword(const std::string& password);
    std::string sign(LogEntry entry);
    std::string getHash(LogEntry entry, std::string previous_hash = "");

    std::vector<uint8_t> get_public_key(){return m_public_key;}

    static LogSigner& getInstance() {
        static LogSigner instance;
        return instance;
    }
};


#endif // LOG_SIGNER_H
