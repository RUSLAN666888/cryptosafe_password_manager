// #ifndef PASSWORD_CHANGE_H
// #define PASSWORD_CHANGE_H

// #include <QDialog>
// #include <QStackedWidget>
// #include <QVBoxLayout>
// #include <QHBoxLayout>
// #include <QLabel>
// #include <QPushButton>
// #include <QProgressBar>
// #include <QTimer>
// #include "../../../database/DB_helper/db_helper.h"
// #include "../../../core/crypto/authentication.h"
// #include "../../../core/key_manager.h"
// #include "../src/gui/widgets/password_entry/PasswordEntry.h"

// class ChangePasswordDialog : public QDialog
// {
//     Q_OBJECT

// private:
//     Database &db;

//     // Stacked widget для переключения страниц
//     QStackedWidget *stackedWidget;

//     // Страница 1: верификация
//     QWidget *verifyPage;
//     PasswordEntry *currentPasswordCtrl;
//     QLabel *errorText;
//     QPushButton *verifyNextButton;

//     // Страница 2: смена пароля
//     QWidget *changePage;
//     PasswordEntry *newPasswordCtrl;
//     PasswordEntry *confirmPasswordCtrl;
//     QProgressBar *strengthGauge;
//     QLabel *strengthText;
//     QPushButton *changeButton;
//     QPushButton *cancelButton;
//     QTimer *strengthTimer;

//     // Данные для аутентификации
//     Argon2Data authData;
//     std::vector<uint8_t> encSalt;

//     // Временное хранение пароля
//     std::string tempPassword;

//     void createVerifyPage();
//     void createChangePage();
//     bool loadAuthData();
//     bool verifyCurrentPassword();
//     bool validateNewPassword();
//     void updatePasswordStrength();
//     void switchToChangePage();

// private slots:
//     void onVerifyNext();
//     void onChange();
//     void onCancel();
//     void onPasswordTextChanged();
//     void onStrengthTimer();

// public:
//     ChangePasswordDialog(QWidget *parent, Database &database);
//     ~ChangePasswordDialog();
// };

// #endif // PASSWORD_CHANGE_H
