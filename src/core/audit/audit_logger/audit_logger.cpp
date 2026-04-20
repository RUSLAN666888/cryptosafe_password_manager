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
    eventBus.subscribe(EventType::EntryAdded,
                       [this](const Event& event) { onEntryAdded(event); });
    eventBus.subscribe(EventType::EntryUpdated,
                       [this](const Event& event) { onEntryUpdated(event); });
    eventBus.subscribe(EventType::EntryDeleted,
                       [this](const Event& event) { onEntryDeleted(event); });
    eventBus.subscribe(EventType::UserLoggedIn,
                       [this](const Event& event) { onLoginSuccess(event); });
    eventBus.subscribe(EventType::LoginFailure,
                       [this](const Event& event) { onLoginFailure(event); });
    eventBus.subscribe(EventType::ClipboardCopied,
                       [this](const Event& event) { onClipboardCopied(event); });
    eventBus.subscribe(EventType::ClipboardCleared,
                       [this](const Event& event) { onClipboardCleared(event); });
    eventBus.subscribe(EventType::Lock,
                       [this](const Event& event) { onLock(event); });
    eventBus.subscribe(EventType::Unlock,
                       [this](const Event& event) { onUnlock(event); });
    eventBus.subscribe(EventType::Startup,
                       [this](const Event& event) { onStartup(event); });
    eventBus.subscribe(EventType::Shutdown,
                       [this](const Event& event) { onShutdown(event); });
    eventBus.subscribe(EventType::InactivityTimeout,
                       [this](const Event& event) { onInactivityTimeout(event); });
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

    std::string j = to_json(logEntry).dump();

    int count = m_db->getLogEntryCount();
    std::string previous_hash = m_db->getLastEntryHash();

    std::string hash = LogSigner::getInstance().getHash(logEntry, previous_hash);
    std::vector<uint8_t> signature = LogSigner::getInstance().sign(logEntry);

    m_db->addLogEntry(previous_hash, hash, j, signature, 1, EventType::EntryAdded);
}

void AuditLogger::onEntryUpdated(const Event& event)
{
    LogEntry logEntry;
    logEntry.user_id = 1;
    logEntry.type = EventType::EntryUpdated;
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

    std::string j = to_json(logEntry).dump();

    std::string previous_hash = m_db->getLastEntryHash();

    std::string hash = LogSigner::getInstance().getHash(logEntry, previous_hash);
    std::vector<uint8_t> signature = LogSigner::getInstance().sign(logEntry);

    m_db->addLogEntry(previous_hash, hash, j, signature, 1, EventType::EntryUpdated);
}

void AuditLogger::onEntryDeleted(const Event& event)
{
    LogEntry logEntry;
    logEntry.user_id = 1;
    logEntry.type = EventType::EntryDeleted;
    logEntry.source = event.source;
    logEntry.timestamp = getUTCTimestamp();
    logEntry.severity = Severity::WARN;

    // Получаем details из события
    if (event.hasData()) {
        json details = event.getData<json>();
        logEntry.details = details;

        // Извлекаем entry_id из details
        if (logEntry.details.contains("entry_id")) {
            logEntry.entry_id = logEntry.details["entry_id"].get<int>();
        }
    }

    std::string j = to_json(logEntry).dump();

    std::string previous_hash = m_db->getLastEntryHash();

    std::string hash = LogSigner::getInstance().getHash(logEntry, previous_hash);
    std::vector<uint8_t> signature = LogSigner::getInstance().sign(logEntry);

    m_db->addLogEntry(previous_hash, hash, j, signature, 1, EventType::EntryDeleted);
}

void AuditLogger::onLoginSuccess(const Event& event){
    LogEntry logEntry;
    logEntry.user_id = 1;
    logEntry.type = EventType::UserLoggedIn;
    logEntry.source = event.source;
    logEntry.timestamp = getUTCTimestamp();
    logEntry.severity = Severity::INFO;


    if (event.hasData()) {
        json details = event.getData<json>();
        logEntry.details = details;

        // Извлекаем entry_id из details
        if (logEntry.details.contains("entry_id")) {
            logEntry.entry_id = logEntry.details["entry_id"].get<int>();
        }
        else{
            logEntry.entry_id = -1;
        }
    }

    std::string j = to_json(logEntry).dump();

    std::string previous_hash = m_db->getLastEntryHash();

    std::string hash = LogSigner::getInstance().getHash(logEntry, previous_hash);
    std::vector<uint8_t> signature = LogSigner::getInstance().sign(logEntry);

    m_db->addLogEntry(previous_hash, hash, j, signature, 1, EventType::UserLoggedIn);
}

void AuditLogger::onLoginFailure(const Event& event){
    LogEntry logEntry;
    logEntry.user_id = 1;
    logEntry.type = EventType::LoginFailure;
    logEntry.source = event.source;
    logEntry.timestamp = getUTCTimestamp();
    logEntry.severity = Severity::WARN;


    if (event.hasData()) {
        json details = event.getData<json>();
        logEntry.details = details;

        // Извлекаем entry_id из details
        if (logEntry.details.contains("entry_id")) {
            logEntry.entry_id = logEntry.details["entry_id"].get<int>();
        }
        else{
            logEntry.entry_id = -1;
        }
    }

    std::string j = to_json(logEntry).dump();

    std::string previous_hash = m_db->getLastEntryHash();

    std::string hash = LogSigner::getInstance().getHash(logEntry, previous_hash);
    std::vector<uint8_t> signature = LogSigner::getInstance().sign(logEntry);

    m_db->addLogEntry(previous_hash, hash, j, signature, 1, EventType::LoginFailure);
}


void AuditLogger::onLock(const Event& event){
    LogEntry logEntry;
    logEntry.user_id = 1;
    logEntry.type = EventType::Lock;
    logEntry.source = event.source;
    logEntry.timestamp = getUTCTimestamp();
    logEntry.severity = Severity::INFO;


    if (event.hasData()) {
        json details = event.getData<json>();
        logEntry.details = details;

        // Извлекаем entry_id из details
        if (logEntry.details.contains("entry_id")) {
            logEntry.entry_id = logEntry.details["entry_id"].get<int>();
        }
        else{
            logEntry.entry_id = -1;
        }
    }

    std::string j = to_json(logEntry).dump();

    std::string previous_hash = m_db->getLastEntryHash();

    std::string hash = LogSigner::getInstance().getHash(logEntry, previous_hash);
    std::vector<uint8_t> signature = LogSigner::getInstance().sign(logEntry);

    m_db->addLogEntry(previous_hash, hash, j, signature, 1, EventType::Lock);
}

void AuditLogger::onUnlock(const Event& event){
    LogEntry logEntry;
    logEntry.user_id = 1;
    logEntry.type = EventType::Unlock;
    logEntry.source = event.source;
    logEntry.timestamp = getUTCTimestamp();
    logEntry.severity = Severity::INFO;


    if (event.hasData()) {
        json details = event.getData<json>();
        logEntry.details = details;

        // Извлекаем entry_id из details
        if (logEntry.details.contains("entry_id")) {
            logEntry.entry_id = logEntry.details["entry_id"].get<int>();
        }
        else{
            logEntry.entry_id = -1;
        }
    }

    std::string j = to_json(logEntry).dump();

    std::string previous_hash = m_db->getLastEntryHash();

    std::string hash = LogSigner::getInstance().getHash(logEntry, previous_hash);
    std::vector<uint8_t> signature = LogSigner::getInstance().sign(logEntry);

    m_db->addLogEntry(previous_hash, hash, j, signature, 1, EventType::Unlock);
}

void AuditLogger::onStartup(const Event& event){
    LogEntry logEntry;
    logEntry.user_id = 1;
    logEntry.type = EventType::Startup;
    logEntry.source = event.source;
    logEntry.timestamp = getUTCTimestamp();
    logEntry.severity = Severity::INFO;


    if (event.hasData()) {
        json details = event.getData<json>();
        logEntry.details = details;

        // Извлекаем entry_id из details
        if (logEntry.details.contains("entry_id")) {
            logEntry.entry_id = logEntry.details["entry_id"].get<int>();
        }
        else{
            logEntry.entry_id = -1;
        }
    }

    std::string j = to_json(logEntry).dump();

    std::string previous_hash = m_db->getLastEntryHash();

    std::string hash = LogSigner::getInstance().getHash(logEntry, previous_hash);
    std::vector<uint8_t> signature = LogSigner::getInstance().sign(logEntry);

    m_db->addLogEntry(previous_hash, hash, j, signature, 1, EventType::Startup);
}

void AuditLogger::onShutdown(const Event& event){
    LogEntry logEntry;
    logEntry.user_id = 1;
    logEntry.type = EventType::Shutdown;
    logEntry.source = event.source;
    logEntry.timestamp = getUTCTimestamp();
    logEntry.severity = Severity::INFO;


    if (event.hasData()) {
        json details = event.getData<json>();
        logEntry.details = details;

        // Извлекаем entry_id из details
        if (logEntry.details.contains("entry_id")) {
            logEntry.entry_id = logEntry.details["entry_id"].get<int>();
        }
        else{
            logEntry.entry_id = -1;
        }
    }

    std::string j = to_json(logEntry).dump();

    std::string previous_hash = m_db->getLastEntryHash();

    std::string hash = LogSigner::getInstance().getHash(logEntry, previous_hash);
    std::vector<uint8_t> signature = LogSigner::getInstance().sign(logEntry);

    m_db->addLogEntry(previous_hash, hash, j, signature, 1, EventType::Shutdown);
}

void AuditLogger::onClipboardCleared(const Event& event){
    LogEntry logEntry;
    logEntry.user_id = 1;
    logEntry.type = EventType::ClipboardCleared;
    logEntry.source = event.source;
    logEntry.timestamp = getUTCTimestamp();
    logEntry.severity = Severity::INFO;


    if (event.hasData()) {
        json details = event.getData<json>();
        logEntry.details = details;

        // Извлекаем entry_id из details
        if (logEntry.details.contains("entry_id")) {
            logEntry.entry_id = logEntry.details["entry_id"].get<int>();
        }
        else{
            logEntry.entry_id = -1;
        }
    }

    std::string j = to_json(logEntry).dump();

    std::string previous_hash = m_db->getLastEntryHash();

    std::string hash = LogSigner::getInstance().getHash(logEntry, previous_hash);
    std::vector<uint8_t> signature = LogSigner::getInstance().sign(logEntry);

    m_db->addLogEntry(previous_hash, hash, j, signature, 1, EventType::ClipboardCleared);
}

void AuditLogger::onInactivityTimeout(const Event& event){
    LogEntry logEntry;
    logEntry.user_id = 1;
    logEntry.type = EventType::InactivityTimeout;
    logEntry.source = event.source;
    logEntry.timestamp = getUTCTimestamp();
    logEntry.severity = Severity::WARN;


    if (event.hasData()) {
        json details = event.getData<json>();
        logEntry.details = details;

        // Извлекаем entry_id из details
        if (logEntry.details.contains("entry_id")) {
            logEntry.entry_id = logEntry.details["entry_id"].get<int>();
        }
        else{
            logEntry.entry_id = -1;
        }
    }

    std::string j = to_json(logEntry).dump();

    std::string previous_hash = m_db->getLastEntryHash();

    std::string hash = LogSigner::getInstance().getHash(logEntry, previous_hash);
    std::vector<uint8_t> signature = LogSigner::getInstance().sign(logEntry);

    m_db->addLogEntry(previous_hash, hash, j, signature, 1, EventType::InactivityTimeout);
}

void AuditLogger::onClipboardCopied(const Event& event)
{
    LogEntry logEntry;
    logEntry.user_id = 1;
    logEntry.type = EventType::ClipboardCopied;
    logEntry.source = event.source;
    logEntry.timestamp = getUTCTimestamp();
    logEntry.severity = Severity::INFO;

    // Получаем details из события
    if (event.hasData()) {
        json details = event.getData<json>();
        logEntry.details = details;

        // Извлекаем entry_id и type из details
        if (logEntry.details.contains("entry_id")) {
            logEntry.entry_id = logEntry.details["entry_id"].get<int>();
        }
    }

    std::string j = to_json(logEntry).dump();

    std::string previous_hash = m_db->getLastEntryHash();

    std::string hash = LogSigner::getInstance().getHash(logEntry, previous_hash);
    std::vector<uint8_t> signature = LogSigner::getInstance().sign(logEntry);

    m_db->addLogEntry(previous_hash, hash, j, signature, 1, EventType::ClipboardCopied);
}
