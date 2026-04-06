#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include "../src/core/config_handler.h"
#include "../src/database/DB_helper/db_helper.h"

class SettingsDialog : public QDialog
{
    Q_OBJECT

private:
    ConfigHander &config;
    QTabWidget *tabWidget;

    Database& m_db;

    // Виджеты для настроек генератора паролей
    QSpinBox *m_passwordLengthSpin;
    QCheckBox *m_useUppercaseCheck;
    QCheckBox *m_useLowercaseCheck;
    QCheckBox *m_useDigitsCheck;
    QCheckBox *m_useSymbolsCheck;
    QCheckBox *m_excludeAmbiguousCheck;

    QCheckBox* m_notifyOnCopy;
    QCheckBox* m_notifyOnWarning;
    QCheckBox* m_notifyOnClear;
    QComboBox* m_securityLevel;

    void createGeneralTab();
    void createAdvancedTab();
    void createPasswordGeneratorTab();
    void loadPasswordSettings();
    void savePasswordSettings();

    void createClipboardTab();
    void loadClipboardSettings();
    void saveClipboardSettings();

    QSpinBox* m_clipboardTimeoutSpin;
    QCheckBox* m_clipboardNeverClear;

private slots:
    void onOk();
    void onCancel();
    void onPasswordLengthChanged(int value);
    void onUseUppercaseChanged(int state);
    void onUseLowercaseChanged(int state);
    void onUseDigitsChanged(int state);
    void onUseSymbolsChanged(int state);
    void onExcludeAmbiguousChanged(int state);
    void onSecurityLevelChanged(int index);

public:
    SettingsDialog(Database& db, QWidget *parent, ConfigHander &cfg);
    ~SettingsDialog();
};

#endif // SETTINGS_DIALOG_H
