#ifndef CLIPBOARD_SERVICE_H
#define CLIPBOARD_SERVICE_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include "../database/DB_helper/db_helper.h"


class ClipboardService : public QObject
{
    Q_OBJECT

public:
    static ClipboardService& getInstance();

    // CLIP-1: Поддержка разных типов данных
    void copyText(const QString& text, const QString& source, const QString& type);
    void clear();

    // CLIP-2: Настройка таймера
    void setAutoClearTimeout(int seconds);  // 5-300 секунд
    int getAutoClearTimeout() const;
    bool isTimerActive() const;
    int getRemainingSeconds() const;

    // CLIP-4: Проверка состояния
    bool hasContent() const;
    QString getCurrentContent() const;
    QString getCurrentSource() const;
    QString getCurrentDataType() const;

    void init(Database* database) {
        m_db = database;
        loadSettings();
    }

    void loadSettings();

    // void saveRemainingTime();
    // void restoreRemainingTime();

    void resetTimer();

signals:
    void clipboardCopied(const QString& dataType, const QString& source);
    void clipboardCleared();
    void clipboardWillClear(int secondsLeft);  // Предупреждение за 5 секунд


private slots:
    void onTimerTimeout();
    void onClipboardChanged();

private:
    ClipboardService();

    void startAutoClearTimer();
    void stopAutoClearTimer();
    void showClearWarning();

    QTimer* m_timer;
    QString m_currentContent;
    QString m_currentSource;
    QString m_currentDataType;
    QDateTime m_copyTime;
    int m_timeoutSeconds;
    bool m_isOwnChange;
    bool m_warningShown;

    Database* m_db = nullptr;

    QDateTime m_lastCopyTime;

    QTimer* m_updateTimer;
};

#endif // CLIPBOARD_SERVICE_H
