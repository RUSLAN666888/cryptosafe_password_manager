// LoginDialog.h
// LoginDialog.h
#ifndef LOGIN_DIALOG_H
#define LOGIN_DIALOG_H

#include "../core/config_handler.h"
#include "../core/crypto/authentication.h"
#include "../core/crypto/key_derivation.h"
#include "../core/events.h"
#include "../core/key_manager.h"
#include "../src/database/DB_helper/db_helper.h"
#include <wx/timer.h>
#include <wx/wx.h>

class LoginDialog : public wxDialog
{
private:
  ConfigHander &config;
  Database &db;
  Argon2Data authData;
  std::vector<uint8_t> encSalt;

  // Элементы UI
  wxTextCtrl *passwordCtrl;
  wxStaticText *errorText;
  wxButton *loginButton;
  wxButton *cancelButton;

  // Для exponential backoff
  int failedAttempts;
  wxTimer *backoffTimer;
  int currentDelay;

  void onLogin(wxCommandEvent &event);
  void onPasswordEnter(wxCommandEvent &event);
  void onBackoffTimer(wxTimerEvent &event);
  void updateUIForBackoff();
  void resetBackoff();
  bool loadAuthData();

public:
  LoginDialog(wxWindow *parent, ConfigHander &cfg,
              Database &database); // убрали KeyManager&
  ~LoginDialog();

  wxDECLARE_EVENT_TABLE();
};

#endif