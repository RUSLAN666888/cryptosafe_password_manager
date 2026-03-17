#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include "../src/core/config_handler.h"
#include <wx/notebook.h>
#include <wx/wx.h>

class SettingsDialog : public wxDialog
{
private:
  ConfigHander &config;

  wxNotebook *notebook;

  // В Sprint 1 - все методы для создания вкладок
  void createGeneralTab(wxNotebook *notebook);  // Реальная информация
  void createSecurityTab(wxNotebook *notebook); // Заглушка
  void createAdvancedTab(wxNotebook *notebook); // Заглушка

  // Обработчики
  void onOk(wxCommandEvent &event);
  void onCancel(wxCommandEvent &event);

public:
  SettingsDialog(wxWindow *parent, ConfigHander &cfg);

  wxDECLARE_EVENT_TABLE();
};

#endif // SETTINGS_DIALOG_H