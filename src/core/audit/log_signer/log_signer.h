#ifndef LOG_SIGNER_H
#define LOG_SIGNER_H

#include <string>
#include <openssl/evp.h>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>


#include "../src/core/key_manager.h"
#include "../src/core/crypto/key_derivation.h"
#include "../src/core/LogEntry.h"

using json = nlohmann::json;


std::vector<uint8_t> initFromMasterPassword(const std::string& password)
{
    std::vector<uint8_t> private_key(32);
    derive_log_seed(password, private_key);
    KeyManager::getInstance().store_private_sign_key(private_key);

    KeyManager::KeyData d;
    KeyManager::getInstance().get_private_sign_key(d);

    EVP_PKEY* pkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, NULL, d.data, d.size);

    std::vector<uint8_t> public_key(32);
    size_t len = 32;
    EVP_PKEY_get_raw_public_key(pkey, public_key.data(), &len);

    return public_key;
}

std::string sign(LogEntry entry){
    json j = to_json(entry);
    std::string j_string = j.dump();
    std::vector<uint8_t> entry_data(j_string.begin(), j_string.end());

    EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();

    KeyManager::KeyData d;
    KeyManager::getInstance().get_private_sign_key(d);
    EVP_PKEY* pkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, NULL, d.data, d.size);
    EVP_DigestSignInit_ex(md_ctx, NULL, NULL, NULL, NULL, pkey, NULL);

    size_t sig_len = 0;

    EVP_DigestSign(md_ctx, NULL, &sig_len, entry_data.data(), entry_data.size());

    std::vector<uint8_t> signature(sig_len);

    EVP_DigestSign(md_ctx, signature.data(), &sig_len, entry_data.data(), entry_data.size());

    std::string result(signature.begin(), signature.end());

    EVP_MD_CTX_free(md_ctx);
    EVP_PKEY_free(pkey);

    return result;
}


#endif // LOG_SIGNER_H
