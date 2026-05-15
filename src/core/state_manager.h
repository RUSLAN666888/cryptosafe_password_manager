// session_manager.h
#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <chrono>
#include <mutex>
#include <QObject>
#include <QTimer>

class StateManager : public QObject
{
    Q_OBJECT

private:
    StateManager() = default;

    bool m_isLoggedIn = false;
    bool m_isLocked = false;
    std::chrono::steady_clock::time_point m_lastActivity;

    int m_autoLockSeconds = 3600;
    QTimer* m_timer = nullptr;

    void startTimer() {
        if (m_timer) m_timer->start();
    }

    void stopTimer() {
        if (m_timer) m_timer->stop();
    }

public:
    static StateManager& getInstance() {
        static StateManager instance;
        return instance;
    }

    void init(int autoLockSeconds = 3600) {
        m_autoLockSeconds = autoLockSeconds;
        m_timer = new QTimer(this);
        m_timer->setInterval(1000);
        connect(m_timer, &QTimer::timeout, this, [this]() {
            if (m_isLoggedIn && !m_isLocked) {
                auto now = std::chrono::steady_clock::now();
                auto inactive = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastActivity).count();
                if (inactive >= m_autoLockSeconds) {
                    lock();
                    emit inactivityTimeout();
                }
            }
        });
    }

    void login() {
        m_isLoggedIn = true;
        m_isLocked = false;
        m_lastActivity = std::chrono::steady_clock::now();
        startTimer();
        emit LoggedIn();
    }

    void logout() {
        m_isLoggedIn = false;
        m_isLocked = false;
        stopTimer();
        emit LoggedOut();
    }

    void lock() {
        if (!m_isLoggedIn) return;
        m_isLocked = true;
        stopTimer();
        emit LoggedOut();
    }

    void unlock() {
        if (!m_isLoggedIn) return;
        m_isLocked = false;
        m_lastActivity = std::chrono::steady_clock::now();
        startTimer();
        emit LoggedIn();
    }

    void updateActivity() {
        if (m_isLoggedIn && !m_isLocked) {
            m_lastActivity = std::chrono::steady_clock::now();
        }
    }

    bool isLoggedIn() const {
        return m_isLoggedIn;
    }

    bool isLocked() const {
        return m_isLocked;
    }

    long getInactivitySeconds() const {
        if (!m_isLoggedIn || m_isLocked) return 0;
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(now - m_lastActivity).count();
    }

    void setAutoLockSeconds(int seconds) {
        m_autoLockSeconds = seconds;
    }

signals:
    void LoggedIn();
    void LoggedOut();
    void inactivityTimeout();
};

#endif
