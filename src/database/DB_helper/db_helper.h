#ifndef DATABASE_HELPER_H
#define DATABASE_HELPER_H

#include "../DBSchema.h"
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <sqlite3.h>
#include <string>
#include <vector>

// Структура для записи пароля
struct VaultEntry
{
  int id;
  std::string title;
  std::string username;
  std::vector<uint8_t> encrypted_password; // BLOB данные
  std::string url;
  std::string notes;
  std::string tags;
  std::string created_at;
  std::string updated_at;

  // Преобразование в map для удобной сериализации
  std::map<std::string, std::string> toMap() const
  {
    return {{"id", std::to_string(id)}, {"title", title},
            {"username", username},     {"url", url},
            {"notes", notes},           {"tags", tags},
            {"created_at", created_at}, {"updated_at", updated_at}};
  }
};

// Структура для настройки
struct Setting
{
  int id;
  std::string key;
  std::string value;
  bool encrypted;
  std::string created_at;
  std::string updated_at;
};

// Структура для записи аудита
struct AuditLog
{
  int id;
  std::string action;
  std::string timestamp;
  int entry_id; // может быть -1 для NULL
  std::string details;
  std::vector<uint8_t> signature;
};

// Класс для работы с базой данных
class Database
{
private:
  std::string db_path;
  std::vector<sqlite3 *> connection_pool; // пул соединений
  std::mutex pool_mutex;        // мьютекс для доступа к пулу
  //std::mutex db_mutex;                    // мьютекс для операций с БД
  int max_connections;

  // Внутренние вспомогательные методы
  sqlite3 *getConnection();
  void releaseConnection(sqlite3 *conn);
  bool executeScript(const std::string &script);
  int getCurrentVersion();
  void setVersion(int version);
  void runMigrations(int current_version);

public:
  // Конструктор/деструктор
  Database(const std::string &path, int max_conn = 5);
  ~Database();

  // Инициализация и миграции
  bool initialize();
  bool createTables();
  void checkMigration();

  void closeAllConnections();

  // CRUD операции для записей
  int addEntry(const std::string &title, const std::string &username,
               const std::vector<uint8_t> &encrypted_password,
               const std::string &url = "", const std::string &notes = "",
               const std::string &tags = "");

  bool updateEntry(int entry_id, const std::string &title,
                   const std::string &username,
                   const std::vector<uint8_t> &encrypted_password,
                   const std::string &url, const std::string &notes,
                   const std::string &tags);

  bool deleteEntry(int entry_id);

  std::unique_ptr<VaultEntry> getEntry(int entry_id);
  std::vector<VaultEntry> getAllEntries();
  std::vector<VaultEntry> searchEntries(const std::string &search_term);

  // Работа с настройками
  std::string getSetting(const std::string &key,
                         const std::string &default_value = "");
  bool setSetting(const std::string &key, const std::string &value,
                  bool encrypted = false);

  // Работа с аудитом
  int addAuditLog(const std::string &action, int entry_id = -1,
                  const std::string &details = "",
                  const std::vector<uint8_t> &signature = {});

  std::vector<AuditLog> getAuditLogs(int limit = 100);

  // Бэкап и восстановление (заглушки для Sprint 8)
  bool backup(const std::string &backup_path = "");
  bool restore(const std::string &backup_path);

  // Утилиты
  int getVersion() { return getCurrentVersion(); }

  // Сохранить данные аутентификации (Argon2 хеш и соль)
  bool saveAuthData(const std::vector<uint8_t> &hash,
                    const std::vector<uint8_t> &salt, uint32_t time_cost,
                    uint32_t memory_cost, uint32_t parallelism,
                    uint32_t hash_len);

  // Получить данные аутентификации
  bool getAuthData(std::vector<uint8_t> &hash, std::vector<uint8_t> &salt,
                   uint32_t &time_cost, uint32_t &memory_cost,
                   uint32_t &parallelism, uint32_t &hash_len);

  // Сохранить соль для PBKDF2 (encryption key derivation)
  bool saveEncSalt(const std::vector<uint8_t> &salt);

  // Получить соль для PBKDF2
  bool getEncSalt(std::vector<uint8_t> &salt);
};

#endif
