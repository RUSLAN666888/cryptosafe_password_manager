#ifndef VAULTMANAGER_H
#define VAULTMANAGER_H

#include "../src/database/DB_helper/db_helper.h"
#include "../src/core/crypto/AES256.h"
#include "../src/core/vault/plaintext_entry.h"
#include <vector>
#include <memory>
#include <map>


class VaultManager{
    Database& db;
    AES256GCM& crypto;
    KeyManager& key_manager;

public:
    VaultManager(Database& database, AES256GCM& crypto, KeyManager& key_mgr);

    int createEntry(const PlaintextEntry& entry);
    std::unique_ptr<PlaintextEntry> getEntry(int entry_id);
    std::vector<PlaintextEntry> getAllEntries();
    bool updateEntry(int entry_id, const PlaintextEntry& entry);
    bool deleteEntry(int entry_id);
};

#endif // VAULTMANAGER_H
