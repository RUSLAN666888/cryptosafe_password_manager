#ifndef FIRST_RUN_WIZARD_H
#define FIRST_RUN_WIZARD_H

#include <QWizard>
#include <QWizardPage>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QProgressBar>
#include <QTimer>
#include "../src/core/config_handler.h"
#include "../src/core/crypto/authentication.h"
#include "../src/gui/widgets/password_entry/PasswordEntry.h"

class FirstRunWizard : public QWizard
{
    Q_OBJECT

private:
    ConfigHander &config;
    QString temp_password;
    QProgressBar *strengthGauge;
    QLabel *strengthText;
    QTimer *strengthTimer;
    Argon2Data pendingAuthData;
    std::vector<uint8_t> encSalt;

    // Страницы
    QWizardPage *welcomePage;
    QWizardPage *passwordPage;
    QWizardPage *databasePage;
    QWizardPage *encryptionPage;
    QWizardPage *finishPage;

    // Элементы для страницы пароля
    PasswordEntry *passwordCtrl;
    PasswordEntry *confirmCtrl;

    // Элементы для страницы базы данных
    QLineEdit *dbPathCtrl;
    QPushButton *browseButton;

    // Элементы для страницы шифрования
    QSpinBox *iterationsSpin;
    QSpinBox *memorySpin;
    QSpinBox *parallelSpin;
    QSpinBox *hashLengthSpin;

    // Создание страниц
    QWizardPage *createWelcomePage();
    QWizardPage *createPasswordPage();
    QWizardPage *createDatabasePage();
    QWizardPage *createEncryptionPage();
    QWizardPage *createFinishPage();

private slots:
    void onBrowseDatabase();
    void onPasswordTextChanged();
    void onStrengthTimer();
    bool validatePassword();

public:
    explicit FirstRunWizard(QWidget *parent, ConfigHander &cfg);

    // Переопределенные виртуальные методы QWizard
    bool validateCurrentPage() override;  // Валидация при переходе
    void accept() override;               // При нажатии Finish

    Argon2Data& getAuthData();
    std::vector<uint8_t> getEncSalt() { return encSalt; }
};

#endif // FIRST_RUN_WIZARD_H
