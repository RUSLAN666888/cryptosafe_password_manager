#include "MainWindow.h"
#include "../src/gui/dialogs/first_run_wizard/FirstRunWizard.h"
#include "../src/gui/dialogs/login_dialog/LoginDialog.h"
#include "../src/gui/dialogs/password_change/password_change.h"
#include "../src/gui/dialogs/settings_dialog/SettingsDialog.h"
#include "../src/core/state_manager.h"
#include "../src/gui/dialogs/entry_dialog/entry_dialog.h"
#include <QMessageBox>
#include <QApplication>
#include <QDebug>

MainWindow::MainWindow(ConfigHander &cfg, Database &database, VaultManager& vaultManager)
    : QMainWindow(nullptr)
    , config(cfg)
    , db(database)
    , m_vaultManager(vaultManager)
    , isLoggedIn(false)
{
    setWindowTitle("CryptoSafe Manager");
    resize(900, 600);

    createMenuBar();
    createToolBar();
    createCentralWidget();
    createStatusBar();

    // Создаем таймер бездействия
    inactivityTimer = new QTimer(this);
    inactivityTimer->setSingleShot(true);
    connect(inactivityTimer, &QTimer::timeout, this, &MainWindow::onInactivityTimeout);


    // Регистрируем обработчики событий
    registerEventHandlers();

    // Скрываем таблицу до логина
    passwordTable->hide();

    // Проверяем первый запуск
    if (config.isFirstRun())
    {
        if (!showFirstRunWizard())
        {
            QMetaObject::invokeMethod(this, &MainWindow::close, Qt::QueuedConnection);
            return;
        }
    }
    else
    {
        if (!showLoginDialog())
        {
            QMetaObject::invokeMethod(this, &MainWindow::close, Qt::QueuedConnection);
            return;
        }
    }

    updateStatusBar();
}

MainWindow::~MainWindow()
{
    if (isLoggedIn)
    {
        KeyManager::getInstance().logout();
    }
}

void MainWindow::registerEventHandlers()
{
    eventBus.subscribe(EventType::UserLoggedIn,
                       [this](const Event& event) { this->onUserLoggedIn(event); });

    eventBus.subscribe(EventType::UserLoggedOut,
                       [this](const Event& event) { this->onUserLoggedOut(event); });
}

void MainWindow::createMenuBar()
{
    menuBar = new QMenuBar(this);

    QMenu *fileMenu = menuBar->addMenu("&File");
    fileMenu->addAction("&New Database", this, &MainWindow::onNewDatabase, QKeySequence::New);
    fileMenu->addAction("&Open Database", this, &MainWindow::onOpenDatabase, QKeySequence::Open);
    fileMenu->addSeparator();
    fileMenu->addAction("&Backup...", this, &MainWindow::onBackup);
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", this, &MainWindow::onExit, QKeySequence::Quit);

    QMenu *editMenu = menuBar->addMenu("&Edit");
    editMenu->addAction("&Add Entry", this, &MainWindow::onAddEntry, QKeySequence("Ctrl+A"));
    editMenu->addAction("&Edit Entry", this, &MainWindow::onEditEntry, QKeySequence("Ctrl+E"));
    editMenu->addAction("&Delete Entry", this, &MainWindow::onDeleteEntry, QKeySequence::Delete);
    editMenu->addSeparator();
    editMenu->addAction("&Lock", this, &MainWindow::onLock, QKeySequence("Ctrl+L"));  // Добавить кнопку Lock
    editMenu->addSeparator();
    editMenu->addAction("&Change Master Password", this, &MainWindow::onChangePassword, QKeySequence("Ctrl+Shift+P"));

    QMenu *viewMenu = menuBar->addMenu("&View");
    viewMenu->addAction("&Audit Logs", this, &MainWindow::onViewLogs);
    viewMenu->addSeparator();
    viewMenu->addAction("&Settings", this, &MainWindow::onSettings);

    QMenu *helpMenu = menuBar->addMenu("&Help");
    helpMenu->addAction("Setup &Wizard", this, &MainWindow::onFirstRunWizard);
    helpMenu->addSeparator();
    helpMenu->addAction("&About", this, &MainWindow::onAbout);

    setMenuBar(menuBar);
}

void MainWindow::createToolBar()
{
    toolBar = addToolBar("Main");
    toolBar->addAction("Add", this, &MainWindow::onAddEntry);
    toolBar->addAction("Edit", this, &MainWindow::onEditEntry);
    toolBar->addAction("Delete", this, &MainWindow::onDeleteEntry);
    toolBar->addSeparator();
    toolBar->addAction("Backup", this, &MainWindow::onBackup);
    toolBar->addSeparator();
    toolBar->addAction("Settings", this, &MainWindow::onSettings);
}

void MainWindow::createCentralWidget()
{
    centralWidget = new QWidget(this);
    mainLayout = new QVBoxLayout(centralWidget);
    passwordTable = new SecureTable(centralWidget);
    mainLayout->addWidget(passwordTable);
    setCentralWidget(centralWidget);
}

void MainWindow::createStatusBar()
{
    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusBar->showMessage("Not logged in");
}

void MainWindow::updateStatusBar()
{
    QString loginStatus = isLoggedIn ? "Logged in" : "Not logged in";
    statusBar->showMessage(loginStatus);
}

bool MainWindow::showFirstRunWizard()
{
    FirstRunWizard wizard(this, config);
    if (wizard.exec() == QDialog::Accepted)
    {
        config.setFirstRun(false);
        Argon2Data d = wizard.getAuthData();
        db.saveAuthData(d.hash, d.salt, d.time_cost, d.memory_cost_mb,
                        d.parallelism, d.hash_len);
        db.saveEncSalt(wizard.getEncSalt());
        return showLoginDialog();
    }
    return false;
}

bool MainWindow::showLoginDialog()
{
    LoginDialog dialog(this, config, db);
    if (dialog.exec() == QDialog::Accepted)
    {
        unlockApplication();
        return true;
    }
    return false;
}

void MainWindow::unlockApplication()
{
    if (!isLoggedIn)
    {
        isLoggedIn = true;
        passwordTable->show();
        updateStatusBar();

        StateManager::getInstance().login();


        // Обновляем таблицу
        auto metadata = m_vaultManager.getAllEntryMetadata();
        passwordTable->clearAll();
        for (const auto& meta : metadata)
        {
            VaultEntry vaultEntry;
            vaultEntry.id = meta.id;
            vaultEntry.title = meta.title;
            vaultEntry.username = meta.username;
            vaultEntry.url = meta.url;
            vaultEntry.tags = meta.tags;
            vaultEntry.created_at = meta.created_at;
            vaultEntry.updated_at = meta.updated_at;
            passwordTable->addEntry(vaultEntry);
        }

        resetInactivityTimer();
    }
}

void MainWindow::lockApplication()
{
    if (isLoggedIn)
    {
        isLoggedIn = false;
        passwordTable->hide();
        updateStatusBar();

        StateManager::getInstance().logout();

        if (inactivityTimer->isActive())
        {
            inactivityTimer->stop();
        }

        struct EmptyData {};
        EmptyData emptyData;
        eventBus.publish(EventType::UserLoggedOut, emptyData, "MainWindow");
    }
}

void MainWindow::resetInactivityTimer()
{
    if (isLoggedIn)
    {
        StateManager::getInstance().updateActivity();
        KeyManager::getInstance().update_activity();
        inactivityTimer->start(INACTIVITY_TIMEOUT_MS);
    }
}

void MainWindow::onInactivityTimeout()
{
    if (isLoggedIn)
    {
        lockApplication();
        KeyManager::getInstance().logout();
        QMessageBox::information(this, "Auto-Lock", "Application locked due to inactivity.");
        showLoginDialog();
    }
}

// ============== ОБРАБОТЧИКИ МЕНЮ ==============

void MainWindow::onNewDatabase()
{
    QMessageBox::information(this, "Info", "New Database - will be implemented in Sprint 2");
}

void MainWindow::onOpenDatabase()
{
    QMessageBox::information(this, "Info", "Open Database - will be implemented in Sprint 2");
}

void MainWindow::onBackup()
{
    QMessageBox::information(this, "Info", "Backup - will be implemented in Sprint 8");
}

void MainWindow::onExit()
{
    if (isLoggedIn)
    {
        KeyManager::getInstance().logout();
    }
    close();
}

void MainWindow::onAddEntry()
{
    if (!isLoggedIn)
    {
        showLoginDialog();
        return;
    }
    resetInactivityTimer();

    if (!isLoggedIn)
    {
        showLoginDialog();
        return;
    }
    resetInactivityTimer();

    EntryDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted)
    {
        PlaintextEntry entry = dialog.getEntry();
        try {
            int id = m_vaultManager.createEntry(entry);
            if (id != -1) {
                // Обновляем таблицу
                auto metadata = m_vaultManager.getAllEntryMetadata();
                passwordTable->clearAll();
                for (const auto& meta : metadata) {
                    VaultEntry vaultEntry;
                    vaultEntry.id = meta.id;
                    vaultEntry.title = meta.title;
                    vaultEntry.username = meta.username;
                    vaultEntry.url = meta.url;
                    vaultEntry.tags = meta.tags;
                    vaultEntry.created_at = meta.created_at;
                    vaultEntry.updated_at = meta.updated_at;
                    passwordTable->addEntry(vaultEntry);
                }
                statusBar->showMessage("Запись успешно добавлена", 3000);
            } else {
                QMessageBox::warning(this, "Ошибка", "Не удалось добавить запись");
            }
        } catch (const std::exception& e) {
            QMessageBox::critical(this, "Ошибка",
                                  QString("Ошибка при добавлении записи: %1").arg(e.what()));
        }
    }
}

void MainWindow::onEditEntry()
{
    if (!isLoggedIn)
    {
        showLoginDialog();
        return;
    }
    resetInactivityTimer();
    long selected = passwordTable->getSelectedId();
    if (selected == -1)
    {
        QMessageBox::warning(this, "No Selection", "Please select an entry");
        return;
    }
    QMessageBox::information(this, "Info", QString("Edit Entry %1 - Sprint 3").arg(selected));
}

void MainWindow::onDeleteEntry()
{
    if (!isLoggedIn)
    {
        showLoginDialog();
        return;
    }
    resetInactivityTimer();
    long selected = passwordTable->getSelectedId();
    if (selected == -1)
    {
        QMessageBox::warning(this, "No Selection", "Please select an entry");
        return;
    }
    QMessageBox::information(this, "Info", QString("Delete Entry %1 - Sprint 3").arg(selected));
}

void MainWindow::onViewLogs()
{
    QMessageBox::information(this, "Info", "Audit Logs - will be implemented in Sprint 5");
}

void MainWindow::onSettings()
{
    SettingsDialog dialog(this, config);
    dialog.exec();
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "About CryptoSafe Manager",
                       "<h2>CryptoSafe Manager</h2>"
                       "<p>Version 2.0 (Sprint 2)</p>"
                       "<p>Secure Password Manager</p>"
                       "<p>Sprint 2: Authentication & Key Management</p>"
                       "<p>Copyright (C) 2024</p>");
}

void MainWindow::onFirstRunWizard()
{
    showFirstRunWizard();
}

void MainWindow::onChangePassword()
{
    if (!isLoggedIn)
    {
        QMessageBox::warning(this, "Not Logged In", "You must be logged in to change password");
        return;
    }
    resetInactivityTimer();

    ChangePasswordDialog dialog(this, db);
    if (dialog.exec() == QDialog::Accepted)
    {
        lockApplication();
        KeyManager::getInstance().logout();
        if (!showLoginDialog())
        {
            QMetaObject::invokeMethod(this, &MainWindow::close, Qt::QueuedConnection);
            return;
        }
    }
}

// ============== ОБРАБОТЧИКИ СОБЫТИЙ ==============

void MainWindow::onUserLoggedIn(const Event& event)
{
    QMetaObject::invokeMethod(this, [this]() {
        unlockApplication();
    }, Qt::QueuedConnection);
}

void MainWindow::onUserLoggedOut(const Event& event)
{
    QMetaObject::invokeMethod(this, [this]() {
        lockApplication();
    }, Qt::QueuedConnection);
}

void MainWindow::onLock()
{
    if (isLoggedIn)
    {
        qDebug() << "User manually locked application";
        lockApplication();
        KeyManager::getInstance().logout();

        if (!showLoginDialog())
        {
            QMetaObject::invokeMethod(this, &MainWindow::close, Qt::QueuedConnection);
            return;
        }
    }
}
