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
  std::cout << "2222222 - getConnection started (thread: "
            << std::this_thread::get_id() << ")" << std::endl;

  sqlite3 *conn = nullptr;

  // Попытка захвата мьютекса с таймаутом для диагностики
  const int max_attempts = 3;
  for (int attempt = 1; attempt <= max_attempts; ++attempt)
  {
    try
    {
      // Пытаемся захватить мьютекс
      if (pool_mutex.try_lock())
      {
        std::cout << "2222222 - mutex locked on attempt " << attempt
                  << std::endl;

        // Используем lock_guard для гарантированного освобождения
        std::lock_guard<std::recursive_mutex> lock(pool_mutex, std::adopt_lock);

        // Если есть свободные соединения в пуле - берем одно
        if (!connection_pool.empty())
        {
          conn = connection_pool.back();
          connection_pool.pop_back();
          std::cout << "Taking connection from pool, remaining: "
                    << connection_pool.size() << std::endl;
          return conn;
        }

        // Выходим из блока - мьютекс автоматически освобождается
        std::cout << "Pool empty, releasing mutex" << std::endl;
        break; // Выходим из цикла попыток
      }
      else
      {
        std::cout << "Warning: Mutex is locked by another thread (attempt "
                  << attempt << "/" << max_attempts << ")" << std::endl;

        if (attempt < max_attempts)
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(100 * attempt));
        }
        else
        {
          std::cerr << "ERROR: Failed to acquire mutex after " << max_attempts
                    << " attempts" << std::endl;
          return nullptr;
        }
      }
    }
    catch (const std::system_error &e)
    {
      std::cerr << "CRITICAL: System error in mutex operation: " << e.what()
                << " (code: " << e.code() << ")" << std::endl;

      // Проверяем специфичные ошибки
      if (e.code() == std::errc::resource_deadlock_would_occur)
      {
        std::cerr << "  -> Deadlock detected!" << std::endl;
      }
      else if (e.code() == std::errc::operation_not_permitted)
      {
        std::cerr << "  -> Operation not permitted (mutex might be destroyed)"
                  << std::endl;
      }
      else if (e.code() == std::errc::device_or_resource_busy)
      {
        std::cerr << "  -> Mutex is busy" << std::endl;
      }

      return nullptr;
    }
    catch (const std::exception &e)
    {
      std::cerr << "CRITICAL: Unexpected exception in mutex phase: " << e.what()
                << std::endl;
      return nullptr;
    }
  }

  // Создаем новое соединение (мьютекс уже освобожден)
  try
  {
    std::cout << "Creating new database connection..." << std::endl;

    // Добавим проверку существования директории перед открытием
    std::filesystem::path dbPath(db_path);
    std::filesystem::path dbDir = dbPath.parent_path();
    if (!dbDir.empty() && !std::filesystem::exists(dbDir))
    {
      std::cout << "Creating database directory: " << dbDir << std::endl;
      std::filesystem::create_directories(dbDir);
    }

    int rc = sqlite3_open(db_path.c_str(), &conn);

    if (rc != SQLITE_OK)
    {
      std::string errorMsg = sqlite3_errmsg(conn);
      std::cerr << "Cannot open database: " << errorMsg << std::endl;
      std::cerr << "  Path: " << db_path << std::endl;
      std::cerr << "  Error code: " << rc << std::endl;

      if (conn)
      {
        sqlite3_close(conn);
        conn = nullptr;
      }
      return nullptr;
    }

    // Проверяем, что соединение действительно работает
    rc = sqlite3_exec(conn, "SELECT 1;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK)
    {
      std::cerr << "Database connection test failed: " << sqlite3_errmsg(conn)
                << std::endl;
      sqlite3_close(conn);
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

    // Устанавливаем таймаут для избежания блокировок
    sqlite3_busy_timeout(conn, 5000); // 5 секунд

    std::cout << "New connection created successfully" << std::endl;
    return conn;
  }
  catch (const std::filesystem::filesystem_error &e)
  {
    std::cerr << "Filesystem error while creating database: " << e.what()
              << std::endl;
    std::cerr << "  Path1: " << e.path1() << std::endl;
    if (!e.path2().empty())
    {
      std::cerr << "  Path2: " << e.path2() << std::endl;
    }
    return nullptr;
  }
  catch (const std::exception &e)
  {
    std::cerr << "CRITICAL: Exception while creating database connection: "
              << e.what() << std::endl;
    if (conn)
    {
      sqlite3_close(conn);
    }
    return nullptr;
  }
}

// Вернуть соединение в пул
void Database::releaseConnection(sqlite3 *conn)
{
  if (!conn)
    return;

  std::lock_guard<std::recursive_mutex> lock(pool_mutex);

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
  std::cout << "Clearing " << std::endl;
  std::lock_guard<std::recursive_mutex> lock(pool_mutex);

  std::cout << "Clearing " << connection_pool.size() << " connections from pool"
            << std::endl;

  // Просто очищаем пул, не закрывая соединения
  // (они закроются сами при разрушении объектов)
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
  std::cout << "INIT" << std::endl;

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

// Добавление записи
int Database::addEntry(const std::string &title, const std::string &username,
                       const std::vector<uint8_t> &encrypted_password,
                       const std::string &url, const std::string &notes,
                       const std::string &tags)
{

  sqlite3 *conn = getConnection();
  if (!conn)
    return -1;

  sqlite3_stmt *stmt;
  const char *sql = R"(
            INSERT INTO vault_entries (title, username, encrypted_password, url, notes, tags)
            VALUES (?, ?, ?, ?, ?, ?)
        )";

  int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
    releaseConnection(conn);
    return -1;
  }

  // Биндим параметры
  sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_blob(stmt, 3, encrypted_password.data(),
                    encrypted_password.size(), SQLITE_STATIC);
  sqlite3_bind_text(stmt, 4, url.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 5, notes.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 6, tags.c_str(), -1, SQLITE_STATIC);

  rc = sqlite3_step(stmt);
  int entry_id = -1;

  if (rc == SQLITE_DONE)
  {
    entry_id = sqlite3_last_insert_rowid(conn);

    // Добавляем запись в аудит
    addAuditLog("EntryAdded", entry_id,
                "{\"title\":\"" + title + "\",\"username\":\"" + username +
                    "\"}");
  }

  sqlite3_finalize(stmt);
  releaseConnection(conn);

  return entry_id;
}

// Обновление записи
bool Database::updateEntry(int entry_id, const std::string &title,
                           const std::string &username,
                           const std::vector<uint8_t> &encrypted_password,
                           const std::string &url, const std::string &notes,
                           const std::string &tags)
{

  sqlite3 *conn = getConnection();
  if (!conn)
    return false;

  sqlite3_stmt *stmt;
  const char *sql = R"(
            UPDATE vault_entries 
            SET title=?, username=?, encrypted_password=?, 
                url=?, notes=?, tags=?, updated_at=CURRENT_TIMESTAMP
            WHERE id=?
        )";

  int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
    releaseConnection(conn);
    return false;
  }

  sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_blob(stmt, 3, encrypted_password.data(),
                    encrypted_password.size(), SQLITE_STATIC);
  sqlite3_bind_text(stmt, 4, url.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 5, notes.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 6, tags.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_int(stmt, 7, entry_id);

  rc = sqlite3_step(stmt);
  bool success = (rc == SQLITE_DONE && sqlite3_changes(conn) > 0);

  if (success)
  {
    addAuditLog("EntryUpdated", entry_id,
                "{\"title\":\"" + title + "\",\"username\":\"" + username +
                    "\"}");
  }

  sqlite3_finalize(stmt);
  releaseConnection(conn);

  return success;
}

// Удаление записи
bool Database::deleteEntry(int entry_id)
{
  // Сначала получаем запись для аудита
  auto entry = getEntry(entry_id);
  if (!entry)
    return false;

  sqlite3 *conn = getConnection();
  if (!conn)
    return false;

  sqlite3_stmt *stmt;
  const char *sql = "DELETE FROM vault_entries WHERE id=?";

  int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
    releaseConnection(conn);
    return false;
  }

  sqlite3_bind_int(stmt, 1, entry_id);

  rc = sqlite3_step(stmt);
  bool success = (rc == SQLITE_DONE && sqlite3_changes(conn) > 0);

  if (success)
  {
    addAuditLog("EntryDeleted", -1,
                "{\"id\":" + std::to_string(entry_id) + ",\"title\":\"" +
                    entry->title + "\"}");
  }

  sqlite3_finalize(stmt);
  releaseConnection(conn);

  return success;
}

// Получить одну запись
std::unique_ptr<VaultEntry> Database::getEntry(int entry_id)
{
  sqlite3 *conn = getConnection();
  if (!conn)
    return nullptr;

  sqlite3_stmt *stmt;
  const char *sql = "SELECT * FROM vault_entries WHERE id = ?";

  int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
    releaseConnection(conn);
    return nullptr;
  }

  sqlite3_bind_int(stmt, 1, entry_id);

  std::unique_ptr<VaultEntry> entry;

  if (sqlite3_step(stmt) == SQLITE_ROW)
  {
    entry = std::make_unique<VaultEntry>();
    entry->id = sqlite3_column_int(stmt, 0);
    entry->title = getColumnString(stmt, 1);
    entry->username = getColumnString(stmt, 2);
    entry->encrypted_password = getColumnBlob(stmt, 3);
    entry->url = getColumnString(stmt, 4);
    entry->notes = getColumnString(stmt, 5);
    entry->tags = getColumnString(stmt, 6);
    entry->created_at = getColumnString(stmt, 7);
    entry->updated_at = getColumnString(stmt, 8);
  }

  sqlite3_finalize(stmt);
  releaseConnection(conn);

  return entry;
}

// Получить все записи
std::vector<VaultEntry> Database::getAllEntries()
{
  std::vector<VaultEntry> entries;

  sqlite3 *conn = getConnection();
  if (!conn)
    return entries;

  sqlite3_stmt *stmt;
  const char *sql =
      "SELECT id, title, username, url, notes, tags, created_at, updated_at "
      "FROM vault_entries ORDER BY title";

  int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
    releaseConnection(conn);
    return entries;
  }

  while (sqlite3_step(stmt) == SQLITE_ROW)
  {
    VaultEntry entry;
    entry.id = sqlite3_column_int(stmt, 0);
    entry.title = getColumnString(stmt, 1);
    entry.username = getColumnString(stmt, 2);
    entry.url = getColumnString(stmt, 3);
    entry.notes = getColumnString(stmt, 4);
    entry.tags = getColumnString(stmt, 5);
    entry.created_at = getColumnString(stmt, 6);
    entry.updated_at = getColumnString(stmt, 7);
    // encrypted_password не получаем для списка
    entries.push_back(entry);
  }

  sqlite3_finalize(stmt);
  releaseConnection(conn);

  return entries;
}

// Поиск записей
std::vector<VaultEntry> Database::searchEntries(const std::string &search_term)
{
  std::vector<VaultEntry> entries;

  sqlite3 *conn = getConnection();
  if (!conn)
    return entries;

  sqlite3_stmt *stmt;
  const char *sql = R"(
            SELECT id, title, username, url, notes, tags, created_at, updated_at
            FROM vault_entries
            WHERE title LIKE ? OR username LIKE ? OR url LIKE ? OR notes LIKE ?
            ORDER BY title
        )";

  int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
    releaseConnection(conn);
    return entries;
  }

  std::string pattern = "%" + search_term + "%";
  sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, pattern.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 3, pattern.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 4, pattern.c_str(), -1, SQLITE_STATIC);

  while (sqlite3_step(stmt) == SQLITE_ROW)
  {
    VaultEntry entry;
    entry.id = sqlite3_column_int(stmt, 0);
    entry.title = getColumnString(stmt, 1);
    entry.username = getColumnString(stmt, 2);
    entry.url = getColumnString(stmt, 3);
    entry.notes = getColumnString(stmt, 4);
    entry.tags = getColumnString(stmt, 5);
    entry.created_at = getColumnString(stmt, 6);
    entry.updated_at = getColumnString(stmt, 7);
    entries.push_back(entry);
  }

  sqlite3_finalize(stmt);
  releaseConnection(conn);

  return entries;
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
  const char *sql = R"(
        INSERT INTO key_store (key_type, key_data, version, created_at) 
        VALUES (?, ?, ?, CURRENT_TIMESTAMP)
    )";

  int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    std::cerr << "Prepare failed: " << sqlite3_errmsg(conn) << std::endl;
    releaseConnection(conn);
    return false;
  }

  bool success = true;

  // Сохраняем хеш
  sqlite3_bind_text(stmt, 1, "auth_hash", -1, SQLITE_STATIC);
  sqlite3_bind_blob(stmt, 2, hash.data(), hash.size(), SQLITE_STATIC);
  sqlite3_bind_int(stmt, 3, 1);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE)
  {
    std::cerr << "Failed to save auth_hash: " << sqlite3_errmsg(conn)
              << std::endl;
    success = false;
  }

  if (success)
  {
    // Сбрасываем statement для нового использования
    sqlite3_reset(stmt);

    // Сохраняем соль
    sqlite3_bind_text(stmt, 1, "auth_salt", -1, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 2, salt.data(), salt.size(), SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, 1);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
      std::cerr << "Failed to save auth_salt: " << sqlite3_errmsg(conn)
                << std::endl;
      success = false;
    }
  }

  if (success)
  {
    // Сбрасываем statement
    sqlite3_reset(stmt);

    // Сохраняем параметры как JSON
    std::string params = "{\"time_cost\":" + std::to_string(time_cost) +
                         ",\"memory_cost\":" + std::to_string(memory_cost) +
                         ",\"parallelism\":" + std::to_string(parallelism) +
                         ",\"hash_len\":" + std::to_string(hash_len) + "}";

    sqlite3_bind_text(stmt, 1, "auth_params", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, params.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, 1);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
      std::cerr << "Failed to save auth_params: " << sqlite3_errmsg(conn)
                << std::endl;
      success = false;
    }
  }

  sqlite3_finalize(stmt);
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
        INSERT INTO key_store (key_type, key_data, version, created_at) 
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
