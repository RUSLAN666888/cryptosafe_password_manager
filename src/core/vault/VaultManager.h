#ifndef VAULTMANAGER_H
#define VAULTMANAGER_H

#include "db_helper.h"
#include "plaintext_entry.h"
#include <vector>
#include <memory>
#include <mutex>

class VaultManager{
    Database& db;

    // Для миграции ключа
    std::mutex key_rotation_mutex;

public:
    VaultManager(Database& database);

    int createEntry(const PlaintextEntry& entry);
    std::unique_ptr<PlaintextEntry> getEntry(int entry_id, bool isKeyRotation = false);
    std::vector<PlaintextEntry> getAllEntries(bool isKeyRotation = false);
    bool updateEntry(int entry_id, const PlaintextEntry& entry);
    bool deleteEntry(int entry_id);

    bool rotate();

    struct EntryMetadata {
        long id;
        std::string title;
        std::string username;
        std::string url;
        std::string tags;
        std::string created_at;
        std::string updated_at;
    };

    std::vector<EntryMetadata> getAllEntryMetadata();
    int getTotalEntriesCount();
};

#endif // VAULTMANAGER_H
