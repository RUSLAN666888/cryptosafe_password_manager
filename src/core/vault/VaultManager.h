#ifndef VAULTMANAGER_H
#define VAULTMANAGER_H

#include "../src/database/DB_helper/db_helper.h"
#include "../src/core/crypto/AES256.h"
#include "../src/core/vault/plaintext_entry.h"
#include <vector>
#include <memory>
#include <queue>
#include <mutex>
#include <atomic>

class VaultManager{
    Database& db;
    AES256GCM& crypto;
    KeyManager& key_manager;

    // Для миграции ключа
    std::mutex migration_mutex;
    std::atomic<bool> is_migrating{false};

    // Очередь отложенных операций
    struct PendingOperation
    {
        enum Type { CREATE, UPDATE, DELETE };
        Type type;
        int entry_id;
        PlaintextEntry entry;
    };
    std::queue<PendingOperation> pending_operations;
    std::mutex queue_mutex;

public:
    VaultManager(Database& database, AES256GCM& crypto, KeyManager& key_mgr);

    int createEntry(const PlaintextEntry& entry);
    std::unique_ptr<PlaintextEntry> getEntry(int entry_id);
    std::vector<PlaintextEntry> getAllEntries();
    bool updateEntry(int entry_id, const PlaintextEntry& entry);
    bool deleteEntry(int entry_id);

    bool changeMasterPassword(const std::string& old_key);

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
};

#endif // VAULTMANAGER_H
