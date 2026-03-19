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

// // ТЕСТ 2: Сохранение и получение соли для PBKDF2
// TEST_F(AuthDataTest, SaveAndGetEncSalt) {
//     // Создаем тестовую соль
//     std::vector<uint8_t> testSalt = createTestData(0x30, 16);

//     // Сохраняем
//     bool saveResult = db->saveEncSalt(testSalt);
//     EXPECT_TRUE(saveResult);

//     // Получаем
//     std::vector<uint8_t> retrievedSalt;
//     bool getResult = db->getEncSalt(retrievedSalt);

//     EXPECT_TRUE(getResult);
//     EXPECT_EQ(testSalt, retrievedSalt);
// }

// // ТЕСТ 3: Попытка получить данные из пустой БД
// TEST_F(AuthDataTest, GetFromEmptyDB) {
//     std::vector<uint8_t> hash, salt;
//     uint32_t time_cost, memory_cost, parallelism, hash_len;

//     // Должно вернуть false
//     bool result = db->getAuthData(hash, salt, time_cost, memory_cost,
//                                   parallelism, hash_len);
//     EXPECT_FALSE(result);
// }

// // ТЕСТ 4: Попытка получить соль из пустой БД
// TEST_F(AuthDataTest, GetEncSaltFromEmptyDB) {
//     std::vector<uint8_t> salt;
//     bool result = db->getEncSalt(salt);
//     EXPECT_FALSE(result);
// }

// // ТЕСТ 5: Сохранение данных с разными параметрами
// TEST_F(AuthDataTest, SaveWithDifferentParams) {
//     std::vector<uint8_t> testHash = createTestData(0x40, 32);
//     std::vector<uint8_t> testSalt = createTestData(0x50, 16);

//     // Тестируем разные комбинации параметров
//     struct TestCase {
//         uint32_t time_cost;
//         uint32_t memory_cost;
//         uint32_t parallelism;
//         uint32_t hash_len;
//     };

//     std::vector<TestCase> testCases = {
//         {3, 64, 4, 32},
//         {4, 128, 8, 32},
//         {5, 256, 4, 48},
//         {2, 32, 2, 24}
//     };

//     for (const auto& tc : testCases) {
//         // Сохраняем
//         bool saveResult = db->saveAuthData(testHash, testSalt,
//                                            tc.time_cost, tc.memory_cost,
//                                            tc.parallelism, tc.hash_len);
//         EXPECT_TRUE(saveResult);

//         // Получаем
//         std::vector<uint8_t> retrievedHash, retrievedSalt;
//         uint32_t rt, rm, rp, rh;
//         bool getResult = db->getAuthData(retrievedHash, retrievedSalt,
//                                          rt, rm, rp, rh);

//         EXPECT_TRUE(getResult);
//         EXPECT_EQ(testHash, retrievedHash);
//         EXPECT_EQ(testSalt, retrievedSalt);
//         EXPECT_EQ(tc.time_cost, rt);
//         EXPECT_EQ(tc.memory_cost, rm);
//         EXPECT_EQ(tc.parallelism, rp);
//         EXPECT_EQ(tc.hash_len, rh);
//     }
// }

// // ТЕСТ 6: Проверка уникальности ключей (должны перезаписываться)
// TEST_F(AuthDataTest, OverwriteExistingData) {
//     // Первые данные
//     std::vector<uint8_t> hash1 = createTestData(0x60, 32);
//     std::vector<uint8_t> salt1 = createTestData(0x70, 16);

//     db->saveAuthData(hash1, salt1, 3, 64, 4, 32);

//     // Вторые данные
//     std::vector<uint8_t> hash2 = createTestData(0x80, 32);
//     std::vector<uint8_t> salt2 = createTestData(0x90, 16);

//     db->saveAuthData(hash2, salt2, 5, 128, 8, 48);

//     // Должны получить вторые данные
//     std::vector<uint8_t> retrievedHash, retrievedSalt;
//     uint32_t rt, rm, rp, rh;

//     db->getAuthData(retrievedHash, retrievedSalt, rt, rm, rp, rh);

//     EXPECT_EQ(hash2, retrievedHash);
//     EXPECT_EQ(salt2, retrievedSalt);
//     EXPECT_EQ(5, rt);
//     EXPECT_EQ(128, rm);
//     EXPECT_EQ(8, rp);
//     EXPECT_EQ(48, rh);
// }

// // ТЕСТ 7: Проверка на некорректные данные (пустые вектора)
// TEST_F(AuthDataTest, SaveEmptyData) {
//     std::vector<uint8_t> emptyHash;
//     std::vector<uint8_t> emptySalt;

//     // Сохранение пустых данных должно работать?
//     // (зависит от требований)
//     bool result = db->saveAuthData(emptyHash, emptySalt, 3, 64, 4, 32);
//     // Ожидаем false, так как соль и хеш должны быть не пустыми
//     EXPECT_FALSE(result);
// }

// // ТЕСТ 8: Сохранение и получение соли с разными размерами
// TEST_F(AuthDataTest, EncSaltDifferentSizes) {
//     std::vector<size_t> sizes = {8, 16, 32, 64};

//     for (size_t size : sizes) {
//         std::vector<uint8_t> testSalt = createTestData(0xA0, size);

//         bool saveResult = db->saveEncSalt(testSalt);
//         EXPECT_TRUE(saveResult);

//         std::vector<uint8_t> retrievedSalt;
//         bool getResult = db->getEncSalt(retrievedSalt);

//         EXPECT_TRUE(getResult);
//         EXPECT_EQ(testSalt, retrievedSalt);
//     }
// }