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
