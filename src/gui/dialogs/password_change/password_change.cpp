#include "../src/gui/dialogs/password_change/password_change.h"
#include <QMessageBox>
#include <QDebug>
#include <QApplication>
#include <QScreen>
#include <QStyle>

ChangePasswordDialog::ChangePasswordDialog(QWidget *parent, Database &database)
    : QDialog(parent)
    , db(database)
{
    setWindowTitle("Change Master Password");
    setMinimumSize(450, 400);
    setModal(true);

    // Загружаем данные аутентификации
    if (!loadAuthData())
    {
        QMessageBox::critical(this, "Error",
                              "Failed to load authentication data. Database may be corrupted.");
        return;
    }

    // Создаем основной layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Создаем stacked widget для переключения страниц
    stackedWidget = new QStackedWidget(this);

    // Создаем страницы
    createVerifyPage();
    createChangePage();

    // Добавляем страницы в stacked widget
    stackedWidget->addWidget(verifyPage);
    stackedWidget->addWidget(changePage);

    mainLayout->addWidget(stackedWidget);

    // Таймер для проверки силы пароля
    strengthTimer = new QTimer(this);
    strengthTimer->setSingleShot(true);
    connect(strengthTimer, &QTimer::timeout, this, &ChangePasswordDialog::onStrengthTimer);

    // Устанавливаем фокус
    currentPasswordCtrl->setFocus();

    setLayout(mainLayout);

    // Центрируем диалог
    adjustSize();
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(),
                                    parent ? parent->geometry() : QApplication::primaryScreen()->geometry()));
}

ChangePasswordDialog::~ChangePasswordDialog()
{
    if (strengthTimer)
    {
        strengthTimer->stop();
    }
}

void ChangePasswordDialog::createVerifyPage()
{
    verifyPage = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(verifyPage);

    // Заголовок
    QLabel *title = new QLabel("Verify Current Password", verifyPage);
    QFont titleFont = title->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    layout->addStretch();

    // Поле пароля
    QLabel *passLabel = new QLabel("Current Password:", verifyPage);
    currentPasswordCtrl = new PasswordEntry(verifyPage, "", QSize(300, -1));

    errorText = new QLabel("", verifyPage);
    errorText->setStyleSheet("color: red;");
    errorText->setAlignment(Qt::AlignCenter);

    layout->addWidget(passLabel);
    layout->addWidget(currentPasswordCtrl);
    layout->addWidget(errorText);

    layout->addStretch();

    // Кнопки
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    verifyNextButton = new QPushButton("Verify & Next", verifyPage);
    QPushButton *cancelBtn = new QPushButton("Cancel", verifyPage);

    buttonLayout->addStretch();
    buttonLayout->addWidget(verifyNextButton);
    buttonLayout->addSpacing(10);
    buttonLayout->addWidget(cancelBtn);
    buttonLayout->addSpacing(20);

    layout->addLayout(buttonLayout);

    // Подключаем сигналы
    connect(verifyNextButton, &QPushButton::clicked, this, &ChangePasswordDialog::onVerifyNext);
    connect(cancelBtn, &QPushButton::clicked, this, &ChangePasswordDialog::onCancel);
    connect(currentPasswordCtrl, &PasswordEntry::textChanged, this, &ChangePasswordDialog::onPasswordTextChanged);
}

void ChangePasswordDialog::createChangePage()
{
    changePage = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(changePage);

    // Заголовок
    QLabel *title = new QLabel("Create New Master Password", changePage);
    QFont titleFont = title->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    layout->addStretch();

    // Новый пароль
    QLabel *newPassLabel = new QLabel("New Password:", changePage);
    newPasswordCtrl = new PasswordEntry(changePage, "", QSize(300, -1));

    // Индикатор силы пароля
    strengthGauge = new QProgressBar(changePage);
    strengthGauge->setRange(0, 4);
    strengthGauge->setValue(0);
    strengthGauge->setMaximumHeight(20);

    strengthText = new QLabel("Enter password to check strength", changePage);
    QPalette pal = strengthText->palette();
    pal.setColor(QPalette::WindowText, QColor(100, 100, 100));
    strengthText->setPalette(pal);

    // Подтверждение пароля
    QLabel *confirmLabel = new QLabel("Confirm Password:", changePage);
    confirmPasswordCtrl = new PasswordEntry(changePage, "", QSize(300, -1));

    layout->addWidget(newPassLabel);
    layout->addWidget(newPasswordCtrl);
    layout->addWidget(strengthGauge);
    layout->addWidget(strengthText);
    layout->addSpacing(10);
    layout->addWidget(confirmLabel);
    layout->addWidget(confirmPasswordCtrl);

    layout->addStretch();

    // Кнопки
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    changeButton = new QPushButton("Change Password", changePage);
    cancelButton = new QPushButton("Cancel", changePage);

    buttonLayout->addStretch();
    buttonLayout->addWidget(changeButton);
    buttonLayout->addSpacing(10);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addSpacing(20);

    layout->addLayout(buttonLayout);

    // Подключаем сигналы
    connect(changeButton, &QPushButton::clicked, this, &ChangePasswordDialog::onChange);
    connect(cancelButton, &QPushButton::clicked, this, &ChangePasswordDialog::onCancel);
    connect(newPasswordCtrl, &PasswordEntry::textChanged, this, &ChangePasswordDialog::onPasswordTextChanged);
    connect(confirmPasswordCtrl, &PasswordEntry::textChanged, this, &ChangePasswordDialog::onPasswordTextChanged);
}

bool ChangePasswordDialog::loadAuthData()
{
    std::vector<uint8_t> hash, salt;
    uint32_t time_cost, memory_cost, parallelism, hash_len;

    if (!db.getAuthData(hash, salt, time_cost, memory_cost, parallelism, hash_len))
    {
        return false;
    }

    if (!db.getEncSalt(encSalt))
    {
        return false;
    }

    authData = Argon2Data(time_cost, memory_cost, parallelism, hash_len);
    authData.hash = std::move(hash);
    authData.salt = std::move(salt);

    return true;
}

bool ChangePasswordDialog::verifyCurrentPassword()
{
    QString password = currentPasswordCtrl->getValue();

    if (password.isEmpty())
    {
        errorText->setText("Password cannot be empty");
        return false;
    }

    std::string pwdStr = password.toStdString();

    if (!verify_password(pwdStr, authData))
    {
        errorText->setText("Invalid password");
        return false;
    }

    // Зануляем пароль в памяти
    volatile char* p = const_cast<char*>(pwdStr.data());
    for (size_t i = 0; i < pwdStr.size(); ++i)
    {
        p[i] = 0;
    }

    errorText->setText("");
    return true;
}

bool ChangePasswordDialog::validateNewPassword()
{
    QString password = newPasswordCtrl->getValue();
    QString confirm = confirmPasswordCtrl->getValue();

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

    tempPassword = pwdStr;
    return true;
}

void ChangePasswordDialog::updatePasswordStrength()
{
    QString password = newPasswordCtrl->getValue();

    if (password.isEmpty())
    {
        strengthGauge->setValue(0);
        strengthText->setText("Enter password to check strength");
        QPalette pal = strengthText->palette();
        pal.setColor(QPalette::WindowText, QColor(100, 100, 100));
        strengthText->setPalette(pal);
        return;
    }

    std::string pwdStr = password.toStdString();
    int score = check_password_strength(pwdStr);

    strengthGauge->setValue(score);

    QColor color;
    QString message;

    switch (score)
    {
    case 0: color = QColor(255, 0, 0); message = "Too weak"; break;
    case 1: color = QColor(255, 100, 0); message = "Very weak"; break;
    case 2: color = QColor(255, 255, 0); message = "Weak"; break;
    case 3: color = QColor(0, 255, 0); message = "Strong"; break;
    case 4: color = QColor(0, 200, 0); message = "Very strong"; break;
    default: color = QColor(100, 100, 100); message = "Unknown";
    }

    strengthText->setText(message);
    QPalette pal = strengthText->palette();
    pal.setColor(QPalette::WindowText, color);
    strengthText->setPalette(pal);
}

void ChangePasswordDialog::switchToChangePage()
{
    stackedWidget->setCurrentIndex(1);
    newPasswordCtrl->setFocus();
}

void ChangePasswordDialog::onVerifyNext()
{
    if (verifyCurrentPassword())
    {
        switchToChangePage();
    }
}

void ChangePasswordDialog::onChange()
{
    if (!validateNewPassword())
    {
        return;
    }

    hash_password(tempPassword, authData);

    db.saveAuthData(authData.hash, authData.salt, authData.time_cost,
                    authData.memory_cost_mb, authData.parallelism, authData.hash_len);

    std::vector<uint8_t> newEncSalt(16);
    randombytes_buf(newEncSalt.data(), newEncSalt.size());
    db.saveEncSalt(newEncSalt);

    QMessageBox::information(this, "Success",
                             "Password changed successfully!\n\n"
                             "You will need to log in again with your new password.");

    KeyManager::KeyData d;
    KeyManager::getInstance().get_key(d);
    std::vector<uint8_t> key_vector(d.data, d.data + d.size);
    KeyManager::getInstance().store_old_key(key_vector);
    KeyManager::getInstance().zero_keyData(d);

    // Выходим из системы
    KeyManager::getInstance().logout();

    // Зануляем временный пароль
    volatile char* p = const_cast<char*>(tempPassword.data());
    for (size_t i = 0; i < tempPassword.size(); ++i)
    {
        p[i] = 0;
    }
    tempPassword.clear();

    // Закрываем диалог
    accept();
}

void ChangePasswordDialog::onCancel()
{
    reject();
}

void ChangePasswordDialog::onPasswordTextChanged()
{
    if (strengthTimer)
    {
        strengthTimer->stop();
        strengthTimer->start(500);
    }
}

void ChangePasswordDialog::onStrengthTimer()
{
    updatePasswordStrength();
}
