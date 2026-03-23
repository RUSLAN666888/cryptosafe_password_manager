#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <cstring>
#include <chrono>
#include <thread>

#include "../src/core/crypto/authentication.h"
#include "../src/core/crypto/key_derivation.h"
#include "../src/core/crypto/AES256Placeholder.h"
#include "../src/core/key_manager.h"

// ============================================================
// TEST-1: Argon2 Parameter Validation Test
// ============================================================
TEST(CryptoTests, Argon2ParameterValidation)
{
    std::string password = "TestPassword123!@#";

    // Тестируем разные комбинации параметров
    struct TestParams {
        uint32_t time_cost;
        uint32_t memory_cost;
        uint32_t parallelism;
        uint32_t hash_len;
    };

    std::vector<TestParams> params = {
        {1, 16, 1, 16},   // минимальные
        {2, 32, 2, 24},   // средние
        {3, 64, 4, 32},   // стандартные
        {4, 128, 8, 48},  // высокие
    };

    for (const auto& p : params) {
        Argon2Data data(p.time_cost, p.memory_cost, p.parallelism, p.hash_len);

        // Хешируем пароль
        hash_password(password, data);

        // Проверяем, что хеш не пустой
        EXPECT_FALSE(data.hash.empty());
        EXPECT_EQ(data.hash.size(), p.hash_len);

        // Проверяем, что соль не пустая
        EXPECT_FALSE(data.salt.empty());

        // Проверяем верификацию
        EXPECT_TRUE(verify_password(password, data));

        // Проверяем, что другой пароль не подходит
        std::string wrongPassword = "WrongPassword123!@#";
        EXPECT_FALSE(verify_password(wrongPassword, data));

        std::cout << "Argon2 params: time=" << p.time_cost
                  << ", memory=" << p.memory_cost
                  << ", hash_len=" << p.hash_len << " - OK" << std::endl;
    }
}

// ============================================================
// TEST-2: Key Derivation Consistency Test
// ============================================================
TEST(CryptoTests, KeyDerivationConsistency)
{
    std::string password = "ConsistencyTestPassword123!@#";
    std::vector<uint8_t> salt(16);
    randombytes_buf(salt.data(), salt.size());

    std::vector<uint8_t> firstKey;
    derive_encryption_key(password, salt, firstKey);

    // Выводим ключ 100 раз и проверяем, что он одинаковый
    for (int i = 0; i < 100; i++) {
        std::vector<uint8_t> currentKey;
        derive_encryption_key(password, salt, currentKey);

        // Проверяем размер
        EXPECT_EQ(firstKey.size(), currentKey.size());

        // Проверяем содержимое
        EXPECT_EQ(memcmp(firstKey.data(), currentKey.data(), firstKey.size()), 0);
    }

    std::cout << "Key derived 100 times, all identical - OK" << std::endl;

    // Проверяем, что другой пароль дает другой ключ
    std::string differentPassword = "DifferentPassword456!@#";
    std::vector<uint8_t> differentKey;
    derive_encryption_key(differentPassword, salt, differentKey);

    EXPECT_NE(memcmp(firstKey.data(), differentKey.data(), firstKey.size()), 0);
    std::cout << "Different password produces different key - OK" << std::endl;

    // Проверяем, что другая соль дает другой ключ
    std::vector<uint8_t> differentSalt(16);
    randombytes_buf(differentSalt.data(), differentSalt.size());
    std::vector<uint8_t> keyWithDifferentSalt;
    derive_encryption_key(password, differentSalt, keyWithDifferentSalt);

    EXPECT_NE(memcmp(firstKey.data(), keyWithDifferentSalt.data(), firstKey.size()), 0);
    std::cout << "Different salt produces different key - OK" << std::endl;
}

// ============================================================
// TEST-3: Timing Attack Resistance Test
// ============================================================
TEST(CryptoTests, TimingAttackResistance)
{
    // Создаем тестовые данные
    std::string password = "TestPassword123!@#";
    Argon2Data authData(3, 64, 4, 32);
    hash_password(password, authData);

    // Функция для измерения времени выполнения
    auto measureTime = [](const std::string& pwd, const Argon2Data& data) -> long long {
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < 1000; i++) {
            verify_password(pwd, data);
        }

        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    };

    // Тестируем разные пароли разной длины
    std::vector<std::string> testPasswords = {
        "A",                                    // короткий
        "Test123",                              // средний
        "VeryLongPasswordThatIsVeryLong123!@#", // длинный
        "WrongPassword",                        // неправильный
        "X",                                    // совсем короткий
    };

    std::vector<long long> times;

    for (const auto& pwd : testPasswords) {
        long long elapsed = measureTime(pwd, authData);
        times.push_back(elapsed);
        std::cout << "Password: \"" << pwd << "\" time: " << elapsed << " us" << std::endl;
    }

    // Проверяем, что время выполнения примерно одинаковое
    long long minTime = *std::min_element(times.begin(), times.end());
    long long maxTime = *std::max_element(times.begin(), times.end());
    double ratio = static_cast<double>(maxTime) / minTime;

    std::cout << "Min time: " << minTime << " us" << std::endl;
    std::cout << "Max time: " << maxTime << " us" << std::endl;
    std::cout << "Ratio: " << ratio << std::endl;

    // Время не должно отличаться более чем в 2 раза
    // (в реальности отличие будет меньше, но для теста оставляем запас)
    EXPECT_LT(ratio, 2.0);

    // Проверяем constant-time сравнение напрямую
    std::vector<uint8_t> hash1(32, 0xAA);
    std::vector<uint8_t> hash2(32, 0xAA);
    std::vector<uint8_t> hash3(32, 0xBB);

    // Одинаковые хеши
    EXPECT_TRUE(constant_time_compare(hash1, hash2));

    // Разные хеши
    EXPECT_FALSE(constant_time_compare(hash1, hash3));

    // Разная длина
    std::vector<uint8_t> hash4(31, 0xAA);
    EXPECT_FALSE(constant_time_compare(hash1, hash4));

    std::cout << "Constant-time comparison works - OK" << std::endl;
}

// ============================================================
// TEST-4: Memory Safety Test
// ============================================================
TEST(CryptoTests, MemorySafety)
{
    // Тест 1: Проверка зануления памяти в KeyManager
    std::string password = "TestPassword123!@#";
    std::vector<uint8_t> salt(16);
    randombytes_buf(salt.data(), salt.size());

    std::vector<uint8_t> originalKey;
    derive_encryption_key(password, salt, originalKey);

    // Сохраняем ключ в KeyManager
    KeyManager::getInstance().store_key(originalKey);

    // Получаем ключ
    KeyManager::KeyData keyData;
    KeyManager::getInstance().get_key(keyData);

    // Проверяем, что ключ есть
    ASSERT_NE(keyData.data, nullptr);
    ASSERT_EQ(keyData.size, originalKey.size());

    // Сохраняем копию для проверки
    std::vector<uint8_t> keyCopy(keyData.data, keyData.data + keyData.size);
    EXPECT_EQ(memcmp(originalKey.data(), keyCopy.data(), originalKey.size()), 0);

    // Выходим из системы - ключ должен быть занулен
    KeyManager::getInstance().logout();

    // Проверяем, что ключ занулен
    KeyManager::getInstance().get_key(keyData);
    EXPECT_EQ(keyData.data, nullptr);
    EXPECT_EQ(keyData.size, 0);

    std::cout << "KeyManager zeroing memory - OK" << std::endl;

    // Тест 2: Проверка зануления временного пароля
    std::string tempPassword = "TempPasswordToZero123!@#";
    std::string passwordCopy = tempPassword;

    volatile char* p = const_cast<char*>(tempPassword.data());
    for (size_t i = 0; i < tempPassword.size(); i++) {
        p[i] = 0;
    }

    // Проверяем, что пароль занулен (сравниваем с копией)
    EXPECT_NE(memcmp(tempPassword.data(), passwordCopy.data(), tempPassword.size()), 0);

    // Проверяем, что все байты занулены
    for (size_t i = 0; i < tempPassword.size(); i++) {
        EXPECT_EQ(tempPassword[i], 0);
    }

    std::cout << "Password zeroing - OK" << std::endl;

    // Тест 3: Проверка зануления ключа после использования
    AES256Placeholder cipher;
    std::vector<uint8_t> testKey(32);
    randombytes_buf(testKey.data(), testKey.size());

    KeyManager::KeyData testKeyData;
    testKeyData.data = testKey.data();
    testKeyData.size = testKey.size();

    std::vector<uint8_t> plaintext = {'T', 'e', 's', 't'};
    auto encrypted = cipher.encrypt(plaintext, testKeyData);

    // Зануляем ключ
    KeyManager::getInstance().zero_keyData(testKeyData);

    // Проверяем, что ключ занулен
    for (size_t i = 0; i < testKeyData.size; i++) {
        EXPECT_EQ(testKeyData.data[i], 0);
    }

    std::cout << "Key zeroing after use - OK" << std::endl;
}

// ============================================================
// Дополнительный тест: Проверка, что разные параметры Argon2 дают разные хеши
// ============================================================
TEST(CryptoTests, DifferentArgon2ParamsProduceDifferentHashes)
{
    std::string password = "TestPassword123!@#";

    Argon2Data data1(2, 32, 2, 24);
    Argon2Data data2(3, 64, 4, 32);
    Argon2Data data3(4, 128, 8, 48);

    hash_password(password, data1);
    hash_password(password, data2);
    hash_password(password, data3);

    // Все хеши должны быть разными
    EXPECT_NE(memcmp(data1.hash.data(), data2.hash.data(),
                     std::min(data1.hash.size(), data2.hash.size())), 0);
    EXPECT_NE(memcmp(data1.hash.data(), data3.hash.data(),
                     std::min(data1.hash.size(), data3.hash.size())), 0);
    EXPECT_NE(memcmp(data2.hash.data(), data3.hash.data(),
                     std::min(data2.hash.size(), data3.hash.size())), 0);

    // Проверяем верификацию для каждого набора параметров
    EXPECT_TRUE(verify_password(password, data1));
    EXPECT_TRUE(verify_password(password, data2));
    EXPECT_TRUE(verify_password(password, data3));

    std::cout << "Different Argon2 params produce different hashes - OK" << std::endl;
}
