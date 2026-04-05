#include "../src/database/DB_helper/db_helper.h"
#include "../src/core/crypto/AES256.h"
#include "../src/core/vault/plaintext_entry.h"
#include "../src/core/vault/VaultManager.h"
#include <sqlite3.h>
#include <vector>
#include <memory>
#include <map>
#include <iostream>
#include <stdexcept>
#include <QApplication>


VaultManager::VaultManager(Database& database, AES256GCM& crypto, KeyManager& key_mgr) :
    db(database), crypto(crypto), key_manager(key_mgr) {}


int VaultManager::createEntry(const PlaintextEntry& entry) {
    sqlite3 *conn = db.getConnection();
    if (!conn) return -1;

    int entry_id = -1;
    int rc;

    // Начинаем транзакцию
    rc = sqlite3_exec(conn, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to begin transaction: " << sqlite3_errmsg(conn) << std::endl;
        db.releaseConnection(conn);
        return -1;
    }

    sqlite3_stmt *stmt;
    const char *sql = R"(
        INSERT INTO vault_entries (encrypted_data, title, username, tags, url)
        VALUES (?, ?, ?, ?, ?)
    )";

    rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
        sqlite3_exec(conn, "ROLLBACK;", nullptr, nullptr, nullptr); // Откат
        db.releaseConnection(conn);
        return -1;
    }

    std::vector<uint8_t> encrypted_data;
    try {
        KeyManager::KeyData d;
        key_manager.get_key(d);
        encrypted_data = crypto.encrypt(d, entry);
    } catch (const std::exception& e) {
        std::cout << "Encryption error: " << e.what() << std::endl;
        sqlite3_finalize(stmt);
        sqlite3_exec(conn, "ROLLBACK;", nullptr, nullptr, nullptr); // Откат
        db.releaseConnection(conn);
        return -1;
    }

    // Биндим параметры
    sqlite3_bind_blob(stmt, 1, encrypted_data.data(),
                      static_cast<int>(encrypted_data.size()), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, entry.title.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, entry.username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, entry.tags.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, entry.url.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) {
        entry_id = sqlite3_last_insert_rowid(conn);
        // Фиксируем транзакцию
        if (sqlite3_exec(conn, "COMMIT;", nullptr, nullptr, nullptr) != SQLITE_OK) {
            std::cerr << "Commit failed: " << sqlite3_errmsg(conn) << std::endl;
            entry_id = -1;
        }
    } else {
        std::cerr << "Insert failed: " << sqlite3_errmsg(conn) << std::endl;
        sqlite3_exec(conn, "ROLLBACK;", nullptr, nullptr, nullptr); // Откат
    }

    sqlite3_finalize(stmt);
    db.releaseConnection(conn);
    return entry_id;
}

std::unique_ptr<PlaintextEntry> VaultManager::getEntry(int entry_id, bool isKeyRotation) {
    sqlite3 *conn = db.getConnection();
    if (!conn) return nullptr;

    std::unique_ptr<PlaintextEntry> result;
    int rc;

    // Транзакция только для чтения
    rc = sqlite3_exec(conn, "BEGIN;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to begin read transaction: " << sqlite3_errmsg(conn) << std::endl;
        db.releaseConnection(conn);
        return nullptr;
    }

    sqlite3_stmt *stmt;
    const char *sql = R"(
        SELECT encrypted_data, title, username, tags, url, created_at, updated_at
        FROM vault_entries
        WHERE rowid = ?
    )";

    rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Prepare select failed: " << sqlite3_errmsg(conn) << std::endl;
        sqlite3_exec(conn, "ROLLBACK;", nullptr, nullptr, nullptr);
        db.releaseConnection(conn);
        return nullptr;
    }

    sqlite3_bind_int(stmt, 1, entry_id);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        // Получаем зашифрованные данные
        const void *encrypted_blob = sqlite3_column_blob(stmt, 0);
        int encrypted_size = sqlite3_column_bytes(stmt, 0);

        std::vector<uint8_t> encrypted_data(
            static_cast<const uint8_t*>(encrypted_blob),
            static_cast<const uint8_t*>(encrypted_blob) + encrypted_size
            );

        try {
            KeyManager::KeyData d;
            if (isKeyRotation)
                key_manager.get_old_key(d);
            else
                key_manager.get_key(d);

            PlaintextEntry decrypted_entry = crypto.decrypt(encrypted_data, d);

            // Заполняем дополнительные поля из БД
            decrypted_entry.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            decrypted_entry.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            const char* tags_col = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

            if (tags_col)
                decrypted_entry.tags = tags_col;

            decrypted_entry.url = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            const char* created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));

            if (created_at)
                decrypted_entry.creation_timestamp = created_at;

            result = std::make_unique<PlaintextEntry>(decrypted_entry);
        } catch (const std::exception& e) {
            std::cout << "Decryption error: " << e.what() << std::endl;
            sqlite3_exec(conn, "ROLLBACK;", nullptr, nullptr, nullptr);
            sqlite3_finalize(stmt);
            db.releaseConnection(conn);
            return nullptr;
        }
    } else if (rc == SQLITE_DONE) {
        std::cerr << "Entry not found: " << entry_id << std::endl;
    } else {
        std::cerr << "Select failed: " << sqlite3_errmsg(conn) << std::endl;
        sqlite3_exec(conn, "ROLLBACK;", nullptr, nullptr, nullptr);
        sqlite3_finalize(stmt);
        db.releaseConnection(conn);
        return nullptr;
    }

    sqlite3_finalize(stmt);
    sqlite3_exec(conn, "COMMIT;", nullptr, nullptr, nullptr); // Завершаем транзакцию
    db.releaseConnection(conn);
    return result;
}

std::vector<PlaintextEntry> VaultManager::getAllEntries(bool rotate)
{
    std::vector<PlaintextEntry> entries;
    sqlite3* conn = db.getConnection();
    if (!conn) return entries;

    int rc;

    // Начинаем транзакцию для чтения
    rc = sqlite3_exec(conn, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to begin transaction: " << sqlite3_errmsg(conn) << std::endl;
        db.releaseConnection(conn);
        return entries;
    }

    sqlite3_stmt* stmt;
    const char* sql = R"(
        SELECT encrypted_data, title, username, tags, url, created_at, updated_at
        FROM vault_entries
        ORDER BY title
    )";

    rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Prepare select failed: " << sqlite3_errmsg(conn) << std::endl;
        sqlite3_exec(conn, "ROLLBACK;", nullptr, nullptr, nullptr);
        db.releaseConnection(conn);
        return entries;
    }

    KeyManager::KeyData d;
    if (rotate)
        key_manager.get_old_key(d);
    else
        key_manager.get_key(d);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        try
        {
            // Получаем зашифрованные данные (колонка 0)
            const void* encrypted_blob = sqlite3_column_blob(stmt, 0);
            int encrypted_size = sqlite3_column_bytes(stmt, 0);

            std::vector<uint8_t> encrypted_data(
                static_cast<const uint8_t*>(encrypted_blob),
                static_cast<const uint8_t*>(encrypted_blob) + encrypted_size
                );

            // Расшифровываем
            PlaintextEntry entry = crypto.decrypt(encrypted_data, d);

            // Заполняем поля из БД (перезаписываем то, что в JSON)
            const char* title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            if (title) entry.title = title;

            const char* username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            if (username) entry.username = username;

            const char* tags = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            if (tags) entry.tags = tags;

            const char* url = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            entry.url = url;

            const char* created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            if (created_at) entry.creation_timestamp = created_at;


            entries.push_back(std::move(entry));
        }
        catch (const std::exception& e)
        {
            std::cerr << "Decryption error: " << e.what() << std::endl;
            // Продолжаем с остальными записями
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_exec(conn, "COMMIT;", nullptr, nullptr, nullptr);
    db.releaseConnection(conn);

    db.addAuditLog("READ_ALL", -1, "All entries accessed");

    return entries;
}

bool VaultManager::updateEntry(int entry_id, const PlaintextEntry& entry)
{
    sqlite3* conn = db.getConnection();
    if (!conn) return false;

    int rc;
    bool success = false;

    // Начинаем транзакцию
    rc = sqlite3_exec(conn, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to begin transaction: " << sqlite3_errmsg(conn) << std::endl;
        db.releaseConnection(conn);
        return false;
    }

    // 1. Шифруем запись
    std::vector<uint8_t> encrypted_data;
    try
    {
        KeyManager::KeyData d;
        key_manager.get_key(d);
        encrypted_data = crypto.encrypt(d, entry);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Encryption error: " << e.what() << std::endl;
        sqlite3_exec(conn, "ROLLBACK;", nullptr, nullptr, nullptr);
        db.releaseConnection(conn);
        return false;
    }

    // 2. Обновляем запись в БД
    // updated_at обновится автоматически через триггер
    sqlite3_stmt* stmt;
    const char* sql = R"(
        UPDATE vault_entries
        SET encrypted_data = ?,
            title = ?,
            username = ?,
            tags = ?,
            url = ?
        WHERE rowid = ?
    )";

    rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Prepare update failed: " << sqlite3_errmsg(conn) << std::endl;
        sqlite3_exec(conn, "ROLLBACK;", nullptr, nullptr, nullptr);
        db.releaseConnection(conn);
        return false;
    }

    // Биндим параметры
    sqlite3_bind_blob(stmt, 1, encrypted_data.data(),
                      static_cast<int>(encrypted_data.size()), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, entry.title.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, entry.username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, entry.tags.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, entry.url.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, entry_id);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE)
    {
        int changes = sqlite3_changes(conn);
        if (changes > 0)
        {
            if (sqlite3_exec(conn, "COMMIT;", nullptr, nullptr, nullptr) == SQLITE_OK)
            {
                success = true;
                db.addAuditLog("UPDATE", entry_id, "Entry updated");
            }
            else
            {
                std::cerr << "Commit failed: " << sqlite3_errmsg(conn) << std::endl;
                sqlite3_exec(conn, "ROLLBACK;", nullptr, nullptr, nullptr);
            }
        }
        else
        {
            std::cerr << "Entry not found: " << entry_id << std::endl;
            sqlite3_exec(conn, "ROLLBACK;", nullptr, nullptr, nullptr);
        }
    }
    else
    {
        std::cerr << "Update failed: " << sqlite3_errmsg(conn) << std::endl;
        sqlite3_exec(conn, "ROLLBACK;", nullptr, nullptr, nullptr);
    }

    sqlite3_finalize(stmt);
    db.releaseConnection(conn);
    return success;
}

bool VaultManager::deleteEntry(int entry_id)
{
    sqlite3* conn = db.getConnection();
    if (!conn) return false;

    int rc;
    bool success = false;

    // Начинаем транзакцию
    rc = sqlite3_exec(conn, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to begin transaction: " << sqlite3_errmsg(conn) << std::endl;
        db.releaseConnection(conn);
        return false;
    }

    sqlite3_stmt* stmt;
    const char* sql = "DELETE FROM vault_entries WHERE rowid = ?";

    rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Prepare delete failed: " << sqlite3_errmsg(conn) << std::endl;
        sqlite3_exec(conn, "ROLLBACK;", nullptr, nullptr, nullptr);
        db.releaseConnection(conn);
        return false;
    }

    sqlite3_bind_int(stmt, 1, entry_id);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE)
    {
        int changes = sqlite3_changes(conn);
        if (changes > 0)
        {
            if (sqlite3_exec(conn, "COMMIT;", nullptr, nullptr, nullptr) == SQLITE_OK)
            {
                success = true;
                db.addAuditLog("DELETE", entry_id, "Entry deleted");
            }
            else
            {
                std::cerr << "Commit failed: " << sqlite3_errmsg(conn) << std::endl;
                sqlite3_exec(conn, "ROLLBACK;", nullptr, nullptr, nullptr);
            }
        }
        else
        {
            std::cerr << "Entry not found: " << entry_id << std::endl;
            sqlite3_exec(conn, "ROLLBACK;", nullptr, nullptr, nullptr);
        }
    }
    else
    {
        std::cerr << "Delete failed: " << sqlite3_errmsg(conn) << std::endl;
        sqlite3_exec(conn, "ROLLBACK;", nullptr, nullptr, nullptr);
    }

    sqlite3_finalize(stmt);
    db.releaseConnection(conn);
    return success;
}


std::vector<VaultManager::EntryMetadata> VaultManager::getAllEntryMetadata()
{
    std::vector<EntryMetadata> result;

    sqlite3* conn = db.getConnection();
    if (!conn) {
        std::cerr << "Failed to get database connection" << std::endl;
        return result;
    }

    sqlite3_stmt* stmt;
    const char* sql = R"(
        SELECT rowid, title, username, url, tags, created_at, updated_at
        FROM vault_entries
        ORDER BY title
    )";

    int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
        db.releaseConnection(conn);
        return result;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        EntryMetadata meta;

        // rowid (всегда есть)
        meta.id = sqlite3_column_int64(stmt, 0);

        // title (NOT NULL)
        const char* title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        meta.title = title ? title : "";

        // username (NOT NULL)
        const char* username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        meta.username = username ? username : "";

        // url (может быть NULL)
        const char* url = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        meta.url = url ? url : "";

        // tags (может быть NULL)
        const char* tags = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        meta.tags = tags ? tags : "";

        // created_at
        const char* created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        meta.created_at = created_at ? created_at : "";

        // updated_at
        const char* updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        meta.updated_at = updated_at ? updated_at : "";

        result.push_back(std::move(meta));
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "Step failed: " << sqlite3_errmsg(conn) << std::endl;
    }

    sqlite3_finalize(stmt);
    db.releaseConnection(conn);

    return result;
}

int VaultManager::getTotalEntriesCount() {
    sqlite3* conn = db.getConnection();
    if (!conn) return -1;

    sqlite3_stmt* stmt;
    int count = 0;

    const char* sql = "SELECT COUNT(*) FROM vault_entries";

    if (sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    db.releaseConnection(conn);
    return count;
}

bool VaultManager::rotate()
{
    sqlite3* conn = db.getConnection();
    if (!conn) return false;

    // Получаем старый и новый ключи
    KeyManager::KeyData old_key, new_key;
    key_manager.get_key(new_key);
    key_manager.get_old_key(old_key);

    std::cout << "NEW KEY size: " << new_key.size << std::endl;
    std::cout << "NEW KEY data: ";
    for (size_t i = 0; i < new_key.size; i++) {
        std::cout << std::hex << (int)new_key.data[i] << " ";
    }
    std::cout << std::dec << std::endl;

    std::cout << "OLD KEY size: " << old_key.size << std::endl;
    std::cout << "OLD KEY data: ";
    for (size_t i = 0; i < old_key.size; i++) {
        std::cout << std::hex << (int)old_key.data[i] << " ";
    }
    std::cout << std::dec << std::endl;

    // Получаем все rowid
    std::vector<int> all_ids;
    sqlite3_stmt* id_stmt;
    const char* id_sql = "SELECT rowid FROM vault_entries";

    if (sqlite3_prepare_v2(conn, id_sql, -1, &id_stmt, nullptr) != SQLITE_OK) {
        db.releaseConnection(conn);
        return false;
    }

    while (sqlite3_step(id_stmt) == SQLITE_ROW) {
        all_ids.push_back(sqlite3_column_int(id_stmt, 0));
    }
    sqlite3_finalize(id_stmt);

    int total = all_ids.size();
    if (total == 0) {
        db.releaseConnection(conn);
        return true;
    }

    // Начинаем одну большую транзакцию
    sqlite3_exec(conn, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr);

    sqlite3_stmt* select_stmt;
    sqlite3_stmt* update_stmt;

    const char* select_sql = "SELECT encrypted_data FROM vault_entries WHERE rowid = ?";
    const char* update_sql = "UPDATE vault_entries SET encrypted_data = ?, updated_at = CURRENT_TIMESTAMP WHERE rowid = ?";

    sqlite3_prepare_v2(conn, select_sql, -1, &select_stmt, nullptr);
    sqlite3_prepare_v2(conn, update_sql, -1, &update_stmt, nullptr);

    bool success = true;
    int processed = 0;

    for (int i = 0; i < all_ids.size(); i++)
    {
        // Читаем зашифрованные данные
        sqlite3_bind_int(select_stmt, 1, all_ids[i]);

        if (sqlite3_step(select_stmt) != SQLITE_ROW) {
            std::cerr << "Failed to read entry " << all_ids[i] << std::endl;
            success = false;
            break;
        }

        const void* encrypted_blob = sqlite3_column_blob(select_stmt, 0);
        int encrypted_size = sqlite3_column_bytes(select_stmt, 0);

        std::vector<uint8_t> encrypted_data(
            static_cast<const uint8_t*>(encrypted_blob),
            static_cast<const uint8_t*>(encrypted_blob) + encrypted_size
            );

        sqlite3_reset(select_stmt);

        try {
            // Расшифровываем старым ключом
            PlaintextEntry entry = crypto.decrypt(encrypted_data, old_key);

            // Шифруем новым ключом
            std::vector<uint8_t> new_encrypted = crypto.encrypt(new_key, entry);

            // Обновляем в БД
            sqlite3_bind_blob(update_stmt, 1, new_encrypted.data(), new_encrypted.size(), SQLITE_STATIC);
            sqlite3_bind_int(update_stmt, 2, all_ids[i]);

            if (sqlite3_step(update_stmt) != SQLITE_DONE) {
                throw std::runtime_error("Update failed");
            }

            sqlite3_reset(update_stmt);
            processed++;

        } catch (const std::exception& e) {
            std::cerr << "Error processing entry " << all_ids[i] << ": " << e.what() << std::endl;
            success = false;
            break;
        }

        if (i % 10 == 0) {
            QCoreApplication::processEvents();
        }
    }

    // Финализируем запросы
    sqlite3_finalize(select_stmt);
    sqlite3_finalize(update_stmt);

    if (success) {
        sqlite3_exec(conn, "COMMIT;", nullptr, nullptr, nullptr);
        key_manager.getInstance().zero_keyData(old_key);
    } else {
        sqlite3_exec(conn, "ROLLBACK;", nullptr, nullptr, nullptr);
    }

    db.releaseConnection(conn);
    return success;
}
