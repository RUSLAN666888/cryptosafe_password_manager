#include "log_signer.h"
#include <openssl/evp.h>
#include <iomanip>
#include <sstream>

LogSigner& LogSigner::getInstance()
{
    static LogSigner instance;
    return instance;
}

void LogSigner::initFromMasterPassword(const char* password, size_t password_len)
{
    std::vector<uint8_t> private_key;

    // Соль для HKDF (фиксированная для аудита)
    const std::vector<uint8_t> salt = {
        0x43, 0x72, 0x79, 0x70, 0x74, 0x6f, 0x53, 0x61,
        0x66, 0x65, 0x5f, 0x41, 0x75, 0x64, 0x69, 0x74
    };

    derive_private_sign_key(password, password_len, salt, "audit-signing", private_key);
    KeyManager::getInstance().storeLogSignKey(private_key);

    // Вычисляем и сохраняем публичный ключ
    KeyData d;
    KeyManager::getInstance().getLogSignKey(d);

    EVP_PKEY* pkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr, d.data, d.size);
    if (pkey) {
        m_public_key.resize(32);
        size_t len = 32;
        EVP_PKEY_get_raw_public_key(pkey, m_public_key.data(), &len);
        EVP_PKEY_free(pkey);
    }

    // Зануляем приватный ключ в памяти
    secure_zero(private_key.data(), private_key.size());
}

std::vector<uint8_t> LogSigner::sign(LogEntry entry)
{
    // Сериализуем запись в JSON
    json j = to_json(entry);
    std::string j_string = j.dump();
    std::vector<uint8_t> entry_data(j_string.begin(), j_string.end());

    // Получаем приватный ключ из KeyManager
    KeyData d;
    KeyManager::getInstance().getLogSignKey(d);

    EVP_PKEY* pkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr, d.data, d.size);
    if (!pkey) {
        return std::vector<uint8_t>();
    }

    EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
    if (!md_ctx) {
        EVP_PKEY_free(pkey);
        return std::vector<uint8_t>();
    }

    EVP_DigestSignInit_ex(md_ctx, nullptr, nullptr, nullptr, nullptr, pkey, nullptr);

    size_t sig_len = 0;
    EVP_DigestSign(md_ctx, nullptr, &sig_len, entry_data.data(), entry_data.size());

    std::vector<uint8_t> signature(sig_len);
    EVP_DigestSign(md_ctx, signature.data(), &sig_len, entry_data.data(), entry_data.size());

    EVP_MD_CTX_free(md_ctx);
    EVP_PKEY_free(pkey);

    return signature;
}

std::string LogSigner::getHash(LogEntry entry, std::string previous_hash)
{
    std::string data = to_json(entry).dump() + previous_hash;

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        return "";
    }

    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, data.c_str(), data.size());

    std::vector<uint8_t> digest(32);
    EVP_DigestFinal_ex(ctx, digest.data(), nullptr);

    EVP_MD_CTX_free(ctx);

    std::stringstream ss;
    for (uint8_t byte : digest) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }

    return ss.str();
}
