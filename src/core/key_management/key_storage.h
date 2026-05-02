#ifndef KEY_STORAGE_H
#define KEY_STORAGE_H

#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <cstring>
#include <vector>

struct KeyData {
    uint8_t* data;
    size_t size;
};

class KeyStorage{
    uint8_t* m_data;
    size_t m_size;

    void secureZero(uint8_t* data, size_t size) {
        if (!data)
            return;

        volatile uint8_t* p = data;
        for (size_t i = 0; i < size; ++i) {
            p[i] = 0;
        }
    }

public:
    void store(std::vector<uint8_t>& source) {
        clear();

        if (source.empty()) return;

        m_size = source.size();
        m_data = new uint8_t[m_size];
        std::copy(source.begin(), source.end(), m_data);

        // Зануляем источник
        volatile uint8_t* src_data = source.data();
        for (size_t i = 0; i < source.size(); ++i) {
            src_data[i] = 0;
        }
        source.clear();
    }

    void get(KeyData& d) const {
        d.data = m_data;
        d.size = m_size;
    }

    void clear() {
        if (m_data) {
            secureZero(m_data, m_size);
            m_data = nullptr;
            m_size = 0;
        }
    }

};

#endif // KEY_STORAGE_H
