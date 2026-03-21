// MainWindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../src/core/config_handler.h"
#include "../src/core/events.h"
#include "../src/core/key_manager.h"
#include "../src/database/DB_helper/db_helper.h"
#include "../src/gui/dialogs/login_dialog/LoginDialog.h"
#include "../src/gui/widgets/secure_table/SecureTable.h"
#include <wx/listctrl.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/timer.h>
#include <wx/wx.h>

class MainWindow : public wxFrame
{
private:
    ConfigHander &config;
    Database &db;
    bool isLoggedIn;
    bool isFirstRun;
    bool isShowingLoginDialog;

    // Элементы UI
    wxMenuBar *menuBar;
    wxToolBar *toolBar;
    wxPanel *mainPanel;
    wxBoxSizer *mainSizer;
    SecureTable *passwordTable;
    wxStatusBar *statusBar;

    // Таймер для проверки бездействия
    wxTimer *inactivityTimer;
    static const int ID_InactivityTimer = 10002;

    // Методы создания UI
    void createMenuBar();
    void createToolBar();
    void createMainPanel();
    void createStatusBar();

    // Методы для работы с состоянием
    void showLoginDialog();
    void lockApplication();
    void unlockApplication();

    // Обработчики
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
    void onTableItemSelected(wxListEvent &event);
    void onChangePassword(wxCommandEvent &event);

    // Обработчики состояния
    void onInactivityTimer(wxTimerEvent &event);

public:
    MainWindow(ConfigHander &cfg, Database &database);
    ~MainWindow();

    void loadSampleData();
    void updateStatusBar();
    void showFirstRunWizard();

    wxDECLARE_EVENT_TABLE();
};

// ID для событий
enum
{
    ID_NewDatabase = 1,
    ID_OpenDatabase,
    ID_Backup,
    ID_Exit,
    ID_AddEntry,
    ID_EditEntry,
    ID_DeleteEntry,
    ID_ViewLogs,
    ID_Settings,
    ID_About,
    ID_FirstRunWizard,
    ID_ChangePassword
};

#endif
