#include "SettingsDialog.h"
#include "../src/core/clipboard_service/clipboard_service.h"
#include <QMessageBox>
#include <QApplication>
#include <QScreen>

SettingsDialog::SettingsDialog(Database& db, QWidget *parent, ConfigHander &cfg)
    : QDialog(parent)
    , config(cfg)
    , m_db(db)
{
    setWindowTitle("Настройки");
    setMinimumSize(500, 450);
    setModal(true);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    tabWidget = new QTabWidget(this);

    createPasswordGeneratorTab();
    createClipboardTab();

    mainLayout->addWidget(tabWidget);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    QPushButton *okButton = new QPushButton("OK", this);
    QPushButton *cancelButton = new QPushButton("Отмена", this);

    buttonLayout->addWidget(okButton);
    buttonLayout->addSpacing(10);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addSpacing(20);

    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);

    connect(okButton, &QPushButton::clicked, this, &SettingsDialog::onOk);
    connect(cancelButton, &QPushButton::clicked, this, &SettingsDialog::onCancel);

    adjustSize();
    QRect screenGeometry = QApplication::primaryScreen()->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);

    loadPasswordSettings();
    loadClipboardSettings();
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::createPasswordGeneratorTab()
{
    QWidget *panel = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(panel);

    QLabel *title = new QLabel("Настройки генератора паролей", panel);
    QFont titleFont = title->font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);

    layout->addSpacing(10);

    QGroupBox *settingsGroup = new QGroupBox("Параметры генерации", panel);
    QFormLayout *formLayout = new QFormLayout(settingsGroup);
    formLayout->setSpacing(10);
    formLayout->setContentsMargins(15, 15, 15, 15);

    m_passwordLengthSpin = new QSpinBox(settingsGroup);
    m_passwordLengthSpin->setRange(8, 64);
    m_passwordLengthSpin->setSuffix(" символов");
    m_passwordLengthSpin->setValue(16);
    formLayout->addRow("Длина пароля:", m_passwordLengthSpin);

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

    m_excludeAmbiguousCheck = new QCheckBox("Исключить неоднозначные символы (l, I, 1, 0, O)", settingsGroup);
    m_excludeAmbiguousCheck->setChecked(true);
    formLayout->addRow("", m_excludeAmbiguousCheck);

    settingsGroup->setLayout(formLayout);
    layout->addWidget(settingsGroup);

    QLabel *infoLabel = new QLabel(
        "Эти настройки будут использоваться при генерации паролей в диалоге добавления и редактирования записей.",
        panel);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #666; font-size: 10px; margin-top: 10px;");
    layout->addWidget(infoLabel);

    layout->addStretch();
    panel->setLayout(layout);
    tabWidget->addTab(panel, "Генератор паролей");
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
    m_securityLevel->addItem("Базовый (30 сек, уведомления выключены)");
    m_securityLevel->addItem("Продвинутый (15 сек, уведомления включены)");
    m_securityLevel->addItem("Параноидальный (5 сек, уведомления включены)");

    securityLayout->addWidget(m_securityLevel);
    securityGroup->setLayout(securityLayout);
    layout->addWidget(securityGroup);

    layout->addSpacing(10);

    // === Настройки авто очистки ===
    QGroupBox* settingsGroup = new QGroupBox("Автоматическая очистка", panel);
    QFormLayout* formLayout = new QFormLayout(settingsGroup);
    formLayout->setSpacing(10);
    formLayout->setContentsMargins(15, 15, 15, 15);

    m_clipboardTimeoutSpin = new QSpinBox(settingsGroup);
    m_clipboardTimeoutSpin->setRange(5, 300);
    m_clipboardTimeoutSpin->setSuffix(" секунд");
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
    notificationLayout->setSpacing(8);
    notificationLayout->setContentsMargins(15, 15, 15, 15);

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
        "Базовый: минимальная защита, без уведомлений.\n"
        "Продвинутый: стандартная защита с уведомлениями.\n"
        "Параноидальный: максимальная защита, быстрая очистка.\n"
        "Пользовательские настройки: используйте свои значения.",
        panel);
    info->setWordWrap(true);
    info->setStyleSheet("color: #666; font-size: 10px; margin-top: 10px;");
    layout->addWidget(info);

    panel->setLayout(layout);
    tabWidget->addTab(panel, "Буфер обмена");

    connect(m_securityLevel, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onSecurityLevelChanged);
}

void SettingsDialog::onOk()
{
    savePasswordSettings();
    saveClipboardSettings();

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
    try {
        std::string lenStr = m_db.getSetting("password_length", "16");
        m_passwordLengthSpin->setValue(std::stoi(lenStr));

        m_useUppercaseCheck->setChecked(m_db.getSetting("password_use_uppercase", "true") == "true");
        m_useLowercaseCheck->setChecked(m_db.getSetting("password_use_lowercase", "true") == "true");
        m_useDigitsCheck->setChecked(m_db.getSetting("password_use_digits", "true") == "true");
        m_useSymbolsCheck->setChecked(m_db.getSetting("password_use_symbols", "true") == "true");
        m_excludeAmbiguousCheck->setChecked(m_db.getSetting("password_exclude_ambiguous", "true") == "true");
    } catch (const std::exception& e) {
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
    m_db.setSetting("password_length", std::to_string(m_passwordLengthSpin->value()));
    m_db.setSetting("password_use_uppercase", m_useUppercaseCheck->isChecked() ? "true" : "false");
    m_db.setSetting("password_use_lowercase", m_useLowercaseCheck->isChecked() ? "true" : "false");
    m_db.setSetting("password_use_digits", m_useDigitsCheck->isChecked() ? "true" : "false");
    m_db.setSetting("password_use_symbols", m_useSymbolsCheck->isChecked() ? "true" : "false");
    m_db.setSetting("password_exclude_ambiguous", m_excludeAmbiguousCheck->isChecked() ? "true" : "false");
}

void SettingsDialog::loadClipboardSettings()
{
    try {
        std::string timeoutStr = m_db.getSetting("clipboard_timeout", "30");
        int timeout = std::stoi(timeoutStr);

        if (timeout == 0) {
            m_clipboardNeverClear->setChecked(true);
        } else {
            m_clipboardNeverClear->setChecked(false);
            m_clipboardTimeoutSpin->setValue(timeout);
        }

        m_notifyOnCopy->setChecked(m_db.getSetting("clipboard_notify_copy", "true") == "true");
        m_notifyOnWarning->setChecked(m_db.getSetting("clipboard_notify_warning", "true") == "true");
        m_notifyOnClear->setChecked(m_db.getSetting("clipboard_notify_clear", "true") == "true");

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
    int timeout;
    if (m_clipboardNeverClear->isChecked()) {
        timeout = 0;
    } else {
        timeout = m_clipboardTimeoutSpin->value();
    }
    m_db.setSetting("clipboard_timeout", std::to_string(timeout));

    m_db.setSetting("clipboard_notify_copy", m_notifyOnCopy->isChecked() ? "true" : "false");
    m_db.setSetting("clipboard_notify_warning", m_notifyOnWarning->isChecked() ? "true" : "false");
    m_db.setSetting("clipboard_notify_clear", m_notifyOnClear->isChecked() ? "true" : "false");
    m_db.setSetting("clipboard_security_level", std::to_string(m_securityLevel->currentIndex()));

    ClipboardService::getInstance().loadSettings();
    ClipboardService::getInstance().loadNotificationSettings();
}

void SettingsDialog::onSecurityLevelChanged(int index)
{
    if (index == 0) return;

    if (index == 1) {
        m_clipboardTimeoutSpin->setValue(30);
        m_clipboardNeverClear->setChecked(false);
        m_notifyOnCopy->setChecked(false);
        m_notifyOnWarning->setChecked(false);
        m_notifyOnClear->setChecked(false);
    }
    else if (index == 2) {
        m_clipboardTimeoutSpin->setValue(15);
        m_clipboardNeverClear->setChecked(false);
        m_notifyOnCopy->setChecked(true);
        m_notifyOnWarning->setChecked(true);
        m_notifyOnClear->setChecked(true);
    }
    else if (index == 3) {
        m_clipboardTimeoutSpin->setValue(5);
        m_clipboardNeverClear->setChecked(false);
        m_notifyOnCopy->setChecked(true);
        m_notifyOnWarning->setChecked(true);
        m_notifyOnClear->setChecked(true);
    }
}
