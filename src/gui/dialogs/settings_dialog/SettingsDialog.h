#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include "../src/core/config_handler.h"

class SettingsDialog : public QDialog
{
    Q_OBJECT

private:
    ConfigHander &config;
    QTabWidget *tabWidget;

    void createGeneralTab();
    void createSecurityTab();
    void createAdvancedTab();

private slots:
    void onOk();
    void onCancel();

public:
    SettingsDialog(QWidget *parent, ConfigHander &cfg);
    ~SettingsDialog();
};

#endif // SETTINGS_DIALOG_H
