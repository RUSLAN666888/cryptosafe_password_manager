#include "LoginDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <cstring>

// Конструктор с новым верификатором
LoginDialog::LoginDialog(QWidget* parent, std::function<bool(const char*, size_t)> verifier)
    : QDialog(parent)
    , m_verifier(verifier)
    , m_failedAttempts(0)
    , m_currentDelay(0)
    , m_backoffTimer(nullptr)
    , m_passwordBuffer(nullptr)
    , m_passwordLen(0)
{
    setWindowTitle("Введите мастер-пароль");
    setMinimumSize(400, 200);
    setModal(true);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addStretch();

    QVBoxLayout* centerLayout = new QVBoxLayout();

    QLabel* title = new QLabel("Введите мастер-пароль", this);
    QFont titleFont = title->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);
    centerLayout->addWidget(title);
    centerLayout->addSpacing(20);

    QHBoxLayout* rowLayout = new QHBoxLayout();
    QLabel* label = new QLabel("Пароль:", this);
    m_passwordCtrl = new QLineEdit(this);
    m_passwordCtrl->setEchoMode(QLineEdit::Password);
    m_passwordCtrl->setMinimumWidth(250);
    rowLayout->addWidget(label);
    rowLayout->addWidget(m_passwordCtrl);
    rowLayout->setAlignment(Qt::AlignCenter);
    centerLayout->addLayout(rowLayout);

    m_errorText = new QLabel("", this);
    m_errorText->setStyleSheet("color: red;");
    m_errorText->setAlignment(Qt::AlignCenter);
    centerLayout->addWidget(m_errorText);
    centerLayout->addSpacing(10);

    mainLayout->addLayout(centerLayout);
    mainLayout->addStretch();

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_loginButton = new QPushButton("Вход", this);
    m_cancelButton = new QPushButton("Отмена", this);

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_loginButton);
    buttonLayout->addSpacing(10);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addSpacing(20);

    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(15);

    connect(m_loginButton, &QPushButton::clicked, this, &LoginDialog::onLogin);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_passwordCtrl, &QLineEdit::returnPressed, this, &LoginDialog::onPasswordEnter);

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
    clearPasswordBuffer();
}

void LoginDialog::clearPasswordBuffer()
{
    if (m_passwordBuffer) {
        volatile char* p = m_passwordBuffer;
        for (size_t i = 0; i < m_passwordLen; ++i) {
            p[i] = 0;
        }
        delete[] m_passwordBuffer;
        m_passwordBuffer = nullptr;
        m_passwordLen = 0;
    }
}


bool LoginDialog::exec(char* outBuffer, size_t bufferSize, size_t& outLen)
{
    if (QDialog::exec() == QDialog::Accepted) {
        if (m_passwordBuffer && m_passwordLen > 0 && m_passwordLen <= bufferSize) {
            memcpy(outBuffer, m_passwordBuffer, m_passwordLen);
            outLen = m_passwordLen;
            clearPasswordBuffer();
            return true;
        }
    }
    clearPasswordBuffer();
    return false;
}

void LoginDialog::onLogin()
{
    // Получаем пароль из QLineEdit в QByteArray (одна копия)
    QByteArray pwdBytes = m_passwordCtrl->text().toUtf8();

    if (pwdBytes.isEmpty()) {
        m_errorText->setText("Пароль не может быть пустым");
        return;
    }

    if (pwdBytes.size() > MAX_PASSWORD_LEN) {
        m_errorText->setText("Пароль слишком длинный");
        memset(pwdBytes.data(), 0, pwdBytes.size());
        return;
    }

    // Проверка backoff
    if (m_failedAttempts > 0 && m_currentDelay > 0) {
        m_errorText->setText(QString("Слишком много попыток. Подождите %1 секунд").arg(m_currentDelay));
        memset(pwdBytes.data(), 0, pwdBytes.size());
        return;
    }

    if (!m_verifier(pwdBytes.constData(), static_cast<size_t>(pwdBytes.size()))) {
        // Неверный пароль - зануляем и обрабатываем ошибку
        memset(pwdBytes.data(), 0, pwdBytes.size());

        m_failedAttempts++;

        // Exponential backoff
        if (m_failedAttempts <= 2) {
            m_currentDelay = 1;
        } else if (m_failedAttempts <= 4) {
            m_currentDelay = 5;
        } else {
            m_currentDelay = 30;
        }

        m_errorText->setText(QString("Неверный пароль. Повторите через %1 секунд").arg(m_currentDelay));
        m_loginButton->setEnabled(false);
        m_passwordCtrl->setEnabled(false);
        m_backoffTimer->start(m_currentDelay * 1000);
        return;
    }

    // Успешный вход - сохраняем пароль в защищенный буфер
    clearPasswordBuffer();
    m_passwordLen = static_cast<size_t>(pwdBytes.size());
    m_passwordBuffer = new char[m_passwordLen];
    memcpy(m_passwordBuffer, pwdBytes.constData(), m_passwordLen);

    // Зануляем временный буфер
    memset(pwdBytes.data(), 0, pwdBytes.size());

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
    m_errorText->setText("Теперь можно повторить попытку");
    m_passwordCtrl->setFocus();
}

void LoginDialog::resetBackoff()
{
    m_failedAttempts = 0;
    m_currentDelay = 0;
    m_loginButton->setEnabled(true);
    m_passwordCtrl->setEnabled(true);
    m_errorText->setText("");
}
