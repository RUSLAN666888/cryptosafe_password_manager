#ifndef EVENTS_H
#define EVENTS_H

#include <any>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <iostream>

// Типы событий
enum class EventType
{
    // Entry events
    EntryAdded,
    EntryUpdated,
    EntryDeleted,
    EntryReaded,

    // Auth events
    UserLoggedIn,
    UserLoggedOut,

    // Clipboard events
    ClipboardCopied,
    ClipboardCleared,
    ClipboardWillClear,

    LoginFailure,

    PasswordChange,

    //System events
    Startup,
    Shutdown,
    Lock,
    Unlock,

    SettingsModification,

    IntegrityCheckFailed,

    InactivityTimeout
};


// Класс события
class Event
{
public:
    EventType type;
    std::any data;
    std::string source;
    std::chrono::system_clock::time_point timestamp;

    Event(EventType t, const std::any &d = std::any(), const std::string &s = "")
      : type(t), data(d), source(s)
    {
    timestamp = std::chrono::system_clock::now();
    }

    template <typename T> T getData() const { return std::any_cast<T>(data); }

    bool hasData() const { return data.has_value(); }

    static std::string eventTypeToString(EventType type)
    {
        switch (type)
        {
        // Entry events
        case EventType::EntryAdded:         return "Entry Added";
        case EventType::EntryUpdated:       return "Entry Updated";
        case EventType::EntryDeleted:       return "Entry Deleted";
        case EventType::EntryReaded:        return "Entry Readed";

        // Auth events
        case EventType::UserLoggedIn:       return "User Logged In";
        case EventType::UserLoggedOut:      return "User Logged Out";

        // Clipboard events
        case EventType::ClipboardCopied:    return "Clipboard Copied";
        case EventType::ClipboardCleared:   return "Clipboard Cleared";
        case EventType::ClipboardWillClear: return "Clipboard Will Clear";

        // Login events
        case EventType::LoginFailure:       return "Login Failure";

        // Password events
        case EventType::PasswordChange:     return "Password Change";

        // System events
        case EventType::Startup:            return "Startup";
        case EventType::Shutdown:           return "Shutdown";
        case EventType::Lock:               return "Lock";
        case EventType::Unlock:             return "Unlock";

        // Settings
        case EventType::SettingsModification: return "Settings Modified";

        // Integrity
        case EventType::IntegrityCheckFailed: return "Integrity Check Failed";

        // Timeout
        case EventType::InactivityTimeout:    return "Inactivity Timeout";

        default: return "Unknown";
        }
    }
};

// Тип для callback функции
using EventCallback = std::function<void(const Event &)>;

// Шина событий
class EventBus
{
private:
  std::map<EventType, std::vector<EventCallback>> subscribers;
  std::recursive_mutex mutex;

  EventBus() = default;

public:
  EventBus(const EventBus &) = delete;
  EventBus &operator=(const EventBus &) = delete;

  static EventBus &getInstance()
  {
    static EventBus instance;
    return instance;
  }

  // Подписка на событие
  void subscribe(EventType type, EventCallback callback)
  {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    subscribers[type].push_back(callback);
  }

  // Отписка от события
  void unsubscribe(EventType type, EventCallback callback)
  {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    auto &vec = subscribers[type];
    for (auto it = vec.begin(); it != vec.end();)
    {
      if (it->target<void(const Event &)>() ==
          callback.target<void(const Event &)>())
      {
        it = vec.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }

  // Публикация события
  void publish(EventType type, const std::any &data = std::any(),
               const std::string &source = "")
  {
      // Отладочный вывод только для json
      if (data.has_value() && data.type() == typeid(std::string)) {
          try {
              std::string j = std::any_cast<std::string>(data);
              std::cout << "=== EVENT PUBLISH (json) ===" << std::endl;
              std::cout << "Event type: " << static_cast<int>(type) << std::endl;
              std::cout << "Source: " << source << std::endl;
              std::cout << "Data: " << j << std::endl;
              std::cout << "============================" << std::endl;
          } catch (const std::exception& e) {
              std::cout << "Failed to dump json: " << e.what() << std::endl;
          }
      }

    Event event(type, data, source);

    std::lock_guard<std::recursive_mutex> lock(mutex);

    auto it = subscribers.find(type);
    if (it != subscribers.end())
    {
      for (const auto &callback : it->second)
      {
        try
        {
              callback(event);
        }
        catch (...)
        {
          // Игнорируем ошибки в подписчиках
        }
      }
    }
  }

  // Очистить все подписки
  void clear()
  {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    subscribers.clear();
  }
};

// Глобальный экземпляр
inline EventBus &eventBus = EventBus::getInstance();

#endif
