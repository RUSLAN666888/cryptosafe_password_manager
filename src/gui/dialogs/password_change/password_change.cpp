#include "../src/gui/dialogs/password_change/password_change.h"
#include <wx/msgdlg.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include "../src/core/crypto/authentication.h"
#include <wx/sizer.h>

wxBEGIN_EVENT_TABLE(ChangePasswordDialog, wxDialog)
    EVT_TEXT(wxID_ANY, ChangePasswordDialog::onPasswordTextChanged)
    EVT_TIMER(ID_STRENGTH_TIMER, ChangePasswordDialog::onStrengthTimer)
wxEND_EVENT_TABLE()

    ChangePasswordDialog::ChangePasswordDialog(wxWindow* parent, Database& database)
    : wxDialog(parent, wxID_ANY, "Change Master Password",wxDefaultPosition, wxSize(450, 400)), db(database)
{
    // Загружаем данные аутентификации
    if (!loadAuthData())
    {
        wxMessageBox("Failed to load authentication data. Database may be corrupted.",
                     "Error", wxOK | wxICON_ERROR, this);
        return;
    }

    // Создаем основной sizer
    mainSizer = new wxBoxSizer(wxVERTICAL);

    // Создаем страницы
    verifyPanel = createVerifyPage();
    changePanel = createChangePage();

    // Показываем только страницу верификации
    mainSizer->Add(verifyPanel, 1, wxEXPAND);
    changePanel->Hide();
    mainSizer->Add(changePanel, 1, wxEXPAND);

    SetSizer(mainSizer);
    Layout();
    Center();

    // Таймер для проверки силы пароля
    strengthTimer = new wxTimer(this, ID_STRENGTH_TIMER);

    // Устанавливаем фокус
    currentPasswordCtrl->SetFocus();
}

ChangePasswordDialog::~ChangePasswordDialog()
{
    if (strengthTimer)
    {
        strengthTimer->Stop();
        delete strengthTimer;
    }
}

wxPanel* ChangePasswordDialog::createVerifyPage()
{
    wxPanel *panel = new wxPanel(this);
    wxBoxSizer *panelSizer = new wxBoxSizer(wxVERTICAL);

    // Заголовок
    wxStaticText *title = new wxStaticText(panel, wxID_ANY, "Verify Current Password");
    wxFont titleFont = title->GetFont();
    titleFont.SetPointSize(14);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    title->SetFont(titleFont);
    panelSizer->Add(title, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 20);

    panelSizer->AddStretchSpacer();

    // Поле пароля
    wxStaticText *passLabel = new wxStaticText(panel, wxID_ANY, "Current Password:");
    currentPasswordCtrl = new PasswordEntry(panel, wxID_ANY, "",
                                            wxDefaultPosition, wxSize(300, -1));

    errorText = new wxStaticText(panel, wxID_ANY, "");
    errorText->SetForegroundColour(wxColour(255, 0, 0));

    panelSizer->Add(passLabel, 0, wxLEFT | wxRIGHT | wxTOP, 10);
    panelSizer->Add(currentPasswordCtrl, 0, wxLEFT | wxRIGHT | wxEXPAND, 10);
    panelSizer->Add(errorText, 0, wxLEFT | wxRIGHT, 10);

    panelSizer->AddStretchSpacer();

    // Кнопки
    wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    verifyNextButton = new wxButton(panel, wxID_ANY, "Verify & Next");
    wxButton *cancelBtn = new wxButton(panel, wxID_ANY, "Cancel");

    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(verifyNextButton, 0, wxRIGHT, 10);
    buttonSizer->Add(cancelBtn, 0);
    buttonSizer->AddSpacer(20);

    panelSizer->Add(buttonSizer, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 15);

    panel->SetSizer(panelSizer);

    // Bind кнопки
    verifyNextButton->Bind(wxEVT_BUTTON, &ChangePasswordDialog::onVerifyNext, this);
    cancelBtn->Bind(wxEVT_BUTTON, &ChangePasswordDialog::onCancel, this);

    return panel;
}

wxPanel* ChangePasswordDialog::createChangePage()
{
    wxPanel *panel = new wxPanel(this);
    wxBoxSizer *panelSizer = new wxBoxSizer(wxVERTICAL);

    // Заголовок
    wxStaticText *title = new wxStaticText(panel, wxID_ANY, "Create New Master Password");
    wxFont titleFont = title->GetFont();
    titleFont.SetPointSize(14);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    title->SetFont(titleFont);
    panelSizer->Add(title, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 20);

    panelSizer->AddStretchSpacer();

    // Новый пароль
    wxStaticText *newPassLabel = new wxStaticText(panel, wxID_ANY, "New Password:");
    newPasswordCtrl = new PasswordEntry(panel, wxID_ANY, "",
                                        wxDefaultPosition, wxSize(300, -1));

    // Индикатор силы пароля
    strengthGauge = new wxGauge(panel, wxID_ANY, 4,
                                wxDefaultPosition, wxSize(300, 20));
    strengthGauge->SetValue(0);

    strengthText = new wxStaticText(panel, wxID_ANY, "Enter password to check strength");
    strengthText->SetForegroundColour(wxColour(100, 100, 100));

    // Подтверждение пароля
    wxStaticText *confirmLabel = new wxStaticText(panel, wxID_ANY, "Confirm Password:");
    confirmPasswordCtrl = new PasswordEntry(panel, wxID_ANY, "",
                                            wxDefaultPosition, wxSize(300, -1));

    panelSizer->Add(newPassLabel, 0, wxLEFT | wxRIGHT | wxTOP, 10);
    panelSizer->Add(newPasswordCtrl, 0, wxLEFT | wxRIGHT | wxEXPAND, 10);
    panelSizer->Add(strengthGauge, 0, wxLEFT | wxRIGHT | wxTOP, 10);
    panelSizer->Add(strengthText, 0, wxLEFT | wxRIGHT | wxBOTTOM, 5);
    panelSizer->Add(confirmLabel, 0, wxLEFT | wxRIGHT | wxTOP, 10);
    panelSizer->Add(confirmPasswordCtrl, 0, wxLEFT | wxRIGHT | wxEXPAND, 10);

    panelSizer->AddStretchSpacer();

    // Кнопки
    wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    changeButton = new wxButton(panel, wxID_ANY, "Change Password");
    cancelButton = new wxButton(panel, wxID_ANY, "Cancel");

    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(changeButton, 0, wxRIGHT, 10);
    buttonSizer->Add(cancelButton, 0);
    buttonSizer->AddSpacer(20);

    panelSizer->Add(buttonSizer, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 15);

    panel->SetSizer(panelSizer);

    // Bind кнопки
    changeButton->Bind(wxEVT_BUTTON, &ChangePasswordDialog::onChange, this);
    cancelButton->Bind(wxEVT_BUTTON, &ChangePasswordDialog::onCancel, this);

    return panel;
}

bool ChangePasswordDialog::loadAuthData()
{
    std::vector<uint8_t> hash, salt;
    uint32_t time_cost, memory_cost, parallelism, hash_len;

    if (!db.getAuthData(hash, salt, time_cost, memory_cost, parallelism, hash_len))
    {
        return false;
    }

    if (!db.getEncSalt(encSalt))
    {
        return false;
    }

    authData = Argon2Data(time_cost, memory_cost, parallelism, hash_len);
    authData.hash = std::move(hash);
    authData.salt = std::move(salt);

    return true;
}

bool ChangePasswordDialog::verifyCurrentPassword()
{
    wxString password = currentPasswordCtrl->GetValue();

    if (password.IsEmpty())
    {
        errorText->SetLabel("Password cannot be empty");
        return false;
    }

    wxScopedCharBuffer pwdBuf = password.ToUTF8();
    std::string pwdStr(pwdBuf.data(), pwdBuf.length());

    if (!verify_password(pwdStr, authData))
    {
        errorText->SetLabel("Invalid password");
        return false;
    }

    // Зануляем пароль в памяти
    volatile char* p = const_cast<char*>(pwdStr.data());
    for (size_t i = 0; i < pwdStr.size(); ++i)
    {
        p[i] = 0;
    }

    errorText->SetLabel("");
    return true;
}

bool ChangePasswordDialog::validateNewPassword()
{
    wxString password = newPasswordCtrl->GetValue();
    wxString confirm = confirmPasswordCtrl->GetValue();

    if (password.IsEmpty())
    {
        wxMessageBox("Password cannot be empty!", "Error", wxOK | wxICON_ERROR, this);
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

    tempPassword = pwdStr;
    return true;
}

void ChangePasswordDialog::updatePasswordStrength()
{
    wxString password = newPasswordCtrl->GetValue();

    if (password.IsEmpty())
    {
        strengthGauge->SetValue(0);
        strengthText->SetLabel("Enter password to check strength");
        strengthText->SetForegroundColour(wxColour(100, 100, 100));
        return;
    }

    wxScopedCharBuffer pwdBuf = password.ToUTF8();
    std::string pwdStr(pwdBuf.data(), pwdBuf.length());
    int score = check_password_strength(pwdStr);

    strengthGauge->SetValue(score);

    wxColour color;
    wxString message;

    switch (score)
    {
    case 0: color = wxColour(255, 0, 0); message = "Too weak"; break;
    case 1: color = wxColour(255, 100, 0); message = "Very weak"; break;
    case 2: color = wxColour(255, 255, 0); message = "Weak"; break;
    case 3: color = wxColour(0, 255, 0); message = "Strong"; break;
    case 4: color = wxColour(0, 200, 0); message = "Very strong"; break;
    default: color = wxColour(100, 100, 100); message = "Unknown";
    }

    strengthText->SetLabel(message);
    strengthText->SetForegroundColour(color);
}

void ChangePasswordDialog::switchToChangePage()
{
    verifyPanel->Hide();
    changePanel->Show();
    mainSizer->Layout();
    newPasswordCtrl->SetFocus();
}

void ChangePasswordDialog::onVerifyNext(wxCommandEvent &event)
{
    if (verifyCurrentPassword())
    {
        switchToChangePage();
    }
}

void ChangePasswordDialog::onChange(wxCommandEvent &event)
{
    if (!validateNewPassword())
    {
        return;
    }

    hash_password(tempPassword, authData);

    db.saveAuthData(authData.hash, authData.salt, authData.time_cost, authData.memory_cost_mb,
                    authData.parallelism, authData.hash_len);

    std::vector<uint8_t> encSalt(16);
    randombytes_buf(encSalt.data(), encSalt.size());
    db.saveEncSalt(encSalt);


    wxMessageBox("Password changed successfully!\n\n"
                 "You will need to log in again with your new password.",
                 "Success", wxOK | wxICON_INFORMATION, this);

    // Выходим из системы
    KeyManager::getInstance().logout();

    // Закрываем диалог
    EndModal(wxID_OK);


    // Зануляем временный пароль
    volatile char* p = const_cast<char*>(tempPassword.data());
    for (size_t i = 0; i < tempPassword.size(); ++i)
    {
        p[i] = 0;
    }
    tempPassword.clear();
}

void ChangePasswordDialog::onCancel(wxCommandEvent &event)
{
    EndModal(wxID_CANCEL);
}

void ChangePasswordDialog::onPasswordTextChanged(wxCommandEvent &event)
{
    if (strengthTimer)
    {
        strengthTimer->Stop();
        strengthTimer->Start(500, wxTIMER_ONE_SHOT);
    }
}

void ChangePasswordDialog::onStrengthTimer(wxTimerEvent &event)
{
    updatePasswordStrength();
}
