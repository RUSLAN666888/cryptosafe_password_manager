// tests/test_aes_placeholder.cpp
#include "../src/core/crypto/AES256Placeholder.h"
#include <gtest/gtest.h>
#include <random>
#include <vector>

using namespace Crypto;

class AES256PlaceholderTest : public ::testing::Test
{
protected:
  AES256Placeholder crypto;
  std::vector<uint8_t> key;
  std::vector<uint8_t> data;

  void SetUp() override
  {
    // Ключ 32 байта (256 бит)
    key = std::vector<uint8_t>(32, 0xAB);

    // Тестовые данные
    data = {0x01, 0x02, 0x03, 0x04, 0x05};
  }
};

// Тест 1: Шифрование и дешифрование должны быть обратимыми
TEST_F(AES256PlaceholderTest, EncryptDecryptRoundTrip)
{
  auto encrypted = crypto.encrypt(data, key);
  auto decrypted = crypto.decrypt(encrypted, key);

  EXPECT_EQ(data, decrypted);
}

// Тест 2: Шифрование должно изменять данные
TEST_F(AES256PlaceholderTest, EncryptChangesData)
{
  auto encrypted = crypto.encrypt(data, key);

  EXPECT_NE(data, encrypted);
}

// Тест 3: Разные ключи дают разные результаты
TEST_F(AES256PlaceholderTest, DifferentKeysProduceDifferentResults)
{
  std::vector<uint8_t> key2(32, 0xCD);

  auto encrypted1 = crypto.encrypt(data, key);
  auto encrypted2 = crypto.encrypt(data, key2);

  EXPECT_NE(encrypted1, encrypted2);
}

// Тест 4: Одинаковые данные с одинаковым ключом дают одинаковый результат
TEST_F(AES256PlaceholderTest, SameInputSameKeySameOutput)
{
  auto encrypted1 = crypto.encrypt(data, key);
  auto encrypted2 = crypto.encrypt(data, key);

  EXPECT_EQ(encrypted1, encrypted2);
}

// Тест 5: Пустые данные
TEST_F(AES256PlaceholderTest, EmptyData)
{
  std::vector<uint8_t> empty;

  auto encrypted = crypto.encrypt(empty, key);
  auto decrypted = crypto.decrypt(encrypted, key);

  EXPECT_TRUE(encrypted.empty());
  EXPECT_TRUE(decrypted.empty());
}

// Тест 6: Большие данные (проверка производительности и корректности)
TEST_F(AES256PlaceholderTest, LargeData)
{
  std::vector<uint8_t> largeData(1024 * 1024, 0xAA); // 1 MB

  auto encrypted = crypto.encrypt(largeData, key);
  auto decrypted = crypto.decrypt(encrypted, key);

  EXPECT_EQ(largeData, decrypted);
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
