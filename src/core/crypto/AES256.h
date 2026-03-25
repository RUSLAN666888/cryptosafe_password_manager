#ifndef AES256_PLACEHOLDER_H
#define AES256_PLACEHOLDER_H

#include "abstract.h"
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <../src/core/key_manager.h>
#include "../src/database/plaintext_entry.h"
#include <openssl/evp.h>
#include <openssl/err.h>
#include <nlohmann/json.hpp>
#include <openssl/rand.h>



using namespace std;
using json = nlohmann::json;


class AES256Placeholder : public EncryptionService
{
    static constexpr size_t KEY_SIZE = 32;   // 256 бит для AES-256
    static constexpr size_t NONCE_LEN = 12;  // 96 бит, рекомендуется для GCM режима
    static constexpr size_t TAG_LEN = 16;    // 128 бит, стандартная длина тега аутентификации

public:
    /**
     * @brief Шифрует PlaintextEntry с использованием AES-256-GCM
     * @param key Ключ шифрования (должен быть 32 байта)
     * @param entry Исходные данные для шифрования
     * @return vector<uint8_t> Сериализованные данные в формате [nonce(12) | ciphertext(N) | tag(16)]
     *
     * Процесс шифрования:
     * 1. Сериализация PlaintextEntry в JSON
     * 2. Генерация случайного nonce (12 байт)
     * 3. Шифрование JSON-строки с помощью AES-256-GCM
     * 4. Получение тега аутентификации (16 байт)
     * 5. Формирование результата: nonce + ciphertext + tag
     */
    std::vector<uint8_t> encrypt(const KeyManager::KeyData& key, const PlaintextEntry& entry) override
    {
        // 1. Сериализуем PlaintextEntry в JSON (требование ENC-3)
        json json_data;
        PlaintextEntry::to_json(json_data, entry);
        std::string plaintext = json_data.dump(); // JSON-строка в виде обычного текста

        // 2. Генерация случайного nonce (вектора инициализации)
        //    Для GCM режима рекомендуется nonce размером 12 байт
        std::vector<uint8_t> nonce(NONCE_LEN);
        if (RAND_bytes(nonce.data(), NONCE_LEN) != 1)
        {
            throw std::runtime_error("Failed to generate nonce");
        }

        // 3. Создание контекста шифрования OpenSSL
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx)
        {
            throw std::runtime_error("Failed to create cipher context");
        }

        // 4. Подготовка буферов для шифротекста и тега аутентификации
        //    Размер шифротекста равен размеру открытого текста (GCM не использует padding)
        std::vector<uint8_t> ciphertext(plaintext.size());
        std::vector<uint8_t> tag(TAG_LEN);

        try
        {
            // 5. Инициализация контекста для шифрования с алгоритмом AES-256-GCM
            if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1)
            {
                throw std::runtime_error("Failed to initialize encryption");
            }

            // 6. Установка ключа и nonce в контекст
            //    Первый nullptr означает использование ранее установленного алгоритма
            if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data, nonce.data()) != 1)
            {
                throw std::runtime_error("Failed to set key and IV");
            }

            // 7. Шифрование данных
            //    len - количество записанных в ciphertext байт
            int len = 0;
            if (EVP_EncryptUpdate(ctx,
                                  reinterpret_cast<unsigned char*>(ciphertext.data()),
                                  &len,
                                  reinterpret_cast<const unsigned char*>(plaintext.data()),
                                  static_cast<int>(plaintext.size())) != 1)
            {
                throw std::runtime_error("Failed to encrypt");
            }

            // 8. Финализация шифрования
            //    Для GCM режима final_len всегда равен 0, но вызов обязателен для завершения операции
            int final_len = 0;
            if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &final_len) != 1)
            {
                throw std::runtime_error("Failed to finalize encryption");
            }

            // 9. Получение тега аутентификации
            //    Тег используется для проверки целостности данных при расшифровке
            if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_LEN, tag.data()) != 1)
            {
                throw std::runtime_error("Failed to get tag");
            }

            // 10. Обрезаем шифротекст до реального размера (для GCM len + final_len = plaintext.size())
            ciphertext.resize(len + final_len);
        }
        catch (...)
        {
            // При возникновении ошибки освобождаем контекст и пробрасываем исключение дальше
            EVP_CIPHER_CTX_free(ctx);
            throw;
        }

        // 11. Освобождаем контекст шифрования
        EVP_CIPHER_CTX_free(ctx);

        // 12. Формируем итоговый результат в формате: nonce + ciphertext + tag
        //     Такой формат удобен для хранения и передачи, так как nonce и tag имеют фиксированный размер
        std::vector<uint8_t> result;
        result.reserve(ciphertext.size() + tag.size() + nonce.size());
        result.insert(result.end(), nonce.begin(), nonce.end());
        result.insert(result.end(), ciphertext.begin(), ciphertext.end());
        result.insert(result.end(), tag.begin(), tag.end());

        return result;
    }

    /**
     * @brief Расшифровывает данные, зашифрованные методом encrypt()
     * @param encrypted_package Зашифрованные данные в формате [nonce(12) | ciphertext(N) | tag(16)]
     * @param key Ключ шифрования (должен быть 32 байта)
     * @return PlaintextEntry Расшифрованная запись
     *
     * Процесс расшифровки:
     * 1. Разделение входных данных на nonce, ciphertext и tag
     * 2. Дешифрование ciphertext с использованием nonce
     * 3. Проверка тега аутентификации (защита от подделки)
     * 4. Парсинг JSON и восстановление PlaintextEntry
     */
    PlaintextEntry decrypt(const std::vector<uint8_t>& encrypted_package, const KeyManager::KeyData& key) override
    {
        // 1. Разделяем входные данные на составляющие
        //    Формат: [nonce (12 байт)][ciphertext (N байт)][tag (16 байт)]
        std::vector<uint8_t> nonce(encrypted_package.begin(), encrypted_package.begin() + NONCE_LEN);
        std::vector<uint8_t> tag(encrypted_package.end() - TAG_LEN, encrypted_package.end());
        std::vector<uint8_t> encrypted_data(encrypted_package.begin() + NONCE_LEN, encrypted_package.end() - TAG_LEN);

        // 2. Создание контекста дешифрования OpenSSL
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx)
        {
            throw std::runtime_error("Failed to create cipher context");
        }

        // 3. Выделяем буфер для расшифрованных данных
        //    Размер буфера равен размеру зашифрованных данных (GCM не меняет размер)
        std::vector<uint8_t> plaintext(encrypted_data.size());

        try
        {
            // 4. Инициализация контекста для дешифрования с алгоритмом AES-256-GCM
            if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1)
            {
                throw std::runtime_error("Failed to initialize decryption");
            }

            // 5. Установка ключа и nonce в контекст
            //    Используем тот же nonce, который был сгенерирован при шифровании
            if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data, nonce.data()) != 1)
            {
                throw std::runtime_error("Failed to set key and IV");
            }

            // 6. Дешифрование данных
            //    out_len - количество записанных в plaintext байт
            int out_len = 0;
            if (EVP_DecryptUpdate(ctx,
                                  plaintext.data(),
                                  &out_len,
                                  encrypted_data.data(),
                                  static_cast<int>(encrypted_data.size())) != 1)
            {
                throw std::runtime_error("Failed to decrypt");
            }

            // 7. Установка тега аутентификации
            //    Важно: тег устанавливается ДО финализации, но ПОСЛЕ дешифрования
            if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_LEN, tag.data()) != 1)
            {
                throw std::runtime_error("Failed to set tag");
            }

            // 8. Финализация и проверка тега
            //    Если тег не совпадает (данные были изменены), функция вернет ошибку
            //    Это главная защита от подделки данных
            int final_len = 0;
            if (EVP_DecryptFinal_ex(ctx, plaintext.data() + out_len, &final_len) != 1)
            {
                throw std::runtime_error("Authentication failed - wrong tag");
            }

            // 9. Обрезаем буфер до реального размера расшифрованных данных
            plaintext.resize(out_len + final_len);
        }
        catch (...)
        {
            // При возникновении ошибки освобождаем контекст и пробрасываем исключение дальше
            EVP_CIPHER_CTX_free(ctx);
            throw;
        }

        // 10. Освобождаем контекст дешифрования
        EVP_CIPHER_CTX_free(ctx);

        // 11. Преобразуем расшифрованные байты в строку JSON
        std::string json_string(plaintext.begin(), plaintext.end());

        // 12. Парсим JSON и восстанавливаем структуру PlaintextEntry
        json json_data = json::parse(json_string);
        PlaintextEntry entry;

        // Извлекаем поля с значениями по умолчанию на случай отсутствия (защита от будущих изменений формата)
        entry.title = json_data.value("title", "");
        entry.username = json_data.value("username", "");
        entry.password = json_data.value("password", "");
        entry.url = json_data.value("url", "");
        entry.notes = json_data.value("notes", "");
        entry.category = json_data.value("category", "");
        entry.creation_timestamp = json_data.value("creation_timestamp", "");
        entry.version = json_data.value("version", 1);

        return entry;
    }
};

#endif
