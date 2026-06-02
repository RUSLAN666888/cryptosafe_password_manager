#include "password_change.h"
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QScreen>
#include <QStyle>

#include <state_manager.h>

ChangePasswordDialog::ChangePasswordDialog(QWidget *parent, Database &database)
    : QDialog(parent)
    , db(database)
    , m_currentPasswordBuffer(nullptr)
    , m_currentPasswordLen(0)
    , m_newPasswordBuffer(nullptr)
    , m_newPasswordLen(0)
{
    setWindowTitle("Смена мастер-пароля");
    setMinimumSize(450, 400);
    setModal(true);

    if (!loadAuthData()) {
        QMessageBox::critical(this, "Ошибка", "Не удалось загрузить данные аутентификации");
        return;
    }

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    stackedWidget = new QStackedWidget(this);

    createVerifyPage();
    createChangePage();

    stackedWidget->addWidget(verifyPage);
    stackedWidget->addWidget(changePage);
    mainLayout->addWidget(stackedWidget);

    strengthTimer = new QTimer(this);
    strengthTimer->setSingleShot(true);
    connect(strengthTimer, &QTimer::timeout, this, &ChangePasswordDialog::onStrengthTimer);

    currentPasswordCtrl->setFocus();
    setLayout(mainLayout);

    adjustSize();
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(),
                                    parent ? parent->geometry() : QApplication::primaryScreen()->geometry()));
}

ChangePasswordDialog::~ChangePasswordDialog()
{
    if (strengthTimer) strengthTimer->stop();
    clearPasswordBuffers();
}

void ChangePasswordDialog::clearPasswordBuffers()
{
    if (m_currentPasswordBuffer) {
        secure_zero(m_currentPasswordBuffer, m_currentPasswordLen);
        delete[] m_currentPasswordBuffer;
        m_currentPasswordBuffer = nullptr;
        m_currentPasswordLen = 0;
    }
    if (m_newPasswordBuffer) {
        secure_zero(m_newPasswordBuffer, m_newPasswordLen);
        delete[] m_newPasswordBuffer;
        m_newPasswordBuffer = nullptr;
        m_newPasswordLen = 0;
    }
}

void ChangePasswordDialog::createVerifyPage()
{
    verifyPage = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(verifyPage);

    QLabel *title = new QLabel("Подтверждение текущего пароля", verifyPage);
    QFont titleFont = title->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);
    layout->addStretch();

    QLabel *passLabel = new QLabel("Текущий пароль:", verifyPage);
    currentPasswordCtrl = new PasswordEntry(verifyPage, "", QSize(300, -1));
    errorText = new QLabel("", verifyPage);
    errorText->setStyleSheet("color: red;");
    errorText->setAlignment(Qt::AlignCenter);

    layout->addWidget(passLabel);
    layout->addWidget(currentPasswordCtrl);
    layout->addWidget(errorText);
    layout->addStretch();

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    verifyNextButton = new QPushButton("Подтвердить и далее", verifyPage);
    QPushButton *cancelBtn = new QPushButton("Отмена", verifyPage);

    buttonLayout->addStretch();
    buttonLayout->addWidget(verifyNextButton);
    buttonLayout->addSpacing(10);
    buttonLayout->addWidget(cancelBtn);
    buttonLayout->addSpacing(20);
    layout->addLayout(buttonLayout);

    connect(verifyNextButton, &QPushButton::clicked, this, &ChangePasswordDialog::onVerifyNext);
    connect(cancelBtn, &QPushButton::clicked, this, &ChangePasswordDialog::onCancel);
    connect(currentPasswordCtrl, &PasswordEntry::textChanged, this, &ChangePasswordDialog::onPasswordTextChanged);
}

void ChangePasswordDialog::createChangePage()
{
    changePage = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(changePage);

    QLabel *title = new QLabel("Создание нового мастер-пароля", changePage);
    QFont titleFont = title->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);
    layout->addStretch();

    QLabel *newPassLabel = new QLabel("Новый пароль:", changePage);
    newPasswordCtrl = new PasswordEntry(changePage, "", QSize(300, -1));

    strengthGauge = new QProgressBar(changePage);
    strengthGauge->setRange(0, 4);
    strengthGauge->setValue(0);
    strengthGauge->setMaximumHeight(20);

    strengthText = new QLabel("Введите пароль для проверки", changePage);
    QPalette pal = strengthText->palette();
    pal.setColor(QPalette::WindowText, QColor(100, 100, 100));
    strengthText->setPalette(pal);

    QLabel *confirmLabel = new QLabel("Подтверждение пароля:", changePage);
    confirmPasswordCtrl = new PasswordEntry(changePage, "", QSize(300, -1));

    layout->addWidget(newPassLabel);
    layout->addWidget(newPasswordCtrl);
    layout->addWidget(strengthGauge);
    layout->addWidget(strengthText);
    layout->addSpacing(10);
    layout->addWidget(confirmLabel);
    layout->addWidget(confirmPasswordCtrl);
    layout->addStretch();

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    changeButton = new QPushButton("Сменить пароль", changePage);
    cancelButton = new QPushButton("Отмена", changePage);

    buttonLayout->addStretch();
    buttonLayout->addWidget(changeButton);
    buttonLayout->addSpacing(10);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addSpacing(20);
    layout->addLayout(buttonLayout);

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
        return false;

    if (!db.getEncSalt(encSalt))
        return false;

    authData = Argon2Data(time_cost, memory_cost, parallelism, hash_len);
    authData.hash = std::move(hash);
    authData.salt = std::move(salt);

    return true;
}

bool ChangePasswordDialog::verifyCurrentPassword()
{
    QByteArray pwdBytes = currentPasswordCtrl->getValue().toUtf8();

    if (pwdBytes.isEmpty()) {
        errorText->setText("Пароль не может быть пустым");
        return false;
    }

    if (!verify_password(pwdBytes.constData(), pwdBytes.size(), authData)) {
        errorText->setText("Неверный пароль");
        memset(pwdBytes.data(), 0, pwdBytes.size());
        return false;
    }

    // Сохраняем текущий пароль для последующей расшифровки
    clearPasswordBuffers();
    m_currentPasswordLen = pwdBytes.size();
    m_currentPasswordBuffer = new char[m_currentPasswordLen];
    memcpy(m_currentPasswordBuffer, pwdBytes.constData(), m_currentPasswordLen);

    memset(pwdBytes.data(), 0, pwdBytes.size());
    errorText->setText("");
    return true;
}

bool ChangePasswordDialog::validateNewPassword()
{
    QByteArray pwdBytes = newPasswordCtrl->getValue().toUtf8();
    QByteArray confirmBytes = confirmPasswordCtrl->getValue().toUtf8();

    if (pwdBytes.isEmpty()) {
        QMessageBox::critical(this, "Ошибка", "Пароль не может быть пустым!");
        return false;
    }

    if (pwdBytes != confirmBytes) {
        QMessageBox::critical(this, "Ошибка", "Пароли не совпадают!");
        memset(pwdBytes.data(), 0, pwdBytes.size());
        memset(confirmBytes.data(), 0, confirmBytes.size());
        return false;
    }

    if (pwdBytes.size() < 12) {
        QMessageBox::critical(this, "Ошибка", "Пароль должен содержать не менее 12 символов!");
        memset(pwdBytes.data(), 0, pwdBytes.size());
        memset(confirmBytes.data(), 0, confirmBytes.size());
        return false;
    }

    int score = check_password_strength(pwdBytes.constData(), pwdBytes.size());
    if (score < 3) {
        QMessageBox::warning(this, "Слабый пароль",
                             "Пароль недостаточно надёжен!\n\n"
                             "Пожалуйста, выберите более надёжный пароль.");
        memset(pwdBytes.data(), 0, pwdBytes.size());
        memset(confirmBytes.data(), 0, confirmBytes.size());
        return false;
    }

    // Сохраняем новый пароль
    m_newPasswordLen = pwdBytes.size();
    m_newPasswordBuffer = new char[m_newPasswordLen];
    memcpy(m_newPasswordBuffer, pwdBytes.constData(), m_newPasswordLen);

    memset(pwdBytes.data(), 0, pwdBytes.size());
    memset(confirmBytes.data(), 0, confirmBytes.size());

    return true;
}

void ChangePasswordDialog::updatePasswordStrength()
{
    QByteArray pwdBytes = newPasswordCtrl->getValue().toUtf8();

    if (pwdBytes.isEmpty()) {
        strengthGauge->setValue(0);
        strengthText->setText("Введите пароль для проверки");
        QPalette pal = strengthText->palette();
        pal.setColor(QPalette::WindowText, QColor(100, 100, 100));
        strengthText->setPalette(pal);
        memset(pwdBytes.data(), 0, pwdBytes.size());
        return;
    }

    int score = check_password_strength(pwdBytes.constData(), pwdBytes.size());
    strengthGauge->setValue(score);

    QColor color;
    QString message;
    switch (score) {
    case 0: color = QColor(255, 0, 0); message = "Слишком слабый"; break;
    case 1: color = QColor(255, 100, 0); message = "Очень слабый"; break;
    case 2: color = QColor(255, 255, 0); message = "Слабый"; break;
    case 3: color = QColor(0, 255, 0); message = "Сильный"; break;
    case 4: color = QColor(0, 200, 0); message = "Очень сильный"; break;
    default: color = QColor(100, 100, 100); message = "Неизвестно";
    }

    strengthText->setText(message);
    QPalette pal = strengthText->palette();
    pal.setColor(QPalette::WindowText, color);
    strengthText->setPalette(pal);
    memset(pwdBytes.data(), 0, pwdBytes.size());
}



void ChangePasswordDialog::switchToChangePage()
{
    stackedWidget->setCurrentIndex(1);
    newPasswordCtrl->setFocus();
}

void ChangePasswordDialog::onVerifyNext()
{
    if (verifyCurrentPassword())
        switchToChangePage();
}

void ChangePasswordDialog::onChange()
{
    if (!validateNewPassword())
        return;

    // 1. Переносим текущий ключ в старый
    KeyManager::getInstance().moveCurrentToOld();

    // 2. Создаем новые параметры аутентификации
    Argon2Data newAuthData(authData.time_cost, authData.memory_cost_mb,
                           authData.parallelism, authData.hash_len);
    hash_password(m_newPasswordBuffer, m_newPasswordLen, newAuthData);

    // 3. Сохраняем новые данные аутентификации
    db.saveAuthData(newAuthData.hash, newAuthData.salt,
                    newAuthData.time_cost, newAuthData.memory_cost_mb,
                    newAuthData.parallelism, newAuthData.hash_len);

    // 4. Генерируем новую соль для PBKDF2
    std::vector<uint8_t> newEncSalt(16);
    RAND_bytes(newEncSalt.data(), newEncSalt.size());
    db.saveEncSalt(newEncSalt);

    // 5. Получаем новый ключ шифрования и сохраняем в KeyManager
    std::vector<uint8_t> newKey;
    derive_encryption_key(m_newPasswordBuffer, m_newPasswordLen, newEncSalt, newKey);
    KeyManager::getInstance().storeEncryptionKey(newKey);

    // 6. Перешифровываем все записи (использует старый и новый ключи из KeyManager)
    int reencryptedCount = 0;
    if (!db.reencryptAllEntries(reencryptedCount)) {
        QMessageBox::critical(this, "Ошибка",
                              "Не удалось перешифровать записи. Смена пароля отменена.");
        return;
    }

    // 7. Очищаем старый ключ
    KeyManager::getInstance().clearOldEncryptionKey();

    QMessageBox::information(this, "Успех",
                             QString("Пароль успешно изменён!\n\n"
                                     "Перешифровано %1 записей новым ключом.\n"
                                     "Вам потребуется войти заново с новым паролем.")
                                 .arg(reencryptedCount));

    // Выходим из сессии
    StateManager::getInstance().logout();

    clearPasswordBuffers();
    accept();
}

void ChangePasswordDialog::onCancel()
{
    clearPasswordBuffers();
    reject();
}

void ChangePasswordDialog::onPasswordTextChanged()
{
    if (strengthTimer) {
        strengthTimer->stop();
        strengthTimer->start(500);
    }
}

void ChangePasswordDialog::onStrengthTimer()
{
    updatePasswordStrength();
}
