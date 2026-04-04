#include "MainWindow.h"
#include "../src/gui/dialogs/first_run_wizard/FirstRunWizard.h"
#include "../src/gui/dialogs/login_dialog/LoginDialog.h"
#include "../src/gui/dialogs/password_change/password_change.h"
#include "../src/gui/dialogs/settings_dialog/SettingsDialog.h"
#include "../src/core/state_manager.h"
#include "../src/gui/dialogs/entry_dialog/entry_dialog.h"
#include "../src/gui/widgets/secure_table/VaultTableModel.h"
#include "../src/gui/widgets/secure_table/PasswordDelegate.h"
#include "../src/core/vault/VaultManager.h"
#include <QMessageBox>
#include <QApplication>
#include <QDebug>
#include <QHeaderView>
#include <QShortcut>
#include <QClipboard>
//#include <random>
#include <unordered_set>

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

    // ПРОВЕРКА НА ПРОИЗОДИТЕЛЬНОСТЬ (ПРОСТО ДОБАВЛЯЕМ ЗАПИСИ В БД)
    // int count = 1000;
    // std::cout << "Generating " << count << " test entries..." << std::endl;

    // // Генератор случайных чисел
    // auto getRandomInt = [](int min, int max) -> int {
    //     static std::random_device rd;
    //     static std::mt19937 gen(rd());
    //     std::uniform_int_distribution<> dis(min, max);
    //     return dis(gen);
    // };

    // auto getRandomString = [&](int minLen, int maxLen) -> std::string {
    //     const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    //     int len = getRandomInt(minLen, maxLen);
    //     std::string result;
    //     for (int i = 0; i < len; ++i) {
    //         result += chars[getRandomInt(0, chars.length() - 1)];
    //     }
    //     return result;
    // };

    // auto getRandomDomain = [&]() -> std::string {
    //     std::vector<std::string> domains = {
    //         "gmail.com", "yahoo.com", "outlook.com", "hotmail.com",
    //         "github.com", "gitlab.com", "bitbucket.org",
    //         "facebook.com", "twitter.com", "linkedin.com",
    //         "amazon.com", "google.com", "microsoft.com",
    //         "apple.com", "netflix.com", "spotify.com",
    //         "work.com", "personal.com", "bank.com", "shopping.com"
    //     };
    //     return domains[getRandomInt(0, domains.size() - 1)];
    // };

    // auto getRandomTitle = [&]() -> std::string {
    //     std::vector<std::string> titles = {
    //         "GitHub", "Gmail", "Facebook", "Twitter", "LinkedIn",
    //         "Amazon", "Netflix", "Spotify", "Google", "Microsoft",
    //         "Apple", "Bank Account", "Work Email", "Personal Email",
    //         "VPN", "Server", "Database", "Admin Panel", "WiFi", "Router"
    //     };
    //     return titles[getRandomInt(0, titles.size() - 1)] + " " + std::to_string(getRandomInt(1, 1000));
    // };

    // auto getRandomPassword = [&]() -> std::string {
    //     const std::string upper = "ABCDEFGHJKLMNPQRSTUVWXYZ";
    //     const std::string lower = "abcdefghijkmnopqrstuvwxyz";
    //     const std::string digits = "23456789";
    //     const std::string symbols = "!@#$%^&*";
    //     std::string all = upper + lower + digits + symbols;

    //     int length = getRandomInt(12, 24);
    //     std::string password;

    //     password += upper[getRandomInt(0, upper.length() - 1)];
    //     password += lower[getRandomInt(0, lower.length() - 1)];
    //     password += digits[getRandomInt(0, digits.length() - 1)];
    //     password += symbols[getRandomInt(0, symbols.length() - 1)];

    //     for (int i = password.length(); i < length; ++i) {
    //         password += all[getRandomInt(0, all.length() - 1)];
    //     }

    //     for (int i = password.length() - 1; i > 0; --i) {
    //         int j = getRandomInt(0, i);
    //         std::swap(password[i], password[j]);
    //     }

    //     return password;
    // };

    // auto getRandomCategory = [&]() -> std::string {
    //     std::vector<std::string> categories = {"Work", "Personal", "Finance", "Social", "Development", "Entertainment", "Shopping", "Travel"};
    //     return categories[getRandomInt(0, categories.size() - 1)];
    // };

    // auto getRandomTags = [&]() -> std::string {
    //     std::vector<std::string> allTags = {"work", "personal", "dev", "finance", "social", "important", "backup", "frequent"};
    //     int tagCount = getRandomInt(1, 3);
    //     std::string tags;
    //     for (int i = 0; i < tagCount; ++i) {
    //         if (i > 0) tags += ",";
    //         tags += allTags[getRandomInt(0, allTags.size() - 1)];
    //     }
    //     return tags;
    // };

    // auto getRandomNotes = [&]() -> std::string {
    //     std::vector<std::string> notes = {
    //         "Important account",
    //         "Used for 2FA",
    //         "Shared with team",
    //         "Regularly updated",
    //         "Backup codes stored",
    //         "Recovery email set",
    //         "This is a test note for performance testing",
    //         "Additional information about this account",
    //         "Password last changed recently"
    //     };
    //     return getRandomInt(0, 5) == 0 ? notes[getRandomInt(0, notes.size() - 1)] : "";
    // };

    // // Получаем текущее время
    // auto now = std::chrono::system_clock::now();
    // auto now_time_t = std::chrono::system_clock::to_time_t(now);
    // std::string currentTimestamp = std::ctime(&now_time_t);
    // if (!currentTimestamp.empty() && currentTimestamp.back() == '\n') {
    //     currentTimestamp.pop_back();
    // }

    // // Генерируем записи
    // for (int i = 0; i < count; ++i) {
    //     PlaintextEntry entry;
    //     entry.title = getRandomTitle();
    //     entry.username = "user_" + getRandomString(5, 12) + "@" + getRandomDomain();
    //     entry.password = getRandomPassword();
    //     entry.url = "https://" + getRandomDomain();
    //     entry.notes = getRandomNotes();
    //     entry.category = getRandomCategory();
    //     entry.tags = getRandomTags();
    //     entry.creation_timestamp = currentTimestamp;
    //     entry.version = 1;

    //     m_vaultManager.createEntry(entry);

    //     if ((i + 1) % 100 == 0) {
    //         std::cout << "  Generated " << (i + 1) << "/" << count << " entries..." << std::endl;
    //     }
    // }

    // std::cout << "Done! Generated " << count << " test entries." << std::endl;
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

    toolBar->addSeparator();

    // Кнопка для переключения видимости паролей
    QAction* togglePasswordsAction = toolBar->addAction("👁");
    togglePasswordsAction->setToolTip("Показать/скрыть пароли (Ctrl+Shift+P)");
    togglePasswordsAction->setCheckable(true);
    connect(togglePasswordsAction, &QAction::toggled, this, [this](bool checked) {
        m_tableModel->setPasswordsVisible(checked);
    });

    // Горячая клавиша Ctrl+Shift+P
    QShortcut* shortcut = new QShortcut(QKeySequence("Ctrl+Shift+P"), this);
    connect(shortcut, &QShortcut::activated, togglePasswordsAction, &QAction::toggle);



    toolBar->addSeparator();

    QAction* testEncryptAction = toolBar->addAction("Test Encrypt");
    connect(testEncryptAction, &QAction::triggered, this, &MainWindow::runEncryptionTest);

    QAction* testCrudAction = toolBar->addAction("Test CRUD");
    connect(testCrudAction, &QAction::triggered, this, &MainWindow::runCrudTest);

    QAction* testPasswordGenAction = toolBar->addAction("Test Password");
    connect(testPasswordGenAction, &QAction::triggered, this, &MainWindow::runPasswordGeneratorTest);
}

void MainWindow::createCentralWidget()
{
    centralWidget = new QWidget(this);
    mainLayout = new QVBoxLayout(centralWidget);

    // Панель поиска
    QHBoxLayout* searchLayout = new QHBoxLayout();
    m_searchField = new QLineEdit(this);
    m_searchField->setPlaceholderText("Поиск...");
    m_searchField->setFixedWidth(250);
    searchLayout->addStretch();
    searchLayout->addWidget(m_searchField);
    mainLayout->addLayout(searchLayout);

    // Создаем модель и прокси
    m_tableModel = new VaultTableModel(m_vaultManager, this);
    m_proxyModel = new SearchProxyModel(this);
    m_proxyModel->setSourceModel(m_tableModel);
    m_proxyModel->setFilterKeyColumn(-1);  // поиск по всем колонкам

    // Создаем таблицу
    m_tableView = new QTableView(this);
    m_tableView->setModel(m_proxyModel);

    // Настройки таблицы
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tableView->setSortingEnabled(true);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->horizontalHeader()->setSectionsMovable(true);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    // Настройка ширины колонок
    m_tableView->setColumnWidth(0, 200);  // Название
    m_tableView->setColumnWidth(1, 150);  // Логин
    m_tableView->setColumnWidth(2, 200);  // Сайт
    m_tableView->setColumnWidth(3, 100);  // Дата

    PasswordDelegate* passwordDelegate = new PasswordDelegate(this);
    m_tableView->setItemDelegateForColumn(VaultTableModel::COL_PASSWORD, passwordDelegate);

    // Подключаем сигнал от делегата к модели
    connect(passwordDelegate, &PasswordDelegate::togglePasswordVisibility,
            this, [this](const QModelIndex& proxyIndex) {
                // Преобразуем индекс из прокси в исходную модель
                QModelIndex sourceIndex = m_proxyModel->mapToSource(proxyIndex);
                if (sourceIndex.isValid()) {
                    m_tableModel->togglePasswordVisibilityForRow(sourceIndex.row());
                }
            });

    mainLayout->addWidget(m_tableView);
    setCentralWidget(centralWidget);

    // Подключаем поиск
    connect(m_searchField, &QLineEdit::textChanged, this, [this](const QString& text) {
        m_proxyModel->setSearchText(text);
    });

    // Подключаем контекстное меню
    m_tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tableView, &QTableView::customContextMenuRequested,
            this, &MainWindow::showContextMenu);
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
        refreshTable();

        refreshTable();
        m_tableView->show();


        updateStatusBar();

        StateManager::getInstance().login();

        resetInactivityTimer();
    }
}

void MainWindow::lockApplication()
{
    if (isLoggedIn)
    {
        isLoggedIn = false;
        m_tableView->hide();
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

    EntryDialog dialog(db, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        PlaintextEntry entry = dialog.getEntry();
        try {
            int id = m_vaultManager.createEntry(entry);
            if (id != -1) {
                refreshTable();  // обновляем таблицу
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
    // олучаем ID выбранной записи
    QModelIndexList selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    QModelIndex sourceIdx = m_proxyModel->mapToSource(selected.first());
    long entryId = m_tableModel->getId(sourceIdx.row());

    // Загружаем текущую запись из БД
    auto entry = m_vaultManager.getEntry(static_cast<int>(entryId));
    if (!entry) {
        QMessageBox::warning(this, "Ошибка", "Запись не найдена");
        return;
    }

    // Открываем диалог с данными записи
    EntryDialog dialog(db, *entry, this);  // конструктор для редактирования
    if (dialog.exec() == QDialog::Accepted)
    {
        PlaintextEntry updatedEntry = dialog.getEntry();

        // Сохраняем оригинальные поля, которые не меняются
        updatedEntry.creation_timestamp = entry->creation_timestamp;

        // Обновляем в БД
        if (m_vaultManager.updateEntry(static_cast<int>(entryId), updatedEntry)) {
            // Обновляем модель (таблицу)
            m_tableModel->refresh();

            // Если пароль был в кэше - обновляем его
            m_tableModel->updatePasswordInCache(entryId, updatedEntry.password);
        }
    }
}

void MainWindow::onDeleteEntry()
{
    if (!isLoggedIn)
    {
        showLoginDialog();
        return;
    }
    resetInactivityTimer();

    // Получаем выделенные строки
    QModelIndexList selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty())
    {
        QMessageBox::warning(this, "Нет выбора", "Пожалуйста, выберите запись для удаления");
        return;
    }

    // Собираем ID выбранных записей
    QList<long> ids;
    for (const QModelIndex& idx : selected) {
        QModelIndex sourceIdx = m_proxyModel->mapToSource(idx);
        long id = m_tableModel->getId(sourceIdx.row());
        if (id != -1) {
            ids.append(id);
        }
    }

    if (ids.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Не удалось получить ID записей");
        return;
    }

    // Подтверждение удаления
    QString message = ids.size() == 1
                          ? "Вы уверены, что хотите удалить эту запись?"
                          : QString("Вы уверены, что хотите удалить %1 записей?").arg(ids.size());

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Подтверждение удаления", message,
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply == QMessageBox::Yes)
    {
        bool allSuccess = true;
        int deletedCount = 0;

        for (long id : ids) {
            if (m_vaultManager.deleteEntry(static_cast<int>(id))) {
                deletedCount++;
            } else {
                allSuccess = false;
            }
        }

        if (allSuccess) {
            // Обновляем таблицу
            m_tableModel->refresh();
            statusBar->showMessage(QString("Удалено %1 записей").arg(deletedCount), 3000);
        } else {
            QMessageBox::warning(this, "Ошибка",
                                 QString("Удалено %1 из %2 записей. Некоторые записи не удалось удалить.")
                                     .arg(deletedCount).arg(ids.size()));
            m_tableModel->refresh();  // все равно обновляем, чтобы показать актуальное состояние
        }
    }
}

void MainWindow::onViewLogs()
{
    QMessageBox::information(this, "Info", "Audit Logs - will be implemented in Sprint 5");
}

void MainWindow::onSettings()
{
    SettingsDialog dialog(db, this, config);
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

void MainWindow::refreshTable()
{
    m_tableModel->refresh();
}

// ============== КОНТЕКСТНОЕ МЕНЮ ==============

void MainWindow::showContextMenu(const QPoint& pos)
{
    QModelIndex index = m_tableView->indexAt(pos);
    if (!index.isValid()) return;

    // Выделяем строку, если она не выделена
    if (!m_tableView->selectionModel()->isSelected(index)) {
        m_tableView->selectionModel()->clear();
        m_tableView->selectionModel()->select(index,
                                              QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }

    QModelIndexList selected = m_tableView->selectionModel()->selectedRows();
    bool singleSelected = (selected.size() == 1);
    bool multiSelected = (selected.size() > 1);

    QMenu menu(this);

    if (singleSelected) {
        // Действия для одной записи
        menu.addAction("Редактировать", this, &MainWindow::onEditEntry);
        menu.addSeparator();
        menu.addAction("Копировать логин", this, &MainWindow::onCopyUsername);
        menu.addAction("Копировать пароль", this, &MainWindow::onCopyPassword);
        menu.addSeparator();
        menu.addAction("Удалить", this, &MainWindow::onDeleteEntry);
    } else if (multiSelected) {
        // Действия для нескольких записей
        menu.addAction(QString("Удалить (%1 записей)").arg(selected.size()),
                       this, &MainWindow::onDeleteEntry);
    }

    menu.exec(m_tableView->viewport()->mapToGlobal(pos));
}

void MainWindow::onCopyUsername()
{
    QModelIndexList selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    QModelIndex sourceIdx = m_proxyModel->mapToSource(selected.first());
    long id = m_tableModel->getId(sourceIdx.row());

    try {
        auto entry = m_vaultManager.getEntry(static_cast<int>(id));
        if (entry) {
            QApplication::clipboard()->setText(QString::fromStdString(entry->username));
            statusBar->showMessage("Логин скопирован", 2000);
        }
    } catch (const std::exception& e) {
        statusBar->showMessage("Ошибка при копировании", 2000);
    }
}

void MainWindow::onCopyPassword()
{
    QModelIndexList selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    QModelIndex sourceIdx = m_proxyModel->mapToSource(selected.first());
    long id = m_tableModel->getId(sourceIdx.row());

    try {
        auto entry = m_vaultManager.getEntry(static_cast<int>(id));
        if (entry) {
            QApplication::clipboard()->setText(QString::fromStdString(entry->password));
            statusBar->showMessage("Пароль скопирован", 2000);

            // TODO: Запланировать очистку буфера через 30 секунд (Sprint 4)
        }
    } catch (const std::exception& e) {
        statusBar->showMessage("Ошибка при копировании", 2000);
    }
}

void MainWindow::runEncryptionTest()
{
    std::cout << "\n========== ENCRYPTION ROUND-TRIP TEST ==========" << std::endl;

    // 1. Создаем запись с известными данными
    PlaintextEntry original;
    original.title = "Test Entry";
    original.username = "testuser@example.com";
    original.password = "MySecretPassword123!";
    original.url = "https://test.com";
    original.notes = "Test notes for encryption";
    original.category = "Test";
    original.tags = "test,encryption";
    original.creation_timestamp = "2024-01-01 12:00:00";
    original.version = 1;

    std::cout << "1. Original entry created:" << std::endl;
    std::cout << "   Title: " << original.title << std::endl;
    std::cout << "   Password: " << original.password << std::endl;

    // 2. Шифруем
    KeyManager::KeyData key;
    KeyManager::getInstance().get_key(key);
    auto encrypted = m_crypto.encrypt(key, original);

    std::cout << "2. Encrypted BLOB size: " << encrypted.size() << " bytes" << std::endl;

    // 3. Проверяем, что зашифрованные данные не содержат открытый текст
    std::string encryptedStr(encrypted.begin(), encrypted.end());
    bool isPlaintextVisible = encryptedStr.find(original.password) != std::string::npos;

    if (!isPlaintextVisible) {
        std::cout << "   Encrypted BLOB does NOT contain plaintext password" << std::endl;
    } else {
        std::cout << "   FAILED: Plaintext password found in encrypted BLOB!" << std::endl;
    }

    // 4. Расшифровываем
    auto decrypted = m_crypto.decrypt(encrypted, key);

    std::cout << "3. Decrypted entry:" << std::endl;
    std::cout << "   Title: " << decrypted.title << std::endl;
    std::cout << "   Password: " << decrypted.password << std::endl;

    // 5. Проверяем целостность
    bool integrityOk = (original.title == decrypted.title &&
                        original.username == decrypted.username &&
                        original.password == decrypted.password &&
                        original.url == decrypted.url &&
                        original.notes == decrypted.notes);

    if (integrityOk) {
        std::cout << "   Data integrity verified" << std::endl;
    } else {
        std::cout << "   FAILED: Data mismatch!" << std::endl;
    }

    std::cout << "========== TEST COMPLETE ==========\n" << std::endl;
}

void MainWindow::runCrudTest()
{
    std::cout << "\n========== CRUD INTEGRATION TEST ==========" << std::endl;

    // 1. Создаем 100 записей
    std::vector<int> ids;
    std::cout << "1. Creating 100 entries..." << std::endl;

    for (int i = 0; i < 100; ++i) {
        PlaintextEntry entry;
        entry.title = "Test Entry " + std::to_string(i);
        entry.username = "user" + std::to_string(i) + "@test.com";
        entry.password = "Password" + std::to_string(i) + "!";
        entry.url = "https://test" + std::to_string(i) + ".com";
        entry.notes = "Test notes for entry " + std::to_string(i);
        entry.category = (i % 2 == 0) ? "Even" : "Odd";
        entry.tags = "test";
        entry.creation_timestamp = "";
        entry.version = 1;

        int id = m_vaultManager.createEntry(entry);
        if (id != -1) {
            ids.push_back(id);
        }
    }

    std::cout << "   Created " << ids.size() << " entries" << std::endl;

    // 2. Проверяем количество
    auto allEntries = m_vaultManager.getAllEntryMetadata();
    std::cout << "2. Total entries in DB: " << allEntries.size() << std::endl;

    if (allEntries.size() == ids.size()) {
        std::cout << "   Count matches" << std::endl;
    } else {
        std::cout << "   Count mismatch! Expected " << ids.size()
                  << ", got " << allEntries.size() << std::endl;
    }

    // 3. Обновляем каждую вторую запись
    std::cout << "3. Updating 50 entries..." << std::endl;
    int updatedCount = 0;

    for (size_t i = 0; i < ids.size(); i += 2) {
        auto entry = m_vaultManager.getEntry(ids[i]);
        if (entry) {
            entry->title = "Updated Entry " + std::to_string(ids[i]);
            entry->notes = "Updated notes";
            if (m_vaultManager.updateEntry(ids[i], *entry)) {
                updatedCount++;
            }
        }
    }

    std::cout << "   Updated " << updatedCount << " entries" << std::endl;

    // 4. Проверяем обновленные данные
    std::cout << "4. Verifying updated entries..." << std::endl;
    int verifiedCount = 0;

    for (size_t i = 0; i < ids.size(); i += 2) {
        auto entry = m_vaultManager.getEntry(ids[i]);
        if (entry && entry->title == "Updated Entry " + std::to_string(ids[i])) {
            verifiedCount++;
        }
    }

    if (verifiedCount == updatedCount) {
        std::cout << "   All updates verified" << std::endl;
    } else {
        std::cout << "   Some updates failed verification" << std::endl;
    }

    // 5. Удаляем 30 записей
    std::cout << "5. Deleting 30 entries..." << std::endl;
    int deletedCount = 0;

    for (size_t i = 0; i < 30; ++i) {
        if (m_vaultManager.deleteEntry(ids[i])) {
            deletedCount++;
        }
    }

    std::cout << "   Deleted " << deletedCount << " entries" << std::endl;

    // 6. Проверяем финальное количество
    auto finalEntries = m_vaultManager.getAllEntryMetadata();
    int expectedCount = ids.size() - deletedCount;

    std::cout << "6. Final entries in DB: " << finalEntries.size() << std::endl;

    if (finalEntries.size() == expectedCount) {
        std::cout << "   Final count matches expected (" << expectedCount << ")" << std::endl;
    } else {
        std::cout << "   Count mismatch! Expected " << expectedCount
                  << ", got " << finalEntries.size() << std::endl;
    }

    std::cout << "========== TEST COMPLETE ==========\n" << std::endl;
}

void MainWindow::runPasswordGeneratorTest()
{
    std::cout << "\n========== PASSWORD GENERATOR TEST ==========" << std::endl;

    // Параметры теста
    const int TEST_COUNT = 10000;
    const int PASSWORD_LENGTH = 16;
    const bool USE_UPPERCASE = true;
    const bool USE_LOWERCASE = true;
    const bool USE_DIGITS = true;
    const bool USE_SYMBOLS = true;
    const bool EXCLUDE_AMBIGUOUS = true;

    std::cout << "Generating " << TEST_COUNT << " passwords..." << std::endl;
    std::cout << "Settings: length=" << PASSWORD_LENGTH
              << ", uppercase=" << USE_UPPERCASE
              << ", lowercase=" << USE_LOWERCASE
              << ", digits=" << USE_DIGITS
              << ", symbols=" << USE_SYMBOLS
              << ", excludeAmbiguous=" << EXCLUDE_AMBIGUOUS << std::endl;

    // Наборы символов (как в generateSecurePassword)
    QString uppercase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    QString lowercase = "abcdefghijklmnopqrstuvwxyz";
    QString digits = "0123456789";
    QString symbols = "!@#$%^&*";

    if (EXCLUDE_AMBIGUOUS) {
        uppercase.remove('I');
        uppercase.remove('O');
        lowercase.remove('l');
        digits.remove('0');
        digits.remove('1');
    }

    // Собираем выбранные наборы
    QString allChars;
    if (USE_UPPERCASE) allChars += uppercase;
    if (USE_LOWERCASE) allChars += lowercase;
    if (USE_DIGITS) allChars += digits;
    if (USE_SYMBOLS) allChars += symbols;

    if (allChars.isEmpty()) {
        allChars = lowercase;
    }

    // Криптостойкий генератор
    auto getRandomInt = [](int max) -> int {
        unsigned int value;
        if (RAND_bytes(reinterpret_cast<unsigned char*>(&value), sizeof(value)) != 1) {
            throw std::runtime_error("Failed to generate random number");
        }
        return value % max;
    };

    // Функция генерации пароля (копия из generateSecurePassword)
    auto generatePassword = [&]() -> QString {
        int actualLength = std::clamp(PASSWORD_LENGTH, 8, 64);

        QString password;

        if (USE_UPPERCASE && !uppercase.isEmpty()) {
            password += uppercase[getRandomInt(uppercase.length())];
        }
        if (USE_LOWERCASE && !lowercase.isEmpty()) {
            password += lowercase[getRandomInt(lowercase.length())];
        }
        if (USE_DIGITS && !digits.isEmpty()) {
            password += digits[getRandomInt(digits.length())];
        }
        if (USE_SYMBOLS && !symbols.isEmpty()) {
            password += symbols[getRandomInt(symbols.length())];
        }

        int remaining = actualLength - password.length();
        for (int i = 0; i < remaining; ++i) {
            password += allChars[getRandomInt(allChars.length())];
        }

        for (int i = password.length() - 1; i > 0; --i) {
            int j = getRandomInt(i + 1);
            if (i != j) {
                QChar temp = password[i];
                password[i] = password[j];
                password[j] = temp;
            }
        }

        return password;
    };

    // Хранилище для проверки дубликатов
    std::unordered_set<QString> passwords;
    std::unordered_map<QString, int> duplicates;

    // Статистика по символам
    int hasUppercase = 0;
    int hasLowercase = 0;
    int hasDigits = 0;
    int hasSymbols = 0;

    // Статистика по длине
    int lengthCount[65] = {0};  // 8-64

    // Статистика по силе пароля
    int strengthCount[5] = {0};  // 0-4

    std::cout << "\nGenerating passwords..." << std::endl;

    for (int i = 0; i < TEST_COUNT; ++i) {
        QString password = generatePassword();

        // Проверка длины
        int len = password.length();
        if (len >= 8 && len <= 64) {
            lengthCount[len]++;
        }

        // Проверка дубликатов
        if (passwords.find(password) != passwords.end()) {
            duplicates[password]++;
        } else {
            passwords.insert(password);
        }

        // Проверка набора символов
        bool hasUpper = false, hasLower = false, hasDigit = false, hasSymbol = false;
        for (QChar ch : password) {
            if (ch.isUpper()) hasUpper = true;
            else if (ch.isLower()) hasLower = true;
            else if (ch.isDigit()) hasDigit = true;
            else if (QString("!@#$%^&*").contains(ch)) hasSymbol = true;
        }

        if (hasUpper) hasUppercase++;
        if (hasLower) hasLowercase++;
        if (hasDigit) hasDigits++;
        if (hasSymbol) hasSymbols++;

        // Оценка силы пароля
        int strength = 0;
        if (len >= 12) strength++;
        if (hasUpper) strength++;
        if (hasLower) strength++;
        if (hasDigit) strength++;
        if (hasSymbol) strength++;
        strengthCount[std::min(strength, 4)]++;

        if ((i + 1) % 1000 == 0) {
            std::cout << "  Generated " << (i + 1) << "/" << TEST_COUNT << " passwords..." << std::endl;
        }
    }

    // ========== ВЫВОД РЕЗУЛЬТАТОВ ==========

    std::cout << "\n========== TEST RESULTS ==========" << std::endl;

    // 1. Проверка дубликатов
    std::cout << "\n1. DUPLICATES CHECK:" << std::endl;
    int duplicateCount = 0;
    for (const auto& pair : duplicates) {
        duplicateCount += pair.second;
    }

    if (duplicateCount == 0) {
        std::cout << "  No duplicates found (" << TEST_COUNT << " unique passwords)" << std::endl;
    } else {
        std::cout << "  Found " << duplicateCount << " duplicates!" << std::endl;
        for (const auto& pair : duplicates) {
            std::cout << "      Password '" << pair.first.toStdString()
                      << "' appears " << (pair.second + 1) << " times" << std::endl;
        }
    }

    // 2. Проверка соответствия наборам символов
    std::cout << "\n2. CHARACTER SET COMPLIANCE:" << std::endl;

    int expectedWithUpper = USE_UPPERCASE ? TEST_COUNT : 0;
    int expectedWithLower = USE_LOWERCASE ? TEST_COUNT : 0;
    int expectedWithDigits = USE_DIGITS ? TEST_COUNT : 0;
    int expectedWithSymbols = USE_SYMBOLS ? TEST_COUNT : 0;

    auto printSetCheck = [&](const std::string& name, int actual, int expected) {
        if (expected > 0) {
            if (actual == expected) {
                std::cout << "   " << name << ": " << actual << "/" << expected << " passwords contain this set" << std::endl;
            } else {
                std::cout << "   " << name << ": Only " << actual << "/" << expected << " passwords contain this set" << std::endl;
            }
        }
    };

    printSetCheck("Uppercase (A-Z)", hasUppercase, expectedWithUpper);
    printSetCheck("Lowercase (a-z)", hasLowercase, expectedWithLower);
    printSetCheck("Digits (0-9)", hasDigits, expectedWithDigits);
    printSetCheck("Symbols (!@#$%^&*)", hasSymbols, expectedWithSymbols);

    // Проверка, что все пароли содержат минимум по одному символу из каждого выбранного набора
    bool allHaveRequiredSets = true;
    if (USE_UPPERCASE && hasUppercase != TEST_COUNT) allHaveRequiredSets = false;
    if (USE_LOWERCASE && hasLowercase != TEST_COUNT) allHaveRequiredSets = false;
    if (USE_DIGITS && hasDigits != TEST_COUNT) allHaveRequiredSets = false;
    if (USE_SYMBOLS && hasSymbols != TEST_COUNT) allHaveRequiredSets = false;

    if (allHaveRequiredSets) {
        std::cout << "   All passwords contain at least one character from each selected set (GEN-3)" << std::endl;
    } else {
        std::cout << "   GEN-3 requirement NOT met: Some passwords missing required character sets" << std::endl;
    }

    // 3. Проверка длины
    std::cout << "\n3. LENGTH CHECK:" << std::endl;
    bool lengthOk = true;
    for (int len = 8; len <= 64; ++len) {
        if (lengthCount[len] > 0) {
            // Ожидаемая длина только PASSWORD_LENGTH
            if (len != PASSWORD_LENGTH) {
                lengthOk = false;
            }
        }
    }

    if (lengthOk) {
        std::cout << "   All passwords have correct length (" << PASSWORD_LENGTH << " characters)" << std::endl;
    } else {
        std::cout << "   Some passwords have incorrect length" << std::endl;
        for (int len = 8; len <= 64; ++len) {
            if (len != PASSWORD_LENGTH && lengthCount[len] > 0) {
                std::cout << "      " << lengthCount[len] << " passwords with length " << len << std::endl;
            }
        }
    }

    // 4. Проверка силы пароля
    std::cout << "\n4. STRENGTH CHECK:" << std::endl;
    std::cout << "   Score 0 (very weak): " << strengthCount[0] << " passwords" << std::endl;
    std::cout << "   Score 1 (weak): " << strengthCount[1] << " passwords" << std::endl;
    std::cout << "   Score 2 (medium): " << strengthCount[2] << " passwords" << std::endl;
    std::cout << "   Score 3 (strong): " << strengthCount[3] << " passwords" << std::endl;
    std::cout << "   Score 4 (very strong): " << strengthCount[4] << " passwords" << std::endl;

    int strongCount = strengthCount[3] + strengthCount[4];
    if (strongCount == TEST_COUNT) {
        std::cout << "   All passwords are strong (score >= 3)" << std::endl;
    } else {
        std::cout << "   " << (TEST_COUNT - strongCount) << " passwords are weak (score < 3)" << std::endl;
    }

    // 5. Вероятностная оценка дубликатов
    std::cout << "\n5. PROBABILITY ANALYSIS:" << std::endl;

    // Общее количество возможных паролей
    uint64_t possiblePasswords = 1;
    int charSetSize = allChars.length();
    for (int i = 0; i < PASSWORD_LENGTH; ++i) {
        possiblePasswords *= charSetSize;
    }

    std::cout << "   Character set size: " << charSetSize << std::endl;
    std::cout << "   Possible passwords: ~" << std::fixed << std::setprecision(2)
              << (possiblePasswords / 1e18) << "e+18" << std::endl;

    // Ожидаемое количество дубликатов при 10000 попытках (приблизительно)
    double expectedDuplicates = (TEST_COUNT * (TEST_COUNT - 1.0)) / (2.0 * possiblePasswords);

    std::cout << "   Expected duplicates: ~" << std::fixed << std::setprecision(6)
              << expectedDuplicates << std::endl;

    if (duplicateCount == 0 && expectedDuplicates < 0.01) {
        std::cout << "   No duplicates found (as expected)" << std::endl;
    } else if (duplicateCount == 0) {
        std::cout << "   No duplicates found" << std::endl;
    }

    std::cout << "\n========== TEST COMPLETE ==========\n" << std::endl;
}
