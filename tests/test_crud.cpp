#include <gtest/gtest.h>
#include <filesystem>
#include "../src/database/DB_helper/db_helper.h"
#include "../src/core/vault/VaultManager.h"
#include "../src/core/crypto/AES256.h"
#include "../src/core/key_manager.h"

class CrudTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        test_db_path = "test_crud.db";

        if (std::filesystem::exists(test_db_path)) {
            std::filesystem::remove(test_db_path);
        }

        m_db = std::make_unique<Database>(test_db_path);
        m_db->initialize();

        // Инициализируем ключ
        std::vector<uint8_t> testKey = {
            0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
            0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0, 0x01,
            0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
            0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00
        };
        KeyManager::getInstance().store_key(testKey);

        m_vaultManager = std::make_unique<VaultManager>(*m_db, m_crypto, KeyManager::getInstance());
    }

    void TearDown() override
    {
        m_vaultManager.reset();
        m_db.reset();

        if (std::filesystem::exists(test_db_path)) {
            std::filesystem::remove(test_db_path);
        }

        KeyManager::getInstance().logout();
    }

    PlaintextEntry createTestEntry(int index)
    {
        PlaintextEntry entry;
        entry.title = "Test Entry " + std::to_string(index);
        entry.username = "user" + std::to_string(index) + "@test.com";
        entry.password = "Password" + std::to_string(index) + "!";
        entry.url = "https://test" + std::to_string(index) + ".com";
        entry.notes = "Test notes for entry " + std::to_string(index);
        entry.category = (index % 2 == 0) ? "Even" : "Odd";
        entry.tags = "test";
        entry.version = 1;
        return entry;
    }

    std::string test_db_path;
    std::unique_ptr<Database> m_db;
    std::unique_ptr<VaultManager> m_vaultManager;
    AES256GCM m_crypto;
};

TEST_F(CrudTest, CreateEntry)
{
    auto entry = createTestEntry(1);
    int id = m_vaultManager->createEntry(entry);

    EXPECT_GT(id, 0) << "Failed to create entry";

    auto retrieved = m_vaultManager->getEntry(id);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(entry.title, retrieved->title);
    EXPECT_EQ(entry.username, retrieved->username);
    EXPECT_EQ(entry.password, retrieved->password);
}

TEST_F(CrudTest, ReadEntry)
{
    auto entry = createTestEntry(1);
    int id = m_vaultManager->createEntry(entry);

    auto retrieved = m_vaultManager->getEntry(id);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(entry.title, retrieved->title);
}

TEST_F(CrudTest, UpdateEntry)
{
    auto entry = createTestEntry(1);
    int id = m_vaultManager->createEntry(entry);

    entry.title = "Updated Title";
    entry.notes = "Updated notes";
    bool updated = m_vaultManager->updateEntry(id, entry);

    EXPECT_TRUE(updated);

    auto retrieved = m_vaultManager->getEntry(id);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ("Updated Title", retrieved->title);
    EXPECT_EQ("Updated notes", retrieved->notes);
}

TEST_F(CrudTest, DeleteEntry)
{
    auto entry = createTestEntry(1);
    int id = m_vaultManager->createEntry(entry);

    bool deleted = m_vaultManager->deleteEntry(id);
    EXPECT_TRUE(deleted);

    auto retrieved = m_vaultManager->getEntry(id);
    EXPECT_EQ(retrieved, nullptr);
}

TEST_F(CrudTest, CreateMultipleEntries)
{
    const int count = 100;
    std::vector<int> ids;

    for (int i = 0; i < count; i++) {
        int id = m_vaultManager->createEntry(createTestEntry(i));
        if (id != -1) {
            ids.push_back(id);
        }
    }

    EXPECT_EQ(ids.size(), count);

    auto allEntries = m_vaultManager->getAllEntryMetadata();
    EXPECT_EQ(allEntries.size(), count);
}

TEST_F(CrudTest, UpdateMultipleEntries)
{
    const int count = 50;
    std::vector<int> ids;

    for (int i = 0; i < count; i++) {
        ids.push_back(m_vaultManager->createEntry(createTestEntry(i)));
    }

    int updatedCount = 0;
    for (int id : ids) {
        auto entry = m_vaultManager->getEntry(id);
        if (entry) {
            entry->title = "Updated " + entry->title;
            if (m_vaultManager->updateEntry(id, *entry)) {
                updatedCount++;
            }
        }
    }

    EXPECT_EQ(updatedCount, count);

    for (int id : ids) {
        auto entry = m_vaultManager->getEntry(id);
        ASSERT_NE(entry, nullptr);
        EXPECT_TRUE(entry->title.find("Updated") == 0);
    }
}

TEST_F(CrudTest, DeleteMultipleEntries)
{
    const int count = 50;
    std::vector<int> ids;

    for (int i = 0;i < count; i++) {
        ids.push_back(m_vaultManager->createEntry(createTestEntry(i)));
    }

    int deletedCount = 0;
    for (int i = 0; i < 30; i++) {
        if (m_vaultManager->deleteEntry(ids[i])) {
            deletedCount++;
        }
    }

    EXPECT_EQ(deletedCount, 30);

    auto remaining = m_vaultManager->getAllEntryMetadata();
    EXPECT_EQ(remaining.size(), 20);
}

TEST_F(CrudTest, EntryNotFound)
{
    auto entry = m_vaultManager->getEntry(99999);
    EXPECT_EQ(entry, nullptr);

    bool deleted = m_vaultManager->deleteEntry(99999);
    EXPECT_FALSE(deleted);

    auto entry2 = createTestEntry(1);
    bool updated = m_vaultManager->updateEntry(99999, entry2);
    EXPECT_FALSE(updated);
}
