#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <chrono>
#include <mutex>

class StateManager
{
private:
    bool loggedIn;
    std::chrono::system_clock::time_point loginTime;
    std::chrono::steady_clock::time_point lastActivity;
    int failedAttempts;
    std::mutex mutex;

    StateManager() : loggedIn(false), failedAttempts(0) {}

public:
    static StateManager& getInstance()
    {
        static StateManager instance;
        return instance;
    }

    void login()
    {
        std::lock_guard<std::mutex> lock(mutex);
        loggedIn = true;
        loginTime = std::chrono::system_clock::now();
        lastActivity = std::chrono::steady_clock::now();
        failedAttempts = 0;
    }

    void logout()
    {
        std::lock_guard<std::mutex> lock(mutex);
        loggedIn = false;
    }

    void updateActivity()
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (loggedIn)
        {
            lastActivity = std::chrono::steady_clock::now();
        }
    }

    void addFailedAttempt()
    {
        std::lock_guard<std::mutex> lock(mutex);
        failedAttempts++;
    }

    bool isLoggedIn()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return loggedIn;
    }

    int getFailedAttempts()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return failedAttempts;
    }

    // Получить время бездействия в секундах
    long getInactivitySeconds()
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (!loggedIn) return 0;

        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(now - lastActivity).count();
    }
};

#endif
