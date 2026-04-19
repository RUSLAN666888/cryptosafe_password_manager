#include "../src/core/audit/audit_logger/audit_logger.h"
#include "../src/core/audit/log_signer/log_signer.h"
#include "../src/core/LogEntry.h"
#include "../src/database/DB_helper/db_helper.h"

#include <string>
#include "../src/core/events.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;


void AuditLogger::init(Database& db)
{
    // Подписываемся на событие добавления записи
    eventBus.subscribe(EventType::EntryAdded,
                       [this](const Event& event) {
                           onEntryAdded(event);
                       });

    m_db = &db;
}

void AuditLogger::onEntryAdded(const Event& event)
{
    LogEntry logEntry;
    logEntry.user_id = 1;
    logEntry.type = EventType::EntryAdded;
    logEntry.source = event.source;
    logEntry.timestamp = getUTCTimestamp();
    logEntry.severity = Severity::INFO;

    // Получаем details из события
    if (event.hasData()) {
        json details = event.getData<json>();
        logEntry.details = details;

        // Извлекаем entry_id из details
        if (logEntry.details.contains("entry_id")) {
            logEntry.entry_id = logEntry.details["entry_id"].get<int>();
        }
    }

    json j = to_json(logEntry);

    int count = m_db->getLogEntryCount();
    std::string previous_hash = m_db->getLastEntryHash();

    std::string hash = LogSigner::getInstance().getHash(logEntry, previous_hash);
    std::string signature = LogSigner::getInstance().sign(logEntry);

    m_db->addLogEntry(previous_hash, hash, j.dump(), signature, 1, EventType::EntryAdded);
}
