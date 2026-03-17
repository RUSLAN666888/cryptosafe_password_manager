#include "SettingsDialog.h"
#include <wx/msgdlg.h>

wxBEGIN_EVENT_TABLE(SettingsDialog, wxDialog)
    EVT_BUTTON(wxID_OK, SettingsDialog::onOk)
        EVT_BUTTON(wxID_CANCEL, SettingsDialog::onCancel) wxEND_EVENT_TABLE()

            SettingsDialog::SettingsDialog(wxWindow *parent, ConfigHander &cfg)
    : wxDialog(parent, wxID_ANY, "Settings", wxDefaultPosition,
               wxSize(450, 300)),
      config(cfg)
{
  std::cout << "SettingsDialog constructor start" << std::endl;

  wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
  std::cout << "MainSizer created" << std::endl;

  // Создаем вкладки (все с заглушками)
  notebook = new wxNotebook(this, wxID_ANY);
  std::cout << "Notebook created" << std::endl;

  std::cout << "Creating General tab..." << std::endl;
  createGeneralTab(notebook);
  std::cout << "General tab created" << std::endl;

  std::cout << "Creating Security tab..." << std::endl;
  createSecurityTab(notebook);
  std::cout << "Security tab created" << std::endl;

  std::cout << "Creating Advanced tab..." << std::endl;
  createAdvancedTab(notebook);
  std::cout << "Advanced tab created" << std::endl;

  mainSizer->Add(notebook, 1, wxEXPAND | wxALL, 10);
  std::cout << "Notebook added to sizer" << std::endl;

  // Кнопки OK/Cancel
  wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
  buttonSizer->AddStretchSpacer();
  std::cout << "Button sizer created" << std::endl;

  wxButton *okButton = new wxButton(this, wxID_OK, "OK");
  wxButton *cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");
  std::cout << "Buttons created" << std::endl;

  buttonSizer->Add(okButton, 0, wxRIGHT, 10);
  buttonSizer->Add(cancelButton, 0);
  std::cout << "Buttons added to sizer" << std::endl;

  mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 10);
  std::cout << "Button sizer added to main sizer" << std::endl;

  SetSizer(mainSizer);
  std::cout << "Sizer set" << std::endl;

  Centre();
  std::cout << "Dialog centered" << std::endl;
}

// ============== ВКЛАДКА GENERAL (единственная с реальными данными в Sprint 1)
// ==============
void SettingsDialog::createGeneralTab(wxNotebook *notebook)
{
  std::cout << "createGeneralTab: start" << std::endl;

  // Минимальный код, который точно должен работать
  wxPanel *panel = new wxPanel(notebook);
  std::cout << "panel created" << std::endl;

  wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
  std::cout << "sizer created" << std::endl;

  // Простой текст без всяких вызовов config
  wxStaticText *simpleText = new wxStaticText(panel, wxID_ANY, "Test Text");
  std::cout << "static text created" << std::endl;

  mainSizer->Add(simpleText, 0, wxALL, 10);
  std::cout << "text added to sizer" << std::endl;

  panel->SetSizer(mainSizer);
  std::cout << "sizer set" << std::endl;

  notebook->AddPage(panel, "General");
  std::cout << "page added to notebook" << std::endl;

  std::cout << "createGeneralTab: end" << std::endl;
}

// ============== ВКЛАДКА SECURITY (ЗАГЛУШКА) ==============
void SettingsDialog::createSecurityTab(wxNotebook *notebook)
{
  wxPanel *panel = new wxPanel(notebook);
  wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

  wxStaticText *title = new wxStaticText(panel, wxID_ANY, "Security Settings");
  wxFont titleFont = title->GetFont();
  titleFont.SetWeight(wxFONTWEIGHT_BOLD);
  title->SetFont(titleFont);
  mainSizer->Add(title, 0, wxALL, 10);

  wxStaticText *placeholder =
      new wxStaticText(panel, wxID_ANY,
                       "Security settings will be available in Sprint 4:\n"
                       "- Clipboard timeout\n"
                       "- Auto-lock timeout\n"
                       "- Panic key\n"
                       "- Memory wipe options");
  mainSizer->Add(placeholder, 0, wxALL, 10);

  mainSizer->AddStretchSpacer();
  panel->SetSizer(mainSizer);
  notebook->AddPage(panel, "Security");
}

// ============== ВКЛАДКА ADVANCED (ЗАГЛУШКА) ==============
void SettingsDialog::createAdvancedTab(wxNotebook *notebook)
{
  wxPanel *panel = new wxPanel(notebook);
  wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

  wxStaticText *title = new wxStaticText(panel, wxID_ANY, "Advanced Settings");
  wxFont titleFont = title->GetFont();
  titleFont.SetWeight(wxFONTWEIGHT_BOLD);
  title->SetFont(titleFont);
  mainSizer->Add(title, 0, wxALL, 10);

  wxStaticText *placeholder = new wxStaticText(
      panel, wxID_ANY,
      "Advanced settings will be available in future sprints:\n"
      "- Import/Export (Sprint 6)\n"
      "- Backup (Sprint 8)\n"
      "- Theme (future)\n"
      "- Language (future)");
  mainSizer->Add(placeholder, 0, wxALL, 10);

  mainSizer->AddStretchSpacer();
  panel->SetSizer(mainSizer);
  notebook->AddPage(panel, "Advanced");
}

// ============== ОБРАБОТЧИКИ ==============
void SettingsDialog::onOk(wxCommandEvent &event)
{
  // В Sprint 1 ничего не сохраняем, просто показываем сообщение
  wxMessageBox("Settings dialog is a placeholder for Sprint 1.\n\n"
               "Real settings will be implemented in future sprints:\n"
               "- Security settings - Sprint 4\n"
               "- Import/Export - Sprint 6\n"
               "- Backup - Sprint 8\n"
               "- Theme/Language - future",
               "CryptoSafe Manager", wxOK | wxICON_INFORMATION, this);

  EndModal(wxID_OK);
}

void SettingsDialog::onCancel(wxCommandEvent &event) { EndModal(wxID_CANCEL); }