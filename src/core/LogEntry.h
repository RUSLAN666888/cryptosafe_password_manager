#ifndef LOGENTRY_H
#define LOGENTRY_H

#include <any>
#include "../src/core/events.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <vector>
#include <cstdint>

using json = nlohmann::json;

enum class Severity {
    INFO,
    WARN,
    ERROR,
    CRITICAL
};

struct LogEntry {
    int user_id;
    EventType type;
    int entry_id;
    std::string source;
    json details;
    Severity severity;
    std::string timestamp;

};

inline std::string getUTCTimestamp(){
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);

    std::tm* utc_time = std::gmtime(&now_time_t);

    std::ostringstream oss;
    oss << std::put_time(utc_time, "%Y-%m-%dT%H:%M:%S") << "Z";

    return oss.str();
}

inline json to_json(const LogEntry& entry) {

    json j = json::object();
    j["user_id"] = entry.user_id;
    j["event_type"] = static_cast<int>(entry.type);
    j["entry_id"] = entry.entry_id;
    j["source"] = entry.source;
    j["details"] = entry.details;
    j["severity"] = static_cast<int>(entry.severity);
    j["timestamp"] = entry.timestamp;

    return j;
}


#endif // LOGENTRY_H
