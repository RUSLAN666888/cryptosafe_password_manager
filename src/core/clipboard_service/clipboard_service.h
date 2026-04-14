#ifndef CLIPBOARD_SERVICE_H
#define CLIPBOARD_SERVICE_H

#include <QObject>
#include <QTimer>
#include <QDateTime>

#include "../src/database/DB_helper/db_helper.h"

class ClipboardService : public QObject
{
    Q_OBJECT

public:
    static ClipboardService& getInstance();

    void copyText(const QString& text, const QString& source, const QString& type);
    void clear();

    void setAutoClearTimeout(int seconds);  // 5-300 секунд
    int getAutoClearTimeout() const;
    bool isTimerActive() const;
    int getRemainingSeconds() const;

    bool hasContent() const;
    QString getCurrentContent() const;
    QString getCurrentSource() const;
    QString getCurrentDataType() const;

#ifndef  CLIPBOARD_TEST_MODE
    void init(Database* database) {
        m_db = database;
        //loadSettings();
    }
#endif

    void initForTest() {
        m_timeoutSeconds = 30;
        m_notifyOnCopy = true;
        m_notifyOnWarning = true;
        m_notifyOnClear = true;
    }

    void loadSettings();
    bool isNotifyOnCopy() const;
    bool isNotifyOnWarning() const;
    bool isNotifyOnClear() const;
    void loadNotificationSettings();

    // void saveRemainingTime();
    // void restoreRemainingTime();
    void checkAndRestoreTimer();  // Проверка и восстановление таймера после краша

    void resetTimer();

    void test_setTimeOut(int seconds){
        m_timeoutSeconds = seconds;
    }

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

    void saveCurrentRemainingTime();

    QTimer* m_timer;
    QTimer* m_saveTimer;
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

    bool m_notifyOnCopy = true;
    bool m_notifyOnWarning = true;
    bool m_notifyOnClear = true;
};

#endif // CLIPBOARD_SERVICE_H
