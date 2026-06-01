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
#include <cstdint>

#include "../src/core/LogEntry.h"

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
    int max_connections;

    // Внутренние вспомогательные методы
    bool executeScript(const std::string &script);
    int getCurrentVersion();
    void setVersion(int version);
    void runMigrations(int current_version);

public:
    // Конструктор/деструктор
    Database(const std::string &path, int max_conn = 5);
    ~Database();

    sqlite3* getConnection();
    void releaseConnection(sqlite3 *conn);

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


    bool addLogEntry(std::string& previous_hash,
                               std::string& current_hash,
                               std::string& entry_data,
                               std::vector<uint8_t>& signature,
                               int key_version,
                               EventType type);

    bool getLogEntry(int sequence_number,
                               std::string& previous_hash,
                               std::string& current_hash,
                               std::string& entry_data,
                               std::vector<uint8_t>& signature,
                               int& key_version,
                               std::string& created_at,
                               std::string& event_type);

    bool addPublicKey(const std::vector<uint8_t>& publicKey,
                                int keyVersion,
                      int validFromSequence);

    std::vector<AuditEntryDisplay> getAuditPage(
        int offset,                          // с какой записи начать (пропустить N записей)
        int limit,                           // сколько записей взять (размер страницы)
        std::string& sortColumn,           // по какой колонке сортировать
        bool sortOrder,             // порядок сортировки (ASC - true/DESC - false)
        std::string& eventTypeFilter,      // фильтр по типу события (пустая строка = все)
        const std::string& dateFrom,               // фильтр по дате "с" (невалидная = без фильтра)
        const std::string& dateTo,                 // фильтр по дате "по" (невалидная = без фильтра)
        const std::string& searchText            // поисковый текст (пустая строка = без поиска)
        );

    bool getPublicKeyForSequence(int sequenceNumber, std::vector<uint8_t>& publicKey, int& keyVersion);

    std::string getLastEntryHash();

    int getLogEntryCount();

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

    void initDefaultSettings();

    // Добавление нового контакта
    bool addContact(const std::string& name, const std::string& publicKeyPEM);

    // Получение всех контактов
    std::vector<std::pair<int, std::string>> getAllContacts();  // возвращает (id, name)

    // Получение публичного ключа по имени контакта
    bool getContactPublicKey(const std::string& name, std::vector<uint8_t>& publicKey);

    // Получение публичного ключа по id контакта
    bool getContactPublicKeyById(int contactId, std::vector<uint8_t>& publicKey);

    // Удаление контакта
    bool deleteContact(int contactId);

    // Обновление времени последнего использования
    bool updateContactLastUsed(int contactId);

    bool reencryptAllEntries(int& reencryptedCount);
};

#endif
