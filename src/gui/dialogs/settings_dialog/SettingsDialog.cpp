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

SettingsDialog::SettingsDialog(Database& db, QWidget *parent, ConfigHander &cfg)
    : QDialog(parent)
    , config(cfg)
    , m_db(db)
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
    createPasswordGeneratorTab();

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

    QLabel *dbPathLabel = new QLabel(QString::fromStdString(config.getDatabasePath()), settingsGroup);
    dbPathLabel->setWordWrap(true);
    dbPathLabel->setStyleSheet("color: #666;");
    formLayout->addRow("Database Path:", dbPathLabel);

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

void SettingsDialog::createPasswordGeneratorTab()
{
    QWidget *panel = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(panel);

    // Заголовок
    QLabel *title = new QLabel("Настройки генератора паролей", panel);
    QFont titleFont = title->font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);

    layout->addSpacing(10);

    // Группа настроек генератора
    QGroupBox *settingsGroup = new QGroupBox("Параметры генерации", panel);
    QFormLayout *formLayout = new QFormLayout(settingsGroup);
    formLayout->setSpacing(10);
    formLayout->setContentsMargins(15, 15, 15, 15);

    // Длина пароля
    m_passwordLengthSpin = new QSpinBox(settingsGroup);
    m_passwordLengthSpin->setRange(8, 64);
    m_passwordLengthSpin->setSuffix(" символов");
    m_passwordLengthSpin->setValue(16);
    formLayout->addRow("Длина пароля:", m_passwordLengthSpin);

    // Наборы символов
    m_useUppercaseCheck = new QCheckBox("Заглавные буквы (A-Z)", settingsGroup);
    m_useUppercaseCheck->setChecked(true);
    formLayout->addRow("", m_useUppercaseCheck);

    m_useLowercaseCheck = new QCheckBox("Строчные буквы (a-z)", settingsGroup);
    m_useLowercaseCheck->setChecked(true);
    formLayout->addRow("", m_useLowercaseCheck);

    m_useDigitsCheck = new QCheckBox("Цифры (0-9)", settingsGroup);
    m_useDigitsCheck->setChecked(true);
    formLayout->addRow("", m_useDigitsCheck);

    m_useSymbolsCheck = new QCheckBox("Символы (!@#$%^&*)", settingsGroup);
    m_useSymbolsCheck->setChecked(true);
    formLayout->addRow("", m_useSymbolsCheck);

    // Исключение неоднозначных символов
    m_excludeAmbiguousCheck = new QCheckBox("Исключить неоднозначные символы (l, I, 1, 0, O)", settingsGroup);
    m_excludeAmbiguousCheck->setChecked(true);
    formLayout->addRow("", m_excludeAmbiguousCheck);

    settingsGroup->setLayout(formLayout);
    layout->addWidget(settingsGroup);

    // Пояснение
    QLabel *infoLabel = new QLabel(
        "Эти настройки будут использоваться при генерации паролей в диалоге добавления/редактирования записей.",
        panel);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #666; font-size: 10px; margin-top: 10px;");
    layout->addWidget(infoLabel);

    layout->addStretch();

    panel->setLayout(layout);
    tabWidget->addTab(panel, "Генератор паролей");

    // Загружаем настройки из БД
    loadPasswordSettings();

    // Подключаем сигналы (для сохранения, без предварительного просмотра)
    connect(m_passwordLengthSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SettingsDialog::onPasswordLengthChanged);
    connect(m_useUppercaseCheck, &QCheckBox::stateChanged, this, &SettingsDialog::onUseUppercaseChanged);
    connect(m_useLowercaseCheck, &QCheckBox::stateChanged, this, &SettingsDialog::onUseLowercaseChanged);
    connect(m_useDigitsCheck, &QCheckBox::stateChanged, this, &SettingsDialog::onUseDigitsChanged);
    connect(m_useSymbolsCheck, &QCheckBox::stateChanged, this, &SettingsDialog::onUseSymbolsChanged);
    connect(m_excludeAmbiguousCheck, &QCheckBox::stateChanged, this, &SettingsDialog::onExcludeAmbiguousChanged);
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
    // Сохраняем настройки генератора паролей
    savePasswordSettings();

    QMessageBox::information(this, "CryptoSafe Manager",
                             "Settings saved successfully.\n\n"
                             "Password generator settings will take effect immediately.",
                             QMessageBox::Ok);

    accept();
}

void SettingsDialog::onCancel()
{
    reject();
}

void SettingsDialog::loadPasswordSettings()
{
    // Загружаем настройки из БД
    try {
        std::string lenStr = m_db.getSetting("password_length", "16");
        m_passwordLengthSpin->setValue(std::stoi(lenStr));

        m_useUppercaseCheck->setChecked(m_db.getSetting("password_use_uppercase", "true") == "true");
        m_useLowercaseCheck->setChecked(m_db.getSetting("password_use_lowercase", "true") == "true");
        m_useDigitsCheck->setChecked(m_db.getSetting("password_use_digits", "true") == "true");
        m_useSymbolsCheck->setChecked(m_db.getSetting("password_use_symbols", "true") == "true");
        m_excludeAmbiguousCheck->setChecked(m_db.getSetting("password_exclude_ambiguous", "true") == "true");
    } catch (const std::exception& e) {
        // Если ошибка, используем значения по умолчанию
        m_passwordLengthSpin->setValue(16);
        m_useUppercaseCheck->setChecked(true);
        m_useLowercaseCheck->setChecked(true);
        m_useDigitsCheck->setChecked(true);
        m_useSymbolsCheck->setChecked(true);
        m_excludeAmbiguousCheck->setChecked(true);
    }
}

void SettingsDialog::savePasswordSettings()
{
    std::cout << "LENGTH" << std::to_string(m_passwordLengthSpin->value()) << std::endl;
    m_db.setSetting("password_length", std::to_string(m_passwordLengthSpin->value()));
    m_db.setSetting("password_use_uppercase", m_useUppercaseCheck->isChecked() ? "true" : "false");
    m_db.setSetting("password_use_lowercase", m_useLowercaseCheck->isChecked() ? "true" : "false");
    m_db.setSetting("password_use_digits", m_useDigitsCheck->isChecked() ? "true" : "false");
    m_db.setSetting("password_use_symbols", m_useSymbolsCheck->isChecked() ? "true" : "false");
    m_db.setSetting("password_exclude_ambiguous", m_excludeAmbiguousCheck->isChecked() ? "true" : "false");
}

void SettingsDialog::onPasswordLengthChanged(int value)
{
    Q_UNUSED(value);
    // Изменения будут сохранены при нажатии OK
}

void SettingsDialog::onUseUppercaseChanged(int state)
{
    Q_UNUSED(state);
}

void SettingsDialog::onUseLowercaseChanged(int state)
{
    Q_UNUSED(state);
}

void SettingsDialog::onUseDigitsChanged(int state)
{
    Q_UNUSED(state);
}

void SettingsDialog::onUseSymbolsChanged(int state)
{
    Q_UNUSED(state);
}

void SettingsDialog::onExcludeAmbiguousChanged(int state)
{
    Q_UNUSED(state);
}
