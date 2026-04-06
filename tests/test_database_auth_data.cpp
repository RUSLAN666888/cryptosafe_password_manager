// tests/test_auth_data.cpp
#include "../src/database/DBSchema.h"
#include "../src/database/DB_helper/db_helper.h"
#include <filesystem>
#include <gtest/gtest.h>
#include <vector>

class AuthDataTest : public ::testing::Test
{
protected:
  std::unique_ptr<Database> db;
  std::string test_db_path = "test_auth.db";

  void SetUp() override
  {
    std::filesystem::remove(test_db_path);
    db = std::make_unique<Database>(test_db_path);
    db->initialize(); // вместо executeScript
  }

  void TearDown() override
  {
    // Закрываем соединения и удаляем тестовый файл
    db.reset();
    std::filesystem::remove(test_db_path);
  }

  // Вспомогательная функция для создания тестовых данных
  std::vector<uint8_t> createTestData(uint8_t start, size_t size)
  {
    std::vector<uint8_t> data(size);
    for (size_t i = 0; i < size; i++)
    {
      data[i] = start + static_cast<uint8_t>(i);
    }
    return data;
  }
};

// ТЕСТ 1: Сохранение и получение данных аутентификации
TEST_F(AuthDataTest, SaveAndGetAuthData)
{
  // Создаем тестовые данные
  std::vector<uint8_t> testHash = createTestData(0x10, 32); // 32 байта
  std::vector<uint8_t> testSalt = createTestData(0x20, 16); // 16 байт
  uint32_t time_cost = 3;
  uint32_t memory_cost = 64;
  uint32_t parallelism = 4;
  uint32_t hash_len = 32;

  // Сохраняем данные
  bool saveResult = db->saveAuthData(testHash, testSalt, time_cost, memory_cost,
                                     parallelism, hash_len);
  EXPECT_TRUE(saveResult);

  // Получаем данные обратно
  std::vector<uint8_t> retrievedHash;
  std::vector<uint8_t> retrievedSalt;
  uint32_t retrieved_time, retrieved_memory, retrieved_parallel, retrieved_len;

  bool getResult =
      db->getAuthData(retrievedHash, retrievedSalt, retrieved_time,
                      retrieved_memory, retrieved_parallel, retrieved_len);

  EXPECT_TRUE(getResult);

  // Проверяем что все совпадает
  EXPECT_EQ(testHash, retrievedHash);
  EXPECT_EQ(testSalt, retrievedSalt);
  EXPECT_EQ(time_cost, retrieved_time);
  EXPECT_EQ(memory_cost, retrieved_memory);
  EXPECT_EQ(parallelism, retrieved_parallel);
  EXPECT_EQ(hash_len, retrieved_len);
}

