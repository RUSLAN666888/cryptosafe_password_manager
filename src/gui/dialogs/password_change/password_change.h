#ifndef PASSWORD_CHANGE_H
#define PASSWORD_CHANGE_H

#include <QDialog>
#include <QStackedWidget>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include "../src/gui/widgets/password_entry/PasswordEntry.h"
#include "../src/database/DB_helper/db_helper.h"
#include "../src/core/crypto/authentication.h"
#include "../src/core/crypto/key_derivation.h"
#include "key_manager.h"

class ChangePasswordDialog : public QDialog
{
    Q_OBJECT

public:
    ChangePasswordDialog(QWidget *parent, Database &database);
    ~ChangePasswordDialog();

private slots:
    void onVerifyNext();
    void onChange();
    void onCancel();
    void onPasswordTextChanged();
    void onStrengthTimer();

private:
    void createVerifyPage();
    void createChangePage();
    bool loadAuthData();
    bool verifyCurrentPassword();
    bool validateNewPassword();
    void updatePasswordStrength();
    void switchToChangePage();
    void clearPasswordBuffers();
    void reencryptAllEntries(const std::vector<uint8_t>& oldKey, const std::vector<uint8_t>& newKey);

    Database &db;

    // Данные аутентификации
    Argon2Data authData;
    std::vector<uint8_t> encSalt;

    // Страницы
    QStackedWidget* stackedWidget;
    QWidget* verifyPage;
    QWidget* changePage;

    // UI элементы первой страницы
    PasswordEntry* currentPasswordCtrl;
    QLabel* errorText;
    QPushButton* verifyNextButton;

    // UI элементы второй страницы
    PasswordEntry* newPasswordCtrl;
    PasswordEntry* confirmPasswordCtrl;
    QProgressBar* strengthGauge;
    QLabel* strengthText;
    QPushButton* changeButton;
    QPushButton* cancelButton;

    // Буферы для паролей (безопасное хранение)
    char* m_currentPasswordBuffer;
    size_t m_currentPasswordLen;
    char* m_newPasswordBuffer;
    size_t m_newPasswordLen;

    QTimer* strengthTimer;

    static constexpr size_t MAX_PASSWORD_LEN = 4096;
};

#endif
