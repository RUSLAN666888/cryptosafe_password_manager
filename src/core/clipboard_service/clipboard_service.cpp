#include "clipboard_service.h"
#include <QGuiApplication>
#include <QClipboard>
#include <QSettings>
#include <QDateTime>
#include <iostream>

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

ClipboardService::~ClipboardService()
{
    saveSettings();
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

    // Запускаем таймер
    startAutoClearTimer();

    // Сигнал и событие
    //emit clipboardCopied(type, source);

    // TODO: Публикация в EventBus
    // EventBus::getInstance().publish(EventType::ClipboardCopied, source.toStdString(), type.toStdString());
}

// CLIP-4: Очистка
void ClipboardService::clear()
{
    std::cout << "CLEAR" << std::endl;
    if (m_timer->isActive()) {
        m_timer->stop();
    }

    m_isOwnChange = true;
    m_currentContent.clear();
    m_currentSource.clear();
    m_currentDataType.clear();

    QGuiApplication::clipboard()->setText("");
    emit clipboardCleared();

    // TODO: Публикация в EventBus
    // EventBus::getInstance().publish(EventType::ClipboardCleared, "", "");
}

// CLIP-2: Настройка таймера
void ClipboardService::setAutoClearTimeout(int seconds)
{
    if (seconds < 5) seconds = 5;
    if (seconds > 30) seconds = 30;
    m_timeoutSeconds = seconds;
    saveSettings();
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

// CLIP-4: Таймер истёк
void ClipboardService::onTimerTimeout()
{
    clear();
}

// Обнаружение внешних изменений
void ClipboardService::onClipboardChanged()
{
    if (m_isOwnChange) {
        m_isOwnChange = false;
        return;
    }

    // Внешнее изменение - ускоряем очистку
    if (m_timer->isActive()) {
        m_timer->start(5000);  // Очистить через 5 секунд
        emit clipboardTimerUpdated(5);
    }
}

void ClipboardService::startAutoClearTimer()
{
    if (m_timeoutSeconds > 0) {
        m_timer->start(m_timeoutSeconds * 1000);

        // Предупреждение за 5 секунд
        if (m_timeoutSeconds > 5) {
            QTimer::singleShot((m_timeoutSeconds - 5) * 1000, this, &ClipboardService::showClearWarning);
        }
    }
}

void ClipboardService::showClearWarning()
{
    if (m_timer->isActive() && !m_warningShown) {
        m_warningShown = true;
        emit clipboardWillClear(5);
    }
}

void ClipboardService::saveSettings()
{
    m_db->setSetting("clipboard_timeout", std::to_string(m_timeoutSeconds));
}

void ClipboardService::loadSettings()
{
    try {
        std::string timeoutStr = m_db->getSetting("clipboard_timeout", "30");
        m_timeoutSeconds = std::stoi(timeoutStr);

    } catch (const std::exception& e) {
        m_timeoutSeconds = 30;  // Значение по умолчанию при ошибке
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
