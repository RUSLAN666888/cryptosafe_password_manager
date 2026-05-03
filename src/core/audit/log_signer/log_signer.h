#ifndef LOG_SIGNER_H
#define LOG_SIGNER_H

#include <string>
#include <cstdint>
#include <vector>

#include "key_manager.h"
#include "key_derivation.h"
#include "LogEntry.h"
#include "Ed25519.h"
#include "Serializer.h"
#include "sha256.h"

using json = nlohmann::json;

class LogSigner{
public:
    std::vector<uint8_t> m_public_key;

    void initFromMasterPassword(const std::string& password) {
        // Выводим приватный ключ из пароля
        std::vector<uint8_t> private_key;
        derive_private_sign_key(password, private_key,
                                {0x43, 0x72, 0x79, 0x70, 0x74, 0x6f, 0x53, 0x61, 0x66, 0x65, 0x5f, 0x41, 0x75, 0x64, 0x69, 0x74},
                                "audit-signing");

        // Сохраняем приватный ключ в KeyManager
        KeyManager::getInstance().storeLogSignKey(private_key);

        // Вычисляем и сохраняем публичный ключ
        m_public_key = derivePublicKey("logSign");
    }
    std::vector<uint8_t> sign(LogEntry entry){
        auto data = Serializer::serialize(entry);
        return sign_data("logSign", data);
    }
    std::string getHash(LogEntry entry, std::string previous_hash = ""){
        std::string data = to_json(entry).dump() + previous_hash;

        std::vector<uint8_t> digest = sha256(std::vector<uint8_t>(data.begin(), data.end()));


        std::stringstream ss;
        for (uint8_t byte : digest) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
        }
        return ss.str();
    }

    std::vector<uint8_t> get_public_key(){return m_public_key;}

    static LogSigner& getInstance() {
        static LogSigner instance;
        return instance;
    }

};


#endif // LOG_SIGNER_H
