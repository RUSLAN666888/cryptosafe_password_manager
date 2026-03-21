#ifndef PASSWORD_CHANGE_H
#define PASSWORD_CHANGE_H

#include <wx/wx.h>
#include "../../../database/DB_helper/db_helper.h"
#include "../../../core/crypto/authentication.h"
#include "../../../core/key_manager.h"
#include "../src/gui/widgets/password_entry/PasswordEntry.h"

class ChangePasswordDialog : public wxDialog
{
private:
    Database &db;

    // Панели
    wxPanel *verifyPanel;
    wxPanel *changePanel;
    wxBoxSizer *mainSizer;

    // Страница 1: верификация
    PasswordEntry *currentPasswordCtrl;
    wxStaticText *errorText;
    wxButton *verifyNextButton;

    // Страница 2: смена пароля
    PasswordEntry *newPasswordCtrl;
    PasswordEntry *confirmPasswordCtrl;
    wxGauge *strengthGauge;
    wxStaticText *strengthText;
    wxButton *changeButton;
    wxButton *cancelButton;
    wxTimer *strengthTimer;

    // Данные для аутентификации
    Argon2Data authData;
    std::vector<uint8_t> encSalt;

    // Временное хранение пароля
    std::string tempPassword;

    static const int ID_STRENGTH_TIMER = 10005;

    void onVerifyNext(wxCommandEvent &event);
    void onChange(wxCommandEvent &event);
    void onCancel(wxCommandEvent &event);
    void onPasswordTextChanged(wxCommandEvent &event);
    void onStrengthTimer(wxTimerEvent &event);
    void updatePasswordStrength();
    bool loadAuthData();
    bool verifyCurrentPassword();
    bool validateNewPassword();
    void switchToChangePage();

    wxPanel* createVerifyPage();
    wxPanel* createChangePage();

public:
    ChangePasswordDialog(wxWindow* parent, Database& database);
    ~ChangePasswordDialog();

    wxDECLARE_EVENT_TABLE();
};

#endif // PASSWORD_CHANGE_H
