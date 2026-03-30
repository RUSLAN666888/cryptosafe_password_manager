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

    void createGeneralTab();
    void createSecurityTab();
    void createAdvancedTab();
    void createPasswordGeneratorTab();
    void loadPasswordSettings();
    void savePasswordSettings();

private slots:
    void onOk();
    void onCancel();
    void onPasswordLengthChanged(int value);
    void onUseUppercaseChanged(int state);
    void onUseLowercaseChanged(int state);
    void onUseDigitsChanged(int state);
    void onUseSymbolsChanged(int state);
    void onExcludeAmbiguousChanged(int state);

public:
    SettingsDialog(Database& db, QWidget *parent, ConfigHander &cfg);
    ~SettingsDialog();
};

#endif // SETTINGS_DIALOG_H
