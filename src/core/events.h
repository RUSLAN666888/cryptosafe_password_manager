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

    LoginSuccess,
    LoginFailure,

    PasswordChange,

    //System events
    Startup,
    Shutdown,
    Lock,
    Unlock,

    SettingsModification
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
