#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <string>

class StateManager
{
private:
  bool m_isLoggedIn;         // Вошел ли пользователь
  std::string m_currentUser; // Имя текущего пользователя

public:
  StateManager() : m_isLoggedIn(false) {}

  // Простейшие методы - только каркас
  void login(const std::string &username)
  {
    m_isLoggedIn = true;
    m_currentUser = username;
  }

  void logout()
  {
    m_isLoggedIn = false;
    m_currentUser.clear();
  }

  bool isLoggedIn() const { return m_isLoggedIn; }
  std::string getCurrentUser() const { return m_currentUser; }
};

#endif