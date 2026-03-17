// tests/test_events.cpp
#include "../src/core/events.h"
#include <atomic>
#include <gtest/gtest.h>

// Тест 1: Singleton паттерн
TEST(EventBusTest, Singleton)
{
  auto &bus1 = EventBus::getInstance();
  auto &bus2 = EventBus::getInstance();

  EXPECT_EQ(&bus1, &bus2);
}

// Тест 2: Подписка и публикация события
TEST(EventBusTest, SubscribeAndPublish)
{
  auto &bus = EventBus::getInstance();
  bus.clear(); // Очищаем перед тестом

  std::atomic<bool> called = false;

  bus.subscribe(EventType::UserLoggedIn,
                [&called](const Event &e)
                {
                  called = true;
                  EXPECT_EQ(e.type, EventType::UserLoggedIn);
                });

  bus.publish(EventType::UserLoggedIn);

  EXPECT_TRUE(called);
}

// Тест 3: Передача данных в событии
TEST(EventBusTest, EventWithData)
{
  auto &bus = EventBus::getInstance();
  bus.clear();

  std::string testData = "test_user";

  bus.subscribe(EventType::UserLoggedIn,
                [&testData](const Event &e)
                {
                  auto data = e.getData<std::string>();
                  EXPECT_EQ(data, testData);
                });

  bus.publish(EventType::UserLoggedIn, testData);
}

// Тест 4: Несколько подписчиков на одно событие
TEST(EventBusTest, MultipleSubscribers)
{
  auto &bus = EventBus::getInstance();
  bus.clear();

  std::atomic<int> counter = 0;

  auto callback = [&counter](const Event &) { counter++; };

  bus.subscribe(EventType::EntryAdded, callback);
  bus.subscribe(EventType::EntryAdded, callback);

  bus.publish(EventType::EntryAdded);

  EXPECT_EQ(counter, 2);
}

// Тест 5: Разные типы событий
TEST(EventBusTest, DifferentEventTypes)
{
  auto &bus = EventBus::getInstance();
  bus.clear();

  std::atomic<int> entryCounter = 0;
  std::atomic<int> authCounter = 0;

  bus.subscribe(EventType::EntryAdded,
                [&entryCounter](const Event &) { entryCounter++; });

  bus.subscribe(EventType::UserLoggedIn,
                [&authCounter](const Event &) { authCounter++; });

  bus.publish(EventType::EntryAdded);
  bus.publish(EventType::UserLoggedIn);
  bus.publish(EventType::EntryAdded);

  EXPECT_EQ(entryCounter, 2);
  EXPECT_EQ(authCounter, 1);
}

// Тест 6: Отписка от события
TEST(EventBusTest, Unsubscribe)
{
  auto &bus = EventBus::getInstance();
  bus.clear();

  std::atomic<int> counter = 0;

  std::function<void(const Event &)> callback = [&counter](const Event &)
  { counter++; };

  bus.subscribe(EventType::EntryDeleted, callback);
  bus.publish(EventType::EntryDeleted);
  EXPECT_EQ(counter, 1);

  bus.unsubscribe(EventType::EntryDeleted, callback);
  bus.publish(EventType::EntryDeleted);
  EXPECT_EQ(counter, 1); // Не увеличилось
}

// Тест 7: Событие с source
TEST(EventBusTest, EventSource)
{
  auto &bus = EventBus::getInstance();
  bus.clear();

  std::string testSource = "TestModule";

  bus.subscribe(EventType::UserLoggedOut, [&testSource](const Event &e)
                { EXPECT_EQ(e.source, testSource); });

  bus.publish(EventType::UserLoggedOut, std::any(), testSource);
}

// Тест 8: Обработка исключений в callback
TEST(EventBusTest, ExceptionHandling)
{
  auto &bus = EventBus::getInstance();
  bus.clear();

  // Этот callback выбросит исключение
  bus.subscribe(EventType::EntryUpdated, [](const Event &)
                { throw std::runtime_error("Test exception"); });

  // Этот callback должен выполниться несмотря на исключение в первом
  std::atomic<bool> secondCallbackCalled = false;
  bus.subscribe(EventType::EntryUpdated, [&secondCallbackCalled](const Event &)
                { secondCallbackCalled = true; });

  // Не должно выбросить исключение
  EXPECT_NO_THROW(bus.publish(EventType::EntryUpdated));

  EXPECT_TRUE(secondCallbackCalled);
}

// Тест 9: Отписка несуществующего callback
TEST(EventBusTest, UnsubscribeNonExistent)
{
  auto &bus = EventBus::getInstance();
  bus.clear();

  auto callback = [](const Event &) {};

  // Не должно упасть
  EXPECT_NO_THROW(bus.unsubscribe(EventType::EntryAdded, callback));
}

// Тест 10: Публикация без подписчиков
TEST(EventBusTest, PublishNoSubscribers)
{
  auto &bus = EventBus::getInstance();
  bus.clear();

  // Не должно упасть
  EXPECT_NO_THROW(bus.publish(EventType::EntryAdded));
}

// Тест 11: Данные разных типов
TEST(EventBusTest, DifferentDataTypes)
{
  auto &bus = EventBus::getInstance();
  bus.clear();

  struct TestStruct
  {
    int id;
    std::string name;
  };

  TestStruct testData{42, "test"};

  bus.subscribe(EventType::EntryAdded,
                [&testData](const Event &e)
                {
                  auto data = e.getData<TestStruct>();
                  EXPECT_EQ(data.id, testData.id);
                  EXPECT_EQ(data.name, testData.name);
                });

  bus.publish(EventType::EntryAdded, testData);
}

// Тест 12: hasData() работает правильно
TEST(EventBusTest, HasData)
{
  Event event1(EventType::UserLoggedIn);
  EXPECT_FALSE(event1.hasData());

  Event event2(EventType::UserLoggedIn, std::string("test"));
  EXPECT_TRUE(event2.hasData());
}

// Тест 13: Timestamp устанавливается автоматически
TEST(EventBusTest, Timestamp)
{
  Event event(EventType::UserLoggedIn);

  auto now = std::chrono::system_clock::now();
  auto diff = now - event.timestamp;

  // Разница должна быть меньше 1 секунды
  EXPECT_LT(std::chrono::abs(diff).count(), 1000000);
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}