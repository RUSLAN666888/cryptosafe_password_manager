#include "FirstRunWizard.h"
#include "../src/core/crypto/authentication.h"
#include "../src/core/crypto/key_derivation.h"
#include "key_manager.h"
#include "key_storage.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QGroupBox>
#include <zxcvbn.h>

FirstRunWizard::FirstRunWizard(QWidget *parent, ConfigHander &cfg)
    : QWizard(parent)
    , config(cfg)
{
    setWindowTitle("CryptoSafe Setup Wizard");
    setWizardStyle(QWizard::ModernStyle);
    setMinimumSize(500, 500);

    // Создаем страницы
    welcomePage = createWelcomePage();
    passwordPage = createPasswordPage();
    databasePage = createDatabasePage();
    encryptionPage = createEncryptionPage();
    finishPage = createFinishPage();

    // Добавляем страницы в wizard
    addPage(welcomePage);
    addPage(passwordPage);
    addPage(databasePage);
    addPage(encryptionPage);
    addPage(finishPage);

    // Создаем таймер для задержки проверки
    strengthTimer = new QTimer(this);
    strengthTimer->setSingleShot(true);
    connect(strengthTimer, &QTimer::timeout, this, &FirstRunWizard::onStrengthTimer);
}

QWizardPage *FirstRunWizard::createWelcomePage()
{
    QWizardPage *page = new QWizardPage;
    page->setTitle("Welcome to CryptoSafe Manager!");

    QVBoxLayout *layout = new QVBoxLayout(page);

    // Текст приветствия
    QLabel *text = new QLabel(
        "This wizard will help you set up your password manager.\n\n"
        "You will need to:\n"
        "- Create a master password\n"
        "- Choose database location\n"
        "- Configure encryption settings",
        page);
    text->setWordWrap(true);
    layout->addWidget(text);

    // Инструкция
    QLabel *instruction = new QLabel("Click Next to begin setup.", page);
    instruction->setAlignment(Qt::AlignCenter);
    layout->addWidget(instruction);

    layout->addStretch();

    return page;
}

QWizardPage *FirstRunWizard::createPasswordPage()
{
    QWizardPage *page = new QWizardPage;
    page->setTitle("Create Master Password");
    page->setSubTitle("Choose a strong master password to protect your vault.");

    QVBoxLayout *layout = new QVBoxLayout(page);

    // Поля ввода
    QLabel *passLabel = new QLabel("Password:", page);
    passwordCtrl = new PasswordEntry(page, "", QSize(300, -1));

    QLabel *confirmLabel = new QLabel("Confirm:", page);
    confirmCtrl = new PasswordEntry(page, "", QSize(300, -1));

    layout->addWidget(passLabel);
    layout->addWidget(passwordCtrl);

    // Индикатор силы пароля
    strengthGauge = new QProgressBar(page);
    strengthGauge->setRange(0, 4);
    strengthGauge->setValue(0);
    strengthGauge->setMaximumHeight(20);
    layout->addWidget(strengthGauge);

    // Текст с описанием силы пароля
    strengthText = new QLabel("Enter password to check strength", page);
    QPalette pal = strengthText->palette();
    pal.setColor(QPalette::WindowText, QColor(100, 100, 100));
    strengthText->setPalette(pal);
    layout->addWidget(strengthText);

    layout->addSpacing(15);
    layout->addWidget(confirmLabel);
    layout->addWidget(confirmCtrl);

    layout->addStretch();

    // Подключаем сигналы изменения текста
    connect(passwordCtrl, &PasswordEntry::textChanged,
            this, &FirstRunWizard::onPasswordTextChanged);
    connect(confirmCtrl, &PasswordEntry::textChanged,
            this, &FirstRunWizard::onPasswordTextChanged);

    return page;
}

QWizardPage *FirstRunWizard::createDatabasePage()
{
    QWizardPage *page = new QWizardPage;
    page->setTitle("Database Location");
    page->setSubTitle("Choose where to store your encrypted vault.");

    QVBoxLayout *layout = new QVBoxLayout(page);

    // Поле выбора пути
    QHBoxLayout *pathLayout = new QHBoxLayout;

    QLabel *pathLabel = new QLabel("Path:", page);
    pathLayout->addWidget(pathLabel);

    dbPathCtrl = new QLineEdit(page);
    dbPathCtrl->setText(QString::fromStdString(config.getDatabasePath()));
    pathLayout->addWidget(dbPathCtrl);

    browseButton = new QPushButton("Browse...", page);
    pathLayout->addWidget(browseButton);

    layout->addLayout(pathLayout);

    // Пояснение
    QLabel *info = new QLabel("All passwords will be stored in this file", page);
    QPalette pal = info->palette();
    pal.setColor(QPalette::WindowText, QColor(100, 100, 100));
    info->setPalette(pal);
    layout->addWidget(info);

    layout->addStretch();

    // Подключаем сигнал
    connect(browseButton, &QPushButton::clicked, this, &FirstRunWizard::onBrowseDatabase);

    return page;
}

QWizardPage *FirstRunWizard::createEncryptionPage()
{
    QWizardPage *page = new QWizardPage;
    page->setTitle("Encryption Settings");
    page->setSubTitle("Configure Argon2id parameters for key derivation.");

    QVBoxLayout *layout = new QVBoxLayout(page);

    // Пояснение
    QLabel *note = new QLabel(
        "These settings control how your master password is strengthened.\n"
        "Higher values = more secure but slower unlock.",
        page);
    note->setWordWrap(true);
    QPalette pal = note->palette();
    pal.setColor(QPalette::WindowText, QColor(100, 100, 100));
    note->setPalette(pal);
    layout->addWidget(note);

    layout->addSpacing(20);  // Добавляем отступ

    // Группа настроек Argon2id
    QGroupBox *settingsGroup = new QGroupBox("Argon2id Parameters", page);
    QGridLayout *gridLayout = new QGridLayout(settingsGroup);

    // Устанавливаем отступы внутри группы
    gridLayout->setContentsMargins(20, 20, 20, 20);
    gridLayout->setSpacing(15);  // Расстояние между строками

    // 1. Time cost (итерации)
    QLabel *timeLabel = new QLabel("Time cost (iterations):", settingsGroup);
    timeLabel->setMinimumHeight(30);
    gridLayout->addWidget(timeLabel, 0, 0);

    iterationsSpin = new QSpinBox(settingsGroup);
    iterationsSpin->setRange(1, 20);
    iterationsSpin->setValue(3);
    iterationsSpin->setMinimumWidth(120);
    gridLayout->addWidget(iterationsSpin, 0, 1);

    // 2. Memory cost (MB)
    QLabel *memoryLabel = new QLabel("Memory cost (MiB):", settingsGroup);
    memoryLabel->setMinimumHeight(30);
    gridLayout->addWidget(memoryLabel, 1, 0);

    memorySpin = new QSpinBox(settingsGroup);
    memorySpin->setRange(16, 1024);
    memorySpin->setValue(64);
    memorySpin->setMinimumWidth(120);
    gridLayout->addWidget(memorySpin, 1, 1);

    // 3. Parallelism (потоки)
    QLabel *parallelLabel = new QLabel("Parallelism (threads):", settingsGroup);
    parallelLabel->setMinimumHeight(30);
    gridLayout->addWidget(parallelLabel, 2, 0);

    parallelSpin = new QSpinBox(settingsGroup);
    parallelSpin->setRange(1, 16);
    parallelSpin->setValue(4);
    parallelSpin->setMinimumWidth(120);
    gridLayout->addWidget(parallelSpin, 2, 1);

    // 4. Hash length (bytes)
    QLabel *hashLabel = new QLabel("Hash length (bytes):", settingsGroup);
    hashLabel->setMinimumHeight(30);
    gridLayout->addWidget(hashLabel, 3, 0);

    hashLengthSpin = new QSpinBox(settingsGroup);
    hashLengthSpin->setRange(16, 64);
    hashLengthSpin->setValue(32);
    hashLengthSpin->setMinimumWidth(120);
    gridLayout->addWidget(hashLengthSpin, 3, 1);

    // Настройка растяжения колонок
    gridLayout->setColumnStretch(0, 1);
    gridLayout->setColumnStretch(1, 0);

    // Выравнивание содержимого
    gridLayout->setAlignment(Qt::AlignTop);

    settingsGroup->setLayout(gridLayout);
    settingsGroup->setMinimumHeight(250);  // Минимальная высота группы
    settingsGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    layout->addWidget(settingsGroup);

    layout->addSpacing(20);  // Отступ перед пояснением



    layout->addStretch();  // Растягиваем вниз

    return page;
}

QWizardPage *FirstRunWizard::createFinishPage()
{
    QWizardPage *page = new QWizardPage;
    page->setTitle("Setup Complete!");

    QVBoxLayout *layout = new QVBoxLayout(page);

    // Текст завершения
    QLabel *text = new QLabel(
        "Your password manager is ready to use.\n\n"
        "Click Finish to start the application.",
        page);
    text->setWordWrap(true);
    text->setAlignment(Qt::AlignCenter);
    layout->addWidget(text);

    layout->addStretch();

    return page;
}

void FirstRunWizard::onBrowseDatabase()
{
    QString filename = QFileDialog::getSaveFileName(
        this,
        "Select database file",
        "",
        "SQLite files (*.db);;All files (*.*)"
        );

    if (!filename.isEmpty())
    {
        dbPathCtrl->setText(filename);
    }
}

void FirstRunWizard::onPasswordTextChanged()
{
    // Перезапускаем таймер при каждом изменении текста
    strengthTimer->stop();
    strengthTimer->start(500);
}

void FirstRunWizard::onStrengthTimer()
{
    QString password = passwordCtrl->getValue();

    if (password.isEmpty())
    {
        strengthGauge->setValue(0);
        strengthText->setText("Enter password to check strength");
        QPalette pal = strengthText->palette();
        pal.setColor(QPalette::WindowText, QColor(100, 100, 100));
        strengthText->setPalette(pal);
        return;
    }

    // Конвертируем QString в std::string
    std::string pwdStr = password.toStdString();

    // Получаем оценку силы пароля (0-4)
    int score = check_password_strength(pwdStr);

    // Обновляем индикатор
    strengthGauge->setValue(score);

    // Обновляем текст и цвет в зависимости от оценки
    QColor color;
    QString message;

    switch (score)
    {
    case 0:
        color = QColor(255, 0, 0);
        message = "Too weak - easily guessable";
        break;
    case 1:
        color = QColor(255, 100, 0);
        message = "Very weak";
        break;
    case 2:
        color = QColor(255, 255, 0);
        message = "Weak";
        break;
    case 3:
        color = QColor(0, 255, 0);
        message = "Strong";
        break;
    case 4:
        color = QColor(0, 200, 0);
        message = "Very strong";
        break;
    default:
        color = QColor(100, 100, 100);
        message = "Unknown";
    }

    strengthText->setText(message);
    QPalette pal = strengthText->palette();
    pal.setColor(QPalette::WindowText, color);
    strengthText->setPalette(pal);
}

bool FirstRunWizard::validatePassword()
{
    QString password = passwordCtrl->getValue();
    QString confirm = confirmCtrl->getValue();

    if (password.isEmpty())
    {
        QMessageBox::critical(this, "Error", "Password cannot be empty!");
        return false;
    }

    if (password != confirm)
    {
        QMessageBox::critical(this, "Error", "Passwords do not match!");
        return false;
    }

    if (password.length() < 12)
    {
        QMessageBox::critical(this, "Error",
                              "Password must be at least 12 characters!");
        return false;
    }

    // Проверка силы пароля через zxcvbn
    std::string pwdStr = password.toStdString();
    int score = check_password_strength(pwdStr);

    if (score < 3)
    {
        QMessageBox::warning(this, "Weak Password",
                             "Password is not strong enough!\n\n"
                             "Please choose a stronger password that is not common, "
                             "doesn't contain dictionary words, and has good entropy.");
        return false;
    }

    temp_password = password;
    return true;
}

bool FirstRunWizard::validateCurrentPage()
{
    // Проверяем валидацию только для страницы пароля
    if (currentPage() == passwordPage)
    {
        return validatePassword();
    }
    return QWizard::validateCurrentPage();
}

void FirstRunWizard::accept()
{
    // 1. Database path
    config.setDatabasePath(dbPathCtrl->text().toStdString());

    // 2. Argon2id parameters
    config.setArgon2TimeCost(iterationsSpin->value());
    config.setArgon2MemoryCost(memorySpin->value());
    config.setArgon2Parallelism(parallelSpin->value());
    config.setArgon2HashLength(hashLengthSpin->value());

    // БД еще нет, но мы готовим данные
    Argon2Data authData(iterationsSpin->value(), memorySpin->value(),
                        parallelSpin->value(), hashLengthSpin->value());

    // Конвертируем QString в std::string для хеширования
    std::string pwdStr = temp_password.toStdString();

    // Хешируем пароль
    hash_password(pwdStr, authData);

    // Сохраняем данные аутентификации во временные поля мастера
    pendingAuthData = std::move(authData);

    encSalt.resize(16);
    randombytes_buf(encSalt.data(), encSalt.size());

    std::vector<uint8_t> key;
    derive_encryption_key(pwdStr, encSalt, key);

    KeyManager::getInstance().storeEncryptionKey(key);

    // Зануляем пароль в памяти
    volatile char *p = const_cast<char*>(pwdStr.data());
    for (size_t i = 0; i < pwdStr.size(); ++i)
        p[i] = 0;

    //QMessageBox::information(this, "CryptoSafe",
                             //"Setup completed successfully!");

    QWizard::accept();
}

Argon2Data& FirstRunWizard::getAuthData()
{
    return pendingAuthData;
}
