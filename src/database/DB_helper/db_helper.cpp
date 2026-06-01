#include "db_helper.h"
#include "../src/database/DBSchema.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sqlite3.h>
#include <thread>
#include <stdexcept>

#include <key_storage.h>
#include <aes_gcm.h>
#include <authentication.h>

using json = nlohmann::json;

// Вспомогательная функция для преобразования SQLite result в string
std::string getColumnString(sqlite3_stmt *stmt, int col)
{
  const unsigned char *text = sqlite3_column_text(stmt, col);
  return text ? reinterpret_cast<const char *>(text) : "";
}

// Вспомогательная функция для преобразования SQLite result в blob
std::vector<uint8_t> getColumnBlob(sqlite3_stmt *stmt, int col)
{
  const void *blob = sqlite3_column_blob(stmt, col);
  int size = sqlite3_column_bytes(stmt, col);
  if (blob && size > 0)
  {
    const uint8_t *data = static_cast<const uint8_t *>(blob);
    return std::vector<uint8_t>(data, data + size);
  }
  return {};
}

// Вспомогательная функция для преобразования int
int getColumnInt(sqlite3_stmt *stmt, int col)
{
  return sqlite3_column_int(stmt, col);
}

// Конструктор
Database::Database(const std::string &path, int max_conn)
    : db_path(path), max_connections(max_conn)
{
  // Создаем директорию для базы данных, если её нет
  std::filesystem::path fs_path(path);
  std::filesystem::path dir = fs_path.parent_path();
  if (!dir.empty() && !std::filesystem::exists(dir))
  {
    std::filesystem::create_directories(dir);
  }
}

// Деструктор
Database::~Database()
{

    closeAllConnections();
}

// Получить соединение из пула
sqlite3 *Database::getConnection()
{
  std::lock_guard<std::mutex> lock(pool_mutex);

  sqlite3 *conn = nullptr;

  // Если есть свободные соединения в пуле - берем одно
  if (!connection_pool.empty())
  {
    conn = connection_pool.back();
    connection_pool.pop_back();
    return conn;
  }

  // Иначе создаем новое соединение
  int rc = sqlite3_open(db_path.c_str(), &conn);
  if (rc != SQLITE_OK)
  {
    std::cerr << "Cannot open database: " << sqlite3_errmsg(conn) << std::endl;
    return nullptr;
  }

  // Включаем поддержку внешних ключей
  char *errMsg = nullptr;
  rc = sqlite3_exec(conn, "PRAGMA foreign_keys = ON;", nullptr, nullptr,
                    &errMsg);
  if (rc != SQLITE_OK)
  {
    std::cerr << "Failed to enable foreign keys: " << errMsg << std::endl;
    sqlite3_free(errMsg);
  }

  return conn;
}

// Вернуть соединение в пул
void Database::releaseConnection(sqlite3 *conn)
{
  if (!conn)
    return;

  std::lock_guard<std::mutex> lock(pool_mutex);

  // Если пул не переполнен - возвращаем соединение
  if (connection_pool.size() < max_connections)
  {
    connection_pool.push_back(conn);
  }
  else
  {
    // Иначе закрываем соединение
    sqlite3_close(conn);
  }
}

// Закрыть все соединения
void Database::closeAllConnections()
{
    std::lock_guard<std::mutex> lock(pool_mutex);
    for (sqlite3* conn : connection_pool) {
        if (conn) {
            sqlite3_close(conn);
        }
    }

    connection_pool.clear();
}

// Выполнить SQL скрипт
bool Database::executeScript(const std::string &script)
{
  sqlite3 *conn = getConnection();
  if (!conn)
    return false;

  char *errMsg = nullptr;
  int rc = sqlite3_exec(conn, script.c_str(), nullptr, nullptr, &errMsg);

  if (rc != SQLITE_OK)
  {
    std::cerr << "SQL error: " << errMsg << std::endl;
    sqlite3_free(errMsg);
    releaseConnection(conn);
    return false;
  }

  releaseConnection(conn);
  return true;
}

// Получить текущую версию базы
int Database::getCurrentVersion()
{
  sqlite3 *conn = getConnection();
  if (!conn)
    return -1;

  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(conn, "PRAGMA user_version;", -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    releaseConnection(conn);
    return -1;
  }

  int version = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW)
  {
    version = sqlite3_column_int(stmt, 0);
  }

  sqlite3_finalize(stmt);
  releaseConnection(conn);
  return version;
}

// Установить версию базы
void Database::setVersion(int version)
{
  sqlite3 *conn = getConnection();
  if (!conn)
    return;

  std::string pragma = "PRAGMA user_version = " + std::to_string(version) + ";";
  char *errMsg = nullptr;
  sqlite3_exec(conn, pragma.c_str(), nullptr, nullptr, &errMsg);

  if (errMsg)
  {
    std::cerr << "Error setting version: " << errMsg << std::endl;
    sqlite3_free(errMsg);
  }

  releaseConnection(conn);
}

// Инициализация базы данных
bool Database::initialize()
{

  // Проверяем, существует ли файл БД
  bool dbExists = std::filesystem::exists(db_path);

  if (!dbExists)
  {
    std::cout << "New database, creating tables..." << std::endl;
    // Новая БД - просто создаем таблицы версии 2
    return executeScript(CREATE_TABLES_V2);
  }

  // Существующая БД - проверяем миграции
  checkMigration();

  initDefaultSettings();

  return true;
}

void Database::initDefaultSettings()
{
    // Проверяем каждую настройку отдельно
    if (getSetting("password_length", "").empty()) {
        setSetting("password_length", "16");
    }

    if (getSetting("password_use_uppercase", "").empty()) {
        setSetting("password_use_uppercase", "true");
    }

    if (getSetting("password_use_lowercase", "").empty()) {
        setSetting("password_use_lowercase", "true");
    }

    if (getSetting("password_use_digits", "").empty()) {
        setSetting("password_use_digits", "true");
    }

    if (getSetting("password_use_symbols", "").empty()) {
        setSetting("password_use_symbols", "true");
    }

    if (getSetting("password_exclude_ambiguous", "").empty()) {
        setSetting("password_exclude_ambiguous", "true");
    }
}

// Создание таблиц
bool Database::createTables()
{
  // Этот скрипт создает таблицы и устанавливает версию = 1
  // Если таблицы уже есть - ничего не меняет
  return executeScript(CREATE_TABLES);
}

// Проверка миграций
void Database::checkMigration()
{
  int current_version = getCurrentVersion();

  if (current_version < CURRENT_VERSION)
  {
    runMigrations(current_version);
  }
  else
  {
    std::cout << "CREATE_TABLES_V2" << std::endl;
    executeScript(CREATE_TABLES_V2);
  }
}

// Выполнение миграций
void Database::runMigrations(int current_version)
{

  sqlite3 *conn = getConnection();
  if (!conn)
    return;

  char *errMsg = nullptr;

  // Миграция с 1 на 2
  if (current_version < 2)
  {
    int rc =
        sqlite3_exec(conn, MIGRATE_TO_V2.c_str(), nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK)
    {
      std::cerr << "Migration failed: " << errMsg << std::endl;
      sqlite3_free(errMsg);
    }
    else
    {
      std::cout << "Migration completed successfully" << std::endl;
    }
  }

  releaseConnection(conn);
}


// Получить настройку
std::string Database::getSetting(const std::string &key,
                                 const std::string &default_value)
{
  sqlite3 *conn = getConnection();
  if (!conn)
    return default_value;

  sqlite3_stmt *stmt;
  const char *sql = "SELECT setting_value FROM settings WHERE setting_key = ?";

  int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
    releaseConnection(conn);
    return default_value;
  }

  sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);

  std::string value = default_value;
  if (sqlite3_step(stmt) == SQLITE_ROW)
  {
    value = getColumnString(stmt, 0);
  }

  sqlite3_finalize(stmt);
  releaseConnection(conn);

  return value;
}

// Установить настройку
bool Database::setSetting(const std::string &key, const std::string &value,
                          bool encrypted)
{
  sqlite3 *conn = getConnection();
  if (!conn)
    return false;

  sqlite3_stmt *stmt;
  const char *sql = R"(
            INSERT INTO settings (setting_key, setting_value, encrypted) 
            VALUES (?, ?, ?)
            ON CONFLICT(setting_key) 
            DO UPDATE SET setting_value=excluded.setting_value, 
                        encrypted=excluded.encrypted,
                        updated_at=CURRENT_TIMESTAMP
        )";

  int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
    releaseConnection(conn);
    return false;
  }

  sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, value.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_int(stmt, 3, encrypted ? 1 : 0);

  rc = sqlite3_step(stmt);
  bool success = (rc == SQLITE_DONE);

  sqlite3_finalize(stmt);
  releaseConnection(conn);

  return success;
}



// Бэкап базы данных (заглушка для Sprint 8)
bool Database::backup(const std::string &backup_path) { return true; }

// Восстановление базы (заглушка для Sprint 8)
bool Database::restore(const std::string &backup_path) { return true; }

// Сохранить данные аутентификации (Argon2 хеш и соль)
bool Database::saveAuthData(const std::vector<uint8_t> &hash,
                            const std::vector<uint8_t> &salt,
                            uint32_t time_cost, uint32_t memory_cost,
                            uint32_t parallelism, uint32_t hash_len)
{
    sqlite3 *conn = getConnection();
    if (!conn)
        return false;

    sqlite3_stmt *stmt;
    bool success = true;

    // Сохраняем хеш с INSERT OR REPLACE
    const char *sql_hash = R"(
    INSERT OR REPLACE INTO key_store (key_type, key_data, version, created_at)
    VALUES ('auth_hash', ?, 1, CURRENT_TIMESTAMP)
  )";

    int rc = sqlite3_prepare_v2(conn, sql_hash, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
        releaseConnection(conn);
        return false;
    }

    sqlite3_bind_blob(stmt, 1, hash.data(), hash.size(), SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        std::cerr << "Failed to save auth_hash" << std::endl;
        success = false;
    }

    // Сохраняем соль
    if (success)
    {
        const char *sql_salt = R"(
      INSERT OR REPLACE INTO key_store (key_type, key_data, version, created_at)
      VALUES ('auth_salt', ?, 1, CURRENT_TIMESTAMP)
    )";

        rc = sqlite3_prepare_v2(conn, sql_salt, -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
            releaseConnection(conn);
            return false;
        }

        sqlite3_bind_blob(stmt, 1, salt.data(), salt.size(), SQLITE_STATIC);
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc != SQLITE_DONE)
        {
            std::cerr << "Failed to save auth_salt" << std::endl;
            success = false;
        }
    }

    // Сохраняем параметры
    if (success)
    {
        std::string params = "{\"time_cost\":" + std::to_string(time_cost) +
                             ",\"memory_cost\":" + std::to_string(memory_cost) +
                             ",\"parallelism\":" + std::to_string(parallelism) +
                             ",\"hash_len\":" + std::to_string(hash_len) + "}";

        const char *sql_params = R"(
      INSERT OR REPLACE INTO key_store (key_type, key_data, version, created_at)
      VALUES ('auth_params', ?, 1, CURRENT_TIMESTAMP)
    )";

        rc = sqlite3_prepare_v2(conn, sql_params, -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
            releaseConnection(conn);
            return false;
        }

        sqlite3_bind_text(stmt, 1, params.c_str(), -1, SQLITE_STATIC);
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc != SQLITE_DONE)
        {
            std::cerr << "Failed to save auth_params" << std::endl;
            success = false;
        }
    }

    releaseConnection(conn);
    return success;
}

// Получить данные аутентификации
bool Database::getAuthData(std::vector<uint8_t> &hash,
                           std::vector<uint8_t> &salt, uint32_t &time_cost,
                           uint32_t &memory_cost, uint32_t &parallelism,
                           uint32_t &hash_len)
{

  std::cout << "11111" << std::endl;
  sqlite3 *conn = getConnection();
  std::cout << "11111" << std::endl;
  if (!conn)
    return false;

  sqlite3_stmt *stmt;
  bool success = true;

  // Получаем хеш
  const char *sql_hash = "SELECT key_data FROM key_store WHERE key_type = "
                         "'auth_hash' AND version = 1";
  int rc = sqlite3_prepare_v2(conn, sql_hash, -1, &stmt, nullptr);
  if (rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW)
  {
    hash = getColumnBlob(stmt, 0);
  }
  else
  {
    success = false;
  }
  sqlite3_finalize(stmt);

  if (!success)
  {
    releaseConnection(conn);
    return false;
  }

  // Получаем соль
  const char *sql_salt = "SELECT key_data FROM key_store WHERE key_type = "
                         "'auth_salt' AND version = 1";
  rc = sqlite3_prepare_v2(conn, sql_salt, -1, &stmt, nullptr);
  if (rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW)
  {
    salt = getColumnBlob(stmt, 0);
  }
  else
  {
    success = false;
  }
  sqlite3_finalize(stmt);

  if (!success)
  {
    releaseConnection(conn);
    return false;
  }

  // Получаем параметры
  const char *sql_params = "SELECT key_data FROM key_store WHERE key_type = "
                           "'auth_params' AND version = 1";
  rc = sqlite3_prepare_v2(conn, sql_params, -1, &stmt, nullptr);
  if (rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW)
  {
    std::string params_str = getColumnString(stmt, 0);

    try
    {
      // Парсим JSON через nlohmann
      json params = json::parse(params_str);

      // Извлекаем значения с проверкой
      if (params.contains("time_cost") && params["time_cost"].is_number())
      {
        time_cost = params["time_cost"].get<uint32_t>();
      }

      if (params.contains("memory_cost") && params["memory_cost"].is_number())
      {
        memory_cost = params["memory_cost"].get<uint32_t>();
      }

      if (params.contains("parallelism") && params["parallelism"].is_number())
      {
        parallelism = params["parallelism"].get<uint32_t>();
      }

      if (params.contains("hash_len") && params["hash_len"].is_number())
      {
        hash_len = params["hash_len"].get<uint32_t>();
      }
    }
    catch (const json::parse_error &e)
    {
      std::cerr << "Failed to parse auth_params JSON: " << e.what()
                << std::endl;
    }
    catch (const json::type_error &e)
    {
      std::cerr << "JSON type error: " << e.what() << std::endl;
    }
  }

  sqlite3_finalize(stmt);
  releaseConnection(conn);

  return success;
}

// Сохранить соль для PBKDF2 (encryption key derivation)
bool Database::saveEncSalt(const std::vector<uint8_t> &salt)
{
    sqlite3 *conn = getConnection();
    if (!conn)
        return false;

    sqlite3_stmt *stmt;
    const char *sql = R"(
        INSERT OR REPLACE INTO key_store (key_type, key_data, version, created_at)
        VALUES ('enc_salt', ?, 1, CURRENT_TIMESTAMP)
    )";

    int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
        releaseConnection(conn);
        return false;
    }

    sqlite3_bind_blob(stmt, 1, salt.data(), salt.size(), SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    bool success = (rc == SQLITE_DONE);

    sqlite3_finalize(stmt);
    releaseConnection(conn);

    return success;
}

// Получить соль для PBKDF2
bool Database::getEncSalt(std::vector<uint8_t> &salt)
{
  sqlite3 *conn = getConnection();
  if (!conn)
    return false;

  sqlite3_stmt *stmt;
  const char *sql = "SELECT key_data FROM key_store WHERE key_type = "
                    "'enc_salt' AND version = 1";

  int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
    releaseConnection(conn);
    return false;
  }

  bool found = false;
  if (sqlite3_step(stmt) == SQLITE_ROW)
  {
    salt = getColumnBlob(stmt, 0);
    found = true;
  }

  sqlite3_finalize(stmt);
  releaseConnection(conn);

  return found;
}


bool Database::addLogEntry(std::string& previous_hash,
                           std::string& current_hash,
                           std::string& entry_data,
                           std::vector<uint8_t>& signature,
                           int key_version,
                           EventType type){

    sqlite3 *conn = getConnection();
    if (!conn)
        return false;

    sqlite3_stmt *stmt;
    const char *sql = R"(
        INSERT INTO audit_log (previous_hash, current_hash, entry_data, signature, key_version, created_at, event_type)
        VALUES (?, ?, ?, ?, ?, CURRENT_TIMESTAMP, ?)
    )";

    int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
        releaseConnection(conn);
        return false;
    }

    sqlite3_bind_text(stmt, 1, previous_hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, current_hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, entry_data.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 4, signature.data(), signature.size(), SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, key_version);
    sqlite3_bind_int(stmt, 6, static_cast<int>(type));

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);
    releaseConnection(conn);

    return (rc == SQLITE_DONE);
}

bool Database::getLogEntry(int sequence_number,
                           std::string& previous_hash,
                           std::string& current_hash,
                           std::string& entry_data,
                           std::vector<uint8_t>& signature,
                           int& key_version,
                           std::string& created_at,
                           std::string& event_type) {
    sqlite3 *conn = getConnection();
    if (!conn)
        return false;

    sqlite3_stmt *stmt;
    const char *sql = R"(
        SELECT previous_hash, current_hash, entry_data, signature, key_version, created_at, event_type
        FROM audit_log
        WHERE sequence_number = ?
    )";

    int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
        releaseConnection(conn);
        return false;
    }

    sqlite3_bind_int(stmt, 1, sequence_number);

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        // previous_hash (колонка 0) - TEXT
        const unsigned char* text = sqlite3_column_text(stmt, 0);
        previous_hash = text ? reinterpret_cast<const char*>(text) : "";

        // current_hash (колонка 1) - TEXT
        text = sqlite3_column_text(stmt, 1);
        current_hash = text ? reinterpret_cast<const char*>(text) : "";

        // entry_data (колонка 2) - TEXT
        text = sqlite3_column_text(stmt, 2);
        entry_data = text ? reinterpret_cast<const char*>(text) : "";

        // signature (колонка 3) - BLOB
        const void* blob = sqlite3_column_blob(stmt, 3);
        int blobSize = sqlite3_column_bytes(stmt, 3);
        if (blob && blobSize > 0) {
            signature.assign(static_cast<const uint8_t*>(blob), static_cast<const uint8_t*>(blob) + blobSize);
        } else {
            signature.clear();
        }

        // key_version (колонка 4) - INTEGER
        key_version = sqlite3_column_int(stmt, 4);

        // created_at (колонка 5) - TEXT
        text = sqlite3_column_text(stmt, 5);
        created_at = text ? reinterpret_cast<const char*>(text) : "";

        // event_type (колонка 6) - TEXT
        text = sqlite3_column_text(stmt, 6);
        event_type = text ? reinterpret_cast<const char*>(text) : "";

        sqlite3_finalize(stmt);
        releaseConnection(conn);
        return true;
    }

    sqlite3_finalize(stmt);
    releaseConnection(conn);
    return false;
}

int Database::getLogEntryCount() {
    sqlite3 *conn = getConnection();
    if (!conn)
        return -1;

    sqlite3_stmt *stmt;
    const char *sql = "SELECT COUNT(*) FROM audit_log";

    int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
        releaseConnection(conn);
        return -1;
    }

    rc = sqlite3_step(stmt);
    int count = -1;

    if (rc == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    releaseConnection(conn);
    return count;
}

std::string Database::getLastEntryHash() {
    sqlite3 *conn = getConnection();
    if (!conn)
        return "";

    sqlite3_stmt *stmt;
    const char *sql = R"(
        SELECT current_hash
        FROM audit_log
        ORDER BY sequence_number DESC
        LIMIT 1
    )";

    int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
        releaseConnection(conn);
        return "";
    }

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        const unsigned char* current_hash = sqlite3_column_text(stmt, 0);
        std::string result = current_hash ? reinterpret_cast<const char*>(current_hash) : "";

        sqlite3_finalize(stmt);
        releaseConnection(conn);
        return result;
    }

    sqlite3_finalize(stmt);
    releaseConnection(conn);
    return "";
}


bool Database::addPublicKey(const std::vector<uint8_t>& publicKey,
                            int keyVersion,
                            int validFromSequence) {

    sqlite3 *conn = getConnection();
    if (!conn)
        return false;

    sqlite3_stmt *stmt;
    const char *sql = R"(
        INSERT OR IGNORE INTO public_keys (public_key, key_version, valid_from_sequence, valid_to_sequence)
        VALUES (?, ?, ?, NULL)
    )";

    int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
        releaseConnection(conn);
        return false;
    }

    sqlite3_bind_blob(stmt, 1, publicKey.data(), publicKey.size(), SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, keyVersion);
    sqlite3_bind_int(stmt, 3, validFromSequence);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    releaseConnection(conn);

    return true;
}

bool Database::getPublicKeyForSequence(int sequenceNumber, std::vector<uint8_t>& publicKey, int& keyVersion) {
    sqlite3 *conn = getConnection();
    if (!conn)
        return false;

    sqlite3_stmt *stmt;
    const char *sql = R"(
        SELECT pk.public_key, pk.key_version
        FROM public_keys pk
        WHERE pk.valid_from_sequence <= ?
        AND (pk.valid_to_sequence IS NULL OR pk.valid_to_sequence >= ?)
        ORDER BY pk.key_version DESC
        LIMIT 1
    )";

    int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
        releaseConnection(conn);
        return false;
    }

    sqlite3_bind_int(stmt, 1, sequenceNumber);
    sqlite3_bind_int(stmt, 2, sequenceNumber);
    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        const void* blob = sqlite3_column_blob(stmt, 0);
        int blobSize = sqlite3_column_bytes(stmt, 0);

        publicKey.resize(blobSize);
        memcpy(publicKey.data(), blob, blobSize);

        keyVersion = sqlite3_column_int(stmt, 1);

        sqlite3_finalize(stmt);
        releaseConnection(conn);
        return true;
    }

    sqlite3_finalize(stmt);
    releaseConnection(conn);
    return false;
}


std::vector<AuditEntryDisplay> Database::getAuditPage(
    int offset,
    int limit,
    std::string& sortColumn,
    bool sortOrder,
    std::string& eventTypeFilter,
    const std::string& dateFrom,
    const std::string& dateTo,
    const std::string& searchText)
{
    sqlite3 *conn = getConnection();
    if (!conn)
        throw std::runtime_error("Failed to get connection");

    // Белый список для sortColumn
    std::vector<std::string> allowedColumns = {"sequence_number", "created_at", "event_type"};
    if (std::find(allowedColumns.begin(), allowedColumns.end(), sortColumn) == allowedColumns.end()) {
        sortColumn = "sequence_number";
    }

    std::string orderDirection = sortOrder ? "ASC" : "DESC";

    // Собираем SQL динамически
    std::string sql = "SELECT * FROM audit_log WHERE 1=1";

    if (!eventTypeFilter.empty()) {
        sql += " AND event_type = ?";
    }
    if (!dateFrom.empty()) {
        sql += " AND DATE(created_at) >= ?";
    }
    if (!dateTo.empty()) {
        sql += " AND DATE(created_at) <= ?";
    }
    if (!searchText.empty()) {
        sql += " AND entry_data LIKE ?";
    }

    sql += " ORDER BY " + sortColumn + " " + orderDirection;
    sql += " LIMIT ? OFFSET ?";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(conn, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(conn)));
    }

    int paramIndex = 1;

    if (!eventTypeFilter.empty()) {
        sqlite3_bind_int(stmt, paramIndex++, std::stoi(eventTypeFilter));
    }
    if (!dateFrom.empty()) {
        sqlite3_bind_text(stmt, paramIndex++, dateFrom.c_str(), -1, SQLITE_TRANSIENT);
    }
    if (!dateTo.empty()) {
        sqlite3_bind_text(stmt, paramIndex++, dateTo.c_str(), -1, SQLITE_TRANSIENT);
    }
    if (!searchText.empty()) {
        std::string likePattern = "%" + searchText + "%";
        sqlite3_bind_text(stmt, paramIndex++, likePattern.c_str(), -1, SQLITE_TRANSIENT);
    }

    sqlite3_bind_int(stmt, paramIndex++, limit);
    sqlite3_bind_int(stmt, paramIndex++, offset);

    std::vector<AuditEntryDisplay> entries;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        AuditEntryDisplay entry;
        entry.sequence_number = sqlite3_column_int(stmt, 0);

        const unsigned char* previous_hash = sqlite3_column_text(stmt, 1);
        entry.previous_hash = previous_hash ? reinterpret_cast<const char*>(previous_hash) : "";

        const unsigned char* current_hash = sqlite3_column_text(stmt, 2);
        entry.current_hash = current_hash ? reinterpret_cast<const char*>(current_hash) : "";

        const unsigned char* entry_data = sqlite3_column_text(stmt, 3);
        entry.entry_data = entry_data ? reinterpret_cast<const char*>(entry_data) : "";

        const void* signature_blob = sqlite3_column_blob(stmt, 4);
        int signature_size = sqlite3_column_bytes(stmt, 4);
        if (signature_blob && signature_size > 0) {
            const uint8_t* bytes = static_cast<const uint8_t*>(signature_blob);
            entry.signature.assign(bytes, bytes + signature_size);
        }

        entry.key_version = sqlite3_column_int(stmt, 5);

        const unsigned char* created_at = sqlite3_column_text(stmt, 6);
        entry.created_at = created_at ? reinterpret_cast<const char*>(created_at) : "";

        EventType event_type_enum = static_cast<EventType>(sqlite3_column_int(stmt, 7));
        entry.event_type = Event::eventTypeToString(event_type_enum);

        // Значения по умолчанию
        entry.severity = "";
        entry.user_id = 0;
        entry.source = "";
        entry.entry_id = -1;
        entry.signature_valid = false;

        // Парсим JSON
        if (!entry.entry_data.empty()) {
            try {
                json j = json::parse(entry.entry_data);

                if (j.contains("severity")) {
                    int severityInt = j["severity"].get<int>();
                    switch (severityInt) {
                    case 0: entry.severity = "INFO"; break;
                    case 1: entry.severity = "WARN"; break;
                    case 2: entry.severity = "ERROR"; break;
                    case 3: entry.severity = "CRITICAL"; break;
                    default: entry.severity = "INFO"; break;
                    }
                }

                if (j.contains("user_id")) entry.user_id = j["user_id"].get<int>();

                if (j.contains("source")) entry.source = j["source"].get<std::string>();

                if (j.contains("entry_id")) entry.entry_id = j["entry_id"].get<int>();

            } catch (const std::exception& e) {
                // ошибка парсинга
            }
        }

        entries.push_back(entry);
    }

    sqlite3_finalize(stmt);
    return entries;
}

bool Database::addContact(const std::string& name, const std::string& publicKeyPEM)
{
    sqlite3* conn = getConnection();
    if (!conn) return false;

    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO contacts (name, public_key, created_at) VALUES (?, ?, CURRENT_TIMESTAMP)";

    int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
        releaseConnection(conn);
        return false;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, publicKeyPEM.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    releaseConnection(conn);

    return (rc == SQLITE_DONE);
}

std::vector<std::pair<int, std::string>> Database::getAllContacts()
{
    std::vector<std::pair<int, std::string>> contacts;

    sqlite3* conn = getConnection();
    if (!conn) return contacts;

    sqlite3_stmt* stmt;
    const char* sql = "SELECT id, name FROM contacts ORDER BY name";

    int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
        releaseConnection(conn);
        return contacts;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        contacts.push_back({id, name ? name : ""});
    }

    sqlite3_finalize(stmt);
    releaseConnection(conn);
    return contacts;
}

bool Database::getContactPublicKey(const std::string& name, std::vector<uint8_t>& publicKey)
{
    sqlite3* conn = getConnection();
    if (!conn) return false;

    sqlite3_stmt* stmt;
    const char* sql = "SELECT public_key FROM contacts WHERE name = ?";

    int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
        releaseConnection(conn);
        return false;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        const char* keyData = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (keyData) {
            publicKey.assign(keyData, keyData + strlen(keyData));
        }
        sqlite3_finalize(stmt);
        releaseConnection(conn);
        return true;
    }

    sqlite3_finalize(stmt);
    releaseConnection(conn);
    return false;
}

bool Database::getContactPublicKeyById(int contactId, std::vector<uint8_t>& publicKey)
{
    sqlite3* conn = getConnection();
    if (!conn) return false;

    sqlite3_stmt* stmt;
    const char* sql = "SELECT public_key FROM contacts WHERE id = ?";

    int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
        releaseConnection(conn);
        return false;
    }

    sqlite3_bind_int(stmt, 1, contactId);
    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        const char* keyData = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (keyData) {
            publicKey.assign(keyData, keyData + strlen(keyData));
        }
        sqlite3_finalize(stmt);
        releaseConnection(conn);
        return true;
    }

    sqlite3_finalize(stmt);
    releaseConnection(conn);
    return false;
}

bool Database::deleteContact(int contactId)
{
    sqlite3* conn = getConnection();
    if (!conn) return false;

    sqlite3_stmt* stmt;
    const char* sql = "DELETE FROM contacts WHERE id = ?";

    int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
        releaseConnection(conn);
        return false;
    }

    sqlite3_bind_int(stmt, 1, contactId);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    releaseConnection(conn);

    return (rc == SQLITE_DONE);
}

bool Database::updateContactLastUsed(int contactId)
{
    sqlite3* conn = getConnection();
    if (!conn) return false;

    sqlite3_stmt* stmt;
    const char* sql = "UPDATE contacts SET last_used = CURRENT_TIMESTAMP WHERE id = ?";

    int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
        releaseConnection(conn);
        return false;
    }

    sqlite3_bind_int(stmt, 1, contactId);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    releaseConnection(conn);

    return (rc == SQLITE_DONE);
}

bool Database::reencryptAllEntries(int& reencryptedCount)
{
    reencryptedCount = 0;

    std::cout << "[REENCRYPT] Starting re-encryption process..." << std::endl;

    // Получаем ключи из KeyManager
    KeyData oldKeyData;
    KeyData newKeyData;
    KeyManager::getInstance().getOldEncryptionKey(oldKeyData);
    KeyManager::getInstance().getEncryptionKey(newKeyData);

    if (!oldKeyData.data || oldKeyData.size == 0) {
        std::cerr << "[REENCRYPT] Old key is empty!" << std::endl;
        return false;
    }
    if (!newKeyData.data || newKeyData.size == 0) {
        std::cerr << "[REENCRYPT] New key is empty!" << std::endl;
        return false;
    }

    std::cout << "[REENCRYPT] Old key size: " << oldKeyData.size << std::endl;
    std::cout << "[REENCRYPT] New key size: " << newKeyData.size << std::endl;

    sqlite3* conn = getConnection();
    if (!conn) {
        std::cerr << "[REENCRYPT] Failed to get database connection" << std::endl;
        return false;
    }
    std::cout << "[REENCRYPT] Database connection acquired" << std::endl;

    // Начинаем транзакцию
    char* errMsg = nullptr;
    int rc = sqlite3_exec(conn, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[REENCRYPT] Failed to begin transaction: " << (errMsg ? errMsg : "unknown") << std::endl;
        sqlite3_free(errMsg);
        releaseConnection(conn);
        return false;
    }
    std::cout << "[REENCRYPT] Transaction started" << std::endl;

    // Проверяем, есть ли записи в таблице
    const char* countSql = "SELECT COUNT(*) FROM vault_entries";
    sqlite3_stmt* countStmt = nullptr;
    rc = sqlite3_prepare_v2(conn, countSql, -1, &countStmt, nullptr);
    if (rc == SQLITE_OK && sqlite3_step(countStmt) == SQLITE_ROW) {
        int total = sqlite3_column_int(countStmt, 0);
        std::cout << "[REENCRYPT] Total entries in vault: " << total << std::endl;
    }
    sqlite3_finalize(countStmt);

    // Выбираем все записи
    const char* selectSql = "SELECT rowid, encrypted_data FROM vault_entries";
    sqlite3_stmt* selectStmt = nullptr;
    rc = sqlite3_prepare_v2(conn, selectSql, -1, &selectStmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[REENCRYPT] Failed to prepare select statement: " << sqlite3_errmsg(conn) << std::endl;
        sqlite3_exec(conn, "ROLLBACK;", nullptr, nullptr, nullptr);
        releaseConnection(conn);
        return false;
    }
    std::cout << "[REENCRYPT] Select statement prepared" << std::endl;

    // Готовим UPDATE запрос
    const char* updateSql = "UPDATE vault_entries SET encrypted_data = ? WHERE rowid = ?";
    sqlite3_stmt* updateStmt = nullptr;
    rc = sqlite3_prepare_v2(conn, updateSql, -1, &updateStmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[REENCRYPT] Failed to prepare update statement: " << sqlite3_errmsg(conn) << std::endl;
        sqlite3_finalize(selectStmt);
        sqlite3_exec(conn, "ROLLBACK;", nullptr, nullptr, nullptr);
        releaseConnection(conn);
        return false;
    }
    std::cout << "[REENCRYPT] Update statement prepared" << std::endl;

    // Настройка шифрования
    AESGCM<256> cipher;
    int totalEntries = 0;
    int successCount = 0;

    while (sqlite3_step(selectStmt) == SQLITE_ROW) {
        totalEntries++;
        int rowid = sqlite3_column_int(selectStmt, 0);
        std::cout << "[REENCRYPT] Processing entry " << rowid << " (" << totalEntries << ")" << std::endl;

        const void* blob = sqlite3_column_blob(selectStmt, 1);
        int blobSize = sqlite3_column_bytes(selectStmt, 1);

        std::cout << "[REENCRYPT]   Blob size: " << blobSize << " bytes" << std::endl;

        if (blob && blobSize > 0) {
            std::vector<uint8_t> encryptedData(
                static_cast<const uint8_t*>(blob),
                static_cast<const uint8_t*>(blob) + blobSize
                );

            try {
                // Расшифровываем старым ключом
                std::cout << "[REENCRYPT]   Decrypting with old key..." << std::endl;
                std::vector<uint8_t> plaintext = cipher.decrypt(oldKeyData, encryptedData);
                std::cout << "[REENCRYPT]   Decrypted size: " << plaintext.size() << " bytes" << std::endl;

                // Шифруем новым ключом
                std::cout << "[REENCRYPT]   Encrypting with new key..." << std::endl;
                std::vector<uint8_t> newEncrypted = cipher.encrypt(newKeyData, plaintext);
                std::cout << "[REENCRYPT]   Encrypted size: " << newEncrypted.size() << " bytes" << std::endl;

                // Обновляем в БД
                sqlite3_bind_blob(updateStmt, 1, newEncrypted.data(), newEncrypted.size(), SQLITE_STATIC);
                sqlite3_bind_int(updateStmt, 2, rowid);

                rc = sqlite3_step(updateStmt);
                if (rc == SQLITE_DONE) {
                    successCount++;
                    std::cout << "[REENCRYPT]   Entry " << rowid << " updated successfully" << std::endl;
                } else {
                    std::cerr << "[REENCRYPT]   Failed to update entry " << rowid << ", rc=" << rc << std::endl;
                }

                sqlite3_reset(updateStmt);
                sqlite3_clear_bindings(updateStmt);

                // Зануляем временные данные
                secure_zero(plaintext.data(), plaintext.size());
                secure_zero(newEncrypted.data(), newEncrypted.size());

            } catch (const std::exception& e) {
                std::cerr << "[REENCRYPT]   Exception: " << e.what() << std::endl;
                sqlite3_finalize(selectStmt);
                sqlite3_finalize(updateStmt);
                sqlite3_exec(conn, "ROLLBACK;", nullptr, nullptr, nullptr);
                releaseConnection(conn);
                return false;
            }
        } else {
            std::cout << "[REENCRYPT]   No blob data for entry " << rowid << std::endl;
        }
    }

    std::cout << "[REENCRYPT] Total entries processed: " << totalEntries << std::endl;
    std::cout << "[REENCRYPT] Successfully re-encrypted: " << successCount << std::endl;

    sqlite3_finalize(selectStmt);
    sqlite3_finalize(updateStmt);

    // Фиксируем транзакцию
    rc = sqlite3_exec(conn, "COMMIT;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[REENCRYPT] Failed to commit transaction: " << (errMsg ? errMsg : "unknown") << std::endl;
        sqlite3_free(errMsg);
        releaseConnection(conn);
        return false;
    }
    std::cout << "[REENCRYPT] Transaction committed" << std::endl;

    releaseConnection(conn);
    reencryptedCount = successCount;

    std::cout << "[REENCRYPT] Re-encryption completed. Success: " << (successCount == totalEntries && totalEntries > 0) << std::endl;

    return successCount == totalEntries && totalEntries > 0;
}
