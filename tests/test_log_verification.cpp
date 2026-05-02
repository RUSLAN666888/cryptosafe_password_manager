// #include <gtest/gtest.h>
// #include <thread>
// #include <chrono>
// #include <nlohmann/json.hpp>
// #include <sqlite3.h>
// #include <fstream>

// #include "../src/database/DB_helper/db_helper.h"
// #include "../src/core/audit/log_signer/log_signer.h"
// #include "../src/core/audit/log_verifier/log_verifier.h"
// #include "../src/core/audit/audit_logger/audit_logger.h"
// #include "../src/gui/dialogs/audit_dialog/audit_log_dialog.h"
// #include "../src/core/key_manager.h"
// #include "../src/core/LogEntry.h"

// using json = nlohmann::json;

// class IntegrityTest : public ::testing::Test {
// protected:
//     void SetUp() override {
//         // Удаляем старую БД
//         std::remove("/tmp/test_audit.db");

//         // Создаём и инициализируем БД
//         db.initialize();

//         // Создаём тестовый ключ (фиксированный для теста)
//         // std::vector<uint8_t> privateKey(32);
//         // std::vector<uint8_t> publicKey(32);

//         // // Заполняем фиксированными значениями (не нулями)
//         // for (int i = 0; i < 32; i++) {
//         //     privateKey[i] = static_cast<uint8_t>(i + 0x40);
//         //     publicKey[i] = static_cast<uint8_t>(i + 0x80);
//         // }

//         // // Сохраняем ключи
//         // KeyManager::getInstance().store_private_sign_key(privateKey);
//         // db.addPublicKey(publicKey, 1, 1);

//         LogSigner::getInstance().initFromMasterPassword("Whvcw324jfFdwd");
//         db.addPublicKey(LogSigner::getInstance().get_public_key(), 1, 1);

//         // Инициализируем верификатор
//         LogVerifier::getInstance().init(&db);
//     }

//     void TearDown() override {
//         db.closeAllConnections();
//         std::remove("/tmp/test_audit.db");
//         KeyManager::getInstance().logout();
//     }

//     // Добавление тестовой записи
//     void addTestEntry(int id) {
//         LogEntry logEntry;
//         logEntry.user_id = 1;
//         logEntry.type = EventType::EntryAdded;
//         logEntry.source = "VaultManager";
//         logEntry.timestamp = getUTCTimestamp();
//         logEntry.severity = Severity::INFO;


//         std::string title = "test";
//         std::string username = "test";
//         std::string category = "test";
//         std::string action = "test";

//         json details = json::object();
//         details["entry_id"] = id;
//         details["title"] = title;
//         details["username"] = username;
//         details["category"] = category;
//         details["action"] = action;
//         logEntry.details = details;

//         // Извлекаем entry_id из details
//         if (logEntry.details.contains("entry_id")) {
//             logEntry.entry_id = logEntry.details["entry_id"].get<int>();
//         }


//         std::string j = to_json(logEntry).dump();



//         int count = db.getLogEntryCount();
//         std::string previous_hash = db.getLastEntryHash();

//         std::string hash = LogSigner::getInstance().getHash(logEntry, previous_hash);
//         std::vector<uint8_t> signature = LogSigner::getInstance().sign(logEntry);

//         bool a = j.empty();
//         db.addLogEntry(previous_hash, hash, j, signature, 1, EventType::EntryAdded);
//     }

//     // Генерация 1000 записей
//     void generate1000Entries() {
//         for (int i = 1; i <= 1000; i++) {
//             addTestEntry(i);
//         }
//     }

//     // Изменение записи в БД напрямую (симуляция взлома)
//     void tamperWithEntry(int sequenceNumber, const std::string& fakeData) {
//         sqlite3* conn = db.getConnection();
//         if (!conn) return;

//         sqlite3_stmt* stmt;
//         const char* sql = "UPDATE audit_log SET entry_data = ? WHERE sequence_number = ?";

//         sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
//         sqlite3_bind_text(stmt, 1, fakeData.c_str(), -1, SQLITE_STATIC);
//         sqlite3_bind_int(stmt, 2, sequenceNumber);
//         sqlite3_step(stmt);

//         sqlite3_finalize(stmt);
//         db.releaseConnection(conn);
//     }
//      Database db = Database("/tmp/test_audit.db");
// };

// // Тест: Генерация 1000 записей + взлом + проверка обнаружения
// TEST_F(IntegrityTest, IntegrityTestWithTampering) {
//     // Генерируем 1000 записей
//     generate1000Entries();

//     int count = db.getLogEntryCount();
//     ASSERT_EQ(count, 1000);

//     // Проверяем, что без взлома всё валидно
//     auto& verifier = LogVerifier::getInstance();
//     auto result = verifier.verifyAllLogs();

//     ASSERT_TRUE(result.isValid);
//     ASSERT_TRUE(result.hashChainValid);
//     ASSERT_TRUE(result.signaturesValid);
//     ASSERT_EQ(result.verifiedCount, 1000);

//     //Взламываем запись №500 (изменяем данные)
//     std::string fakeData = R"({"fake": "data", "hacked": true, "original": "modified"})";
//     tamperWithEntry(500, fakeData);

//     // Проверяем, что взлом обнаружен
//     result = verifier.verifyAllLogs();

//     // Должно быть обнаружено нарушение целостности
//     ASSERT_FALSE(result.isValid);

//     // Проверяем, что ошибка обнаружена именно на записи 500
//     EXPECT_EQ(result.failedSequence, 500);

//     // Проверяем, что в списке взломанных записей есть 500
//     bool found500 = false;
//     for (int seq : result.tamperedEntries) {
//         if (seq == 500) found500 = true;
//     }
//     EXPECT_TRUE(found500);

// }

// // Тест: Удаление записи
// TEST_F(IntegrityTest, DeletionTest) {
//     // Генерируем 1000 записей
//     generate1000Entries();

//     // Проверяем валидность
//     auto& verifier = LogVerifier::getInstance();
//     auto result = verifier.verifyAllLogs();
//     ASSERT_TRUE(result.isValid);

//     // Удаляем запись №300 (через прямой SQL)
//     sqlite3* conn = db.getConnection();
//     sqlite3_stmt* stmt;
//     sqlite3_prepare_v2(conn, "DELETE FROM audit_log WHERE sequence_number = 2", -1, &stmt, nullptr);
//     sqlite3_step(stmt);
//     sqlite3_finalize(stmt);
//     db.releaseConnection(conn);

//     result = verifier.verifyAllLogs();

//     ASSERT_FALSE(result.isValid);
// }

