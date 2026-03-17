#include "../src/gui/MainWindow.h"
#include "../src/gui/dialogs/first_run_wizard/FirstRunWizard.h"
#include "../src/gui/dialogs/settings_dialog/SettingsDialog.h"
#include "../src/gui/widgets/audit_log_viewer/AuditLogViewer.h"
#include <wx/aboutdlg.h>
#include <wx/artprov.h>
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

    // Кнопки (просто активация, без функционала)
    EVT_BUTTON(ID_AddEntry, MainWindow::onAddEntry)
        EVT_BUTTON(ID_EditEntry, MainWindow::onEditEntry)
            EVT_BUTTON(ID_DeleteEntry,
                       MainWindow::onDeleteEntry) wxEND_EVENT_TABLE()

                MainWindow::MainWindow(ConfigHander &cfg, Database &database)
    : wxFrame(nullptr, wxID_ANY, "CryptoSafe Manager", wxDefaultPosition,
              wxSize(900, 600)),
      config(cfg), db(database), isLoggedIn(false)
{
  createMenuBar();
  createToolBar();
  createMainPanel();
  createStatusBar();
  Center();

  // Загружаем тестовые данные
  loadSampleData();

  // Проверяем первый запуск
  if (config.isFirstRun())
  {
    showFirstRunWizard();
  }

  updateStatusBar();
}

MainWindow::~MainWindow()
{
  // Ничего не нужно
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

  // Таблица с паролями (без панели поиска в Sprint 1)
  passwordTable = new SecureTable(mainPanel, wxID_ANY);
  mainSizer->Add(passwordTable, 1, wxEXPAND | wxALL, 5);

  mainPanel->SetSizer(mainSizer);
}

void MainWindow::createStatusBar()
{
  statusBar = CreateStatusBar(2); // Только 2 секции в Sprint 1

  int widths[] = {200, -1};
  statusBar->SetStatusWidths(2, widths);

  statusBar->SetStatusText("Not logged in", 0);
  statusBar->SetStatusText("Sprint 1 Foundation", 1);
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
  }
}

// ============== ОБРАБОТЧИКИ СОБЫТИЙ (ВСЕ ЗАГЛУШКИ) ==============

void MainWindow::onNewDatabase(wxCommandEvent &event)
{
  // Заглушка - будет в Sprint 2
}

void MainWindow::onOpenDatabase(wxCommandEvent &event)
{
  // Заглушка - будет в Sprint 2
}

void MainWindow::onBackup(wxCommandEvent &event)
{
  // Заглушка - будет в Sprint 8
}

void MainWindow::onExit(wxCommandEvent &event) { Close(true); }

void MainWindow::onAddEntry(wxCommandEvent &event)
{
  // Заглушка - будет в Sprint 3
}

void MainWindow::onEditEntry(wxCommandEvent &event)
{
  // Заглушка - будет в Sprint 3
}

void MainWindow::onDeleteEntry(wxCommandEvent &event)
{
  // Заглушка - будет в Sprint 3
}

void MainWindow::onViewLogs(wxCommandEvent &event)
{
  // Заглушка - будет в Sprint 5
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
  info.SetVersion("1.0 (Sprint 1)");
  info.SetDescription("Secure Password Manager\n\n"
                      "Sprint 1: Foundation with GUI shell");
  info.SetCopyright("(C) 2024");

  wxAboutBox(info, this);
}

void MainWindow::onFirstRunWizard(wxCommandEvent &event)
{
  showFirstRunWizard();
}

void MainWindow::onTableItemSelected(wxListEvent &event)
{
  // В Sprint 1 просто ничего не делаем
  // Кнопки Edit/Delete остаются неактивными
}