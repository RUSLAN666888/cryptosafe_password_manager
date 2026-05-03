// #include <openssl/evp.h>
// #include <vector>
// #include <cstdint>
// #include <nlohmann/json.hpp>
// #include <iostream>


// #include "key_manager.h"
// #include "key_derivation.h"
// #include "LogEntry.h"
// #include "log_signer.h"


/*void LogSigner::initFromMasterPassword(const std::string& password)
{
    std::vector<uint8_t> private_key(32);
    derive_private_sign_key(password, private_key);
    KeyManager::getInstance().storeSignKey(private_key);

    KeyData d;
    KeyManager::getInstance().getSignKey(d);

    EVP_PKEY* pkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, NULL, d.data, d.size);

    std::vector<uint8_t> public_key(32);
    size_t len = 32;
    EVP_PKEY_get_raw_public_key(pkey, public_key.data(), &len);

    m_public_key = public_key;
}

std::vector<uint8_t> LogSigner::sign(LogEntry entry){
    json j = to_json(entry);
    std::string j_string = j.dump();
    std::vector<uint8_t> entry_data(j_string.begin(), j_string.end());

    EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();

    KeyData d;
    KeyManager::getInstance().getSignKey(d);

    EVP_PKEY* pkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, NULL, d.data, d.size);
    EVP_DigestSignInit_ex(md_ctx, NULL, NULL, NULL, NULL, pkey, NULL);

    size_t sig_len = 0;

    EVP_DigestSign(md_ctx, NULL, &sig_len, entry_data.data(), entry_data.size());

    std::vector<uint8_t> signature(sig_len);

    EVP_DigestSign(md_ctx, signature.data(), &sig_len, entry_data.data(), entry_data.size());

    EVP_MD_CTX_free(md_ctx);
    EVP_PKEY_free(pkey);

    return signature;
}

std::string LogSigner::getHash(LogEntry entry, std::string previous_hash){
    std::string data = to_json(entry).dump() + previous_hash;

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, data.c_str(), data.size());


    std::vector<uint8_t> digest(32);
    EVP_DigestFinal_ex(ctx, digest.data(), NULL);

    std::stringstream ss;
    for (uint8_t byte : digest) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    }
    return ss.str();
}*/
