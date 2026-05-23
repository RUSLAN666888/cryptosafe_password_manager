#ifndef FIRST_RUN_WIZARD_H
#define FIRST_RUN_WIZARD_H

#include <QWizard>
#include <QLineEdit>
#include <QProgressBar>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include "gui/widgets/password_entry/PasswordEntry.h"
#include "config_handler.h"
#include "core/crypto/authentication.h"

class FirstRunWizard : public QWizard
{
    Q_OBJECT

public:
    FirstRunWizard(QWidget *parent, ConfigHander &cfg);
    ~FirstRunWizard();

    Argon2Data& getAuthData();
    std::vector<uint8_t>& getEncSalt() { return encSalt; }

protected:
    bool validateCurrentPage() override;
    void accept() override;

private slots:
    void onBrowseDatabase();
    void onPasswordTextChanged();
    void onStrengthTimer();

private:
    QWizardPage* createWelcomePage();
    QWizardPage* createPasswordPage();
    QWizardPage* createDatabasePage();
    QWizardPage* createEncryptionPage();
    QWizardPage* createFinishPage();

    bool validatePassword();
    void clearPasswordBuffer();

    ConfigHander &config;

    // UI элементы
    QWizardPage *welcomePage;
    QWizardPage *passwordPage;
    QWizardPage *databasePage;
    QWizardPage *encryptionPage;
    QWizardPage *finishPage;

    PasswordEntry *passwordCtrl;
    PasswordEntry *confirmCtrl;
    QProgressBar *strengthGauge;
    QLabel *strengthText;

    QLineEdit *dbPathCtrl;
    QPushButton *browseButton;

    QSpinBox *iterationsSpin;
    QSpinBox *memorySpin;
    QSpinBox *parallelSpin;
    QSpinBox *hashLengthSpin;

    QTimer *strengthTimer;

    // Буфер для пароля (вместо QString)
    char* m_passwordBuffer;
    size_t m_passwordLen;

    Argon2Data pendingAuthData;
    std::vector<uint8_t> encSalt;

    static constexpr size_t MAX_PASSWORD_LEN = 4096;
};

#endif
