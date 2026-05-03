// login_dialog.h
#ifndef LOGIN_DIALOG_H
#define LOGIN_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QTimer>
#include <string>
#include <functional>

// Интерфейс для проверки пароля (внешняя зависимость)
class IPasswordVerifier {
public:
    virtual ~IPasswordVerifier() = default;
    virtual bool verify(const std::string& password) = 0;
};

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    // Принимает функцию проверки пароля
    LoginDialog(QWidget* parent, std::function<bool(const std::string&)> verifier);
    ~LoginDialog();

    // Возвращает пароль и статус
    bool exec(std::string& outPassword);

private slots:
    void onLogin();
    void onPasswordEnter();
    void onBackoffTimer();
    void updateUIForBackoff();

private:
    void resetBackoff();

    std::function<bool(const std::string&)> m_verifier;

    // UI элементы
    QLineEdit* m_passwordCtrl;
    QLabel* m_errorText;
    QPushButton* m_loginButton;
    QPushButton* m_cancelButton;

    // Backoff
    int m_failedAttempts;
    int m_currentDelay;
    QTimer* m_backoffTimer;

    // Полученный пароль
    std::string m_password;
};

#endif
