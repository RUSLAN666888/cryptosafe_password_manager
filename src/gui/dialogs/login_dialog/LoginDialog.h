#ifndef LOGIN_DIALOG_H
#define LOGIN_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <functional>
#include <cstring>

class IPasswordVerifier {
public:
    virtual ~IPasswordVerifier() = default;
    virtual bool verify(const char* password, size_t len) = 0;
};

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    // Новый конструктор с верификатором на указателях
    LoginDialog(QWidget* parent, std::function<bool(const char*, size_t)> verifier);


    ~LoginDialog();


    // Новая версия с буфером (без копирования)
    bool exec(char* outBuffer, size_t bufferSize, size_t& outLen);

private slots:
    void onLogin();
    void onPasswordEnter();
    void onBackoffTimer();

private:
    void resetBackoff();
    void clearPasswordBuffer();

    std::function<bool(const char*, size_t)> m_verifier;  // Новый тип верификатора

    // UI элементы
    QLineEdit* m_passwordCtrl;
    QLabel* m_errorText;
    QPushButton* m_loginButton;
    QPushButton* m_cancelButton;

    // Backoff
    int m_failedAttempts;
    int m_currentDelay;
    QTimer* m_backoffTimer;

    // Буфер для пароля (вместо std::string)
    char* m_passwordBuffer;
    size_t m_passwordLen;
    static constexpr size_t MAX_PASSWORD_LEN = 4096;
};

#endif
