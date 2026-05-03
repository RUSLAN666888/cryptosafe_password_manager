#ifndef SHA256_H
#define SHA256_H

#include <openssl/evp.h>
#include <vector>
#include <cstdint>

inline std::vector<uint8_t> sha256(std::vector<uint8_t> data){
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, data.data(), data.size());


    std::vector<uint8_t> digest(32);
    EVP_DigestFinal_ex(ctx, digest.data(), NULL);

    return digest;
}

#endif // SHA256_H
