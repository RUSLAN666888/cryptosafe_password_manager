#ifndef BASE64_H
#define BASE64_H

#include <QByteArray>
#include <vector>

inline std::string base64Encode(const std::vector<uint8_t>& data) {
    QByteArray byteArray(reinterpret_cast<const char*>(data.data()), static_cast<int>(data.size()));
    return byteArray.toBase64().toStdString();
}

inline std::vector<uint8_t> base64Decode(const std::string& base64_str) {
    QByteArray decoded = QByteArray::fromBase64(QByteArray::fromStdString(base64_str));
    return std::vector<uint8_t>(decoded.begin(), decoded.end());
}

#endif // BASE64_H
