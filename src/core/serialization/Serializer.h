#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <vector>
#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

class Serializer {
public:
    template<typename T>
    static std::vector<uint8_t> serialize(const T& obj) {
        nlohmann::json j;
        obj.to_json(j);
        std::string str = j.dump();
        return std::vector<uint8_t>(str.begin(), str.end());
    }

    template<typename T>
    static T deserialize(const std::vector<uint8_t>& bytes) {
        std::string str(bytes.begin(), bytes.end());
        nlohmann::json j = nlohmann::json::parse(str);
        T obj;
        obj.from_json(j);
        return obj;
    }

    template<typename T>
    static std::vector<uint8_t> serializeBatch(const std::vector<T>& objects) {
        nlohmann::json j_array = nlohmann::json::array();
        for (const auto& obj : objects) {
            nlohmann::json j;
            obj.to_json(j);
            j_array.push_back(j);
        }
        std::string str = j_array.dump();
        return std::vector<uint8_t>(str.begin(), str.end());
    }

    template<typename T>
    static std::vector<T> deserializeBatch(const std::vector<uint8_t>& bytes) {
        std::string str(bytes.begin(), bytes.end());
        nlohmann::json j_array = nlohmann::json::parse(str);
        std::vector<T> objects;
        for (const auto& j : j_array) {
            T obj;
            obj.from_json(j);
            objects.push_back(obj);
        }
        return objects;
    }
};

#endif // SERIALIZER_H
