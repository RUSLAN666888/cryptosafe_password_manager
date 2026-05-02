#ifndef KEY_MANAGER_H
#define KEY_MANAGER_H

#include "key_storage.h"

class KeyManager{
    KeyStorage m_encryptionKey;
    KeyStorage m_oldEncryptionKey;
    KeyStorage m_signKey;
    KeyStorage m_exportKey;

public:
    static KeyManager& getInstance() {
        static KeyManager instance;
        return instance;
    }

    void storeEncryptionKey(std::vector<uint8_t>& key) {
        m_encryptionKey.store(key);
    }

    void getEncryptionKey(KeyData& d) {
        m_encryptionKey.get(d);
    }

    void clearEncryptionKey() {
        m_encryptionKey.clear();
    }

    void storeOldEncryptionKey(std::vector<uint8_t>& key) {
        m_oldEncryptionKey.store(key);
    }

    void getOldEncryptionKey(KeyData& d) {
        m_oldEncryptionKey.get(d);
    }

    void clearOldEncryptionKey() {
        m_oldEncryptionKey.clear();
    }

    void storeSignKey(std::vector<uint8_t>& key) {
        m_signKey.store(key);
    }

    void getSignKey(KeyData& d) {
        m_signKey.get(d);
    }

    void clearSignKey() {
        m_signKey.clear();
    }

    void storeExportKey(std::vector<uint8_t>& key) {
        m_exportKey.store(key);
    }

    void getExportKey(KeyData& d) {
        m_exportKey.get(d);
    }

    void clearExportKey() {
        m_exportKey.clear();
    }
};

#endif // KEY_MANAGER_H
