#include "SettingsDialog.h"
#include <QMessageBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QApplication>
#include <QScreen>

SettingsDialog::SettingsDialog(QWidget *parent, ConfigHander &cfg)
    : QDialog(parent)
    , config(cfg)
{
    setWindowTitle("Settings");
    setMinimumSize(500, 400);
    setModal(true);

    // Основной layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Создаем вкладки
    tabWidget = new QTabWidget(this);

    createGeneralTab();
    createSecurityTab();
    createAdvancedTab();

    mainLayout->addWidget(tabWidget);

    // Кнопки OK/Cancel
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    QPushButton *okButton = new QPushButton("OK", this);
    QPushButton *cancelButton = new QPushButton("Cancel", this);

    buttonLayout->addWidget(okButton);
    buttonLayout->addSpacing(10);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addSpacing(20);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);

    // Подключаем сигналы
    connect(okButton, &QPushButton::clicked, this, &SettingsDialog::onOk);
    connect(cancelButton, &QPushButton::clicked, this, &SettingsDialog::onCancel);

    // Центрируем окно
    adjustSize();
    QRect screenGeometry = QApplication::primaryScreen()->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::createGeneralTab()
{
    QWidget *panel = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(panel);

    // Заголовок
    QLabel *title = new QLabel("General Settings", panel);
    QFont titleFont = title->font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);

    layout->addSpacing(10);

    // Группа настроек
    QGroupBox *settingsGroup = new QGroupBox("Application Settings", panel);
    QFormLayout *formLayout = new QFormLayout(settingsGroup);
    formLayout->setSpacing(10);
    formLayout->setContentsMargins(15, 15, 15, 15);

    // Database path (отображаем, но не редактируем в Sprint 1)
    QLabel *dbPathLabel = new QLabel(QString::fromStdString(config.getDatabasePath()), settingsGroup);
    dbPathLabel->setWordWrap(true);
    dbPathLabel->setStyleSheet("color: #666;");
    formLayout->addRow("Database Path:", dbPathLabel);

    // Startup behavior
    QCheckBox *startMinimized = new QCheckBox("Start minimized to tray", settingsGroup);
    startMinimized->setChecked(false);
    startMinimized->setEnabled(false); // Заглушка
    formLayout->addRow("", startMinimized);

    settingsGroup->setLayout(formLayout);
    layout->addWidget(settingsGroup);

    layout->addStretch();

    // Информация
    QLabel *info = new QLabel("General settings will be fully implemented in Sprint 4-5", panel);
    info->setStyleSheet("color: #888;");
    info->setWordWrap(true);
    layout->addWidget(info);

    panel->setLayout(layout);
    tabWidget->addTab(panel, "General");
}

void SettingsDialog::createSecurityTab()
{
    QWidget *panel = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(panel);

    // Заголовок
    QLabel *title = new QLabel("Security Settings", panel);
    QFont titleFont = title->font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);

    layout->addSpacing(10);

    // Группа настроек
    QGroupBox *settingsGroup = new QGroupBox("Auto-Lock Settings", panel);
    QFormLayout *formLayout = new QFormLayout(settingsGroup);
    formLayout->setSpacing(10);
    formLayout->setContentsMargins(15, 15, 15, 15);

    QSpinBox *autoLockSpin = new QSpinBox(settingsGroup);
    autoLockSpin->setRange(1, 60);
    autoLockSpin->setValue(5);
    autoLockSpin->setSuffix(" minutes");
    autoLockSpin->setEnabled(false); // Заглушка
    formLayout->addRow("Auto-lock after inactivity:", autoLockSpin);

    QCheckBox *lockOnMinimize = new QCheckBox("Lock when minimized", settingsGroup);
    lockOnMinimize->setChecked(true);
    lockOnMinimize->setEnabled(false); // Заглушка
    formLayout->addRow("", lockOnMinimize);

    QSpinBox *clipboardSpin = new QSpinBox(settingsGroup);
    clipboardSpin->setRange(5, 60);
    clipboardSpin->setValue(10);
    clipboardSpin->setSuffix(" seconds");
    clipboardSpin->setEnabled(false); // Заглушка
    formLayout->addRow("Clear clipboard after:", clipboardSpin);

    settingsGroup->setLayout(formLayout);
    layout->addWidget(settingsGroup);

    layout->addSpacing(10);

    // Группа дополнительных настроек
    QGroupBox *extraGroup = new QGroupBox("Additional Protection", panel);
    QVBoxLayout *extraLayout = new QVBoxLayout(extraGroup);

    QCheckBox *panicKey = new QCheckBox("Enable panic key (Ctrl+Shift+P)", extraGroup);
    panicKey->setChecked(false);
    panicKey->setEnabled(false); // Заглушка
    extraLayout->addWidget(panicKey);

    QCheckBox *memoryWipe = new QCheckBox("Wipe memory on lock", extraGroup);
    memoryWipe->setChecked(true);
    memoryWipe->setEnabled(false); // Заглушка
    extraLayout->addWidget(memoryWipe);

    extraGroup->setLayout(extraLayout);
    layout->addWidget(extraGroup);

    layout->addStretch();

    // Информация
    QLabel *info = new QLabel("Security settings will be fully implemented in Sprint 4", panel);
    info->setStyleSheet("color: #888;");
    info->setWordWrap(true);
    layout->addWidget(info);

    panel->setLayout(layout);
    tabWidget->addTab(panel, "Security");
}

void SettingsDialog::createAdvancedTab()
{
    QWidget *panel = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(panel);

    // Заголовок
    QLabel *title = new QLabel("Advanced Settings", panel);
    QFont titleFont = title->font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);

    layout->addSpacing(10);

    // Группа импорта/экспорта
    QGroupBox *importExportGroup = new QGroupBox("Import/Export", panel);
    QVBoxLayout *importExportLayout = new QVBoxLayout(importExportGroup);

    QPushButton *importBtn = new QPushButton("Import from CSV...", importExportGroup);
    importBtn->setEnabled(false); // Заглушка
    QPushButton *exportBtn = new QPushButton("Export to CSV...", importExportGroup);
    exportBtn->setEnabled(false); // Заглушка

    importExportLayout->addWidget(importBtn);
    importExportLayout->addWidget(exportBtn);

    importExportGroup->setLayout(importExportLayout);
    layout->addWidget(importExportGroup);

    layout->addSpacing(10);

    // Группа резервного копирования
    QGroupBox *backupGroup = new QGroupBox("Backup", panel);
    QVBoxLayout *backupLayout = new QVBoxLayout(backupGroup);

    QPushButton *backupBtn = new QPushButton("Create Backup...", backupGroup);
    backupBtn->setEnabled(false); // Заглушка
    QPushButton *restoreBtn = new QPushButton("Restore from Backup...", backupGroup);
    restoreBtn->setEnabled(false); // Заглушка

    backupLayout->addWidget(backupBtn);
    backupLayout->addWidget(restoreBtn);

    backupGroup->setLayout(backupLayout);
    layout->addWidget(backupGroup);

    layout->addSpacing(10);

    // Группа темы
    QGroupBox *themeGroup = new QGroupBox("Appearance", panel);
    QFormLayout *themeLayout = new QFormLayout(themeGroup);

    QComboBox *themeCombo = new QComboBox(themeGroup);
    themeCombo->addItem("System Default");
    themeCombo->addItem("Light");
    themeCombo->addItem("Dark");
    themeCombo->setEnabled(false); // Заглушка
    themeLayout->addRow("Theme:", themeCombo);

    themeGroup->setLayout(themeLayout);
    layout->addWidget(themeGroup);

    layout->addStretch();

    // Информация
    QLabel *info = new QLabel("Advanced settings will be implemented in Sprint 6-8", panel);
    info->setStyleSheet("color: #888;");
    info->setWordWrap(true);
    layout->addWidget(info);

    panel->setLayout(layout);
    tabWidget->addTab(panel, "Advanced");
}

void SettingsDialog::onOk()
{
    QMessageBox::information(this, "CryptoSafe Manager",
                             "Settings dialog is a placeholder for Sprint 2.\n\n"
                             "Real settings will be implemented in future sprints:\n"
                             "• Security settings - Sprint 4\n"
                             "• Import/Export - Sprint 6\n"
                             "• Backup - Sprint 8\n"
                             "• Theme/Language - future",
                             QMessageBox::Ok);

    accept();
}

void SettingsDialog::onCancel()
{
    reject();
}
