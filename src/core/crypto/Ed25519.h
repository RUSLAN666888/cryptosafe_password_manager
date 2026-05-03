#ifndef ED255195SIGNER_H
#define ED255195SIGNER_H

#include <vector>
#include <cstdint>
#include <openssl/evp.h>
#include <stdexcept>
#include "key_storage.h"
#include "key_manager.h"


inline std::vector<uint8_t> sign_data(const std::string& keyType, const std::vector<uint8_t>& data) {
    KeyData keyData;

    if (keyType == "logSign")
        KeyManager::getInstance().getLogSignKey(keyData);
    else if (keyType == "exportSign")
        KeyManager::getInstance().getExportSignKey(keyData);


    EVP_PKEY* pkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr, keyData.data, keyData.size);

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestSignInit(ctx, nullptr, nullptr, nullptr, pkey);

    size_t sig_len = 0;
    EVP_DigestSign(ctx, nullptr, &sig_len, data.data(), data.size());

    std::vector<uint8_t> signature(sig_len);
    EVP_DigestSign(ctx, signature.data(), &sig_len, data.data(), data.size());

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);

    return signature;
}

inline bool verify(const std::vector<uint8_t>& data, const std::vector<uint8_t>& signature, const std::vector<uint8_t>& public_key) {

    EVP_PKEY* pkey = EVP_PKEY_new_raw_public_key(
        EVP_PKEY_ED25519, nullptr,
        public_key.data(), public_key.size());

    if (!pkey) {
        return false;
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestVerifyInit(ctx, nullptr, nullptr, nullptr, pkey);

    int result = EVP_DigestVerify(ctx, signature.data(), signature.size(),
                                  data.data(), data.size());

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);

    return (result == 1);
}

inline std::vector<uint8_t> derivePublicKey(const std::string& keyType) {

    KeyData keyData;

    if (keyType == "logSign")
        KeyManager::getInstance().getLogSignKey(keyData);
    else if (keyType == "exportSign")
        KeyManager::getInstance().getExportSignKey(keyData);

    EVP_PKEY* pkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr, keyData.data, keyData.size);

    std::vector<uint8_t> public_key(32);
    size_t len = 32;
    if (EVP_PKEY_get_raw_public_key(pkey, public_key.data(), &len) != 1) {
        EVP_PKEY_free(pkey);
        throw std::runtime_error("Failed to derive public key");
    }

    EVP_PKEY_free(pkey);
    return public_key;
}

#endif // ED255195SIGNER_H
