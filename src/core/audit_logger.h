#ifndef AUDIT_LOGGER_H
#define AUDIT_LOGGER_H

#include "events.h"
#include <chrono>
#include <iomanip>
#include <iostream>

// Структура для данных событий
struct EntryEventData
{
  int id;
  std::string title;
  std::string username;
};

struct UserEventData
{
  std::string username;
};

struct ClipboardEventData
{
  int entry_id;
  int timeout;
};

// Логгер аудита (как в Python)
class AuditLogger
{
private:
  void writeLog(const std::string &message)
  {
    // В Sprint 1 - просто заглушка
    // В Sprint 5 - запись в БД с подписью
  }

public:
  AuditLogger()
  {
    // Подписываемся на события записей
    eventBus.subscribe(EventType::EntryAdded,
                       [this](const Event &e) { onEntryAdded(e); });

    eventBus.subscribe(EventType::EntryUpdated,
                       [this](const Event &e) { onEntryUpdated(e); });

    eventBus.subscribe(EventType::EntryDeleted,
                       [this](const Event &e) { onEntryDeleted(e); });

    // Подписываемся на события пользователя
    eventBus.subscribe(EventType::UserLoggedIn,
                       [this](const Event &e) { onUserLoggedIn(e); });

    eventBus.subscribe(EventType::UserLoggedOut,
                       [this](const Event &e) { onUserLoggedOut(e); });

    // Подписываемся на события буфера (Sprint 4)
    eventBus.subscribe(EventType::ClipboardCopied,
                       [this](const Event &e) { onClipboardCopied(e); });

    eventBus.subscribe(EventType::ClipboardCleared,
                       [this](const Event &e) { onClipboardCleared(e); });
  }

  void onEntryAdded(const Event &event)
  {
    if (event.hasData())
    {
      auto data = event.getData<EntryEventData>();
      writeLog("Entry added: " + data.title +
               " [ID: " + std::to_string(data.id) + "]");
    }
    else
    {
      writeLog("Entry added");
    }
  }

  void onEntryUpdated(const Event &event)
  {
    if (event.hasData())
    {
      auto data = event.getData<EntryEventData>();
      writeLog("Entry updated: " + data.title +
               " [ID: " + std::to_string(data.id) + "]");
    }
    else
    {
      writeLog("Entry updated");
    }
  }

  void onEntryDeleted(const Event &event)
  {
    if (event.hasData())
    {
      auto data = event.getData<EntryEventData>();
      writeLog("Entry deleted: " + data.title +
               " [ID: " + std::to_string(data.id) + "]");
    }
    else
    {
      writeLog("Entry deleted");
    }
  }

  void onUserLoggedIn(const Event &event)
  {
    if (event.hasData())
    {
      auto data = event.getData<UserEventData>();
      writeLog("User logged in: " + data.username);
    }
    else
    {
      writeLog("User logged in");
    }
  }

  void onUserLoggedOut(const Event &event)
  {
    if (event.hasData())
    {
      auto data = event.getData<UserEventData>();
      writeLog("User logged out: " + data.username);
    }
    else
    {
      writeLog("User logged out");
    }
  }

  void onClipboardCopied(const Event &event)
  {
    if (event.hasData())
    {
      auto data = event.getData<ClipboardEventData>();
      writeLog(
          "Password copied to clipboard [ID: " + std::to_string(data.entry_id) +
          ", timeout: " + std::to_string(data.timeout) + "s]");
    }
    else
    {
      writeLog("Password copied to clipboard");
    }
  }

  void onClipboardCleared(const Event &event) { writeLog("Clipboard cleared"); }
};

#endif // AUDIT_LOGGER_H