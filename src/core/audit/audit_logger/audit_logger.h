#ifndef AUDIT_LOGGER_H
#define AUDIT_LOGGER_H

#include "../src/core/events.h"
#include "../src/database/DB_helper/db_helper.h"


class AuditLogger{
public:
    void init(Database& db);

    void onEntryAdded(const Event& event);
    void onEntryUpdated(const Event& event);
    void onEntryDeleted(const Event& event);
    void onLoginSuccess(const Event& event);
    void onLoginFailure(const Event& event);
    void onLock(const Event& event);
    void onUnlock(const Event& event);
    void onStartup(const Event& event);
    void onShutdown(const Event& event);
    void onClipboardCleared(const Event& event);
    void onInactivityTimeout(const Event& event);
    void onClipboardCopied(const Event& event);

    Database* m_db = nullptr;

    static AuditLogger& getInstance() {
        static AuditLogger instance;
        return instance;
    }
};

#endif // AUDIT_LOGGER_H
