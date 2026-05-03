// login_dialog.cpp
#include "LoginDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

LoginDialog::LoginDialog(QWidget* parent, std::function<bool(const std::string&)> verifier)
    : QDialog(parent)
    , m_verifier(verifier)
    , m_failedAttempts(0)
    , m_currentDelay(0)
    , m_backoffTimer(nullptr)
{
    setWindowTitle("Enter Master Password");
    setMinimumSize(400, 200);
    setModal(true);

    // Основной layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addStretch();

    // Центрированное содержимое
    QVBoxLayout* centerLayout = new QVBoxLayout();

    // Заголовок
    QLabel* title = new QLabel("Enter Master Password", this);
    QFont titleFont = title->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);
    centerLayout->addWidget(title);
    centerLayout->addSpacing(20);

    // Поле ввода пароля
    QHBoxLayout* rowLayout = new QHBoxLayout();
    QLabel* label = new QLabel("Password:", this);
    m_passwordCtrl = new QLineEdit(this);
    m_passwordCtrl->setEchoMode(QLineEdit::Password);
    m_passwordCtrl->setMinimumWidth(250);
    rowLayout->addWidget(label);
    rowLayout->addWidget(m_passwordCtrl);
    rowLayout->setAlignment(Qt::AlignCenter);
    centerLayout->addLayout(rowLayout);

    // Текст ошибки
    m_errorText = new QLabel("", this);
    m_errorText->setStyleSheet("color: red;");
    m_errorText->setAlignment(Qt::AlignCenter);
    centerLayout->addWidget(m_errorText);
    centerLayout->addSpacing(10);

    mainLayout->addLayout(centerLayout);
    mainLayout->addStretch();

    // Кнопки
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_loginButton = new QPushButton("Login", this);
    m_cancelButton = new QPushButton("Cancel", this);

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_loginButton);
    buttonLayout->addSpacing(10);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addSpacing(20);

    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(15);

    // Подключаем сигналы
    connect(m_loginButton, &QPushButton::clicked, this, &LoginDialog::onLogin);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_passwordCtrl, &QLineEdit::returnPressed, this, &LoginDialog::onPasswordEnter);

    // Таймер для backoff
    m_backoffTimer = new QTimer(this);
    m_backoffTimer->setSingleShot(true);
    connect(m_backoffTimer, &QTimer::timeout, this, &LoginDialog::onBackoffTimer);

    m_passwordCtrl->setFocus();
}

LoginDialog::~LoginDialog()
{
    if (m_backoffTimer) {
        m_backoffTimer->stop();
    }
}

bool LoginDialog::exec(std::string& outPassword)
{
    if (QDialog::exec() == QDialog::Accepted) {
        outPassword = m_password;

        // Очищаем пароль из памяти диалога
        volatile char* p = const_cast<char*>(m_password.data());
        for (size_t i = 0; i < m_password.size(); ++i) {
            p[i] = 0;
        }
        m_password.clear();

        return true;
    }
    return false;
}

void LoginDialog::onLogin()
{
    QString qPassword = m_passwordCtrl->text();

    if (qPassword.isEmpty()) {
        m_errorText->setText("Password cannot be empty");
        return;
    }

    // Проверка backoff
    if (m_failedAttempts > 0 && m_currentDelay > 0) {
        m_errorText->setText(QString("Too many attempts. Wait %1 seconds").arg(m_currentDelay));
        return;
    }

    std::string password = qPassword.toStdString();

    // Проверка пароля через внешний верификатор
    if (!m_verifier(password)) {
        m_failedAttempts++;

        // Exponential backoff
        if (m_failedAttempts <= 2) {
            m_currentDelay = 1;
        } else if (m_failedAttempts <= 4) {
            m_currentDelay = 5;
        } else {
            m_currentDelay = 30;
        }

        m_errorText->setText(QString("Invalid password. Try again in %1 seconds").arg(m_currentDelay));

        m_loginButton->setEnabled(false);
        m_passwordCtrl->setEnabled(false);

        m_backoffTimer->start(m_currentDelay * 1000);
        return;
    }

    // Успешный вход
    m_password = password;
    resetBackoff();
    accept();
}

void LoginDialog::onPasswordEnter()
{
    onLogin();
}

void LoginDialog::onBackoffTimer()
{
    m_currentDelay = 0;
    m_loginButton->setEnabled(true);
    m_passwordCtrl->setEnabled(true);
    m_errorText->setText("You can try again now");
    m_passwordCtrl->setFocus();
}

void LoginDialog::updateUIForBackoff()
{
    // Оставлено для совместимости, логика перенесена в onBackoffTimer
}

void LoginDialog::resetBackoff()
{
    m_failedAttempts = 0;
    m_currentDelay = 0;
    m_loginButton->setEnabled(true);
    m_passwordCtrl->setEnabled(true);
    m_errorText->setText("");
}
