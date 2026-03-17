// tests/test_database.cpp
#include "../src/database/DB_helper/db_helper.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

class DatabaseTest : public ::testing::Test
{
protected:
  std::unique_ptr<Database> db;
  std::string test_db_path = "test_vault.db";

  void SetUp() override
  {
    // Удаляем старый тестовый файл, если он существует
    if (std::filesystem::exists(test_db_path))
    {
      std::filesystem::remove(test_db_path);
    }

    // Создаем новую базу данных для каждого теста
    db = std::make_unique<Database>(test_db_path);

    // Инициализируем базу (создаем таблицы)
    bool init_result = db->initialize();
    ASSERT_TRUE(init_result) << "Failed to initialize database";
  }

  void TearDown() override
  {
    // Закрываем все соединения
    db.reset();

    // Удаляем тестовый файл после каждого теста
    if (std::filesystem::exists(test_db_path))
    {
      std::filesystem::remove(test_db_path);
    }
  }

  // Вспомогательная функция для создания тестовой записи
  int createTestEntry(const std::string &title = "Test Title",
                      const std::string &username = "testuser",
                      const std::vector<uint8_t> &password = {0x01, 0x02, 0x03},
                      const std::string &url = "https://test.com",
                      const std::string &notes = "Test notes",
                      const std::string &tags = "[\"test\"]")
  {
    return db->addEntry(title, username, password, url, notes, tags);
  }
};

// ========== ТЕСТЫ СОЗДАНИЯ БАЗЫ ==========

// Тест 1: Проверка инициализации базы
TEST_F(DatabaseTest, DatabaseInitialization)
{
  // Проверяем, что файл базы создан
  EXPECT_TRUE(std::filesystem::exists(test_db_path));
}

// Тест 2: Проверка создания таблиц
TEST_F(DatabaseTest, TablesExist)
{
  // Пытаемся вставить запись - если таблицы нет, будет ошибка
  int entry_id = createTestEntry();
  EXPECT_GT(entry_id, 0) << "Failed to insert into vault_entries";

  // Проверяем, что запись можно получить
  auto entry = db->getEntry(entry_id);
  EXPECT_NE(entry, nullptr);
}

// ========== ТЕСТЫ VAULT ENTRIES ==========

// Тест 3: Добавление записи
TEST_F(DatabaseTest, AddEntry)
{
  std::vector<uint8_t> password = {0x10, 0x20, 0x30, 0x40};

  int entry_id =
      createTestEntry("GitHub", "octocat", password, "https://github.com",
                      "Main account", "[\"work\", \"dev\"]");

  EXPECT_GT(entry_id, 0);

  // Получаем запись и проверяем поля
  auto entry = db->getEntry(entry_id);
  ASSERT_NE(entry, nullptr);

  EXPECT_EQ(entry->title, "GitHub");
  EXPECT_EQ(entry->username, "octocat");
  EXPECT_EQ(entry->encrypted_password, password);
  EXPECT_EQ(entry->url, "https://github.com");
  EXPECT_EQ(entry->notes, "Main account");
  EXPECT_EQ(entry->tags, "[\"work\", \"dev\"]");
  EXPECT_FALSE(entry->created_at.empty());
  EXPECT_FALSE(entry->updated_at.empty());
}

// Тест 4: Получение несуществующей записи
TEST_F(DatabaseTest, GetNonExistentEntry)
{
  auto entry = db->getEntry(99999);
  EXPECT_EQ(entry, nullptr);
}

// Тест 5: Получение всех записей
TEST_F(DatabaseTest, GetAllEntries)
{
  // Создаем несколько записей
  int id1 = createTestEntry("AAA", "user1");
  int id2 = createTestEntry("BBB", "user2");
  int id3 = createTestEntry("CCC", "user3");

  EXPECT_GT(id1, 0);
  EXPECT_GT(id2, 0);
  EXPECT_GT(id3, 0);

  auto entries = db->getAllEntries();
  EXPECT_EQ(entries.size(), 3);

  // Проверяем, что записи отсортированы по title
  EXPECT_EQ(entries[0].title, "AAA");
  EXPECT_EQ(entries[1].title, "BBB");
  EXPECT_EQ(entries[2].title, "CCC");
}

// Тест 6: Обновление записи
TEST_F(DatabaseTest, UpdateEntry)
{
  int entry_id = createTestEntry("Old Title", "olduser");
  ASSERT_GT(entry_id, 0);

  std::vector<uint8_t> new_password = {0x99, 0x88, 0x77};

  bool updated =
      db->updateEntry(entry_id, "New Title", "newuser", new_password,
                      "https://new.com", "New notes", "[\"updated\"]");

  EXPECT_TRUE(updated);

  auto entry = db->getEntry(entry_id);
  ASSERT_NE(entry, nullptr);

  EXPECT_EQ(entry->title, "New Title");
  EXPECT_EQ(entry->username, "newuser");
  EXPECT_EQ(entry->encrypted_password, new_password);
  EXPECT_EQ(entry->url, "https://new.com");
  EXPECT_EQ(entry->notes, "New notes");
  EXPECT_EQ(entry->tags, "[\"updated\"]");
}

// Тест 7: Обновление несуществующей записи
TEST_F(DatabaseTest, UpdateNonExistentEntry)
{
  bool updated = db->updateEntry(99999, "Title", "user", {}, "", "", "");
  EXPECT_FALSE(updated);
}

// Тест 8: Удаление записи
TEST_F(DatabaseTest, DeleteEntry)
{
  int entry_id = createTestEntry("To Delete", "delete_me");
  ASSERT_GT(entry_id, 0);

  bool deleted = db->deleteEntry(entry_id);
  EXPECT_TRUE(deleted);

  auto entry = db->getEntry(entry_id);
  EXPECT_EQ(entry, nullptr);

  auto entries = db->getAllEntries();
  EXPECT_TRUE(entries.empty());
}

// Тест 9: Удаление несуществующей записи
TEST_F(DatabaseTest, DeleteNonExistentEntry)
{
  bool deleted = db->deleteEntry(99999);
  EXPECT_FALSE(deleted);
}

// ========== ТЕСТЫ ПОИСКА ==========

// Тест 10: Поиск записей
TEST_F(DatabaseTest, SearchEntries)
{

  // Создаем записи с паролями
  std::vector<uint8_t> default_password = {0x01, 0x02, 0x03};

  int id1 = createTestEntry("GitHub", "octocat", default_password,
                            "https://github.com");
  int id2 = createTestEntry("Gmail", "user@gmail.com", default_password,
                            "https://gmail.com");
  int id3 = createTestEntry("Work Email", "user@work.com", default_password,
                            "https://work.com");

  // Проверяем, что ID положительные
  ASSERT_GT(id1, 0);
  ASSERT_GT(id2, 0);
  ASSERT_GT(id3, 0);

  auto results = db->searchEntries("gmail");
  EXPECT_EQ(results.size(), 1);
  EXPECT_EQ(results[0].title, "Gmail");

  results = db->searchEntries("git");
  EXPECT_EQ(results.size(), 1);
  EXPECT_EQ(results[0].title, "GitHub");

  results = db->searchEntries("com");
  EXPECT_EQ(results.size(), 3); // Все три содержат .com

  results = db->searchEntries("nonexistent");
  EXPECT_TRUE(results.empty());
}

// ========== ТЕСТЫ НАСТРОЕК ==========

// Тест 11: Установка и получение настроек
TEST_F(DatabaseTest, Settings)
{
  bool set = db->setSetting("test_key", "test_value");
  EXPECT_TRUE(set);

  std::string value = db->getSetting("test_key", "default");
  EXPECT_EQ(value, "test_value");

  // Получение несуществующей настройки с дефолтом
  value = db->getSetting("nonexistent", "default_value");
  EXPECT_EQ(value, "default_value");
}

// Тест 12: Обновление существующей настройки
TEST_F(DatabaseTest, UpdateSetting)
{
  db->setSetting("theme", "dark");
  EXPECT_EQ(db->getSetting("theme", "light"), "dark");

  db->setSetting("theme", "light");
  EXPECT_EQ(db->getSetting("theme", "dark"), "light");
}

// Тест 13: Зашифрованные настройки
TEST_F(DatabaseTest, EncryptedSetting)
{
  db->setSetting("secret", "sensitive_data", true);

  // В Sprint 1 просто проверяем, что сохранилось
  std::string value = db->getSetting("secret", "");
  EXPECT_EQ(value, "sensitive_data");
}

// ========== ТЕСТЫ АУДИТ ЛОГА ==========

// Тест 14: Добавление записи в аудит
TEST_F(DatabaseTest, AddAuditLog)
{
  int entry_id = createTestEntry();
  ASSERT_GT(entry_id, 0);

  int log_id = db->addAuditLog("TestAction", entry_id, "{\"test\": true}");
  EXPECT_GT(log_id, 0);

  auto logs = db->getAuditLogs(10);
  ASSERT_FALSE(logs.empty());

  bool found = false;
  for (const auto &log : logs)
  {
    if (log.action == "TestAction" && log.entry_id == entry_id)
    {
      found = true;
      EXPECT_EQ(log.details, "{\"test\": true}");
      break;
    }
  }
  EXPECT_TRUE(found);
}

// Тест 15: CRUD операции создают аудит логи
TEST_F(DatabaseTest, CrudCreatesAuditLogs)
{
  // Добавление
  int entry_id = createTestEntry();
  ASSERT_GT(entry_id, 0); // Проверяем, что создалось

  auto logs = db->getAuditLogs(10);
  bool found_add = false;
  for (const auto &log : logs)
  {
    if (log.action == "EntryAdded" && log.entry_id == entry_id)
    {
      found_add = true;
      break;
    }
  }
  EXPECT_TRUE(found_add);

  std::vector<uint8_t> default_password = {0x01, 0x02, 0x03};
  bool updated =
      db->updateEntry(entry_id, "Updated", "user", default_password,
                      "https://updated.com", "Updated notes", "[\"updated\"]");
  EXPECT_TRUE(updated); // Проверяем, что обновление прошло успешно

  logs = db->getAuditLogs(10);
  bool found_update = false;
  for (const auto &log : logs)
  {
    if (log.action == "EntryUpdated" && log.entry_id == entry_id)
    {
      found_update = true;
      break;
    }
  }
  EXPECT_TRUE(found_update);

  // Удаление
  bool deleted = db->deleteEntry(entry_id);
  EXPECT_TRUE(deleted);

  logs = db->getAuditLogs(10);
  bool found_delete = false;
  for (const auto &log : logs)
  {
    if (log.action == "EntryDeleted" &&
        log.details.find(std::to_string(entry_id)) != std::string::npos)
    {
      found_delete = true;
      break;
    }
  }
  EXPECT_TRUE(found_delete);
}

// Тест 16: Получение аудит логов с лимитом
TEST_F(DatabaseTest, AuditLogLimit)
{
  // Создаем 5 записей (каждая создаст лог)
  for (int i = 0; i < 5; i++)
  {
    createTestEntry("Entry " + std::to_string(i), "user");
  }

  auto logs_3 = db->getAuditLogs(3);
  EXPECT_EQ(logs_3.size(), 3);

  auto logs_10 = db->getAuditLogs(10);
  EXPECT_EQ(logs_10.size(), 5); // Всего 5 логов
}

// ========== ТЕСТЫ ПУЛА СОЕДИНЕНИЙ ==========

// Тест 17: Множественные операции используют пул
TEST_F(DatabaseTest, ConnectionPool)
{
  // Выполняем много операций подряд
  for (int i = 0; i < 100; i++)
  {
    int id = createTestEntry("Entry " + std::to_string(i), "user");
    EXPECT_GT(id, 0);

    auto entry = db->getEntry(id);
    EXPECT_NE(entry, nullptr);

    if (i % 2 == 0)
    {
      EXPECT_TRUE(db->deleteEntry(id));
    }
  }

  // Если бы пул не работал, могли быть ошибки "database is locked"
  SUCCEED();
}

// ========== ТЕСТЫ НА ОШИБКИ ==========

// Тест 18: Попытка создать БД в недоступном месте
TEST(DatabaseErrorTest, InvalidPath)
{
  std::string invalid_path = "/nonexistent/directory/vault.db";

  EXPECT_THROW(
      {
        Database db(invalid_path);
        db.initialize();
      },
      std::exception);
}

// Тест 19: Очистка после себя
TEST_F(DatabaseTest, Cleanup)
{
  int entry_id = createTestEntry();
  ASSERT_GT(entry_id, 0);

  db.reset(); // Закрываем БД

  // Проверяем, что файл все еще существует
  EXPECT_TRUE(std::filesystem::exists(test_db_path));
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
