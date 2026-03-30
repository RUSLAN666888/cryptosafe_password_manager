#ifndef ENTRY_DIALOG_H
#define ENTRY_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include "../src/core/vault/plaintext_entry.h"
#include "../src/database/DB_helper/db_helper.h"

class EntryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EntryDialog(Database& db, QWidget* parent);
    explicit EntryDialog(Database& db, const PlaintextEntry& entry, QWidget* parent);

    PlaintextEntry getEntry() const;

private slots:
    void onGeneratePassword();
    void onTogglePasswordVisibility();
    void validateForm();
    void onStrengthTimer();

private:
    void setupUI();
    void setupConnections();
    QString generateSecurePassword(int length = 16);
    void loadGeneratorSettings();
    void updateStrengthDisplay(int score, const QString& message, const QColor& color);
    void loadEntry(const PlaintextEntry& entry);

    struct GeneratorConfig {
        int length = 16;
        bool useUppercase = true;
        bool useLowercase = true;
        bool useDigits = true;
        bool useSymbols = true;
        bool excludeAmbiguous = true;
    };
    GeneratorConfig m_genConfig;
    Database& m_db;


    QLineEdit* m_titleEdit;
    QLineEdit* m_usernameEdit;
    QLineEdit* m_passwordEdit;
    QLineEdit* m_urlEdit;
    QTextEdit* m_notesEdit;
    QLineEdit* m_categoryEdit;
    QLineEdit* m_tagsEdit;
    QPushButton* m_generatePasswordBtn;
    QPushButton* m_togglePasswordBtn;
    QPushButton* m_okBtn;
    QPushButton* m_cancelBtn;
    QProgressBar* m_strengthGauge;
    QLabel* m_strengthText;
    QTimer* m_strengthTimer;

    bool m_passwordVisible = false;

    static constexpr int MIN_PASSWORD_LENGTH = 8;
};

#endif // ENTRY_DIALOG_H
