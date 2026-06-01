#include "log_verifier.h"
#include "log_signer.h"
#include <openssl/evp.h>
#include <openssl/err.h>     // Обработка ошибок OpenSSL (ERR_get_error и др.)

#include <iostream>

void LogVerifier::init(Database* db) {
    m_db = db;
}



std::string LogVerifier::computeHash(const std::string& entry_data,
                                     const std::string& previous_hash) {
    std::string data = entry_data + previous_hash;

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, data.c_str(), data.size());

    std::vector<uint8_t> digest(32);
    unsigned int digest_len = 32;
    EVP_DigestFinal_ex(ctx, digest.data(), &digest_len);
    EVP_MD_CTX_free(ctx);

    // Преобразуем в hex строку
    std::stringstream ss;
    for (uint8_t byte : digest) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    }
    return ss.str();
}

bool LogVerifier::verifySignature(const std::string& entry_data,
                                  std::vector<uint8_t>& signature,
                                  std::vector<uint8_t>& public_key) {
    std::string data_to_verify = entry_data;

    std::vector<uint8_t> data(data_to_verify.begin(), data_to_verify.end());


    EVP_PKEY* pkey = EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr,
                                                 public_key.data(), public_key.size());
    if (!pkey) {
        std::cerr << "Failed to create EVP_PKEY from public key" << std::endl;
        ERR_print_errors_fp(stderr);
        return false;
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        std::cerr << "Failed to create EVP_MD_CTX context" << std::endl;
        EVP_PKEY_free(pkey);
        ERR_print_errors_fp(stderr);
        return false;
    }

    int init_result = EVP_DigestVerifyInit(ctx, nullptr, nullptr, nullptr, pkey);
    if (init_result != 1) {
        std::cerr << "EVP_DigestVerifyInit failed!" << std::endl;
        ERR_print_errors_fp(stderr);
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return false;
    }

    int result = EVP_DigestVerify(ctx, signature.data(), signature.size(),
                                  data.data(), data.size());

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);

    return (result == 1);
}


LogVerifier::VerificationResult LogVerifier::verifyAllLogs() {
    VerificationResult result;
    result.isValid = true;
    result.hashChainValid = true;
    result.signaturesValid = true;
    result.seqValid = true;
    result.failedSequence = -1;
    result.verifiedCount = 0;

    if (!m_db) {
        result.isValid = false;
        result.errorMessage = "Database not initialized";
        return result;
    }

    int totalCount = m_db->getLogEntryCount();
    if (totalCount <= 0) {
        result.isValid = true;
        result.verifiedCount = 0;
        return result;
    }

    std::string expectedPreviousHash = "";

    for (int seq = 1; seq <= totalCount; seq++) {
        std::string previous_hash, current_hash, entry_data, created_at, event_type;
        std::vector<uint8_t> signature;
        int key_version;

        if (!m_db->getLogEntry(seq, previous_hash, current_hash, entry_data,
                               signature, key_version, created_at, event_type)) {
            result.isValid = false;
            result.failedSequence = seq;
            result.seqValid = false;
            result.errorMessage = "Failed to read entry " + std::to_string(seq);
            return result;
        }

        // Проверка хеш-цепочки
        std::string computedHash = computeHash(entry_data, previous_hash);


        if (computedHash != current_hash) {
            std::cout<<"computedHash != current_hash"<<std::endl;
            result.isValid = false;
            result.hashChainValid = false;
            result.failedSequence = seq;
            result.errorMessage = "Hash chain broken at sequence " + std::to_string(seq);
            result.tamperedEntries.push_back(seq);
            return result;
        }

        // Проверка previous_hash
        if (previous_hash != expectedPreviousHash) {
            std::cout<<"cprevious_hash != expectedPreviousHash"<<std::endl;

            result.isValid = false;
            result.hashChainValid = false;
            result.failedSequence = seq;
            result.errorMessage = "Previous hash mismatch at sequence " + std::to_string(seq);
            result.tamperedEntries.push_back(seq);
            return result;
        }

        // std::vector<uint8_t> publicKey;
        // int keyVersion;

        // m_db->getPublicKeyForSequence(seq, publicKey, keyVersion);


        // // Проверка подписи
        // if (!verifySignature(entry_data, signature, publicKey)) {
        //     result.isValid = false;
        //     result.signaturesValid = false;
        //     result.failedSequence = seq;
        //     result.errorMessage = "Invalid signature at sequence " + std::to_string(seq);
        //     result.tamperedEntries.push_back(seq);
        //     return result;
        // }


        // Обновляем expectedPreviousHash для следующей записи
        expectedPreviousHash = current_hash;
        result.verifiedCount++;
    }

    return result;
}

bool LogVerifier::startupVerification() {
    VerificationResult result = verifyAllLogs();

    if (!result.isValid) {
        std::cerr << "=== AUDIT LOG VERIFICATION FAILED ===" << std::endl;
        std::cerr << "Error: " << result.errorMessage << std::endl;
        std::cerr << "Failed at sequence: " << result.failedSequence << std::endl;
        std::cerr << "Hash chain valid: " << result.hashChainValid << std::endl;
        std::cerr << "Signatures valid: " << result.signaturesValid << std::endl;

        EventBus::getInstance().publish(EventType::IntegrityCheckFailed,
                                        result.errorMessage, "LogVerifier");
        return false;
    }

    std::cout << "Audit log verification passed. " << result.verifiedCount << " entries verified." << std::endl;
    return true;
}

std::string LogVerifier::verifyImportedEntry(const std::string& entry_data, const std::string& previous_hash, const std::string& current_hash,
                                      std::vector<uint8_t>& signature, std::vector<uint8_t>& public_key){

    std::string computedHash = computeHash(entry_data, previous_hash);

    if (computedHash != current_hash)
        return "Hash chain broken";




    if (!verifySignature(entry_data, signature, public_key))
        return "Invalid signature at sequence";

    return "";
}
