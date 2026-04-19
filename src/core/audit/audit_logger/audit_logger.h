#ifndef AUDIT_LOGGER_H
#define AUDIT_LOGGER_H

#include "../src/core/events.h"
#include "../src/database/DB_helper/db_helper.h"


class AuditLogger{
public:
    void init(Database& db);
    void onEntryAdded(const Event& event);

    Database* m_db = nullptr;

    static AuditLogger& getInstance() {
        static AuditLogger instance;  // потокобезопасно с C++11
        return instance;
    }
};

#endif // AUDIT_LOGGER_H
