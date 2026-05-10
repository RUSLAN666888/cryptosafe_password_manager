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
#include "../src/core/clipboard_service/clipboard_service.h"
#include "../src/core/audit/log_signer/log_signer.h"
#include "../src/core/audit/audit_logger/audit_logger.h"
#include "../src/gui/dialogs/audit_dialog/audit_log_dialog.h"
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
//#include <random>
#include <QProgressDialog>
#include <nlohmann/json.hpp>
#include "rsa_cipher.h"

using json = nlohmann::json;


MainWindow::MainWindow(ConfigHander &cfg, Database &database, VaultManager& vaultManager)
    : QMainWindow(nullptr)
    , config(cfg)
    , db(database)
    , m_vaultManager(vaultManager)
    , isLoggedIn(false)
    , m_temporaryMessage("")
{
    // json details = json::object();
    // details["action"] = "Startup";
    // EventBus::getInstance().publish(EventType::Startup, details, "MainWindow");

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

    m_clipboardTimer = new QTimer(this);
    m_clipboardTimer->setSingleShot(true);
    m_clipboardSeconds = 30;

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
        json details = json::object();
        details["action"] = "startup";
        eventBus.publish(EventType::UserLoggedIn, details, "StateMnager");
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
        json details = json::object();
        details["reason"] = "manual shutdown";
        eventBus.publish(EventType::UserLoggedOut, details, "MainWindow");
        //ClipboardService::getInstance().saveRemainingTime();
        ClipboardService::getInstance().resetTimer();
        //KeyManager::getInstance().logout();
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

    QMenu *fileMenu = menuBar->addMenu("&File");
    fileMenu->addAction("&New Database", this, &MainWindow::onNewDatabase, QKeySequence::New);
    fileMenu->addAction("&Open Database", this, &MainWindow::onOpenDatabase, QKeySequence::Open);
    fileMenu->addSeparator();
    //fileMenu->addAction("&Backup...", this, &MainWindow::onBackup);
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
    viewMenu->addAction("&Audit Logs", this, &MainWindow::onViewAuditLogs, QKeySequence("Ctrl+Shift+A"));
    viewMenu->addSeparator();
    viewMenu->addAction("&Settings", this, &MainWindow::onSettings, QKeySequence("Ctrl+,"));

    QMenu *helpMenu = menuBar->addMenu("&Help");
    helpMenu->addAction("Setup &Wizard", this, &MainWindow::onFirstRunWizard);
    helpMenu->addSeparator();
    helpMenu->addAction("&About", this, &MainWindow::onAbout);

    QMenu* toolsMenu = menuBar->addMenu("&Tools");
    toolsMenu->addAction("Мой публичный ключ", this, &MainWindow::onExportPublicKey);
    toolsMenu->addAction("Импорт публичного ключа", this, &MainWindow::onImportPublicKey);

    setMenuBar(menuBar);
}

void MainWindow::createToolBar()
{
    toolBar = addToolBar("Main");
    toolBar->addAction("Add", this, &MainWindow::onAddEntry);
    toolBar->addAction("Edit", this, &MainWindow::onEditEntry);
    toolBar->addAction("Delete", this, &MainWindow::onDeleteEntry);
    toolBar->addSeparator();
    toolBar->addAction("Экспорт", this, &MainWindow::onExport);
    toolBar->addAction("Импорт", this, &MainWindow::onImport);
    //toolBar->addAction("Поделиться", this, &MainWindow::onShare);


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

    QTabWidget* tabWidget = new QTabWidget(this);

    QWidget* passwordsTab = new QWidget();
    QVBoxLayout* passwordsLayout = new QVBoxLayout(passwordsTab);

    // Панель поиска
    QHBoxLayout* searchLayout = new QHBoxLayout();
    m_searchField = new QLineEdit(this);
    m_searchField->setPlaceholderText("Поиск...");
    m_searchField->setFixedWidth(250);
    searchLayout->addStretch();
    searchLayout->addWidget(m_searchField);
    passwordsLayout->addLayout(searchLayout);

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

    passwordsLayout->addWidget(m_tableView);

    // Подключаем поиск
    connect(m_searchField, &QLineEdit::textChanged, this, [this](const QString& text) {
        m_proxyModel->setSearchText(text);
    });

    // Подключаем контекстное меню
    m_tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tableView, &QTableView::customContextMenuRequested,
            this, &MainWindow::showContextMenu);

    tabWidget->addTab(passwordsTab, "Пароли");

    QWidget* auditTab = new QWidget();
    QVBoxLayout* auditLayout = new QVBoxLayout(auditTab);
    auditLayout->setContentsMargins(0, 0, 0, 0);

    m_auditLogViewer = new AuditLogViewer(auditTab);
    m_auditLogViewer->setDatabase(&db);
    auditLayout->addWidget(m_auditLogViewer);

    tabWidget->addTab(auditTab, "Аудит Лог");

    mainLayout->addWidget(tabWidget);

    // Устанавливаем центральный виджет
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
    QString msg;

    // Базовый статус
    if (isLoggedIn) {
        msg = "Logged in";
    } else {
        msg = "Not logged in";
    }


    // Временное сообщение (если есть)
    if (!m_temporaryMessage.isEmpty()) {
        msg = m_temporaryMessage + " | " + msg;
    }

    statusBar->showMessage(msg);
}

void MainWindow::showTemporaryMessage(const QString& msg, int timeoutMs)
{
    m_temporaryMessage = msg;
    updateStatusBar();

    // Таймер для очистки временного сообщения
    QTimer::singleShot(timeoutMs, this, [this]() {
        m_temporaryMessage.clear();
        updateStatusBar();
    });
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

// В MainWindow.cpp

// bool MainWindow::showLoginDialog()
// {
//     // Загружаем данные аутентификации из БД
//     std::vector<uint8_t> hash, salt;
//     uint32_t time_cost, memory_cost, parallelism, hash_len;

//     if (!db.getAuthData(hash, salt, time_cost, memory_cost, parallelism, hash_len)) {
//         QMessageBox::critical(this, "Ошибка",
//                               "Не удалось загрузить данные аутентификации. База данных может быть повреждена.");
//         return false;
//     }

//     Argon2Data authData(time_cost, memory_cost, parallelism, hash_len);
//     authData.hash = std::move(hash);
//     authData.salt = std::move(salt);

//     // Создаём диалог с лямбдой для проверки пароля
//     LoginDialog dialog(this, [authData](const std::string& password) -> bool {
//         return verify_password(password, authData);
//     });

//     std::string masterPassword;
//     if (dialog.exec(masterPassword)) {
//         // Загружаем соль для PBKDF2 из БД
//         std::vector<uint8_t> encSalt;
//         if (!db.getEncSalt(encSalt)) {
//             QMessageBox::critical(this, "Ошибка", "Не удалось загрузить соль");
//             return false;
//         }

//         // 1. Выводим ключ шифрования
//         std::vector<uint8_t> encKey;
//         derive_encryption_key(masterPassword, encSalt, encKey);
//         KeyManager::getInstance().storeEncryptionKey(encKey);

//         // 2. Инициализируем подпись для аудита
//         LogSigner::getInstance().initFromMasterPassword(masterPassword);

//         // 3. Публичный ключ в БД (если ещё нет)
//         // int keyVersion;
//         // std::vector<uint8_t> existingKey;
//         // if (!db.getCurrentPublicKey(existingKey, keyVersion)) {
//         //     // Первый запуск или ключ отсутствует
//         //     db.addPublicKey(LogSigner::getInstance().get_public_key(), 1, 1);
//         // }

//         // Очищаем пароль из памяти
//         volatile char* p = const_cast<char*>(masterPassword.data());
//         for (size_t i = 0; i < masterPassword.size(); ++i) {
//             p[i] = 0;
//         }

//         isLoggedIn = true;
//         return true;
//     }

//     return false;
// }

bool MainWindow::showLoginDialog()
{
    // Загружаем данные аутентификации из БД
    std::vector<uint8_t> hash, salt;
    uint32_t time_cost, memory_cost, parallelism, hash_len;

    if (!db.getAuthData(hash, salt, time_cost, memory_cost, parallelism, hash_len)) {
        QMessageBox::critical(this, "Ошибка",
                              "Не удалось загрузить данные аутентификации. База данных может быть повреждена.");
        return false;
    }

    Argon2Data authData(time_cost, memory_cost, parallelism, hash_len);
    authData.hash = std::move(hash);
    authData.salt = std::move(salt);

    // Создаём диалог с лямбдой для проверки пароля
    LoginDialog dialog(this, [authData](const std::string& password) -> bool {
        return verify_password(password, authData);
    });

    std::string masterPassword;
    if (dialog.exec(masterPassword)) {
        // Загружаем соль для PBKDF2 из БД
        std::vector<uint8_t> encSalt;
        if (!db.getEncSalt(encSalt)) {
            QMessageBox::critical(this, "Ошибка", "Не удалось загрузить соль");
            return false;
        }

        // Выводим ключ шифрования
        std::vector<uint8_t> encKey;
        derive_encryption_key(masterPassword, encSalt, encKey);
        KeyManager::getInstance().storeEncryptionKey(encKey);

        // Инициализируем подпись для аудита
        LogSigner::getInstance().initFromMasterPassword(masterPassword);

        // Генерируем RSA ключи для шеринга
        // Проверяем, есть ли уже RSA ключи
        KeyData existingRSA;
        KeyManager::getInstance().getPrivateRSAKey(existingRSA);

        if (existingRSA.size == 0) {
            // Нет ключей - генерируем новые
            try {
                auto rsaKeys = RSACipher::generateKeyPair(2048);

                // Сохраняем приватный ключ
                std::vector<uint8_t> privateKey = rsaKeys.privateKey;
                KeyManager::getInstance().storePrivateRSAKey(privateKey);

                // Сохраняем публичный ключ
                std::vector<uint8_t> publicKey = rsaKeys.publicKey;
                KeyManager::getInstance().storePublicRSAKey(publicKey);


                // Очищаем временные ключи из памяти
                volatile uint8_t* pPriv = privateKey.data();
                for (size_t i = 0; i < privateKey.size(); ++i) pPriv[i] = 0;

                volatile uint8_t* pPub = publicKey.data();
                for (size_t i = 0; i < publicKey.size(); ++i) pPub[i] = 0;

            } catch (const std::exception& e) {
                qWarning() << "Failed to generate RSA keys:" << e.what();
            }
        }

        // 4. Публичный ключ Ed25519 в БД (если ещё нет)
        // int keyVersion;
        // std::vector<uint8_t> existingKey;
        // if (!db.getCurrentPublicKey(existingKey, keyVersion)) {
        //     db.addPublicKey(LogSigner::getInstance().get_public_key(), 1, 1);
        // }

        // Очищаем пароль из памяти
        volatile char* p = const_cast<char*>(masterPassword.data());
        for (size_t i = 0; i < masterPassword.size(); ++i) {
            p[i] = 0;
        }

        isLoggedIn = true;
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

        m_tableView->show();
        m_searchField->show();

        updateStatusBar();

        StateManager::getInstance().login();

        resetInactivityTimer();

        // json details = json::object();
        // details["action"] = "Vault_unlocked";
        // eventBus.publish(EventType::Unlock, details, "MainWindow");
    }
}

void MainWindow::lockApplication()
{
    if (isLoggedIn)
    {
        // Принудительно сбрасываем буфер обмена и таймер
        ClipboardService::getInstance().resetTimer();

        isLoggedIn = false;

        m_tableView->hide();
        m_searchField->hide();
        m_searchField->clear();

        updateStatusBar();

        StateManager::getInstance().logout();

        if (inactivityTimer->isActive())
        {
            inactivityTimer->stop();
        }

        // json details = json::object();
        // details["action"] = "Vault_locked";
        // eventBus.publish(EventType::Lock, details, "MainWindow");
    }
}

void MainWindow::resetInactivityTimer()
{
    if (isLoggedIn)
    {
        StateManager::getInstance().updateActivity();
        //KeyManager::getInstance().update_activity();
        inactivityTimer->start(INACTIVITY_TIMEOUT_MS);
    }
}

void MainWindow::onInactivityTimeout()
{
    if (isLoggedIn)
    {
        json details = json::object();
        details["action"] = "InactivityTimeout";
        EventBus::getInstance().publish(EventType::InactivityTimeout, details, "MainWindow");

        lockApplication();
        //KeyManager::getInstance().logout();
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

void MainWindow::onExport()
{
    std::cout <<"export"<<std::endl;
    ExportDialog dialog(&db, &m_vaultManager, this);
    dialog.exec();
}

void MainWindow::onExit()
{
    if (isLoggedIn)
    {
        //KeyManager::getInstance().logout();
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
                //showTemporaryMessage("Запись успешно добавлена", 3000);
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
            QMessageBox::critical(this, "Ошибка",
                                  QString("Ошибка при добавлении записи: %1").arg(e.what()));
        }
    }


}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_F5) {
        refreshTable();
    }
    QMainWindow::keyPressEvent(event);
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

    PlaintextEntry oldEntry = *entry;

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
            //m_tableModel->refresh();

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

    // Собираем ID и title выбранных записей
    QMap<long, QString> entries;  // id -> title
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

    // Подтверждение удаления
    QString message = entries.size() == 1
                          ? "Вы уверены, что хотите удалить эту запись?"
                          : QString("Вы уверены, что хотите удалить %1 записей?").arg(entries.size());

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Подтверждение удаления", message,
        QMessageBox::Yes | QMessageBox::No
        );

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

        if (allSuccess) {
            m_tableModel->refresh();
            statusBar->showMessage(QString("Удалено %1 записей").arg(deletedCount), 3000);
        } else {
            QMessageBox::warning(this, "Ошибка",
                                 QString("Удалено %1 из %2 записей. Некоторые записи не удалось удалить.")
                                     .arg(deletedCount).arg(entries.size()));
            m_tableModel->refresh();
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
    // if (!isLoggedIn)
    // {
    //     QMessageBox::warning(this, "Not Logged In", "You must be logged in to change password");
    //     return;
    // }
    // resetInactivityTimer();

    // ChangePasswordDialog dialog(this, db);
    // if (dialog.exec() == QDialog::Accepted)
    // {
    //     lockApplication();

    //     QProgressDialog progressDialog("Идёт перешифровка базы данных...\nПожалуйста, подождите.",
    //                                    nullptr, 0, 0, this);
    //     progressDialog.setWindowModality(Qt::WindowModal);
    //     progressDialog.setMinimumDuration(0);
    //     progressDialog.setCancelButton(nullptr);
    //     progressDialog.show();

    //     KeyManager::getInstance().logout();

    //     if (!showLoginDialog()) {
    //         QMetaObject::invokeMethod(this, &MainWindow::close, Qt::QueuedConnection);
    //     }

    //     // Синхронный вызов
    //     bool success = m_vaultManager.rotate();

    //     progressDialog.close();
    //     setEnabled(true);

    //     if (success) {
    //         QMessageBox::information(this, "Успех", "База данных успешно перешифрована");
    //     } else {
    //         QMessageBox::critical(this, "Ошибка", "Не удалось перешифровать базу данных");
    //     }
    // }
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
        //KeyManager::getInstance().logout();

        if (!showLoginDialog())
        {
            QMetaObject::invokeMethod(this, &MainWindow::close, Qt::QueuedConnection);
            return;
        }
        StateManager::getInstance().publishUserLoggedIn();
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
            // Уведомление уже в EventBus, можно убрать или оставить
            showTemporaryMessage("Логин скопирован", 2000);
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
    } catch (const std::exception& e) {
        statusBar->showMessage("Ошибка при копировании", 2000);
    }
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

            QString allData = QString(
                                  "Title: %1\n"
                                  "Username: %2\n"
                                  "Password: %3\n"
                                  "URL: %4\n"
                                  "Notes: %5\n"
                                  "Category: %6\n"
                                  "Tags: %7"
                                  ).arg(QString::fromStdString(entry->title))
                                  .arg(QString::fromStdString(entry->username))
                                  .arg(QString::fromStdString(entry->password))
                                  .arg(QString::fromStdString(entry->url))
                                  .arg(QString::fromStdString(entry->notes))
                                  .arg(QString::fromStdString(entry->category))
                                  .arg(QString::fromStdString(entry->tags));

            ClipboardService::getInstance().copyText(allData, QString::fromStdString(entry->title), "all");
            showTemporaryMessage("Все данные скопированы", 2000);
        }
    } catch (const std::exception& e) {
        statusBar->showMessage("Ошибка при копировании", 2000);
    }
}

void MainWindow::onViewAuditLogs()
{
    // if (!isLoggedIn) {
    //     QMessageBox::warning(this, "Not Logged In",
    //                          "Please log in to view audit logs.");
    //     return;
    // }

    // AuditLogDialog dialog(db, this);
    // dialog.exec();

    // updateStatusBar();
}

void MainWindow::onImport(){
    ImportDialog dialog(&m_vaultManager, &db, this);
    dialog.exec();
}

void MainWindow::onShare(){
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
