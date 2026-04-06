#include <gtest/gtest.h>
#include "../src/core/crypto/AES256.h"
#include "../src/core/key_manager.h"
#include "../src/core/vault/plaintext_entry.h"

class EncryptionTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Создаём тестовый ключ (32 байта)
        std::vector<uint8_t> testKey = {
            0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
            0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0, 0x01,
            0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
            0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00
        };
        KeyManager::getInstance().store_key(testKey);
        KeyManager::getInstance().get_key(m_key);
    }

    void TearDown() override
    {
        KeyManager::getInstance().logout();
    }

    AES256GCM m_crypto;
    KeyManager::KeyData m_key;
};

TEST_F(EncryptionTest, EncryptionRoundTrip)
{
    // Создаём оригинальную запись
    PlaintextEntry original;
    original.title = "Test Entry";
    original.username = "testuser@example.com";
    original.password = "MySecretPassword123!";
    original.url = "https://test.com";
    original.notes = "Test notes for encryption";
    original.category = "Test";
    original.tags = "test,encryption";
    original.version = 1;

    // Шифруем
    auto encrypted = m_crypto.encrypt(m_key, original);
    ASSERT_GT(encrypted.size(), 0) << "Encryption failed - empty BLOB";

    // Проверяем, что зашифрованные данные не содержат открытый текст
    std::string encryptedStr(encrypted.begin(), encrypted.end());
    bool isPlaintextVisible = encryptedStr.find(original.password) != std::string::npos;
    EXPECT_FALSE(isPlaintextVisible) << "Plaintext password found in encrypted BLOB!";

    // Расшифровываем
    auto decrypted = m_crypto.decrypt(encrypted, m_key);

    // Проверяем целостность данных
    EXPECT_EQ(original.title, decrypted.title);
    EXPECT_EQ(original.username, decrypted.username);
    EXPECT_EQ(original.password, decrypted.password);
    EXPECT_EQ(original.url, decrypted.url);
    EXPECT_EQ(original.notes, decrypted.notes);
    EXPECT_EQ(original.category, decrypted.category);
}

TEST_F(EncryptionTest, EncryptionWithEmptyPassword)
{
    PlaintextEntry original;
    original.title = "Empty Password Entry";
    original.username = "user@example.com";
    original.password = "";

    auto encrypted = m_crypto.encrypt(m_key, original);
    ASSERT_GT(encrypted.size(), 0);

    auto decrypted = m_crypto.decrypt(encrypted, m_key);
    EXPECT_EQ(original.password, decrypted.password);
}

TEST_F(EncryptionTest, EncryptionWithLongPassword)
{
    PlaintextEntry original;
    original.title = "Long Password Entry";
    original.username = "user@example.com";
    original.password = std::string(500, 'A');  // 500 символов

    auto encrypted = m_crypto.encrypt(m_key, original);
    auto decrypted = m_crypto.decrypt(encrypted, m_key);

    EXPECT_EQ(original.password, decrypted.password);
}
