#include "clipboard_service.h"
#include "../src/core/events.h"
#include <QGuiApplication>
#include <QClipboard>
#include <QSettings>
#include <QDateTime>
#include <iostream>
#include <QThread>

ClipboardService::ClipboardService()
    : QObject(nullptr)
    , m_timer(new QTimer(this))
    , m_timeoutSeconds(30)
    , m_isOwnChange(false)
    , m_warningShown(false)
{
    // Настройка таймера
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &ClipboardService::onTimerTimeout);

    // Мониторинг внешних изменений
    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged,
            this, &ClipboardService::onClipboardChanged);

}


ClipboardService& ClipboardService::getInstance()
{
    static ClipboardService instance;
    return instance;
}

// CLIP-1: Копирование
void ClipboardService::copyText(const QString& text, const QString& source, const QString& type)
{
    m_isOwnChange = true;
    m_currentContent = text;
    m_currentSource = source;
    m_currentDataType = type;
    m_copyTime = QDateTime::currentDateTime();
    m_warningShown = false;

    // Копируем в системный буфер
    QGuiApplication::clipboard()->setText(text);

    m_lastCopyTime = QDateTime::currentDateTime();
    m_isOwnChange = true;

    // Запускаем таймер
    startAutoClearTimer();

    eventBus.publish(EventType::ClipboardCopied, source.toStdString(), type.toStdString());
}

// CLIP-4: Очистка
void ClipboardService::clear()
{
    std::cout << "=== CLEAR ===" << std::endl;

#ifdef Q_OS_LINUX
    system("echo -n '' | xclip -selection clipboard");
    system("echo -n '' | xclip -selection primary");
#elif defined(Q_OS_WIN)
    // Windows PowerShell
    system("powershell -command \"Set-Clipboard -Value ''\"");
#elif defined(Q_OS_MAC)
    system("echo -n '' | pbcopy");
#endif

    QGuiApplication::clipboard()->clear();

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
        // Публикуем через EventBus
        eventBus.publish(EventType::ClipboardWillClear, 5, "ClipboardService");
    }
}


void ClipboardService::loadSettings()
{    
    try {
        std::string timeoutStr = m_db->getSetting("clipboard_timeout", "30");
        std::cout << "timer " << timeoutStr << std::endl;
        m_timeoutSeconds = std::stoi(timeoutStr);

        // 0 означает "never auto-clear"
        if (m_timeoutSeconds == 0) {
            m_timeoutSeconds = 0;
        } else if (m_timeoutSeconds < 5) {
            m_timeoutSeconds = 5;
        } else if (m_timeoutSeconds > 300) {
            m_timeoutSeconds = 300;
        }
    } catch (const std::exception& e) {
        m_timeoutSeconds = 30;
    }
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

// void ClipboardService::saveRemainingTime()
// {
//     if (!m_db) return;

//     int remaining = getRemainingSeconds();
//     m_db->setSetting("clipboard_remaining_seconds", std::to_string(remaining));
// }

// void ClipboardService::restoreRemainingTime()
// {
//     if (!m_db) return;

//     try {
//         std::string timeoutSeconds = m_db->getSetting("clipboard_remaining_seconds", "0");
//         m_timeoutSeconds = std::stoi(timeoutSeconds);

//         if (m_timeoutSeconds > 0 && m_timeoutSeconds <= 300) {
//             startAutoClearTimer();
//         }

//     } catch (const std::exception& e) {
//         // Игнорируем ошибки
//     }
// }

void ClipboardService::resetTimer()
{
    std::cout << "=== RESET TIMER ===" << std::endl;

    // Останавливаем таймер
    if (m_timer->isActive()) {
        m_timer->stop();
    }

    // Очищаем буфер
    clear();

    // Сбрасываем состояние
    m_currentContent.clear();
    m_currentSource.clear();
    m_currentDataType.clear();
    m_warningShown = false;

    // Удаляем сохранённое время из БД
    if (m_db) {
        m_db->setSetting("clipboard_remaining_seconds", "0");
    }
}
