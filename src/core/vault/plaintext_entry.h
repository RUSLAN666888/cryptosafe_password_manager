#ifndef PLAINTEXT_ENTRY_H
#define PLAINTEXT_ENTRY_H

#include <string>
#include <nlohmann/json.hpp>

struct PlaintextEntry{
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
    void to_json(nlohmann::json& j) const {
        j = nlohmann::json{
            {"title", title},
            {"username", username},
            {"password", password},
            {"url", url},
            {"notes", notes},
            {"category", category},
            {"creation_timestamp", creation_timestamp},
            {"version", version}
        };
    }

    void from_json(const nlohmann::json& j) {
        title = j.value("title", "");
        username = j.value("username", "");
        password = j.value("password", "");
        url = j.value("url", "");
        notes = j.value("notes", "");
        category = j.value("category", "");
        creation_timestamp = j.value("creation_timestamp", "");
        version = j.value("version", 1);
    }
};

#endif // PLAINTEXT_ENTRY_H
