
// mainwindow.cpp
#include "MainWindow.h"
#include "../src/gui/dialogs/first_run_wizard/FirstRunWizard.h"
#include "../src/gui/dialogs/login_dialog/LoginDialog.h"
#include "../src/gui/dialogs/settings_dialog/SettingsDialog.h"
#include "../src/core/state_manager.h"
#include "../src/gui/dialogs/entry_dialog/entry_dialog.h"
#include "../src/gui/widgets/secure_table/PasswordDelegate.h"
#include "../src/core/clipboard_service/clipboard_service.h"
#include "../src/core/audit/log_signer/log_signer.h"
#include "../src/core/audit/audit_logger/audit_logger.h"
#include "../src/core/audit/log_verifier/log_verifier.h"
#include "../src/core/audit/log_formatter/log_formatter.h"
#include "../src/gui/dialogs/export_dialog/ExportDialog.h"
#include "share_dialog.h"
#include "import_dialog.h"
#include "export_pk_dialog.h"
#include "import_pk_dialog.h"
#include <QMessageBox>
#include <QApplication>
#include <QDebug>
#include <QHeaderView>
#include <QShortcut>
#include <QClipboard>
#include <nlohmann/json.hpp>
#include "rsa_cipher.h"
#include "../src/gui/dialogs/password_change/password_change.h"

using json = nlohmann::json;

MainWindow::MainWindow(ConfigHander &cfg, Database &database, VaultManager& vaultManager)
    : QMainWindow(nullptr)
    , config(cfg)
    , db(database)
    , m_vaultManager(vaultManager)
    , isLoggedIn(false)
    , m_temporaryMessage("")
{
    setWindowTitle("CryptoSafe Manager");
    resize(900, 600);

    createMenuBar();
    createToolBar();
    createCentralWidget();

    m_clipboardTimer = new QTimer(this);
    m_clipboardTimer->setSingleShot(true);
    m_clipboardSeconds = 30;

    registerEventHandlers();

    // Инициализация StateManager
    StateManager::getInstance().init(3600);

    // Подключение сигналов StateManager
    connect(&StateManager::getInstance(), &StateManager::LoggedIn,
            this, &MainWindow::onLoggedIn);
    connect(&StateManager::getInstance(), &StateManager::LoggedOut,
            this, &MainWindow::onLoggedOut);
    connect(&StateManager::getInstance(), &StateManager::inactivityTimeout,
            this, &MainWindow::onInactivityTimeout);

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
}

MainWindow::~MainWindow()
{
    if (isLoggedIn)
    {
        json details = json::object();
        details["reason"] = "manual shutdown";
        eventBus.publish(EventType::UserLoggedOut, details, "MainWindow");
        ClipboardService::getInstance().resetTimer();
    }
}

void MainWindow::registerEventHandlers()
{
    eventBus.subscribe(EventType::UserLoggedIn,
                       [this](const Event& event) { this->onUserLoggedIn(event); });
    eventBus.subscribe(EventType::UserLoggedOut,
                       [this](const Event& event) { this->onUserLoggedOut(event); });
    eventBus.subscribe(EventType::ClipboardWillClear,
                       [this](const Event& event) {
                           showTemporaryMessage("ВНИМАНИЕ! Буфер обмена очистится через 5 секунд!", 3000);
                       });
    eventBus.subscribe(EventType::ClipboardCleared,
                       [this](const Event& event) {
                           showTemporaryMessage("Буфер обмена очищен", 3000);
                       });
    eventBus.subscribe(EventType::ClipboardCopied,
                       [this](const Event& event) {
                           int timeout = ClipboardService::getInstance().getAutoClearTimeout();
                           showTemporaryMessage(QString("Пароль скопирован. Очистится через %1 сек").arg(timeout), 3000);
                       });

    AuditLogger::getInstance().init(db);
    LogVerifier::getInstance().init(&db);
    LogFormatter::getInstance().initDatabase(&db);
}

void MainWindow::createMenuBar()
{
    menuBar = new QMenuBar(this);

    QMenu *fileMenu = menuBar->addMenu("&Файл");
    //fileMenu->addAction("&New Database", this, &MainWindow::onNewDatabase, QKeySequence::New);
    //fileMenu->addAction("&Open Database", this, &MainWindow::onOpenDatabase, QKeySequence::Open);
    //fileMenu->addSeparator();
    //fileMenu->addAction("E&xit", this, &MainWindow::onExit, QKeySequence::Quit);

    QMenu *editMenu = menuBar->addMenu("&Правка");
    editMenu->addAction("&Добавить запись", this, &MainWindow::onAddEntry, QKeySequence("Ctrl+A"));
    editMenu->addAction("&Редактировать запись", this, &MainWindow::onEditEntry, QKeySequence("Ctrl+E"));
    editMenu->addAction("&Удалить запись", this, &MainWindow::onDeleteEntry, QKeySequence::Delete);
    editMenu->addSeparator();
    editMenu->addAction("&Заблокировать", this, &MainWindow::onLock, QKeySequence("Ctrl+L"));
    editMenu->addSeparator();
    editMenu->addAction("&Сменить мастер-пароль", this, &MainWindow::onChangePassword, QKeySequence("Ctrl+Shift+P"));

    QMenu *viewMenu = menuBar->addMenu("&Вид");
    //viewMenu->addAction("&Журнал аудита", this, &MainWindow::onViewAuditLogs, QKeySequence("Ctrl+Shift+A"));
    //viewMenu->addSeparator();
    viewMenu->addAction("&Настройки", this, &MainWindow::onSettings, QKeySequence("Ctrl+,"));

    QMenu *helpMenu = menuBar->addMenu("&Справка");
    helpMenu->addAction("&Мастер настройки", this, &MainWindow::onFirstRunWizard);
    helpMenu->addSeparator();
    helpMenu->addAction("&О программе", this, &MainWindow::onAbout);

    QMenu* toolsMenu = menuBar->addMenu("&Инструменты");
    toolsMenu->addAction("Мой публичный ключ", this, &MainWindow::onExportPublicKey);
    toolsMenu->addAction("Импорт публичного ключа", this, &MainWindow::onImportPublicKey);

    setMenuBar(menuBar);
}

void MainWindow::createToolBar()
{
    toolBar = addToolBar("Main");
    toolBar->addAction("Добавить", this, &MainWindow::onAddEntry);
    toolBar->addAction("Редактировать", this, &MainWindow::onEditEntry);
    toolBar->addAction("Удалить", this, &MainWindow::onDeleteEntry);
    toolBar->addSeparator();
    toolBar->addAction("Экспорт", this, &MainWindow::onExport);
    toolBar->addAction("Импорт", this, &MainWindow::onImport);
    toolBar->addSeparator();

    QAction* togglePasswordsAction = toolBar->addAction("👁");
    togglePasswordsAction->setToolTip("Показать/скрыть пароли (Ctrl+Shift+P)");
    togglePasswordsAction->setCheckable(true);
    connect(togglePasswordsAction, &QAction::toggled, this, [this](bool checked) {
        m_tableModel->setPasswordsVisible(checked);
    });

    QShortcut* shortcut = new QShortcut(QKeySequence("Ctrl+Shift+P"), this);
    connect(shortcut, &QShortcut::activated, togglePasswordsAction, &QAction::toggle);
}

void MainWindow::createCentralWidget()
{
    centralWidget = new QWidget(this);
    mainLayout = new QVBoxLayout(centralWidget);

    QTabWidget* tabWidget = new QTabWidget(this);

    // Вкладка Пароли
    QWidget* passwordsTab = new QWidget();
    QVBoxLayout* passwordsLayout = new QVBoxLayout(passwordsTab);

    QHBoxLayout* searchLayout = new QHBoxLayout();
    m_searchField = new QLineEdit(this);
    m_searchField->setPlaceholderText("Поиск...");
    m_searchField->setFixedWidth(250);
    searchLayout->addStretch();
    searchLayout->addWidget(m_searchField);
    passwordsLayout->addLayout(searchLayout);

    m_tableModel = new VaultTableModel(m_vaultManager, this);
    m_proxyModel = new SearchProxyModel(this);
    m_proxyModel->setSourceModel(m_tableModel);
    m_proxyModel->setFilterKeyColumn(-1);

    m_tableView = new QTableView(this);
    m_tableView->setModel(m_proxyModel);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tableView->setSortingEnabled(true);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->horizontalHeader()->setSectionsMovable(true);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_tableView->setColumnWidth(0, 200);
    m_tableView->setColumnWidth(1, 150);
    m_tableView->setColumnWidth(2, 200);
    m_tableView->setColumnWidth(3, 100);

    // ↓↓↓ ИЗМЕНЕНИЯ ЗДЕСЬ ↓↓↓
    // Включаем растяжение последней колонки
    m_tableView->horizontalHeader()->setStretchLastSection(true);


    // Устанавливаем начальные ширины
    m_tableView->setColumnWidth(VaultTableModel::COL_TITLE, 200);
    m_tableView->setColumnWidth(VaultTableModel::COL_USERNAME, 150);
    m_tableView->setColumnWidth(VaultTableModel::COL_URL, 200);
    m_tableView->setColumnWidth(VaultTableModel::COL_PASSWORD, 100);
    // COL_MODIFIED растянется автоматически
    // ↑↑↑ ИЗМЕНЕНИЯ ЗДЕСЬ ↑↑↑

    PasswordDelegate* passwordDelegate = new PasswordDelegate(this);
    m_tableView->setItemDelegateForColumn(VaultTableModel::COL_PASSWORD, passwordDelegate);
    connect(passwordDelegate, &PasswordDelegate::togglePasswordVisibility,
            this, [this](const QModelIndex& proxyIndex) {
                QModelIndex sourceIndex = m_proxyModel->mapToSource(proxyIndex);
                if (sourceIndex.isValid()) {
                    m_tableModel->togglePasswordVisibilityForRow(sourceIndex.row());
                }
            });

    passwordsLayout->addWidget(m_tableView);
    connect(m_searchField, &QLineEdit::textChanged, this, [this](const QString& text) {
        m_proxyModel->setSearchText(text);
    });
    m_tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tableView, &QTableView::customContextMenuRequested,
            this, &MainWindow::showContextMenu);

    tabWidget->addTab(passwordsTab, "Пароли");

    // Вкладка Аудит
    QWidget* auditTab = new QWidget();
    QVBoxLayout* auditLayout = new QVBoxLayout(auditTab);
    auditLayout->setContentsMargins(0, 0, 0, 0);
    m_auditLogViewer = new AuditLogViewer(auditTab);
    m_auditLogViewer->setDatabase(&db);
    auditLayout->addWidget(m_auditLogViewer);
    tabWidget->addTab(auditTab, "Аудит Лог");

    mainLayout->addWidget(tabWidget);

    // Статусная панель
    m_statusBar = new QTextEdit();
    m_statusBar->setReadOnly(true);
    m_statusBar->setMaximumHeight(120);
    m_statusBar->setStyleSheet(
        "QTextEdit {"
        "   background-color: #2c3e50;"
        "   color: #ecf0f1;"
        "   font-family: 'Monospace';"
        "   font-size: 9pt;"
        "}"
        );
    m_statusBar->clear();
    mainLayout->addWidget(m_statusBar);

    setCentralWidget(centralWidget);
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
    std::vector<uint8_t> hash, salt;
    uint32_t time_cost, memory_cost, parallelism, hash_len;

    if (!db.getAuthData(hash, salt, time_cost, memory_cost, parallelism, hash_len)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось загрузить данные аутентификации.");
        return false;
    }

    Argon2Data authData(time_cost, memory_cost, parallelism, hash_len);
    authData.hash = std::move(hash);
    authData.salt = std::move(salt);

    // Лямбда с указателем
    LoginDialog dialog(this, [&authData](const char* password, size_t len) -> bool {
        return verify_password(password, len, authData);
    });

    char masterPassword[4096];
    size_t passwordLen = 0;

    if (dialog.exec(masterPassword, sizeof(masterPassword), passwordLen)) {
        std::vector<uint8_t> encSalt;
        if (!db.getEncSalt(encSalt)) {
            QMessageBox::critical(this, "Ошибка", "Не удалось загрузить соль");
            secure_zero(masterPassword, passwordLen);
            return false;
        }

        std::vector<uint8_t> encKey;
        derive_encryption_key(masterPassword, passwordLen, encSalt, encKey);
        KeyManager::getInstance().storeEncryptionKey(encKey);

        LogSigner::getInstance().initFromMasterPassword(masterPassword, passwordLen);

        KeyData existingRSA;
        KeyManager::getInstance().getPrivateRSAKey(existingRSA);
        if (existingRSA.size == 0) {
            try {
                auto rsaKeys = RSACipher::generateKeyPair(2048);
                KeyManager::getInstance().storePrivateRSAKey(rsaKeys.privateKey);
                KeyManager::getInstance().storePublicRSAKey(rsaKeys.publicKey);
            } catch (const std::exception& e) {
                qWarning() << "Failed to generate RSA keys:" << e.what();
            }
        }

        secure_zero(masterPassword, passwordLen);
        StateManager::getInstance().login();
        return true;
    }

    return false;
}
void MainWindow::unlockApplication()
{
    if (!StateManager::getInstance().isLoggedIn())
    {
        StateManager::getInstance().unlock();
        refreshTable();
        m_tableView->show();
        m_searchField->show();
        updateSessionStatusDisplay();
    }
}

void MainWindow::lockApplication()
{
    if (StateManager::getInstance().isLoggedIn())
    {
        ClipboardService::getInstance().resetTimer();
        StateManager::getInstance().lock();
        m_tableView->hide();
        m_searchField->hide();
        m_searchField->clear();
        updateSessionStatusDisplay();
    }
}

void MainWindow::onLoggedIn()
{
    isLoggedIn = true;
    refreshTable();
    m_tableView->show();
    m_searchField->show();
    updateSessionStatusDisplay();
    appendStatusMessage("Вход выполнен");
}

void MainWindow::onLoggedOut()
{
    isLoggedIn = false;
    m_tableView->hide();
    m_searchField->hide();
    m_searchField->clear();
    ClipboardService::getInstance().resetTimer();
    updateSessionStatusDisplay();
    appendStatusMessage("Выход выполнен");
}

void MainWindow::onInactivityTimeout()
{
    StateManager::getInstance().lock();
    QMessageBox::information(this, "Авто-блокировка", "Приложение заблокировано из-за неактивности");
    showLoginDialog();
}

void MainWindow::onLock()
{
    if (StateManager::getInstance().isLoggedIn())
    {
        StateManager::getInstance().lock();
        showLoginDialog();
    }
}

void MainWindow::updateSessionStatusDisplay()
{
    if (!m_statusBar) return;

    QString currentText = m_statusBar->toPlainText();
    QStringList lines = currentText.split('\n');
    for (int i = 0; i < lines.size(); ++i) {
        if (lines[i].startsWith("Logged In") || lines[i].startsWith("Not Logged In")) {
            lines.removeAt(i);
            break;
        }
    }

    if (StateManager::getInstance().isLoggedIn()) {
        lines.prepend("Logged In");
    } else {
        lines.prepend("Not Logged In");
    }

    m_statusBar->setPlainText(lines.join('\n'));
}

void MainWindow::appendStatusMessage(const QString& msg)
{
    if (!m_statusBar) return;
    m_statusBar->append(msg);
    updateSessionStatusDisplay();
}

void MainWindow::refreshTable()
{
    m_tableModel->refresh();
}

void MainWindow::showTemporaryMessage(const QString& msg, int timeoutMs)
{
    appendStatusMessage(msg);

}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_F5) {
        refreshTable();
    }
    QMainWindow::keyPressEvent(event);
}

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

void MainWindow::onNewDatabase()
{
    QMessageBox::information(this, "Info", "New Database - will be implemented in Sprint 2");
}

void MainWindow::onOpenDatabase()
{
    QMessageBox::information(this, "Info", "Open Database - will be implemented in Sprint 2");
}

void MainWindow::onExport()
{
    ExportDialog dialog(&db, &m_vaultManager, this);
    dialog.exec();
}

void MainWindow::onExit()
{
    close();
}

void MainWindow::onAddEntry()
{
    if (!StateManager::getInstance().isLoggedIn())
    {
        showLoginDialog();
        return;
    }
    StateManager::getInstance().updateActivity();

    EntryDialog dialog(db, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        PlaintextEntry entry = dialog.getEntry();
        try {
            int id = m_vaultManager.createEntry(entry);
            if (id != -1) {
                json details = json::object();
                details["entry_id"] = static_cast<int>(id);
                details["title"] = entry.title;
                details["username"] = entry.username;
                details["category"] = entry.category;
                details["action"] = "create";
                EventBus::getInstance().publish(EventType::EntryAdded, details, "VaultManager");
            } else {
                QMessageBox::warning(this, "Ошибка", "Не удалось добавить запись");
            }
        } catch (const std::exception& e) {
            QMessageBox::critical(this, "Ошибка", QString("Ошибка при добавлении записи: %1").arg(e.what()));
        }
    }
}

void MainWindow::onEditEntry()
{
    if (!StateManager::getInstance().isLoggedIn())
    {
        showLoginDialog();
        return;
    }
    StateManager::getInstance().updateActivity();

    QModelIndexList selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    QModelIndex sourceIdx = m_proxyModel->mapToSource(selected.first());
    long entryId = m_tableModel->getId(sourceIdx.row());

    auto entry = m_vaultManager.getEntry(static_cast<int>(entryId));
    if (!entry) {
        QMessageBox::warning(this, "Ошибка", "Запись не найдена");
        return;
    }

    PlaintextEntry oldEntry = *entry;
    EntryDialog dialog(db, *entry, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        PlaintextEntry updatedEntry = dialog.getEntry();
        updatedEntry.creation_timestamp = entry->creation_timestamp;

        if (m_vaultManager.updateEntry(static_cast<int>(entryId), updatedEntry)) {
            json details = json::object();
            details["entry_id"] = static_cast<int>(entryId);
            details["old_title"] = oldEntry.title;
            details["new_title"] = updatedEntry.title;
            details["old_username"] = oldEntry.username;
            details["new_username"] = updatedEntry.username;
            details["old_category"] = oldEntry.category;
            details["new_category"] = updatedEntry.category;
            details["action"] = "update";
            EventBus::getInstance().publish(EventType::EntryUpdated, details, "VaultManager");
            m_tableModel->updatePasswordInCache(entryId, updatedEntry.password);
        }
    }
}

void MainWindow::onDeleteEntry()
{
    if (!StateManager::getInstance().isLoggedIn())
    {
        showLoginDialog();
        return;
    }
    StateManager::getInstance().updateActivity();

    QModelIndexList selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty())
    {
        QMessageBox::warning(this, "Нет выбора", "Пожалуйста, выберите запись для удаления");
        return;
    }

    QMap<long, QString> entries;
    for (const QModelIndex& idx : selected) {
        QModelIndex sourceIdx = m_proxyModel->mapToSource(idx);
        long id = m_tableModel->getId(sourceIdx.row());
        if (id != -1) {
            QString title = m_tableModel->data(m_tableModel->index(sourceIdx.row(), 0)).toString();
            entries[id] = title;
        }
    }

    if (entries.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Не удалось получить ID записей");
        return;
    }

    QString message = entries.size() == 1
                          ? "Вы уверены, что хотите удалить эту запись?"
                          : QString("Вы уверены, что хотите удалить %1 записей?").arg(entries.size());

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Подтверждение удаления", message, QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        bool allSuccess = true;
        int deletedCount = 0;

        for (auto it = entries.begin(); it != entries.end(); ++it) {
            long id = it.key();
            QString title = it.value();

            if (m_vaultManager.deleteEntry(static_cast<int>(id))) {
                deletedCount++;
                json details = json::object();
                details["entry_id"] = static_cast<int>(id);
                details["title"] = title.toStdString();
                details["action"] = "delete";
                EventBus::getInstance().publish(EventType::EntryDeleted, details, "VaultManager");
            } else {
                allSuccess = false;
            }
        }

        m_tableModel->refresh();
        if (!allSuccess) {
            QMessageBox::warning(this, "Ошибка", QString("Удалено %1 из %2 записей.").arg(deletedCount).arg(entries.size()));
        }
    }
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
                       "<p>Secure Password Manager</p>");
}

void MainWindow::onFirstRunWizard()
{
    showFirstRunWizard();
}

void MainWindow::onChangePassword()
{
    // 1. Проверяем, что пользователь залогинен
    if (!StateManager::getInstance().isLoggedIn()) {
        QMessageBox::warning(this, "Ошибка", "Вы не авторизованы");
        return;
    }

    // 2. Обновляем активность
    StateManager::getInstance().updateActivity();

    // 3. Создаем и показываем диалог смены пароля
    ChangePasswordDialog dialog(this, db);

    if (dialog.exec() == QDialog::Accepted) {
        // 4. После успешной смены пароля выходим из системы
        //    (пользователь должен войти с новым паролем)
        StateManager::getInstance().logout();

        // 5. Очищаем все ключи из памяти
        KeyManager::getInstance().clearAllKeys();

        // 6. Показываем диалог входа с новым паролем
        showLoginDialog();
    }
}

void MainWindow::onImport()
{
    ImportDialog dialog(&m_vaultManager, &db, this);
    dialog.exec();
}

void MainWindow::onShare()
{
    QModelIndexList selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    QModelIndex sourceIdx = m_proxyModel->mapToSource(selected.first());
    long entryId = m_tableModel->getId(sourceIdx.row());

    auto entry = m_vaultManager.getEntry(static_cast<int>(entryId));
    if (!entry) {
        QMessageBox::warning(this, "Ошибка", "Запись не найдена");
        return;
    }

    ShareDialog dialog(&m_vaultManager, &db, *entry, this);
    dialog.exec();
}

void MainWindow::onExportPublicKey()
{
    ExportPublicKeyDialog dialog(this);
    dialog.exec();
}

void MainWindow::onImportPublicKey()
{
    ImportPublicKeyDialog dialog(&db, this);
    if (dialog.exec() == QDialog::Accepted) {
        QMessageBox::information(this, "Успех",
                                 QString("Публичный ключ пользователя '%1' импортирован")
                                     .arg(dialog.getContactName()));
    }
}

void MainWindow::onViewAuditLogs()
{
    // AuditLogDialog dialog(db, this);
    // dialog.exec();
}

void MainWindow::showContextMenu(const QPoint& pos)
{
    QModelIndex index = m_tableView->indexAt(pos);
    if (!index.isValid()) return;

    if (!m_tableView->selectionModel()->isSelected(index)) {
        m_tableView->selectionModel()->clear();
        m_tableView->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }

    QModelIndexList selected = m_tableView->selectionModel()->selectedRows();
    bool singleSelected = (selected.size() == 1);
    bool multiSelected = (selected.size() > 1);

    QMenu menu(this);

    if (singleSelected) {
        menu.addAction("Редактировать", this, &MainWindow::onEditEntry);
        menu.addSeparator();
        menu.addAction("Копировать логин", this, &MainWindow::onCopyUsername);
        menu.addAction("Копировать пароль", this, &MainWindow::onCopyPassword);
        menu.addAction("Копировать всё", this, &MainWindow::onCopyAll);
        menu.addSeparator();
        menu.addAction("Поделиться", this, &MainWindow::onShare);
        menu.addSeparator();
        menu.addAction("Удалить", this, &MainWindow::onDeleteEntry);
    } else if (multiSelected) {
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
            json details = json::object();
            details["entry_id"] = static_cast<int>(id);
            details["copied_field"] = "username";
            details["action"] = "copy";
            EventBus::getInstance().publish(EventType::ClipboardCopied, details, "MainWindow");

            ClipboardService::getInstance().copyText(
                QString::fromStdString(entry->username),
                QString::fromStdString(entry->title),
                "username"
                );
            showTemporaryMessage("Логин скопирован", 2000);
        }
    } catch (const std::exception& e) {}
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
            json details = json::object();
            details["entry_id"] = static_cast<int>(id);
            details["copied_field"] = "password";
            details["action"] = "copy";
            EventBus::getInstance().publish(EventType::ClipboardCopied, details, "MainWindow");

            ClipboardService::getInstance().copyText(
                QString::fromStdString(entry->password),
                QString::fromStdString(entry->title),
                "password"
                );
        }
    } catch (const std::exception& e) {}
}

void MainWindow::onCopyAll()
{
    QModelIndexList selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    QModelIndex sourceIdx = m_proxyModel->mapToSource(selected.first());
    long id = m_tableModel->getId(sourceIdx.row());

    try {
        auto entry = m_vaultManager.getEntry(static_cast<int>(id));
        if (entry) {
            json details = json::object();
            details["entry_id"] = static_cast<int>(id);
            details["copied_field"] = "all";
            details["action"] = "copy";
            EventBus::getInstance().publish(EventType::ClipboardCopied, details, "MainWindow");

            QString allData = QString("Title: %1\nUsername: %2\nPassword: %3\nURL: %4\nNotes: %5\nCategory: %6\nTags: %7")
                                  .arg(QString::fromStdString(entry->title))
                                  .arg(QString::fromStdString(entry->username))
                                  .arg(QString::fromStdString(entry->password))
                                  .arg(QString::fromStdString(entry->url))
                                  .arg(QString::fromStdString(entry->notes))
                                  .arg(QString::fromStdString(entry->category))
                                  .arg(QString::fromStdString(entry->tags));

            ClipboardService::getInstance().copyText(allData, QString::fromStdString(entry->title), "all");
            showTemporaryMessage("Все данные скопированы", 2000);
        }
    } catch (const std::exception& e) {}
}
