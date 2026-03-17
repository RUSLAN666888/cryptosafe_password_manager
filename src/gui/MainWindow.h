#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../src/core/config_handler.h"
#include "../src/database/DB_helper/db_helper.h"
#include "../src/gui/widgets/audit_log_viewer/AuditLogViewer.h"
#include "../src/gui/widgets/secure_table/SecureTable.h"
#include <wx/listctrl.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/timer.h>
#include <wx/wx.h>

// Идентификаторы для меню
enum
{
  ID_NewDatabase = wxID_HIGHEST + 1,
  ID_OpenDatabase,
  ID_Backup,
  ID_Exit,
  ID_AddEntry,
  ID_EditEntry,
  ID_DeleteEntry,
  ID_ViewLogs,
  ID_Settings,
  ID_About,
  ID_ShowPassword,
  ID_CopyPassword,
  ID_FirstRunWizard
};

class MainWindow : public wxFrame
{
private:
  ConfigHander &config;
  Database &db;

  // Виджеты
  wxMenuBar *menuBar;
  wxStatusBar *statusBar;
  wxToolBar *toolBar;
  wxPanel *mainPanel;
  wxBoxSizer *mainSizer;

  // Таблица с паролями
  SecureTable *passwordTable;

  // Панель поиска
  wxPanel *searchPanel;
  wxTextCtrl *searchInput;
  wxButton *searchButton;
  wxButton *clearSearchButton;

  // Кнопки действий
  wxPanel *buttonPanel;
  wxButton *addButton;
  wxButton *editButton;
  wxButton *deleteButton;
  wxButton *copyButton;

  // Статус бар
  wxStaticText *statusLoginText;
  wxStaticText *statusClipboardText;
  wxGauge *statusTimerGauge;

  // Состояние
  bool isLoggedIn;
  int clipboardTimerSeconds;
  wxTimer *clipboardTimer;

private:
  // Создание меню
  void createMenuBar();

  // Создание панели инструментов
  void createToolBar();

  // Создание главной панели
  void createMainPanel();

  // Создание панели поиска
  void createSearchPanel();

  // Создание панели кнопок
  void createButtonPanel();

  // Создание статус бара
  void createStatusBar();

  // Загрузка тестовых данных
  void loadSampleData();

  // Обработчики меню
  void onNewDatabase(wxCommandEvent &event);
  void onOpenDatabase(wxCommandEvent &event);
  void onBackup(wxCommandEvent &event);
  void onExit(wxCommandEvent &event);
  void onAddEntry(wxCommandEvent &event);
  void onEditEntry(wxCommandEvent &event);
  void onDeleteEntry(wxCommandEvent &event);
  void onViewLogs(wxCommandEvent &event);
  void onSettings(wxCommandEvent &event);
  void onAbout(wxCommandEvent &event);
  void onFirstRunWizard(wxCommandEvent &event);

  // Обработчики таблицы
  void onTableItemSelected(wxListEvent &event);
  void onTableItemActivated(wxListEvent &event);

  // Обработчики поиска
  void onSearch(wxCommandEvent &event);
  void onClearSearch(wxCommandEvent &event);
  void onSearchEnter(wxCommandEvent &event);

  // Обработчики кнопок
  void onCopyPassword(wxCommandEvent &event);

  // Таймер для клипборда
  void onClipboardTimer(wxTimerEvent &event);
  void startClipboardTimer(int seconds);
  void stopClipboardTimer();

  // Обновление статуса
  void updateStatusBar();

public:
  MainWindow(ConfigHander &cfg, Database &database);
  virtual ~MainWindow();

  // Показать окно первого запуска
  void showFirstRunWizard();

  // Обновить таблицу с данными из БД
  void refreshPasswordTable();

  wxDECLARE_EVENT_TABLE();
};

#endif // MAINWINDOW_H