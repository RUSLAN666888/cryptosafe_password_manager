// LoginDialog.cpp
#include "LoginDialog.h"
#include <chrono>
#include <thread>

wxBEGIN_EVENT_TABLE(LoginDialog, wxDialog)
    EVT_BUTTON(wxID_OK, LoginDialog::onLogin)
        EVT_TEXT_ENTER(wxID_ANY, LoginDialog::onPasswordEnter)
            EVT_TIMER(wxID_ANY, LoginDialog::onBackoffTimer) wxEND_EVENT_TABLE()

                LoginDialog::LoginDialog(wxWindow *parent, ConfigHander &cfg,
                                         Database &database)
    : wxDialog(parent, wxID_ANY, "Login to CryptoSafe", wxDefaultPosition,
               wxSize(400, 500)),
      config(cfg), db(database), failedAttempts(0), currentDelay(0),
      backoffTimer(nullptr)
{
  // Загружаем данные аутентификации из БД
  if (!loadAuthData())
  {
    wxMessageBox(
        "Failed to load authentication data. Database may be corrupted.",
        "Error", wxOK | wxICON_ERROR);
    return;
  }

  std::cout << "1" << std::endl;
  wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

  // Пустое пространство сверху
  mainSizer->Add(0, 0, 1, wxEXPAND);

  // Центрированное содержимое
  wxBoxSizer *centerSizer = new wxBoxSizer(wxVERTICAL);

  wxStaticText *title =
      new wxStaticText(this, wxID_ANY, "Enter Master Password");
  wxFont titleFont = title->GetFont();
  titleFont.SetPointSize(14);
  titleFont.SetWeight(wxFONTWEIGHT_BOLD);
  title->SetFont(titleFont);
  centerSizer->Add(title, 0, wxALIGN_CENTER | wxBOTTOM, 20);

  wxBoxSizer *rowSizer = new wxBoxSizer(wxHORIZONTAL);
  wxStaticText *label = new wxStaticText(this, wxID_ANY, "Password:");
  passwordCtrl =
      new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(200, -1),
                     wxTE_PASSWORD | wxTE_PROCESS_ENTER);

  rowSizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
  rowSizer->Add(passwordCtrl, 0, wxALIGN_CENTER_VERTICAL);

  centerSizer->Add(rowSizer, 0, wxALIGN_CENTER);

  errorText = new wxStaticText(this, wxID_ANY, "");
  errorText->SetForegroundColour(wxColour(255, 0, 0));
  centerSizer->Add(errorText, 0, wxALIGN_CENTER | wxTOP, 10);

  mainSizer->Add(centerSizer, 0, wxALIGN_CENTER);

  // Пустое пространство снизу
  mainSizer->Add(0, 0, 1, wxEXPAND);

  // Кнопки внизу
  wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
  loginButton = new wxButton(this, wxID_OK, "Login");
  cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");

  buttonSizer->AddStretchSpacer();
  buttonSizer->Add(loginButton, 0, wxRIGHT, 10);
  buttonSizer->Add(cancelButton, 0);
  buttonSizer->AddSpacer(20);

  mainSizer->Add(buttonSizer, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 15);

  SetSizer(mainSizer);
  Centre();
  passwordCtrl->SetFocus();
}

LoginDialog::~LoginDialog()
{
  if (backoffTimer)
  {
    backoffTimer->Stop();
    delete backoffTimer;
  }
}

bool LoginDialog::loadAuthData()
{
  std::cout << "========== loadAuthData started ==========" << std::endl;

  std::vector<uint8_t> hash;
  std::vector<uint8_t> salt;
  uint32_t time_cost, memory_cost, parallelism, hash_len;

  std::cout << "Calling db.getAuthData..." << std::endl;

  try
  {
    if (!db.getAuthData(hash, salt, time_cost, memory_cost, parallelism,
                        hash_len))
    {
      std::cerr << "Failed to get auth data from database" << std::endl;
      return false;
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Exception while getting auth data: " << e.what() << std::endl;
    return false;
  }
  catch (...)
  {
    std::cerr << "Unknown exception while getting auth data" << std::endl;
    return false;
  }

  std::cout << "getAuthData successful" << std::endl;
  std::cout << "Hash size: " << hash.size() << std::endl;
  std::cout << "Salt size: " << salt.size() << std::endl;
  std::cout << "Params: time=" << time_cost << ", memory=" << memory_cost
            << ", parallelism=" << parallelism << ", hash_len=" << hash_len
            << std::endl;

  std::cout << "Calling db.getEncSalt..." << std::endl;

  if (!db.getEncSalt(encSalt))
  {
    std::cerr << "Failed to get enc salt" << std::endl;
    return false;
  }

  std::cout << "EncSalt size: " << encSalt.size() << std::endl;

  // Заполняем Argon2Data
  authData = Argon2Data(time_cost, memory_cost, parallelism, hash_len);
  authData.hash = std::move(hash);
  authData.salt = std::move(salt);

  std::cout << "========== loadAuthData finished ==========" << std::endl;
  return true;
}
void LoginDialog::onLogin(wxCommandEvent &event)
{
  wxString password = passwordCtrl->GetValue();

  if (password.IsEmpty())
  {
    errorText->SetLabel("Password cannot be empty");
    return;
  }

  // Проверка на backoff
  if (failedAttempts > 0 && currentDelay > 0)
  {
    errorText->SetLabel(
        wxString::Format("Too many attempts. Wait %d seconds", currentDelay));
    return;
  }

  // Конвертируем wxString в std::string
  wxScopedCharBuffer pwdBuf = password.ToUTF8();
  std::string pwdStr(pwdBuf.data(), pwdBuf.length());

  // Шаг 1: Verify password against Argon2 hash
  if (!verify_password(pwdStr, authData))
  {
    failedAttempts++;

    // Exponential backoff
    if (failedAttempts <= 2)
    {
      currentDelay = 1;
    }
    else if (failedAttempts <= 4)
    {
      currentDelay = 5;
    }
    else
    {
      currentDelay = 30;
    }

    errorText->SetLabel(wxString::Format(
        "Invalid password. Try again in %d seconds", currentDelay));
    updateUIForBackoff();

    // Запускаем таймер
    if (!backoffTimer)
    {
      backoffTimer = new wxTimer(this);
    }
    backoffTimer->Start(currentDelay * 1000, wxTIMER_ONE_SHOT);

    return;
  }

  // Шаг 2: Derive encryption key via PBKDF2
  std::vector<uint8_t> encKey;
  derive_encryption_key(pwdStr, encSalt, encKey);

  // Зануляем пароль в памяти
  volatile char *p = const_cast<char *>(pwdStr.data());
  for (size_t i = 0; i < pwdStr.size(); ++i)
  {
    p[i] = 0;
  }

  // Шаг 3: Cache encryption key in secure memory
  KeyManager::getInstance().store_key(encKey); // ← используем синглтон!

  // Шаг 4: Publish UserLoggedIn event
  struct LoginEventData
  {
    std::string username;
    std::chrono::system_clock::time_point loginTime;
  };

  LoginEventData eventData{"user", std::chrono::system_clock::now()};
  eventBus.publish(EventType::UserLoggedIn, eventData, "LoginDialog");

  // Сбрасываем счетчик попыток при успешном входе
  resetBackoff();

  // Закрываем диалог с успехом
  EndModal(wxID_OK);
}

void LoginDialog::onPasswordEnter(wxCommandEvent &event)
{
  wxCommandEvent loginEvent(wxEVT_BUTTON, wxID_OK);
  onLogin(loginEvent);
}

void LoginDialog::onBackoffTimer(wxTimerEvent &event)
{
  currentDelay = 0;
  updateUIForBackoff();
  errorText->SetLabel("You can try again now");
  passwordCtrl->SetFocus();
}

void LoginDialog::updateUIForBackoff()
{
  if (currentDelay > 0)
  {
    loginButton->Disable();
    passwordCtrl->Disable();
  }
  else
  {
    loginButton->Enable();
    passwordCtrl->Enable();
  }
}

void LoginDialog::resetBackoff()
{
  failedAttempts = 0;
  currentDelay = 0;
  updateUIForBackoff();
  errorText->SetLabel("");
}