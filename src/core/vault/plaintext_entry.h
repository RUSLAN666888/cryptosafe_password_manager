#ifndef PLAINTEXT_ENTRY_H
#define PLAINTEXT_ENTRY_H

#include <string>
#include <nlohmann/json.hpp>


using json = nlohmann::json;

struct PlaintextEntry {
    std::string title;
    std::string username;
    std::string password;
    std::string url;
    std::string notes;
    std::string category;
    std::string creation_timestamp;
    int version = 1;
    std::string tags;

    // Конструктор по умолчанию
    PlaintextEntry() = default;

    // Полный конструктор
    PlaintextEntry(
        const std::string& title,
        const std::string& username,
        const std::string& password,
        const std::string& url,
        const std::string& notes,
        const std::string& category,
        const std::string& timestamp,
        const std::string& tags
        ) : title(title), username(username), password(password),
        url(url), notes(notes), category(category),
        creation_timestamp(timestamp), tags(tags) {}

    // Сериализация в JSON
    static void to_json(json& j, const PlaintextEntry& entry) {
        j = json{
            {"title", entry.title},
            {"username", entry.username},
            {"password", entry.password},
            {"url", entry.url},
            {"notes", entry.notes},
            {"category", entry.category},
            {"creation_timestamp", entry.creation_timestamp},
            {"version", entry.version}
        };
    }
};

#endif // PLAINTEXT_ENTRY_H
