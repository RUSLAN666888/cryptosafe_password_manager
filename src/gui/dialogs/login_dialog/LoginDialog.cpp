#include "LoginDialog.h"
#include <chrono>
#include <thread>
#include <QMessageBox>
#include "../src/core/state_manager.h"

LoginDialog::LoginDialog(QWidget *parent, ConfigHander &cfg, Database &database)
    : QDialog(parent)
    , config(cfg)
    , db(database)
    , failedAttempts(0)
    , currentDelay(0)
    , backoffTimer(nullptr)
{
    setWindowTitle("Login to CryptoSafe");
    setMinimumSize(400, 200);

    // Загружаем данные аутентификации из БД
    if (!loadAuthData())
    {
        QMessageBox::critical(this, "Error",
                              "Failed to load authentication data. Database may be corrupted.");
        return;
    }

    // Основной вертикальный layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Пустое пространство сверху
    mainLayout->addStretch();

    // Центрированное содержимое
    QVBoxLayout *centerLayout = new QVBoxLayout();

    // Заголовок
    QLabel *title = new QLabel("Enter Master Password", this);
    QFont titleFont = title->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);
    centerLayout->addWidget(title);
    centerLayout->addSpacing(20);

    // Поле ввода пароля
    QHBoxLayout *rowLayout = new QHBoxLayout();
    QLabel *label = new QLabel("Password:", this);
    passwordCtrl = new QLineEdit(this);
    passwordCtrl->setEchoMode(QLineEdit::Password);
    passwordCtrl->setMinimumWidth(200);

    rowLayout->addWidget(label);
    rowLayout->addWidget(passwordCtrl);
    rowLayout->setAlignment(Qt::AlignCenter);
    centerLayout->addLayout(rowLayout);

    // Текст ошибки
    errorText = new QLabel("", this);
    errorText->setStyleSheet("color: red;");
    errorText->setAlignment(Qt::AlignCenter);
    centerLayout->addWidget(errorText);
    centerLayout->addSpacing(10);

    mainLayout->addLayout(centerLayout);

    // Пустое пространство снизу
    mainLayout->addStretch();

    // Кнопки
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    loginButton = new QPushButton("Login", this);
    cancelButton = new QPushButton("Cancel", this);

    buttonLayout->addStretch();
    buttonLayout->addWidget(loginButton);
    buttonLayout->addSpacing(10);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addSpacing(20);

    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(15);

    // Подключаем сигналы
    connect(loginButton, &QPushButton::clicked, this, &LoginDialog::onLogin);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(passwordCtrl, &QLineEdit::returnPressed, this, &LoginDialog::onPasswordEnter);

    // Создаем таймер для backoff
    backoffTimer = new QTimer(this);
    backoffTimer->setSingleShot(true);
    connect(backoffTimer, &QTimer::timeout, this, &LoginDialog::onBackoffTimer);

    // Устанавливаем фокус на поле пароля
    passwordCtrl->setFocus();
}

LoginDialog::~LoginDialog()
{
    if (backoffTimer)
    {
        backoffTimer->stop();
    }
}

bool LoginDialog::loadAuthData()
{
    std::vector<uint8_t> hash;
    std::vector<uint8_t> salt;
    uint32_t time_cost, memory_cost, parallelism, hash_len;

    try
    {
        if (!db.getAuthData(hash, salt, time_cost, memory_cost, parallelism, hash_len))
        {
            return false;
        }
    }
    catch (const std::exception &e)
    {
        return false;
    }
    catch (...)
    {
        return false;
    }

    if (!db.getEncSalt(encSalt))
    {
        return false;
    }

    // Заполняем Argon2Data
    authData = Argon2Data(time_cost, memory_cost, parallelism, hash_len);
    authData.hash = std::move(hash);
    authData.salt = std::move(salt);

    return true;
}

void LoginDialog::onLogin()
{
    QString password = passwordCtrl->text();

    if (password.isEmpty())
    {
        errorText->setText("Password cannot be empty");
        return;
    }

    // Проверка на backoff
    if (failedAttempts > 0 && currentDelay > 0)
    {
        errorText->setText(QString("Too many attempts. Wait %1 seconds").arg(currentDelay));
        return;
    }

    // Конвертируем QString в std::string
    std::string pwdStr = password.toStdString();

    // Шаг 1: Verify password against Argon2 hash
    if (!verify_password(pwdStr, authData))
    {
        StateManager::getInstance().addFailedAttempt();
        failedAttempts = StateManager::getInstance().getFailedAttempts();

        // Exponential backoff
        if (failedAttempts <= 2)
        {
            currentDelay = 1;
        }
        else if (failedAttempts <= 4)
        {
            currentDelay = 5;
        }
        else
        {
            currentDelay = 30;
        }

        errorText->setText(QString("Invalid password. Try again in %1 seconds").arg(currentDelay));
        updateUIForBackoff();

        // Запускаем таймер
        backoffTimer->start(currentDelay * 1000);

        return;
    }

    // Шаг 2: выводим ключ через PBKDF2
    std::vector<uint8_t> encKey;
    derive_encryption_key(pwdStr, encSalt, encKey);

    // Зануляем пароль в памяти
    volatile char *p = const_cast<char*>(pwdStr.data());
    for (size_t i = 0; i < pwdStr.size(); ++i)
    {
        p[i] = 0;
    }

    // Шаг 3: Cache encryption key in secure memory
    KeyManager::getInstance().store_key(encKey);

    // Шаг 4: Publish UserLoggedIn event
    struct LoginEventData
    {
        std::string username;
        std::chrono::system_clock::time_point loginTime;
    };

    LoginEventData eventData{"user", std::chrono::system_clock::now()};
    eventBus.publish(EventType::UserLoggedIn, eventData, "LoginDialog");

    // Сбрасываем счетчик попыток при успешном входе
    resetBackoff();

    // Закрываем диалог с успехом
    accept();
}

void LoginDialog::onPasswordEnter()
{
    onLogin();
}

void LoginDialog::onBackoffTimer()
{
    currentDelay = 0;
    updateUIForBackoff();
    errorText->setText("You can try again now");
    passwordCtrl->setFocus();
}

void LoginDialog::updateUIForBackoff()
{
    if (currentDelay > 0)
    {
        loginButton->setEnabled(false);
        passwordCtrl->setEnabled(false);
    }
    else
    {
        loginButton->setEnabled(true);
        passwordCtrl->setEnabled(true);
    }
}

void LoginDialog::resetBackoff()
{
    failedAttempts = 0;
    currentDelay = 0;
    updateUIForBackoff();
    errorText->setText("");
}
