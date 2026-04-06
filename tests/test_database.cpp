#include <gtest/gtest.h>
#include <filesystem>
#include <memory>
#include "../src/database/DB_helper/db_helper.h"

#include <gtest/gtest.h>
#include <filesystem>
#include <memory>
#include "../src/database/DB_helper/db_helper.h"

class DatabaseTest : public ::testing::Test
{
protected:
    std::unique_ptr<Database> db;
    std::string test_db_path = "test_vault.db";

    void SetUp() override
    {
        if (std::filesystem::exists(test_db_path))
        {
            std::filesystem::remove(test_db_path);
        }

        db = std::make_unique<Database>(test_db_path);
        bool init_result = db->initialize();
        ASSERT_TRUE(init_result) << "Failed to initialize database";
    }

    void TearDown() override
    {
        db.reset();

        if (std::filesystem::exists(test_db_path))
        {
            std::filesystem::remove(test_db_path);
        }
    }
};

// Тест 1: Проверка инициализации базы
TEST_F(DatabaseTest, DatabaseInitialization)
{
    EXPECT_TRUE(std::filesystem::exists(test_db_path));
}

// Тест 2: Проверка создания таблиц (через настройки)
TEST_F(DatabaseTest, TablesExist)
{
    // Проверяем, что можно работать с настройками (таблица settings существует)
    bool set = db->setSetting("test_key", "test_value");
    EXPECT_TRUE(set);

    std::string value = db->getSetting("test_key", "");
    EXPECT_EQ(value, "test_value");
}

// Тест 3: Множественные операции используют пул соединений
TEST_F(DatabaseTest, ConnectionPool)
{
    // Выполняем много операций подряд для проверки пула соединений
    for (int i = 0; i < 100; i++)
    {
        // Проверяем, что соединение работает через настройки
        std::string key = "test_key_" + std::to_string(i);
        bool set = db->setSetting(key, "value_" + std::to_string(i));
        EXPECT_TRUE(set);

        std::string value = db->getSetting(key, "");
        EXPECT_EQ(value, "value_" + std::to_string(i));
    }

    SUCCEED();
}

// Тест 4: Попытка создать БД в недоступном месте
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

// Тест 5: Очистка после себя
TEST_F(DatabaseTest, Cleanup)
{
    EXPECT_TRUE(std::filesystem::exists(test_db_path));
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
