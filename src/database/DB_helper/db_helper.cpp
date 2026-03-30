#include "db_helper.h"
#include "../src/database/DBSchema.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sqlite3.h>
#include <thread>

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
Database::~Database() { closeAllConnections(); }

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

  setSetting("password_length", "16");
  setSetting("password_use_uppercase", "true");
  setSetting("password_use_lowercase", "true");
  setSetting("password_use_digits", "true");
  setSetting("password_use_symbols", "true");
  setSetting("password_exclude_ambiguous", "true");

  return true;
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

// Добавить запись в аудит
int Database::addAuditLog(const std::string &action, int entry_id,
                          const std::string &details,
                          const std::vector<uint8_t> &signature)
{

  sqlite3 *conn = getConnection();
  if (!conn)
    return -1;

  sqlite3_stmt *stmt;
  const char *sql = R"(
            INSERT INTO audit_log (action, entry_id, details, signature)
            VALUES (?, ?, ?, ?)
        )";

  int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
    releaseConnection(conn);
    return -1;
  }

  sqlite3_bind_text(stmt, 1, action.c_str(), -1, SQLITE_STATIC);
  if (entry_id >= 0)
  {
    sqlite3_bind_int(stmt, 2, entry_id);
  }
  else
  {
    sqlite3_bind_null(stmt, 2);
  }
  sqlite3_bind_text(stmt, 3, details.c_str(), -1, SQLITE_STATIC);

  if (!signature.empty())
  {
    sqlite3_bind_blob(stmt, 4, signature.data(), signature.size(),
                      SQLITE_STATIC);
  }
  else
  {
    sqlite3_bind_null(stmt, 4);
  }

  rc = sqlite3_step(stmt);
  int log_id = -1;

  if (rc == SQLITE_DONE)
  {
    log_id = sqlite3_last_insert_rowid(conn);
  }

  sqlite3_finalize(stmt);
  releaseConnection(conn);

  return log_id;
}

// Получить записи аудита
std::vector<AuditLog> Database::getAuditLogs(int limit)
{
  std::vector<AuditLog> logs;

  sqlite3 *conn = getConnection();
  if (!conn)
    return logs;

  sqlite3_stmt *stmt;
  std::string sql = "SELECT * FROM audit_log ORDER BY timestamp DESC LIMIT ?";

  int rc = sqlite3_prepare_v2(conn, sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
    releaseConnection(conn);
    return logs;
  }

  sqlite3_bind_int(stmt, 1, limit);

  while (sqlite3_step(stmt) == SQLITE_ROW)
  {
    AuditLog log;
    log.id = sqlite3_column_int(stmt, 0);
    log.action = getColumnString(stmt, 1);
    log.timestamp = getColumnString(stmt, 2);
    log.entry_id = sqlite3_column_type(stmt, 3) == SQLITE_NULL
                       ? -1
                       : sqlite3_column_int(stmt, 3);
    log.details = getColumnString(stmt, 4);
    log.signature = getColumnBlob(stmt, 5);
    logs.push_back(log);
  }

  sqlite3_finalize(stmt);
  releaseConnection(conn);

  return logs;
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
