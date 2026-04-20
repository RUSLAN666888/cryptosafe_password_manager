// log_verifier.h
#ifndef LOG_VERIFIER_H
#define LOG_VERIFIER_H

#include <string>
#include <vector>
#include "../database/DB_helper/db_helper.h"

class LogVerifier {
public:
    struct VerificationResult {
        bool isValid;
        int failedSequence;
        std::string errorMessage;
        std::vector<int> tamperedEntries;
        bool hashChainValid;
        bool signaturesValid;
        int verifiedCount;
        bool seqValid;
    };

    static LogVerifier& getInstance() {
        static LogVerifier instance;
        return instance;
    }

    void init(Database* db);

    VerificationResult verifyAllLogs();

    bool startupVerification();


private:
    LogVerifier() = default;
    Database* m_db = nullptr;

    bool verifySignature(const std::string& entry_data,
                         const std::string& previous_hash,
                         std::vector<uint8_t>& signature,
                         int sequenceNumber);

    std::string computeHash(const std::string& entry_data,
                            const std::string& previous_hash);
};

#endif
