#ifndef LOGIN_DIALOG_H
#define LOGIN_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "../core/config_handler.h"
#include "../core/crypto/authentication.h"
#include "../core/crypto/key_derivation.h"
#include "../core/events.h"
#include "../core/key_manager.h"
#include "../src/database/DB_helper/db_helper.h"

class LoginDialog : public QDialog
{
    Q_OBJECT

private:
    ConfigHander &config;
    Database &db;
    Argon2Data authData;
    std::vector<uint8_t> encSalt;

    // Элементы UI
    QLineEdit *passwordCtrl;
    QLabel *errorText;
    QPushButton *loginButton;
    QPushButton *cancelButton;

    // Для exponential backoff
    int failedAttempts;
    QTimer *backoffTimer;
    int currentDelay;

private slots:
    void onLogin();
    void onPasswordEnter();
    void onBackoffTimer();
    void updateUIForBackoff();

private:
    void resetBackoff();
    bool loadAuthData();

public:
    LoginDialog(QWidget *parent, ConfigHander &cfg, Database &database);
    ~LoginDialog();
};

#endif // LOGIN_DIALOG_H
