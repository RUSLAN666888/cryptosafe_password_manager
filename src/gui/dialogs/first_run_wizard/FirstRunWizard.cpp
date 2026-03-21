#include "FirstRunWizard.h"
#include "../src/core/crypto/authentication.h"
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/stattext.h>
#include <wx/timer.h>
#include <zxcvbn.h>
#include "../src/core/crypto/key_derivation.h"
#include "../src/core/key_manager.h"

#define ID_BrowseButton 10001
#define ID_STRENGTH_TIMER 10002

wxBEGIN_EVENT_TABLE(FirstRunWizard, wxWizard)
    EVT_BUTTON(ID_BrowseButton,FirstRunWizard::onBrowseDatabase)
    EVT_WIZARD_PAGE_CHANGING(wxID_ANY, FirstRunWizard::onPasswordPageChanging)
    EVT_WIZARD_FINISHED(wxID_ANY, FirstRunWizard::onWizardFinished)
    EVT_TEXT(wxID_ANY, FirstRunWizard::onPasswordTextChanged)
    EVT_TIMER(ID_STRENGTH_TIMER, FirstRunWizard::onStrengthTimer)
    wxEND_EVENT_TABLE()

FirstRunWizard::FirstRunWizard(wxWindow *parent, ConfigHander &cfg)
    : wxWizard(parent, wxID_ANY, "CryptoSafe Setup Wizard", wxNullBitmap,
               wxDefaultPosition, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER), config(cfg)
{
  // Создаем страницы
  welcomePage = createWelcomePage();
  passwordPage = createPasswordPage();
  databasePage = createDatabasePage();
  encryptionPage = createEncryptionPage();
  finishPage = createFinishPage();

  // Связываем страницы в цепочку
  wxWizardPageSimple::Chain(welcomePage, passwordPage);
  wxWizardPageSimple::Chain(passwordPage, databasePage);
  wxWizardPageSimple::Chain(databasePage, encryptionPage);
  wxWizardPageSimple::Chain(encryptionPage, finishPage);

  SetPageSize(wxSize(500, 400));
}

wxWizardPageSimple *FirstRunWizard::createWelcomePage()
{
  wxWizardPageSimple *page = new wxWizardPageSimple(this);

  wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

  // Заголовок
  wxStaticText *title =
      new wxStaticText(page, wxID_ANY, "Welcome to CryptoSafe Manager!");
  wxFont titleFont = title->GetFont();
  titleFont.SetPointSize(16);
  titleFont.SetWeight(wxFONTWEIGHT_BOLD);
  title->SetFont(titleFont);

  mainSizer->Add(title, 0, wxALL | wxALIGN_CENTER, 20);

  // Текст приветствия
  wxStaticText *text = new wxStaticText(
      page, wxID_ANY,
      "This wizard will help you set up your password manager.\n\n"
      "You will need to:\n"
      "• Create a master password\n"
      "• Choose database location\n"
      "• Configure encryption settings",
      wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);

  mainSizer->Add(text, 0, wxALL | wxEXPAND, 20);

  // Инструкция
  wxStaticText *instruction =
      new wxStaticText(page, wxID_ANY, "Click Next to begin setup.");
  mainSizer->Add(instruction, 0, wxALL | wxALIGN_CENTER, 10);

  mainSizer->AddStretchSpacer();
  page->SetSizer(mainSizer);

  return page;
}

wxWizardPageSimple *FirstRunWizard::createPasswordPage()
{
  wxWizardPageSimple *page = new wxWizardPageSimple(this);
  wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

  // Заголовок
  wxStaticText *title =
      new wxStaticText(page, wxID_ANY, "Create Master Password");
  wxFont titleFont = title->GetFont();
  titleFont.SetPointSize(14);
  titleFont.SetWeight(wxFONTWEIGHT_BOLD);
  title->SetFont(titleFont);
  mainSizer->Add(title, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 20);

  mainSizer->AddStretchSpacer();

  // Панель с полями
  wxPanel *panel = new wxPanel(page);
  wxBoxSizer *panelSizer = new wxBoxSizer(wxVERTICAL);

  // Поля ввода
  wxStaticText *passLabel = new wxStaticText(panel, wxID_ANY, "Password:");
  passwordCtrl = new PasswordEntry(panel, wxID_ANY, "", wxDefaultPosition,
                                   wxSize(300, -1));

  wxStaticText *confirmLabel = new wxStaticText(panel, wxID_ANY, "Confirm:");
  confirmCtrl = new PasswordEntry(panel, wxID_ANY, "", wxDefaultPosition,
                                  wxSize(300, -1));

  panelSizer->Add(passLabel, 0, wxLEFT | wxTOP, 5);
  panelSizer->Add(passwordCtrl, 0, wxLEFT | wxRIGHT | wxEXPAND, 5);

  // Индикатор силы пароля
  strengthGauge =
      new wxGauge(panel, wxID_ANY, 4, wxDefaultPosition, wxSize(300, 20));
  strengthGauge->SetValue(0);
  panelSizer->Add(strengthGauge, 0, wxLEFT | wxRIGHT | wxTOP, 10);

  // Текст с описанием силы пароля
  strengthText =
      new wxStaticText(panel, wxID_ANY, "Enter password to check strength");
  strengthText->SetForegroundColour(wxColour(100, 100, 100));
  panelSizer->Add(strengthText, 0, wxLEFT | wxRIGHT | wxBOTTOM, 5);

  panelSizer->Add(confirmLabel, 0, wxLEFT | wxTOP, 15);
  panelSizer->Add(confirmCtrl, 0, wxLEFT | wxRIGHT | wxEXPAND, 5);

  panel->SetSizer(panelSizer);
  mainSizer->Add(panel, 0, wxALIGN_CENTER, 0);

  mainSizer->AddStretchSpacer();

  page->SetSizer(mainSizer);

  // Создаем таймер для задержки проверки
  strengthTimer = new wxTimer(this, ID_STRENGTH_TIMER);

  return page;
}

wxWizardPageSimple *FirstRunWizard::createDatabasePage()
{
  wxWizardPageSimple *page = new wxWizardPageSimple(this);

  wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

  // Заголовок
  wxStaticText *title = new wxStaticText(page, wxID_ANY, "Database Location");
  wxFont titleFont = title->GetFont();
  titleFont.SetPointSize(14);
  titleFont.SetWeight(wxFONTWEIGHT_BOLD);
  title->SetFont(titleFont);

  mainSizer->Add(title, 0, wxALL | wxALIGN_CENTER, 20);

  // Поле выбора пути
  wxBoxSizer *pathSizer = new wxBoxSizer(wxHORIZONTAL);

  wxStaticText *pathLabel = new wxStaticText(page, wxID_ANY, "Path:");
  pathSizer->Add(pathLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

  dbPathCtrl = new wxTextCtrl(page, wxID_ANY, config.getDatabasePath(),
                              wxDefaultPosition, wxSize(300, -1));
  pathSizer->Add(dbPathCtrl, 1, wxRIGHT, 10);

  browseButton = new wxButton(page, ID_BrowseButton, "Browse...");
  pathSizer->Add(browseButton, 0);

  mainSizer->Add(pathSizer, 0, wxEXPAND | wxALL, 20);

  // Пояснение
  wxStaticText *info = new wxStaticText(
      page, wxID_ANY, "All passwords will be stored in this file");
  info->SetForegroundColour(wxColour(100, 100, 100));
  mainSizer->Add(info, 0, wxALL | wxALIGN_CENTER, 5);

  mainSizer->AddStretchSpacer();
  page->SetSizer(mainSizer);

  return page;
}

wxWizardPageSimple *FirstRunWizard::createEncryptionPage()
{
  wxWizardPageSimple *page = new wxWizardPageSimple(this);
  wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

  // Заголовок
  wxStaticText *title = new wxStaticText(page, wxID_ANY, "Encryption Settings");
  wxFont titleFont = title->GetFont();
  titleFont.SetPointSize(14);
  titleFont.SetWeight(wxFONTWEIGHT_BOLD);
  title->SetFont(titleFont);
  mainSizer->Add(title, 0, wxALL | wxALIGN_CENTER, 20);

  // Пояснение
  wxStaticText *note = new wxStaticText(
      page, wxID_ANY,
      "These settings control how your master password is strengthened.\n"
      "Higher values = more secure but slower unlock.");
  note->SetForegroundColour(wxColour(100, 100, 100));
  mainSizer->Add(note, 0, wxALL | wxALIGN_CENTER, 5);

  // Растягиваемый пробел сверху
  mainSizer->AddStretchSpacer();

  // Панель с настройками
  wxPanel *panel = new wxPanel(page);
  wxBoxSizer *panelSizer = new wxBoxSizer(wxVERTICAL);

  // Сетка для настроек Argon2id
  wxFlexGridSizer *gridSizer = new wxFlexGridSizer(2, 15, 15);
  gridSizer->AddGrowableCol(1);

  // 1. Time cost (итерации)
  wxStaticText *iterLabel =
      new wxStaticText(panel, wxID_ANY, "Time cost (iterations):");
  iterationsSpin =
      new wxSpinCtrl(panel, wxID_ANY, "3", wxDefaultPosition, wxSize(100, -1));
  iterationsSpin->SetRange(1, 20);
  iterationsSpin->SetValue(3); // минимум 3 по требованию

  gridSizer->Add(iterLabel, 0, wxALIGN_CENTER_VERTICAL);
  gridSizer->Add(iterationsSpin, 0, wxEXPAND);

  // 2. Memory cost (MB)
  wxStaticText *memoryLabel =
      new wxStaticText(panel, wxID_ANY, "Memory cost (MiB):");
  memorySpin =
      new wxSpinCtrl(panel, wxID_ANY, "64", wxDefaultPosition, wxSize(100, -1));
  memorySpin->SetRange(16, 1024);
  memorySpin->SetValue(64); // 64 MiB по умолчанию

  gridSizer->Add(memoryLabel, 0, wxALIGN_CENTER_VERTICAL);
  gridSizer->Add(memorySpin, 0, wxEXPAND);

  // 3. Parallelism (потоки)
  wxStaticText *parallelLabel =
      new wxStaticText(panel, wxID_ANY, "Parallelism (threads):");
  parallelSpin =
      new wxSpinCtrl(panel, wxID_ANY, "4", wxDefaultPosition, wxSize(100, -1));
  parallelSpin->SetRange(1, 16);
  parallelSpin->SetValue(4); // 4 потока по умолчанию

  gridSizer->Add(parallelLabel, 0, wxALIGN_CENTER_VERTICAL);
  gridSizer->Add(parallelSpin, 0, wxEXPAND);

  // 4. Hash length (bytes)
  wxStaticText *hashLengthLabel =
      new wxStaticText(panel, wxID_ANY, "Hash length (bytes):");
  hashLengthSpin =
      new wxSpinCtrl(panel, wxID_ANY, "32", wxDefaultPosition, wxSize(100, -1));
  hashLengthSpin->SetRange(16, 64);
  hashLengthSpin->SetValue(32); // 32 bytes = 256 bits

  gridSizer->Add(hashLengthLabel, 0, wxALIGN_CENTER_VERTICAL);
  gridSizer->Add(hashLengthSpin, 0, wxEXPAND);

  panelSizer->Add(gridSizer, 0, wxEXPAND | wxALL, 20);

  // Пояснение про настройки Argon2id
  wxStaticText *helpText = new wxStaticText(
      panel, wxID_ANY,
      "Argon2id parameters (NIST recommended minimums):\n"
      "• Time cost: 3 iterations (higher = slower brute-force)\n"
      "• Memory cost: 64 MiB (higher = more RAM required for attack)\n"
      "• Parallelism: 4 threads (higher = faster on multi-core)\n"
      "• Hash length: 32 bytes (256 bits) - sufficient for AES-256",
      wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
  helpText->SetForegroundColour(wxColour(80, 80, 80));
  panelSizer->Add(helpText, 0, wxALL | wxEXPAND, 10);

  panel->SetSizer(panelSizer);
  mainSizer->Add(panel, 0, wxALIGN_CENTER, 0);

  // Растягиваемый пробел снизу
  mainSizer->AddStretchSpacer();

  page->SetSizer(mainSizer);
  return page;
}

wxWizardPageSimple *FirstRunWizard::createFinishPage()
{
  wxWizardPageSimple *page = new wxWizardPageSimple(this);

  wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

  // Заголовок
  wxStaticText *title = new wxStaticText(page, wxID_ANY, "Setup Complete!");
  wxFont titleFont = title->GetFont();
  titleFont.SetPointSize(16);
  titleFont.SetWeight(wxFONTWEIGHT_BOLD);
  title->SetFont(titleFont);

  mainSizer->Add(title, 0, wxALL | wxALIGN_CENTER, 20);

  // Текст завершения
  wxStaticText *text =
      new wxStaticText(page, wxID_ANY,
                       "Your password manager is ready to use.\n\n"
                       "Click Finish to start the application.",
                       wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);

  mainSizer->Add(text, 0, wxALL | wxEXPAND, 20);

  mainSizer->AddStretchSpacer();
  page->SetSizer(mainSizer);

  return page;
}

void FirstRunWizard::onBrowseDatabase(wxCommandEvent &event)
{
  wxFileDialog dialog(this, "Select database file", "", "",
                      "SQLite files (*.db)|*.db|All files (*.*)|*.*",
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

  if (dialog.ShowModal() == wxID_OK)
  {
    dbPathCtrl->SetValue(dialog.GetPath());
  }
}

void FirstRunWizard::onPasswordPageChanging(wxWizardEvent &event)
{
  // Проверяем, пытаемся ли мы уйти СО страницы пароля ВПЕРЕД
  if (event.GetDirection() && GetCurrentPage() == passwordPage)
  {
    if (!validatePassword())
    {
      event.Veto(); // Отменяем переход
    }
  }
}

bool FirstRunWizard::validatePassword()
{
  wxString password = passwordCtrl->GetValue();
  wxString confirm = confirmCtrl->GetValue();

  if (password.IsEmpty())
  {
    wxMessageBox("Password cannot be empty!", "Error", wxOK | wxICON_ERROR,
                 this);
    return false;
  }

  if (password != confirm)
  {
    wxMessageBox("Passwords do not match!", "Error", wxOK | wxICON_ERROR, this);
    return false;
  }

  if (password.length() < 12)
  {
    wxMessageBox("Password must be at least 12 characters!", "Error",
                 wxOK | wxICON_ERROR, this);
    return false;
  }

  // Проверка силы пароля через zxcvbn
  wxScopedCharBuffer pwdBuf = password.ToUTF8();
  std::string pwdStr(pwdBuf.data(), pwdBuf.length());
  int score = check_password_strength(pwdStr);

  if (score < 3)
  {
    wxMessageBox("Password is not strong enough!\n\n"
                 "Please choose a stronger password that is not common, "
                 "doesn't contain dictionary words, and has good entropy.",
                 "Weak Password", wxOK | wxICON_WARNING, this);
    return false;
  }

  temp_password = password;
  return true;
}

void FirstRunWizard::onStrengthTimer(wxTimerEvent &event)
{
  wxString password = passwordCtrl->GetValue();

  if (password.IsEmpty())
  {
    strengthGauge->SetValue(0);
    strengthText->SetLabel("Enter password to check strength");
    strengthText->SetForegroundColour(wxColour(100, 100, 100));
    return;
  }

  // Конвертируем wxString в std::string
  wxScopedCharBuffer pwdBuf = password.ToUTF8();
  std::string pwdStr(pwdBuf.data(), pwdBuf.length());

  // Получаем оценку силы пароля (0-4)
  int score = check_password_strength(pwdStr);
  std::cout << "====================== " << score << std::endl;

  // Обновляем индикатор
  strengthGauge->SetValue(score);

  // Обновляем текст и цвет в зависимости от оценки
  wxColour color;
  wxString message;

  switch (score)
  {
  case 0:
    color = wxColour(255, 0, 0); // Красный
    message = "Too weak - easily guessable";
    break;
  case 1:
    color = wxColour(255, 100, 0); // Оранжевый
    message = "Very weak";
    break;
  case 2:
    color = wxColour(255, 255, 0); // Желтый
    message = "Weak";
    break;
  case 3:
    color = wxColour(0, 255, 0); // Зеленый
    message = "Strong";
    break;
  case 4:
    color = wxColour(0, 200, 0); // Темно-зеленый
    message = "Very strong";
    break;
  default:
    color = wxColour(100, 100, 100); // Серый
    message = "Unknown";
  }

  strengthText->SetLabel(message);
  strengthText->SetForegroundColour(color);
}

void FirstRunWizard::onPasswordTextChanged(wxCommandEvent &event)
{
  // Перезапускаем таймер при каждом изменении текста
  strengthTimer->Stop();
  strengthTimer->Start(
      500, wxTIMER_ONE_SHOT); // Проверка через 500 мс после остановки ввода
}

void FirstRunWizard::onWizardFinished(wxWizardEvent &event)
{
  // 1. Database path
  config.setDatabasePath(dbPathCtrl->GetValue().ToStdString());

  // 2. Argon2id parameters
  config.setArgon2TimeCost(iterationsSpin->GetValue());   // iterations
  config.setArgon2MemoryCost(memorySpin->GetValue());     // MB
  config.setArgon2Parallelism(parallelSpin->GetValue());  // threads
  config.setArgon2HashLength(hashLengthSpin->GetValue()); // bytes

  // БД еще нет, но мы готовим данные
  Argon2Data authData(iterationsSpin->GetValue(), memorySpin->GetValue(),
                      parallelSpin->GetValue(), hashLengthSpin->GetValue());

  // Конвертируем wxString в std::string для хеширования
  wxScopedCharBuffer pwdBuf = temp_password.ToUTF8();
  std::string pwdStr(pwdBuf.data(), pwdBuf.length());

  // Хешируем пароль
  hash_password(pwdStr, authData);

  // Сохраняем данные аутентификации во временные поля мастера
  // (БД будет создана после закрытия мастера)
  pendingAuthData = std::move(authData);

  std::vector<uint8_t> key;
  encSalt.resize(16);

  randombytes_buf(encSalt.data(), encSalt.size());

  derive_encryption_key(pwdStr, encSalt, key);

  KeyManager::getInstance().store_key(key);

  // Зануляем пароль в памяти
  volatile char *p = const_cast<char *>(pwdStr.data());
  for (size_t i = 0; i < pwdStr.size(); ++i)
    p[i] = 0;

  wxMessageBox("Setup completed successfully!", "CryptoSafe",
               wxOK | wxICON_INFORMATION, this);

  event.Skip();
}

Argon2Data &FirstRunWizard::getAuthData() { return pendingAuthData; }

