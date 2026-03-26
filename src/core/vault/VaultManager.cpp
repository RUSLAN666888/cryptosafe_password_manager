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
        INSERT INTO vault_entries (encrypted_data, title, username, tags)
        VALUES (?, ?, ?, ?)
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

std::unique_ptr<PlaintextEntry> VaultManager::getEntry(int entry_id) {
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
        SELECT encrypted_data, title, username, tags, created_at, updated_at
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
            key_manager.get_key(d);
            PlaintextEntry decrypted_entry = crypto.decrypt(encrypted_data, d);

            // Заполняем дополнительные поля из БД
            decrypted_entry.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            decrypted_entry.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            const char* tags_col = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            if (tags_col) decrypted_entry.tags = tags_col;

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

std::vector<PlaintextEntry> VaultManager::getAllEntries()
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
        SELECT encrypted_data, title, username, tags, created_at, updated_at
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

            const char* created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            if (created_at) entry.creation_timestamp = created_at;

            // updated_at (колонка 5) можно добавить в структуру при необходимости

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
            tags = ?
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
    sqlite3_bind_int(stmt, 5, entry_id);

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
