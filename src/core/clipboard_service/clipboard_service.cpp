#include "clipboard_service.h"
#include "../src/core/events.h"
#include <QGuiApplication>
#include <QClipboard>
#include <QSettings>
#include <QDateTime>
#include <iostream>
#include <QThread>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#ifdef Q_OS_LINUX
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

#ifdef Q_OS_MAC
#include <objc/objc-runtime.h>
#endif

ClipboardService::ClipboardService()
    : QObject(nullptr)
    , m_timer(new QTimer(this))
    , m_saveTimer(new QTimer(this))
    , m_timeoutSeconds(30)
    , m_isOwnChange(false)
    , m_warningShown(false)
{
    // Настройка таймера
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &ClipboardService::onTimerTimeout);

    // Настройка таймера для сохранения в БД
            m_saveTimer->setSingleShot(false);  // Циклический
    m_saveTimer->setInterval(3000);     // Каждые 3 секунды
    connect(m_saveTimer, &QTimer::timeout, this, &ClipboardService::saveCurrentRemainingTime);

    // Мониторинг внешних изменений
    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged,
            this, &ClipboardService::onClipboardChanged);
}

void ClipboardService::saveCurrentRemainingTime()
{
    if (!m_db) return;

    int remaining = getRemainingSeconds();
    if (remaining > 0) {
        m_db->setSetting("clipboard_remaining_seconds", std::to_string(remaining));
        std::cout << "Saved remaining time: " << remaining << " seconds" << std::endl;
    }
}

void ClipboardService::checkAndRestoreTimer()
{
    if (!m_db) return;

    try {
        std::string remainingStr = m_db->getSetting("clipboard_remaining_seconds", "0");
        int remaining = std::stoi(remainingStr);

        std::cout << "Check saved timer: " << remaining << " seconds" << std::endl;

        if (remaining > 0 && remaining <= 300) {
            // Восстанавливаем таймер без копирования в буфер
            m_timeoutSeconds = remaining;
            m_copyTime = QDateTime::currentDateTime();
            m_timer->start(remaining * 1000);

            // Запускаем периодическое сохранение
            m_saveTimer->start();

            std::cout << "Timer restored: " << remaining << " seconds remaining" << std::endl;

            // Очищаем сохранённое значение в БД (чтобы не восстановить снова)
            m_db->setSetting("clipboard_remaining_seconds", "0");
        }
    } catch (const std::exception& e) {
        std::cout << "Failed to restore timer: " << e.what() << std::endl;
    }
}

ClipboardService& ClipboardService::getInstance()
{
    static ClipboardService instance;
    return instance;
}

void ClipboardService::copyText(const QString& text, const QString& source, const QString& type)
{
    m_isOwnChange = true;
    m_currentContent = text;
    m_currentSource = source;
    m_currentDataType = type;
    m_copyTime = QDateTime::currentDateTime();
    m_warningShown = false;

    if (m_timer->isActive()) {
        m_timer->stop();
    }

    if (m_saveTimer->isActive()) {
        m_saveTimer->stop();
    }

#ifdef Q_OS_WIN
    if (OpenClipboard(nullptr)) {
        EmptyClipboard();
        int size = (text.size() + 1) * sizeof(wchar_t);
        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
        if (hGlobal) {
            wchar_t* pData = (wchar_t*)GlobalLock(hGlobal);
            MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, pData, text.size() + 1);
            GlobalUnlock(hGlobal);
            SetClipboardData(CF_UNICODETEXT, hGlobal);
        }
        CloseClipboard();
    }

#elif defined(Q_OS_LINUX)
    std::string utf8Text = text.toStdString();
    std::string cmd = "echo -n '" + utf8Text + "' | xclip -selection clipboard";
    int result = system(cmd.c_str());
    if (result != 0) {
        std::cerr << "Failed to copy: xclip not available" << std::endl;
    }
#elif defined(Q_OS_MAC)
    // macOS: NSPasteboard через system
    system(("echo -n '" + text.toStdString() + "' | pbcopy").c_str());
#endif

    m_lastCopyTime = QDateTime::currentDateTime();
    m_isOwnChange = true;

    startAutoClearTimer();

    m_saveTimer->start();

    if (m_notifyOnCopy) {
        eventBus.publish(EventType::ClipboardCopied, source.toStdString(), type.toStdString());
    }
}

void ClipboardService::clear()
{
    std::cout << "=== CLEAR ===" << std::endl;

    // Останавливаем таймер сохранения
    if (m_saveTimer->isActive()) {
        m_saveTimer->stop();
    }

    // Сохраняем 0 в БД
    if (m_db) {
        m_db->setSetting("clipboard_remaining_seconds", "0");
    }

#ifdef Q_OS_WIN
    if (OpenClipboard(nullptr)) {
        // Очищаем
        EmptyClipboard();

        // Добавляем мусор
        std::string garbage(12040, 'X');
        HGLOBAL hGarbage = GlobalAlloc(GMEM_MOVEABLE, garbage.size() + 1);
        if (hGarbage) {
            char* pData = (char*)GlobalLock(hGarbage);
            strcpy(pData, garbage.c_str());
            GlobalUnlock(hGarbage);
            SetClipboardData(CF_TEXT, hGarbage);
        }

        // Снова очищаем
        EmptyClipboard();
        CloseClipboard();
    }
#elif defined(Q_OS_LINUX)
    // Linux: заполняем мусором через X11
    // Очищаем оба буфера
    system("echo -n '' | xclip -selection clipboard");
    system("echo -n '' | xclip -selection primary");

    // Добавляем мусор
    std::string garbage(12040, 'X');
    system(("echo -n '" + garbage + "' | xclip -selection clipboard").c_str());
    std::string garbage2(12040, 'Y');
    system(("echo -n '" + garbage2 + "' | xclip -selection primary").c_str());

    // Снова очищаем
    system("echo -n '' | xclip -selection clipboard");
    system("echo -n '' | xclip -selection primary");
#elif defined(Q_OS_MAC)
    // macOS: заполняем мусором через pbcopy и pbpaste

    // Очищаем буфер
    system("echo -n '' | pbcopy");

    // Добавляем мусор
    std::string garbage(12040, 'X');
    system(("echo -n '" + garbage + "' | pbcopy").c_str());
    std::string garbage2(12040, 'Y');
    system(("echo -n '" + garbage2 + "' | pbcopy").c_str());

    // Снова очищаем
    system("echo -n '' | pbcopy");
#endif


    // Сбрасываем состояние
    m_currentContent.clear();
    m_currentSource.clear();
    m_currentDataType.clear();

    if (m_timer->isActive()) {
        m_timer->stop();
    }

    eventBus.publish(EventType::ClipboardCleared, "", "");
}

int ClipboardService::getAutoClearTimeout() const
{
    return m_timeoutSeconds;
}

bool ClipboardService::isTimerActive() const
{
    return m_timer->isActive();
}

int ClipboardService::getRemainingSeconds() const
{
    if (!m_timer->isActive()) return 0;

    qint64 elapsed = m_copyTime.msecsTo(QDateTime::currentDateTime());
    int remaining = m_timeoutSeconds * 1000 - elapsed;
    return qMax(0, remaining / 1000);
}

void ClipboardService::onTimerTimeout()
{
    clear();

    if (m_notifyOnClear) {
        eventBus.publish(EventType::ClipboardCleared, "", "");
    }
}

// Обнаружение внешних изменений
void ClipboardService::onClipboardChanged()
{
    // Игнорируем вызовы в течение 500 мс после копирования
    if (m_lastCopyTime.isValid() &&
        m_lastCopyTime.msecsTo(QDateTime::currentDateTime()) < 500) {
        return;
    }

    //Внешнее изменение - ускоряем очистку
    // if (m_timer->isActive()) {
    //     m_timer->start(5000);  // Очистить через 5 секунд
    //     emit clipboardTimerUpdated(5);
    // }
}

void ClipboardService::startAutoClearTimer()
{
    if (m_timeoutSeconds > 0) {
        m_timer->start(m_timeoutSeconds * 1000);


        if (m_timeoutSeconds > 5) {
            QTimer::singleShot((m_timeoutSeconds - 5) * 1000, this, &ClipboardService::showClearWarning);
        }
    }

}

void ClipboardService::showClearWarning()
{
    if (m_timer->isActive() && !m_warningShown) {
        m_warningShown = true;
        if (m_notifyOnWarning) {
            eventBus.publish(EventType::ClipboardWillClear, 5, "ClipboardService");
        }
    }
}


void ClipboardService::loadSettings()
{
#ifndef CLIPBOARD_TEST_MODE
    try {
        std::string timeoutStr = m_db->getSetting("clipboard_timeout", "30");
        std::cout << "timer " << timeoutStr << std::endl;
        m_timeoutSeconds = std::stoi(timeoutStr);

        if (m_timeoutSeconds == 0) {
            m_timeoutSeconds = 0;
        } else if (m_timeoutSeconds < 5) {
            m_timeoutSeconds = 5;
        } else if (m_timeoutSeconds > 300) {
            m_timeoutSeconds = 300;
        }
    } catch (const std::exception& e) {
        m_timeoutSeconds = 10;
    }
#else
    // Тестовый режим: используем значения по умолчанию
    m_timeoutSeconds = 30;
    std::cout << "Test mode: using default timeout 30 seconds" << std::endl;
#endif
}

bool ClipboardService::hasContent() const
{
    return !m_currentContent.isEmpty();
}

QString ClipboardService::getCurrentContent() const
{
    return m_currentContent;
}

QString ClipboardService::getCurrentSource() const
{
    return m_currentSource;
}

QString ClipboardService::getCurrentDataType() const
{
    return m_currentDataType;
}


void ClipboardService::resetTimer()
{
    std::cout << "=== RESET TIMER ===" << std::endl;

    // Останавливаем таймер
    if (m_timer->isActive()) {
        m_timer->stop();
    }

    if (m_saveTimer->isActive()) {
        m_saveTimer->stop();
    }

    // Очищаем буфер
    clear();

    // Сбрасываем состояние
    m_currentContent.clear();
    m_currentSource.clear();
    m_currentDataType.clear();
    m_warningShown = false;
}

void ClipboardService::loadNotificationSettings()
{
#ifndef CLIPBOARD_TEST_MODE
    try {
        m_notifyOnCopy = m_db->getSetting("clipboard_notify_copy", "true") == "true";
        m_notifyOnWarning = m_db->getSetting("clipboard_notify_warning", "true") == "true";
        m_notifyOnClear = m_db->getSetting("clipboard_notify_clear", "true") == "true";
    } catch (...) {
        m_notifyOnCopy = true;
        m_notifyOnWarning = true;
        m_notifyOnClear = true;
    }
#else
    // Тестовый режим: используем значения по умолчанию
    m_notifyOnCopy = true;
    m_notifyOnWarning = true;
    m_notifyOnClear = true;
    std::cout << "Test mode: using default notification settings" << std::endl;
#endif
}

bool ClipboardService::isNotifyOnCopy() const { return m_notifyOnCopy; }
bool ClipboardService::isNotifyOnWarning() const { return m_notifyOnWarning; }
bool ClipboardService::isNotifyOnClear() const { return m_notifyOnClear; }
