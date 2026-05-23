#ifndef LOG_SIGNER_H
#define LOG_SIGNER_H

#include <string>
#include <cstdint>
#include <vector>
#include <nlohmann/json.hpp>

#include "key_manager.h"
#include "key_derivation.h"
#include "LogEntry.h"

using json = nlohmann::json;

class LogSigner {
public:
    // Возвращает единственный экземпляр
    static LogSigner& getInstance();

    // Инициализация из мастер-пароля (только указатель + длина)
    void initFromMasterPassword(const char* password, size_t password_len);

    // Подпись записи
    std::vector<uint8_t> sign(LogEntry entry);

    // Получение хеша записи
    std::string getHash(LogEntry entry, std::string previous_hash = "");

    // Получение публичного ключа
    std::vector<uint8_t> get_public_key() const { return m_public_key; }

    // Запрещаем копирование и присваивание
    LogSigner(const LogSigner&) = delete;
    LogSigner& operator=(const LogSigner&) = delete;

private:
    LogSigner() = default;

    std::vector<uint8_t> m_public_key;
};

#endif
