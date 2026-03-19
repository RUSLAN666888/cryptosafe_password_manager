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

wxBEGIN_EVENT_TABLE(MainWindow, wxFrame)
    // Меню File
    EVT_MENU(ID_NewDatabase, MainWindow::onNewDatabase)
        EVT_MENU(ID_OpenDatabase, MainWindow::onOpenDatabase)
            EVT_MENU(ID_Backup, MainWindow::onBackup)
                EVT_MENU(ID_Exit, MainWindow::onExit)

    // Меню Edit
    EVT_MENU(ID_AddEntry, MainWindow::onAddEntry)
        EVT_MENU(ID_EditEntry, MainWindow::onEditEntry)
            EVT_MENU(ID_DeleteEntry, MainWindow::onDeleteEntry)

    // Меню View
    EVT_MENU(ID_ViewLogs, MainWindow::onViewLogs)
        EVT_MENU(ID_Settings, MainWindow::onSettings)

    // Меню Help
    EVT_MENU(ID_About, MainWindow::onAbout)
        EVT_MENU(ID_FirstRunWizard, MainWindow::onFirstRunWizard)

    // Таблица
    EVT_LIST_ITEM_SELECTED(wxID_ANY, MainWindow::onTableItemSelected)

    // Кнопки
    EVT_BUTTON(ID_AddEntry, MainWindow::onAddEntry)
        EVT_BUTTON(ID_EditEntry, MainWindow::onEditEntry)
            EVT_BUTTON(ID_DeleteEntry, MainWindow::onDeleteEntry)

    // События окна
    EVT_ACTIVATE(MainWindow::onActivate)

    // Таймер
    EVT_TIMER(ID_InactivityTimer, MainWindow::onInactivityTimer)
        wxEND_EVENT_TABLE()

            MainWindow::MainWindow(ConfigHander &cfg, Database &database)
    : wxFrame(nullptr, wxID_ANY, "CryptoSafe Manager", wxDefaultPosition,
              wxSize(900, 600)),
      config(cfg), db(database), isLoggedIn(false), isFirstRun(false)
{
  createMenuBar();
  createToolBar();
  createMainPanel();
  createStatusBar();
  Center();

  // Загружаем тестовые данные
  loadSampleData();

  // Скрываем содержимое до логина
  passwordTable->Hide();

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

void MainWindow::createMenuBar()
{
  menuBar = new wxMenuBar();

  // File menu
  wxMenu *fileMenu = new wxMenu();
  fileMenu->Append(ID_NewDatabase, "&New Database\tCtrl+N",
                   "Create new database");
  fileMenu->Append(ID_OpenDatabase, "&Open Database\tCtrl+O",
                   "Open existing database");
  fileMenu->AppendSeparator();
  fileMenu->Append(ID_Backup, "&Backup...", "Create backup");
  fileMenu->AppendSeparator();
  fileMenu->Append(ID_Exit, "E&xit\tAlt+F4", "Exit application");

  // Edit menu
  wxMenu *editMenu = new wxMenu();
  editMenu->Append(ID_AddEntry, "&Add Entry\tCtrl+A", "Add new password entry");
  editMenu->Append(ID_EditEntry, "&Edit Entry\tCtrl+E", "Edit selected entry");
  editMenu->Append(ID_DeleteEntry, "&Delete Entry\tDel",
                   "Delete selected entry");

  // View menu
  wxMenu *viewMenu = new wxMenu();
  viewMenu->Append(ID_ViewLogs, "&Audit Logs", "View audit logs");
  viewMenu->AppendSeparator();
  viewMenu->Append(ID_Settings, "&Settings", "Application settings");

  // Help menu
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

    // Сохраняем данные аутентификации
    Argon2Data d = wizard.getAuthData();

    db.saveAuthData(d.hash, d.salt, d.time_cost, d.memory_cost_mb,
                    d.parallelism, d.hash_len);

    // Генерируем и сохраняем соль для PBKDF2
    std::vector<uint8_t> encSalt(16);
    randombytes_buf(encSalt.data(), encSalt.size());
    db.saveEncSalt(encSalt);
  }
}

void MainWindow::showLoginDialog()
{
  LoginDialog dialog(this, config, db);
  std::cout << "Wфцвцфв" << std::endl;

  if (dialog.ShowModal() == wxID_OK)
  {
    unlockApplication();
  }
  else
  {
    Close(true);
  }
}

void MainWindow::unlockApplication()
{
  isLoggedIn = true;
  passwordTable->Show();
  Layout();
  updateStatusBar();
}

void MainWindow::lockApplication()
{
  isLoggedIn = false;
  passwordTable->Hide();
  Layout();
  updateStatusBar();
}

void MainWindow::onActivate(wxActivateEvent &event)
{
  if (!event.GetActive())
  {
    // Окно потеряло активность (свернули или переключились)
    std::cout << "Window deactivated" << std::endl;
    KeyManager::getInstance().on_app_inactive();
    lockApplication();
    std::cout << "loseAllConnections" << std::endl;
  }
  else
  {
    // Окно стало активным
    std::cout << "Window activated" << std::endl;
    KeyManager::getInstance().on_app_active();

    KeyManager::KeyData keyData;
    KeyManager::getInstance().get_key(keyData);
    if (keyData.data == nullptr)
    {
      showLoginDialog();
    }
    else
    {
      unlockApplication();
    }
  }
  event.Skip();
}

void MainWindow::onInactivityTimer(wxTimerEvent &event)
{
  if (!isLoggedIn)
    return;

  // Обновляем активность
  KeyManager::getInstance().update_activity();

  // Проверяем, не удалил ли KeyManager ключ
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

// ============== ОБРАБОТЧИКИ МЕНЮ (ЗАГЛУШКИ) ==============

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
  // В Sprint 2 просто активируем кнопки
  if (isLoggedIn)
  {
    // Можно активировать кнопки Edit/Delete
  }
}