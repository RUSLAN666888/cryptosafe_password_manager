#include "../src/gui/dialogs/entry_dialog/entry_dialog.h"
#include "../src/core/crypto/authentication.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QRegularExpression>
#include <chrono>
#include <openssl/rand.h>
#include <algorithm>

EntryDialog::EntryDialog(Database& db, QWidget* parent) : QDialog(parent), m_db(db)
{
    setupUI();
    setupConnections();
    loadGeneratorSettings();
    setWindowTitle("Добавление записи");
}

EntryDialog::EntryDialog(Database& db, const PlaintextEntry& entry, QWidget* parent) : QDialog(parent), m_db(db)
{
    setupUI();
    setupConnections();
    loadGeneratorSettings();
    loadEntry(entry);
    setWindowTitle("Редактирование записи");
}

void EntryDialog::loadEntry(const PlaintextEntry& entry)
{
    m_titleEdit->setText(QString::fromStdString(entry.title));
    m_usernameEdit->setText(QString::fromStdString(entry.username));
    m_passwordEntry->setValue(QString::fromStdString(entry.password));
    m_urlEdit->setText(QString::fromStdString(entry.url));
    m_notesEdit->setPlainText(QString::fromStdString(entry.notes));
    m_categoryEdit->setText(QString::fromStdString(entry.category));
    m_tagsEdit->setText(QString::fromStdString(entry.tags));
}

void EntryDialog::loadGeneratorSettings()
{
    try {
        std::string lenStr = m_db.getSetting("password_length", "16");
        m_genConfig.length = std::stoi(lenStr);
        m_genConfig.length = std::clamp(m_genConfig.length, 8, 64);

        m_genConfig.useUppercase = m_db.getSetting("password_use_uppercase", "true") == "true";
        m_genConfig.useLowercase = m_db.getSetting("password_use_lowercase", "true") == "true";
        m_genConfig.useDigits = m_db.getSetting("password_use_digits", "true") == "true";
        m_genConfig.useSymbols = m_db.getSetting("password_use_symbols", "true") == "true";
        m_genConfig.excludeAmbiguous = m_db.getSetting("password_exclude_ambiguous", "true") == "true";

    } catch (const std::exception& e) {
        m_genConfig = GeneratorConfig();
    }
}

void EntryDialog::setupUI()
{
    setMinimumWidth(550);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QFormLayout* formLayout = new QFormLayout();

    // Название (обязательное поле)
    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setPlaceholderText("Например: GitHub, Gmail, и т.д.");
    formLayout->addRow("Название *:", m_titleEdit);

    // Имя пользователя
    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("username@example.com");
    formLayout->addRow("Логин:", m_usernameEdit);

    // Пароль с генератором
    QHBoxLayout* passwordLayout = new QHBoxLayout();

    // Используем готовый виджет PasswordEntry
    m_passwordEntry = new PasswordEntry(this, "", QSize(300, -1));
    m_passwordEntry->setPlaceholderText("Введите пароль или сгенерируйте");
    passwordLayout->addWidget(m_passwordEntry, 1);

    m_generatePasswordBtn = new QPushButton("Сгенерировать", this);
    passwordLayout->addWidget(m_generatePasswordBtn);

    formLayout->addRow("Пароль *:", passwordLayout);

    // Индикатор силы пароля
    m_strengthGauge = new QProgressBar(this);
    m_strengthGauge->setRange(0, 4);
    m_strengthGauge->setValue(0);
    m_strengthGauge->setMaximumHeight(20);
    formLayout->addRow("", m_strengthGauge);

    // Текст с описанием силы пароля
    m_strengthText = new QLabel("Введите пароль для проверки", this);
    QPalette pal = m_strengthText->palette();
    pal.setColor(QPalette::WindowText, QColor(100, 100, 100));
    m_strengthText->setPalette(pal);
    formLayout->addRow("", m_strengthText);

    // URL
    m_urlEdit = new QLineEdit(this);
    m_urlEdit->setPlaceholderText("https://example.com");
    formLayout->addRow("Сайт:", m_urlEdit);

    // Категория
    m_categoryEdit = new QLineEdit(this);
    m_categoryEdit->setPlaceholderText("Работа, Личное, и т.д.");
    formLayout->addRow("Категория:", m_categoryEdit);

    // Теги
    m_tagsEdit = new QLineEdit(this);
    m_tagsEdit->setPlaceholderText("теги, через, запятую");
    formLayout->addRow("Теги:", m_tagsEdit);

    // Заметки
    m_notesEdit = new QTextEdit(this);
    m_notesEdit->setMaximumHeight(80);
    m_notesEdit->setPlaceholderText("Дополнительные заметки...");
    formLayout->addRow("Заметки:", m_notesEdit);

    mainLayout->addLayout(formLayout);

    // Кнопки
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_okBtn = new QPushButton("OK", this);
    m_cancelBtn = new QPushButton("Отмена", this);
    m_okBtn->setEnabled(false);

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_okBtn);
    buttonLayout->addWidget(m_cancelBtn);
    mainLayout->addLayout(buttonLayout);

    m_strengthTimer = new QTimer(this);
    m_strengthTimer->setSingleShot(true);
    connect(m_strengthTimer, &QTimer::timeout, this, &EntryDialog::onStrengthTimer);

    validateForm();
}

void EntryDialog::setupConnections()
{
    connect(m_okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_generatePasswordBtn, &QPushButton::clicked, this, &EntryDialog::onGeneratePassword);
    connect(m_titleEdit, &QLineEdit::textChanged, this, &EntryDialog::validateForm);
    connect(m_passwordEntry, &PasswordEntry::textChanged, this, &EntryDialog::validateForm);
}

void EntryDialog::validateForm()
{
    bool titleOk = !m_titleEdit->text().isEmpty();
    bool passwordOk = m_passwordEntry->getValue().length() >= MIN_PASSWORD_LENGTH;

    m_okBtn->setEnabled(titleOk && passwordOk);

    // Если пароль был сгенерирован, не запускаем проверку надежности
    if (m_isGenerated) {
        return;
    }

    // Перезапускаем таймер при каждом изменении текста
    m_strengthTimer->stop();
    m_strengthTimer->start(500);
}

void EntryDialog::onStrengthTimer()
{
    // Если пароль был сгенерирован, не показываем проверку
    if (m_isGenerated) {
        return;
    }

    QString password = m_passwordEntry->getValue();

    if (password.isEmpty())
    {
        m_strengthGauge->setValue(0);
        m_strengthText->setText("Введите пароль для проверки");
        QPalette pal = m_strengthText->palette();
        pal.setColor(QPalette::WindowText, QColor(100, 100, 100));
        m_strengthText->setPalette(pal);
        return;
    }

    QByteArray pwdBytes = password.toUtf8();
    int strength = check_password_strength(pwdBytes.constData(), pwdBytes.size());

    // Зануляем временный буфер
    secure_zero(pwdBytes.data(), pwdBytes.size());

    m_strengthGauge->setValue(strength);

    QColor color;
    QString message;

    switch (strength)
    {
    case 0:
        color = QColor(255, 0, 0);
        message = "Слишком слабый";
        break;
    case 1:
        color = QColor(255, 100, 0);
        message = "Очень слабый";
        break;
    case 2:
        color = QColor(255, 255, 0);
        message = "Слабый";
        break;
    case 3:
        color = QColor(0, 255, 0);
        message = "Сильный";
        break;
    case 4:
        color = QColor(0, 200, 0);
        message = "Очень сильный";
        break;
    default:
        color = QColor(100, 100, 100);
        message = "Неизвестно";
        break;
    }

    m_strengthText->setText(message);
    QPalette pal = m_strengthText->palette();
    pal.setColor(QPalette::WindowText, color);
    m_strengthText->setPalette(pal);
}

PlaintextEntry EntryDialog::getEntry() const
{
    PlaintextEntry entry;
    entry.title = m_titleEdit->text().toStdString();
    entry.username = m_usernameEdit->text().toStdString();
    entry.password = m_passwordEntry->getValue().toStdString();
    entry.url = m_urlEdit->text().toStdString();
    entry.notes = m_notesEdit->toPlainText().toStdString();
    entry.category = m_categoryEdit->text().toStdString();
    entry.tags = m_tagsEdit->text().toStdString();

    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::string timestamp = std::ctime(&now_time_t);
    if (!timestamp.empty() && timestamp.back() == '\n') {
        timestamp.pop_back();
    }
    entry.creation_timestamp = timestamp;
    entry.version = 1;

    return entry;
}

void EntryDialog::onGeneratePassword()
{
    QString password = generateSecurePassword();
    m_passwordEntry->setValue(password);
    m_isGenerated = true;

    // Очищаем индикатор силы пароля
    m_strengthGauge->setValue(0);
    m_strengthText->setText("Пароль сгенерирован");
    QPalette pal = m_strengthText->palette();
    pal.setColor(QPalette::WindowText, QColor(0, 150, 0));
    m_strengthText->setPalette(pal);

    validateForm();
}

QString EntryDialog::generateSecurePassword(int length)
{
    int actualLength = m_genConfig.length;
    actualLength = std::clamp(actualLength, 8, 64);

    QString uppercase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    QString lowercase = "abcdefghijklmnopqrstuvwxyz";
    QString digits = "0123456789";
    QString symbols = "!@#$%^&*";

    if (m_genConfig.excludeAmbiguous) {
        uppercase.remove('I');
        uppercase.remove('O');
        lowercase.remove('l');
        digits.remove('0');
        digits.remove('1');
    }

    QString allChars;
    if (m_genConfig.useUppercase) allChars += uppercase;
    if (m_genConfig.useLowercase) allChars += lowercase;
    if (m_genConfig.useDigits) allChars += digits;
    if (m_genConfig.useSymbols) allChars += symbols;

    if (allChars.isEmpty()) {
        allChars = lowercase;
    }

    auto getRandomInt = [](int max) -> int {
        unsigned int value;
        if (RAND_bytes(reinterpret_cast<unsigned char*>(&value), sizeof(value)) != 1) {
            throw std::runtime_error("Failed to generate random number");
        }
        return value % max;
    };

    QString password;

    if (m_genConfig.useUppercase && !uppercase.isEmpty()) {
        password += uppercase[getRandomInt(uppercase.length())];
    }
    if (m_genConfig.useLowercase && !lowercase.isEmpty()) {
        password += lowercase[getRandomInt(lowercase.length())];
    }
    if (m_genConfig.useDigits && !digits.isEmpty()) {
        password += digits[getRandomInt(digits.length())];
    }
    if (m_genConfig.useSymbols && !symbols.isEmpty()) {
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
}
