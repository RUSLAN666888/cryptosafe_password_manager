#include "SettingsDialog.h"
#include "../src/core/clipboard_service/clipboard_service.h"
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
    createClipboardTab();
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

    loadClipboardSettings();
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

void SettingsDialog::createClipboardTab()
{
    QWidget* panel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(panel);

    QLabel* title = new QLabel("Настройки буфера обмена", panel);
    QFont titleFont = title->font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);

    layout->addSpacing(10);

    // === Уровень безопасности ===
    QGroupBox* securityGroup = new QGroupBox("Пресеты безопасности", panel);
    QVBoxLayout* securityLayout = new QVBoxLayout(securityGroup);

    m_securityLevel = new QComboBox(securityGroup);
    m_securityLevel->addItem("Пользовательские настройки");
    m_securityLevel->addItem("Basic (30 сек, уведомления выключены)");
    m_securityLevel->addItem("Advanced (15 сек, уведомления включены)");
    m_securityLevel->addItem("Paranoid (5 сек, уведомления включены)");

    securityLayout->addWidget(m_securityLevel);
    securityGroup->setLayout(securityLayout);
    layout->addWidget(securityGroup);

    layout->addSpacing(10);

    // === Настройки авто очистки ===
    QGroupBox* settingsGroup = new QGroupBox("Настройки авто очистки", panel);
    QFormLayout* formLayout = new QFormLayout(settingsGroup);

    m_clipboardTimeoutSpin = new QSpinBox(settingsGroup);
    m_clipboardTimeoutSpin->setRange(5, 300);
    m_clipboardTimeoutSpin->setSuffix(" seconds");
    m_clipboardTimeoutSpin->setValue(30);
    formLayout->addRow("Очищать буфер через:", m_clipboardTimeoutSpin);

    m_clipboardNeverClear = new QCheckBox("Не очищать буфер автоматически (не рекомендуется)", settingsGroup);
    formLayout->addRow("", m_clipboardNeverClear);

    connect(m_clipboardNeverClear, &QCheckBox::toggled, m_clipboardTimeoutSpin, &QSpinBox::setDisabled);

    settingsGroup->setLayout(formLayout);
    layout->addWidget(settingsGroup);

    layout->addSpacing(10);

    // === Настройки уведомлений ===
    QGroupBox* notificationGroup = new QGroupBox("Уведомления", panel);
    QVBoxLayout* notificationLayout = new QVBoxLayout(notificationGroup);

    m_notifyOnCopy = new QCheckBox("Показывать уведомление при копировании", notificationGroup);
    m_notifyOnWarning = new QCheckBox("Показывать предупреждение за 5 секунд до очистки", notificationGroup);
    m_notifyOnClear = new QCheckBox("Показывать уведомление при очистке", notificationGroup);

    notificationLayout->addWidget(m_notifyOnCopy);
    notificationLayout->addWidget(m_notifyOnWarning);
    notificationLayout->addWidget(m_notifyOnClear);

    notificationGroup->setLayout(notificationLayout);
    layout->addWidget(notificationGroup);

    layout->addStretch();

    // Пояснение
    QLabel* info = new QLabel(
        "Basic: минимальная защита, без уведомлений.\n"
        "Advanced: стандартная защита с уведомлениями.\n"
        "Paranoid: максимальная защита, быстрая очистка.\n"
        "Пользовательские настройки: используйте свои значения.",
        panel);
    info->setWordWrap(true);
    info->setStyleSheet("color: #666; font-size: 10px;");
    layout->addWidget(info);

    panel->setLayout(layout);
    tabWidget->addTab(panel, "Clipboard");

    // Подключаем сигнал изменения пресета
    connect(m_securityLevel, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onSecurityLevelChanged);
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
    // Сохраняем настройки
    savePasswordSettings();
    saveClipboardSettings();
    //ClipboardService::getInstance().loadSettings();

    QMessageBox::information(this, "CryptoSafe Manager",
                             "Настройки успешно сохранены.",
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

void SettingsDialog::onSecurityLevelChanged(int index)
{
    // Индекс 0 = Пользовательские настройки (не меняем)
    if (index == 0) return;

    // Basic
    if (index == 1) {
        m_clipboardTimeoutSpin->setValue(30);
        m_clipboardNeverClear->setChecked(false);
        m_notifyOnCopy->setChecked(false);
        m_notifyOnWarning->setChecked(false);
        m_notifyOnClear->setChecked(false);
    }
    // Advanced
    else if (index == 2) {
        m_clipboardTimeoutSpin->setValue(15);
        m_clipboardNeverClear->setChecked(false);
        m_notifyOnCopy->setChecked(true);
        m_notifyOnWarning->setChecked(true);
        m_notifyOnClear->setChecked(true);
    }
    // Paranoid
    else if (index == 3) {
        m_clipboardTimeoutSpin->setValue(5);
        m_clipboardNeverClear->setChecked(false);
        m_notifyOnCopy->setChecked(true);
        m_notifyOnWarning->setChecked(true);
        m_notifyOnClear->setChecked(true);
    }
}

void SettingsDialog::loadClipboardSettings()
{
    try {
        // Загружаем таймер
        std::string timeoutStr = m_db.getSetting("clipboard_timeout", "30");
        int timeout = std::stoi(timeoutStr);

        if (timeout == 0) {
            m_clipboardNeverClear->setChecked(true);
        } else {
            m_clipboardNeverClear->setChecked(false);
            m_clipboardTimeoutSpin->setValue(timeout);
        }

        // Загружаем уведомления
        m_notifyOnCopy->setChecked(m_db.getSetting("clipboard_notify_copy", "true") == "true");
        m_notifyOnWarning->setChecked(m_db.getSetting("clipboard_notify_warning", "true") == "true");
        m_notifyOnClear->setChecked(m_db.getSetting("clipboard_notify_clear", "true") == "true");

        // Загружаем пресет (всегда пользовательский при загрузке)
        m_securityLevel->setCurrentIndex(0);

    } catch (const std::exception& e) {
        m_clipboardNeverClear->setChecked(false);
        m_clipboardTimeoutSpin->setValue(30);
        m_notifyOnCopy->setChecked(true);
        m_notifyOnWarning->setChecked(true);
        m_notifyOnClear->setChecked(true);
        m_securityLevel->setCurrentIndex(0);
    }
}

void SettingsDialog::saveClipboardSettings()
{
    // Сохраняем таймер
    int timeout;
    if (m_clipboardNeverClear->isChecked()) {
        timeout = 0;
    } else {
        timeout = m_clipboardTimeoutSpin->value();
    }
    m_db.setSetting("clipboard_timeout", std::to_string(timeout));

    // Сохраняем уведомления
    m_db.setSetting("clipboard_notify_copy", m_notifyOnCopy->isChecked() ? "true" : "false");
    m_db.setSetting("clipboard_notify_warning", m_notifyOnWarning->isChecked() ? "true" : "false");
    m_db.setSetting("clipboard_notify_clear", m_notifyOnClear->isChecked() ? "true" : "false");

    // Сохраняем выбранный пресет (пользовательский = 0)
    m_db.setSetting("clipboard_security_level", std::to_string(m_securityLevel->currentIndex()));

    // Применяем к ClipboardService
    ClipboardService::getInstance().loadSettings();
    ClipboardService::getInstance().loadNotificationSettings();
}
