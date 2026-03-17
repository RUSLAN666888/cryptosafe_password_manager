// tests/test_config_integration.cpp
#include "../src/core/config_handler.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

class ConfigTest : public ::testing::Test
{
protected:
  std::string home_backup;
  std::string test_home = "/tmp/cryptosafe_test";

  void SetUp() override
  {
    const char *home = std::getenv("HOME");
    home_backup = home ? home : "";

    std::filesystem::remove_all(test_home);
    std::filesystem::create_directories(test_home);

    setenv("HOME", test_home.c_str(), 1);
  }

  void TearDown() override
  {
    setenv("HOME", home_backup.c_str(), 1);
    std::filesystem::remove_all(test_home);
  }

  void createCorruptedConfig()
  {
    std::string config_path = test_home + "/.cryptosafe/config.json";
    std::filesystem::create_directories(test_home + "/.cryptosafe");
    std::ofstream file(config_path);
    file << "{ this is not valid json }";
    file.close();
  }
};

// ТЕСТ 1: Загрузка настроек по умолчанию при отсутствии файла
TEST_F(ConfigTest, LoadsDefaultsWhenFileMissing)
{
  std::string configPath = test_home + "/.cryptosafe/config.json";
  EXPECT_FALSE(std::filesystem::exists(configPath));

  ConfigHander config;

  EXPECT_EQ(config.getDatabasePath(), test_home + "/.cryptosafe/vault.db");
  EXPECT_EQ(config.getBackupPath(), test_home + "/.cryptosafe/backups");
  EXPECT_EQ(config.getConnectionTimeout(), 5);
  EXPECT_EQ(config.getEncryptionAlgorithm(), "AES-256-GCM");
  EXPECT_EQ(config.getKeyLength(), 32);
  EXPECT_EQ(config.getArgon2TimeCost(), 3);
  EXPECT_EQ(config.getArgon2MemoryCost(), 64);
  EXPECT_EQ(config.getArgon2Parallelism(), 4);
  EXPECT_EQ(config.getArgon2HashLength(), 32);
  EXPECT_TRUE(config.isFirstRun());

  EXPECT_TRUE(std::filesystem::exists(configPath));
}

// ТЕСТ 2: Загрузка из существующего файла
TEST_F(ConfigTest, LoadsFromExistingFile)
{
  ConfigHander config1;
  config1.setDatabasePath("/custom/path/vault.db");
  config1.setArgon2TimeCost(10);
  config1.setArgon2MemoryCost(256);
  config1.setArgon2Parallelism(8);
  config1.setArgon2HashLength(64);
  config1.setFirstRun(false);

  ConfigHander config2;

  EXPECT_EQ(config2.getDatabasePath(), "/custom/path/vault.db");
  EXPECT_EQ(config2.getArgon2TimeCost(), 10);
  EXPECT_EQ(config2.getArgon2MemoryCost(), 256);
  EXPECT_EQ(config2.getArgon2Parallelism(), 8);
  EXPECT_EQ(config2.getArgon2HashLength(), 64);
  EXPECT_FALSE(config2.isFirstRun());
}

// ТЕСТ 3: Обработка поврежденного файла
TEST_F(ConfigTest, HandlesCorruptedFile)
{
  createCorruptedConfig();

  ConfigHander config;

  EXPECT_EQ(config.getDatabasePath(), test_home + "/.cryptosafe/vault.db");
  EXPECT_EQ(config.getArgon2TimeCost(), 3);
  EXPECT_TRUE(config.isFirstRun());
}

// ТЕСТ 4: Сохранение изменений
TEST_F(ConfigTest, SavesChanges)
{
  ConfigHander config1;
  config1.setDatabasePath("/new/path.db");
  config1.setArgon2TimeCost(15);
  config1.setArgon2MemoryCost(512);
  config1.setArgon2Parallelism(12);
  config1.setArgon2HashLength(48);
  config1.setFirstRun(false);

  ConfigHander config2;

  EXPECT_EQ(config2.getDatabasePath(), "/new/path.db");
  EXPECT_EQ(config2.getArgon2TimeCost(), 15);
  EXPECT_EQ(config2.getArgon2MemoryCost(), 512);
  EXPECT_EQ(config2.getArgon2Parallelism(), 12);
  EXPECT_EQ(config2.getArgon2HashLength(), 48);
  EXPECT_FALSE(config2.isFirstRun());
}

// ТЕСТ 5: Множественные изменения
TEST_F(ConfigTest, MultipleSettingsPersist)
{
  ConfigHander config1;
  config1.setDatabasePath("/path1.db");
  config1.setArgon2TimeCost(5);
  config1.setArgon2MemoryCost(128);

  ConfigHander config2;
  EXPECT_EQ(config2.getDatabasePath(), "/path1.db");
  EXPECT_EQ(config2.getArgon2TimeCost(), 5);
  EXPECT_EQ(config2.getArgon2MemoryCost(), 128);

  config2.setDatabasePath("/path2.db");
  config2.setArgon2TimeCost(10);

  ConfigHander config3;
  EXPECT_EQ(config3.getDatabasePath(), "/path2.db");
  EXPECT_EQ(config3.getArgon2TimeCost(), 10);
  EXPECT_EQ(config3.getArgon2MemoryCost(), 128);
}

// ТЕСТ 6: Пути с пробелами
TEST_F(ConfigTest, HandlesPathsWithSpaces)
{
  ConfigHander config;
  std::string specialPath = "/path/with spaces/vault.db";
  config.setDatabasePath(specialPath);

  ConfigHander config2;
  EXPECT_EQ(config2.getDatabasePath(), specialPath);
}