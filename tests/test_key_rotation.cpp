#include "../src/database/DB_helper/db_helper.h"
#include "../src/core/vault/VaultManager.h"
#include "../src/core/vault/plaintext_entry.h"
#include "../src/core/key_manager.h"
#include "../src/core/crypto/AES256.h"
#include <gtest/gtest.h>
#include <vector>

class KeyRotationTest : public ::testing::Test
{
protected:

    std::string test_db_path = "test_vault.db";
    AES256GCM crypto;
    Database* db_ptr = nullptr;
    VaultManager* vm_ptr = nullptr;

    void SetUp() override
    {
        // Удаляем старый тестовый файл, если он существует
        if (std::filesystem::exists(test_db_path))
        {
            std::filesystem::remove(test_db_path);
        }

        // Создаем новую базу данных для каждого теста
        static Database db(test_db_path);
        static VaultManager vm(db, crypto, KeyManager::getInstance());

        db_ptr = &db;
        vm_ptr = &vm;

        // Инициализируем базу (создаем таблицы)
        bool init_result = db.initialize();
        ASSERT_TRUE(init_result) << "Failed to initialize database";
    }

    void TearDown() override
    {
        // Закрываем все соединения
        db_ptr->closeAllConnections();

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
        PlaintextEntry entry;
        entry.title = title;
        entry.username = username;
        entry.password = std::string(password.begin(), password.end());  // преобразуем vector<uint8_t> в string
        entry.url = url;
        entry.notes = notes;
        entry.category = "General";  // значение по умолчанию
        entry.creation_timestamp = "";  // будет установлено БД
        entry.tags = tags;
        entry.version = 1;

        return vm_ptr->createEntry(entry);
    }
};

TEST_F(KeyRotationTest, key_rotation_test)
{
    std::vector<uint8_t> old_key = {
        0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
        0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0, 0x01,
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00
    };

    std::vector<uint8_t> new_key = {
        0x11, 0x21, 0x31, 0x41, 0x51, 0x61, 0x71, 0x81,
        0x91, 0xA1, 0xB1, 0xC1, 0xD1, 0xE1, 0xF1, 0x02,
        0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78, 0x89,
        0x9A, 0xAB, 0xBC, 0xCD, 0xDE, 0xEF, 0x01, 0x02
    };
    std::vector<int> ids;

    KeyManager::getInstance().store_key(old_key);

    ids.push_back(createTestEntry("Entry 1"));
    ids.push_back(createTestEntry("Entry 2", "user2"));
    ids.push_back(createTestEntry("Entry 3", "user3", {0x01, 0x02, 0x03}));
    ids.push_back(createTestEntry("Entry 4", "user4", {0x04, 0x05}, "https://site4.com"));
    ids.push_back(createTestEntry("Entry 5", "user5", {0x06}, "https://site5.com", "notes5"));
    ids.push_back(createTestEntry("Entry 6", "user6", {0x07, 0x08}, "https://site6.com", "notes6", "[\"tag1\"]"));
    ids.push_back(createTestEntry("Entry 7", "user7", {0x09}, "https://site7.com"));
    ids.push_back(createTestEntry("Entry 8", "user8", {0x0A, 0x0B, 0x0C}, "https://site8.com", "notes8"));
    ids.push_back(createTestEntry("Entry 9", "user9", {0x0D}, "https://site9.com", "notes9", "[\"tag2\", \"tag3\"]"));
    ids.push_back(createTestEntry("Entry 10", "user10"));

    ASSERT_EQ(ids.size(), 10);

    for (int id : ids){
        auto entry = vm_ptr->getEntry(id, false);
        ASSERT_NE(entry, nullptr);
    }

    old_key = {
        0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
        0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0, 0x01,
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00
    };

    KeyManager::getInstance().store_old_key(old_key);
    KeyManager::getInstance().store_key(new_key);

    ASSERT_EQ(true, vm_ptr->rotate());

    for (int id : ids) {
        auto entry = vm_ptr->getEntry(id, false);
        ASSERT_NE(entry, nullptr);
    }
}
