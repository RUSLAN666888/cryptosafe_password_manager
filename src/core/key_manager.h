#ifndef KEY_MANAGER_H
#define KEY_MANAGER_H

#include "../src/core/events.h"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

class KeyManager
{
private:
  std::unique_ptr<uint8_t[]> key;
  size_t key_size;
  std::chrono::steady_clock::time_point last_activity;
  bool is_unlocked;
  bool is_active;
  std::mutex mutex;

  void zero_memory()
  {
    volatile uint8_t *data = key.get();
    for (size_t i = 0; i < key_size; i++)
    {
      data[i] = 0;
    }
    key.reset();
    key_size = 0;
  }

  KeyManager() : is_unlocked(false), is_active(true) {}

public:
  struct KeyData
  {
    uint8_t *data;
    size_t size;
  };

  KeyManager(const KeyManager &) = delete;
  KeyManager &operator=(const KeyManager &) = delete;

  static KeyManager &getInstance()
  {
    static KeyManager instance;
    return instance;
  }

  // Сохранить ключ
  void store_key(std::vector<uint8_t> &source)
  {
    std::lock_guard<std::mutex> lock(mutex);

    zero_memory(); // очищаем старый ключ

    // Забираем память у вектора
    key_size = source.size();
    key = std::make_unique<uint8_t[]>(key_size);
    std::copy(source.begin(), source.end(), key.get());

    // Зануляем источник
    volatile uint8_t *src_data = source.data();
    for (size_t i = 0; i < source.size(); i++)
    {
      src_data[i] = 0;
    }
    source.clear();

    last_activity = std::chrono::steady_clock::now();
    is_unlocked = true;

    EventBus &eb = EventBus::getInstance();
    // Публикуем событие успешного входа
    eb.publish(EventType::UserLoggedIn, "KeyManager", "store_key");
  }

  // Получить ключ
  void get_key(KeyData &d)
  {
    std::lock_guard<std::mutex> lock(mutex);
    d.data = key.get();
    d.size = key_size;
  }

  void zero_keyData(KeyData &d)
  {
    volatile uint8_t *data = d.data;
    for (size_t i = 0; i < d.size; i++)
    {
      data[i] = 0;
    }
    d.size = 0;
  }

  // Обновить активность
  void update_activity()
  {
    std::lock_guard<std::mutex> lock(mutex);
    last_activity = std::chrono::steady_clock::now();

    auto now = std::chrono::steady_clock::now();
    auto elapsed =
        std::chrono::duration_cast<std::chrono::hours>(now - last_activity)
            .count();

    if (elapsed >= 1)
    { // 1 час бездействия
      zero_memory();
      is_unlocked = false;

      EventBus &eb = EventBus::getInstance();

      eb.publish(EventType::UserLoggedOut, "auto_lock", "KeyManager");
    }
  }

  // // Приложение свернулось/потеряло фокус
  // void on_app_inactive()
  // {
  //   std::lock_guard<std::mutex> lock(mutex);
  //   is_active = false;
  //   zero_memory();
  //   is_unlocked = false;

  //   EventBus &eb = EventBus::getInstance();

  //   eb.publish(EventType::UserLoggedOut, "app_minimized", "KeyManager");
  // }

  // // Приложение активно
  // void on_app_active()
  // {
  //   std::lock_guard<std::mutex> lock(mutex);
  //   is_active = true;
  // }

  // Выход из системы
  void logout()
  {
    std::lock_guard<std::mutex> lock(mutex);
    zero_memory();
    is_unlocked = false;

    EventBus &eb = EventBus::getInstance();
    eb.publish(EventType::UserLoggedOut, "user_logout", "KeyManager");
  }

  ~KeyManager() { logout(); }
};

#endif
