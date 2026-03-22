// MainWindow.cpp
#include "../src/gui/MainWindow.h"
#include "../src/core/crypto/authentication.h"
#include "../src/gui/dialogs/first_run_wizard/FirstRunWizard.h"
#include "../src/gui/dialogs/settings_dialog/SettingsDialog.h"
#include "../src/gui/widgets/audit_log_viewer/AuditLogViewer.h"
#include <wx/aboutdlg.h>
#include <wx/artprov.h>
#include <wx/icon.h>
#include <wx/msgdlg.h>
#include <../src/gui/dialogs/password_change/password_change.h>

wxBEGIN_EVENT_TABLE(MainWindow, wxFrame)
    EVT_MENU(ID_NewDatabase, MainWindow::onNewDatabase)
    EVT_MENU(ID_OpenDatabase, MainWindow::onOpenDatabase)
    EVT_MENU(ID_Backup, MainWindow::onBackup)
    EVT_MENU(ID_Exit, MainWindow::onExit)
    EVT_MENU(ID_AddEntry, MainWindow::onAddEntry)
    EVT_MENU(ID_EditEntry, MainWindow::onEditEntry)
    EVT_MENU(ID_DeleteEntry, MainWindow::onDeleteEntry)
    EVT_MENU(ID_ViewLogs, MainWindow::onViewLogs)
    EVT_MENU(ID_Settings, MainWindow::onSettings)
    EVT_MENU(ID_About, MainWindow::onAbout)
    EVT_MENU(ID_FirstRunWizard, MainWindow::onFirstRunWizard)
    EVT_LIST_ITEM_SELECTED(wxID_ANY, MainWindow::onTableItemSelected)
    EVT_BUTTON(ID_AddEntry, MainWindow::onAddEntry)
    EVT_BUTTON(ID_EditEntry, MainWindow::onEditEntry)
    EVT_BUTTON(ID_DeleteEntry, MainWindow::onDeleteEntry)
    EVT_TIMER(ID_InactivityTimer, MainWindow::onInactivityTimer)
    EVT_MENU(ID_ChangePassword, MainWindow::onChangePassword)
    //EVT_ACTIVATE(MainWindow::onActivate)
wxEND_EVENT_TABLE()

MainWindow::MainWindow(ConfigHander &cfg, Database &database)
    : wxFrame(nullptr, wxID_ANY, "CryptoSafe Manager", wxDefaultPosition, wxSize(900, 600)),
    config(cfg), db(database), isLoggedIn(false), isFirstRun(false),
    isShowingLoginDialog(false)
{
    createMenuBar();
    createToolBar();
    createMainPanel();
    createStatusBar();
    Center();

    // Регистрируем обработчики событий
    registerEventHandlers();

    // Скрываем содержимое до логина
    //passwordTable->Hide();

    // Проверяем первый запуск
    if (config.isFirstRun())
    {
        showFirstRunWizard();
    }
    else
    {
        // Показываем диалог входа
        showLoginDialog();
    }

    // Запускаем таймер проверки бездействия (каждую минуту)
    inactivityTimer = new wxTimer(this, ID_InactivityTimer);
    inactivityTimer->Start(60000);

    updateStatusBar();
}

MainWindow::~MainWindow()
{
    if (inactivityTimer)
    {
        inactivityTimer->Stop();
        delete inactivityTimer;
    }
}

void MainWindow::registerEventHandlers()
{
    // Подписываемся на события через EventBus
    eventBus.subscribe(EventType::UserLoggedIn,
                       [this](const Event& event) { this->onUserLoggedIn(event); });

    eventBus.subscribe(EventType::UserLoggedOut,
                       [this](const Event& event) { this->onUserLoggedOut(event); });
}

void MainWindow::createMenuBar()
{
    menuBar = new wxMenuBar();

    wxMenu *fileMenu = new wxMenu();
    fileMenu->Append(ID_NewDatabase, "&New Database\tCtrl+N", "Create new database");
    fileMenu->Append(ID_OpenDatabase, "&Open Database\tCtrl+O", "Open existing database");
    fileMenu->AppendSeparator();
    fileMenu->Append(ID_Backup, "&Backup...", "Create backup");
    fileMenu->AppendSeparator();
    fileMenu->Append(ID_Exit, "E&xit\tAlt+F4", "Exit application");

    wxMenu *editMenu = new wxMenu();
    editMenu->Append(ID_AddEntry, "&Add Entry\tCtrl+A", "Add new password entry");
    editMenu->Append(ID_EditEntry, "&Edit Entry\tCtrl+E", "Edit selected entry");
    editMenu->Append(ID_DeleteEntry, "&Delete Entry\tDel", "Delete selected entry");

    editMenu->AppendSeparator();

    editMenu->Append(ID_ChangePassword, "&Change Master Password\tCtrl+Shift+P",
                     "Change master password");


    wxMenu *viewMenu = new wxMenu();
    viewMenu->Append(ID_ViewLogs, "&Audit Logs", "View audit logs");
    viewMenu->AppendSeparator();
    viewMenu->Append(ID_Settings, "&Settings", "Application settings");

    wxMenu *helpMenu = new wxMenu();
    helpMenu->Append(ID_FirstRunWizard, "Setup &Wizard", "Run first-time setup");
    helpMenu->AppendSeparator();
    helpMenu->Append(ID_About, "&About", "About CryptoSafe Manager");

    menuBar->Append(fileMenu, "&File");
    menuBar->Append(editMenu, "&Edit");
    menuBar->Append(viewMenu, "&View");
    menuBar->Append(helpMenu, "&Help");

    SetMenuBar(menuBar);
}

void MainWindow::createToolBar()
{
    toolBar = CreateToolBar();

    toolBar->AddTool(ID_AddEntry, "Add",
                     wxArtProvider::GetBitmap(wxART_PLUS, wxART_TOOLBAR));
    toolBar->AddTool(ID_EditEntry, "Edit",
                     wxArtProvider::GetBitmap(wxART_EDIT, wxART_TOOLBAR));
    toolBar->AddTool(ID_DeleteEntry, "Delete",
                     wxArtProvider::GetBitmap(wxART_DELETE, wxART_TOOLBAR));
    toolBar->AddSeparator();
    toolBar->AddTool(ID_Backup, "Backup",
                     wxArtProvider::GetBitmap(wxART_FLOPPY, wxART_TOOLBAR));
    toolBar->AddSeparator();
    toolBar->AddTool(ID_Settings, "Settings",
                     wxArtProvider::GetBitmap(wxART_HELP, wxART_TOOLBAR));

    toolBar->Realize();
}

void MainWindow::createMainPanel()
{
    mainPanel = new wxPanel(this);
    mainSizer = new wxBoxSizer(wxVERTICAL);

    passwordTable = new SecureTable(mainPanel, wxID_ANY);
    mainSizer->Add(passwordTable, 1, wxEXPAND | wxALL, 5);

    mainPanel->SetSizer(mainSizer);
}

void MainWindow::createStatusBar()
{
    statusBar = CreateStatusBar(2);
    int widths[] = {200, -1};
    statusBar->SetStatusWidths(2, widths);
    statusBar->SetStatusText("Not logged in", 0);
    statusBar->SetStatusText("CryptoSafe Manager", 1);
}

void MainWindow::loadSampleData() { passwordTable->addSampleData(); }

void MainWindow::updateStatusBar()
{
    wxString loginStatus = isLoggedIn ? "Logged in" : "Not logged in";
    statusBar->SetStatusText(loginStatus, 0);
}

void MainWindow::showFirstRunWizard()
{
    FirstRunWizard wizard(this, config);
    if (wizard.RunWizard(wizard.GetFirstPage()))
    {
        config.setFirstRun(false);

        Argon2Data d = wizard.getAuthData();
        db.saveAuthData(d.hash, d.salt, d.time_cost, d.memory_cost_mb,
                        d.parallelism, d.hash_len);


        db.saveEncSalt(wizard.getEncSalt());
    }
}

void MainWindow::showLoginDialog()
{
    if (isShowingLoginDialog) {
        return;
    }

    isShowingLoginDialog = true;

    LoginDialog dialog(this, config, db);

    if (dialog.ShowModal() == wxID_OK)
    {
        unlockApplication();
    }
    else
    {
        Close(true);
    }

    isShowingLoginDialog = false;
}

void MainWindow::unlockApplication()
{
    if (!isLoggedIn)
    {
        isLoggedIn = true;
        passwordTable->Show();
        Layout();
        updateStatusBar();

        KeyManager::getInstance().update_activity();

        if (inactivityTimer)
        {
            inactivityTimer->Stop();
            inactivityTimer->Start(60000);
        }
    }
}

void MainWindow::lockApplication()
{
    if (isLoggedIn)
    {
        isLoggedIn = false;
        passwordTable->Hide();
        Layout();
        updateStatusBar();

        if (inactivityTimer)
        {
            inactivityTimer->Stop();
        }
    }
}

// void MainWindow::onActivate(wxActivateEvent& event)
// {
//     if (!event.GetActive()) {
//         // Окно потеряло активность (свернули или переключились)
//         KeyManager::getInstance().on_app_inactive();
//         lockApplication();
//     } else {
//         // Окно стало активным
//         KeyManager::getInstance().on_app_active();

//         KeyManager::KeyData keyData;
//         KeyManager::getInstance().get_key(keyData);
//         if (keyData.data == nullptr) {
//             showLoginDialog();
//         } else {
//             unlockApplication();
//         }
//     }
//     event.Skip();
// }

void MainWindow::onInactivityTimer(wxTimerEvent &event)
{
    if (!isLoggedIn)
        return;

    KeyManager::getInstance().update_activity();

    KeyManager::KeyData keyData;
    KeyManager::getInstance().get_key(keyData);

    if (keyData.data == nullptr && isLoggedIn)
    {
        lockApplication();
        wxMessageBox("Application locked due to inactivity", "Auto-Lock",
                     wxOK | wxICON_INFORMATION);
        showLoginDialog();
    }
}

// ============== ОБРАБОТЧИКИ МЕНЮ ==============

void MainWindow::onNewDatabase(wxCommandEvent &event)
{
    wxMessageBox("New Database - will be implemented in Sprint 2", "Info",
                 wxOK | wxICON_INFORMATION);
}

void MainWindow::onOpenDatabase(wxCommandEvent &event)
{
    wxMessageBox("Open Database - will be implemented in Sprint 2", "Info",
                 wxOK | wxICON_INFORMATION);
}

void MainWindow::onBackup(wxCommandEvent &event)
{
    wxMessageBox("Backup - will be implemented in Sprint 8", "Info",
                 wxOK | wxICON_INFORMATION);
}

void MainWindow::onExit(wxCommandEvent &event)
{
    KeyManager::getInstance().logout();
    Close(true);
}

void MainWindow::onAddEntry(wxCommandEvent &event)
{
    wxMessageBox("Add Entry - will be implemented in Sprint 3", "Info",
                 wxOK | wxICON_INFORMATION);
}

void MainWindow::onEditEntry(wxCommandEvent &event)
{
    if (!isLoggedIn)
    {
        showLoginDialog();
        return;
    }

    long selected = passwordTable->getSelectedId();
    if (selected == -1)
    {
        wxMessageBox("Please select an entry", "No Selection",
                     wxOK | wxICON_WARNING);
        return;
    }

    wxMessageBox(wxString::Format("Edit Entry %ld - Sprint 3", selected), "Info",
                 wxOK | wxICON_INFORMATION);
}

void MainWindow::onDeleteEntry(wxCommandEvent &event)
{
    if (!isLoggedIn)
    {
        showLoginDialog();
        return;
    }

    long selected = passwordTable->getSelectedId();
    if (selected == -1)
    {
        wxMessageBox("Please select an entry", "No Selection",
                     wxOK | wxICON_WARNING);
        return;
    }

    wxMessageBox(wxString::Format("Delete Entry %ld - Sprint 3", selected),
                 "Info", wxOK | wxICON_INFORMATION);
}

void MainWindow::onViewLogs(wxCommandEvent &event)
{
    wxMessageBox("Audit Logs - will be implemented in Sprint 5", "Info",
                 wxOK | wxICON_INFORMATION);
}

void MainWindow::onSettings(wxCommandEvent &event)
{
    SettingsDialog dialog(this, config);
    dialog.ShowModal();
}

void MainWindow::onAbout(wxCommandEvent &event)
{
    wxAboutDialogInfo info;
    info.SetName("CryptoSafe Manager");
    info.SetVersion("2.0 (Sprint 2)");
    info.SetDescription("Secure Password Manager\n\n"
                        "Sprint 2: Authentication & Key Management");
    info.SetCopyright("(C) 2024");
    wxAboutBox(info, this);
}

void MainWindow::onFirstRunWizard(wxCommandEvent &event)
{
    showFirstRunWizard();
}

void MainWindow::onTableItemSelected(wxListEvent &event)
{
    if (isLoggedIn)
    {
        // Можно активировать кнопки Edit/Delete
    }
}

void MainWindow::onChangePassword(wxCommandEvent &event)
{
    // Проверяем, залогинен ли пользователь
    if (!isLoggedIn)
    {
        wxMessageBox("You must be logged in to change password",
                     "Not Logged In", wxOK | wxICON_WARNING);
        return;
    }

    // Создаем и показываем диалог смены пароля
    ChangePasswordDialog dialog(this, db);

    if (dialog.ShowModal() == wxID_OK)
    {
        // Пароль успешно изменен
        // Блокируем приложение и выходим
        lockApplication();
        KeyManager::getInstance().logout();

        // Показываем сообщение
        wxMessageBox("Password changed successfully!\n\n"
                     "Please log in again with your new password.",
                     "Success", wxOK | wxICON_INFORMATION, this);

        // Показываем диалог логина
        showLoginDialog();
    }
}

void MainWindow::onUserLoggedIn(const Event& event)
{
    std::cout << "User logged in event received" << std::endl;

    // Вызываем в главном потоке, так как EventBus может вызывать из любого потока
    wxTheApp->CallAfter([this]() {
        unlockApplication();
    });
}

void MainWindow::onUserLoggedOut(const Event& event)
{
    std::cout << "User logged out event received" << std::endl;

    wxTheApp->CallAfter([this]() {
        lockApplication();
    });
}
