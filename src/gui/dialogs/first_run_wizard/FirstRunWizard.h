#ifndef FIRST_RUN_WIZARD_H
#define FIRST_RUN_WIZARD_H

#include "../src/core/config_handler.h"
#include "../src/gui/widgets/password_entry/PasswordEntry.h"
#include <wx/spinctrl.h>
#include <wx/wizard.h>
#include <wx/wx.h>

class FirstRunWizard : public wxWizard
{
private:
  ConfigHander &config;
  wxString temp_password;

  // Страницы
  wxWizardPageSimple *welcomePage;
  wxWizardPageSimple *passwordPage;
  wxWizardPageSimple *databasePage;
  wxWizardPageSimple *encryptionPage;
  wxWizardPageSimple *finishPage;

  // Элементы для страницы пароля
  PasswordEntry *passwordCtrl;
  PasswordEntry *confirmCtrl;
  wxStaticText *strengthText;

  // Элементы для страницы базы данных
  wxTextCtrl *dbPathCtrl;
  wxButton *browseButton;

  // Элементы для страницы шифрования
  wxSpinCtrl *iterationsSpin; // time cost (итерации)
  wxSpinCtrl *memorySpin;     // memory cost (MB)
  wxSpinCtrl *parallelSpin;   // parallelism (потоки)
  wxSpinCtrl *hashLengthSpin; // hash length (bytes)

  // Создание страниц
  wxWizardPageSimple *createWelcomePage();
  wxWizardPageSimple *createPasswordPage();
  wxWizardPageSimple *createDatabasePage();
  wxWizardPageSimple *createEncryptionPage();
  wxWizardPageSimple *createFinishPage();

  // Обработчики
  void onBrowseDatabase(wxCommandEvent &event);
  void onWizardFinished(wxWizardEvent &event);
  void onPasswordPageChanging(wxWizardEvent &event);

  bool validatePassword();

public:
  wxWizardPage *GetFirstPage() const { return welcomePage; }
  FirstRunWizard(wxWindow *parent, ConfigHander &cfg);

  wxDECLARE_EVENT_TABLE();
};

#endif // FIRST_RUN_WIZARD_H