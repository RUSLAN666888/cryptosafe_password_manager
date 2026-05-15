// share_dialog.cpp
#include "share_dialog.h"
#include <QDate>

// ShareDialog::ShareDialog (исправленный конструктор)
ShareDialog::ShareDialog(VaultManager* vaultManager, Database* db,
                         const PlaintextEntry& entry, QWidget* parent)
    : QDialog(parent)
    , m_vaultManager(vaultManager)
    , m_db(db)
    , m_entry(entry)
{
    setWindowTitle("Поделиться записью");
    setMinimumSize(500, 500);
    setModal(true);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QFormLayout* formLayout = new QFormLayout();

    // Информация о выбранной записи
    QString entryInfo = QString::fromStdString(entry.title + " (" + entry.username + ")");
    m_entryInfoLabel = new QLabel(entryInfo);
    m_entryInfoLabel->setStyleSheet("font-weight: bold; color: #2c3e50;");
    formLayout->addRow("Запись:", m_entryInfoLabel);

    // Отправитель
    m_sharerEdit = new QLineEdit();
    formLayout->addRow("Ваше имя:", m_sharerEdit);

    // Метод шифрования
    m_methodCombo = new QComboBox();
    m_methodCombo->addItem("Пароль", "password");
    m_methodCombo->addItem("Публичный ключ (RSA)", "public_key");
    formLayout->addRow("Метод шифрования:", m_methodCombo);
    connect(m_methodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ShareDialog::onMethodChanged);

    // Пароль
    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    formLayout->addRow("Пароль:", m_passwordEdit);

    // Контакт (для публичного ключа)
    m_contactCombo = new QComboBox();
    formLayout->addRow("Контакт:", m_contactCombo);
    connect(m_contactCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ShareDialog::onContactSelected);

    // Информация о выбранном ключе
    m_keyInfoLabel = new QLabel("Контакт не выбран");
    m_keyInfoLabel->setStyleSheet("color: gray;");
    formLayout->addRow("", m_keyInfoLabel);

    // Срок действия
    m_expirationDate = new QDateEdit();
    m_expirationDate->setDate(QDate::currentDate().addDays(7));
    m_expirationDate->setMinimumDate(QDate::currentDate().addDays(1));
    m_expirationDate->setMaximumDate(QDate::currentDate().addDays(30));
    formLayout->addRow("Действителен до:", m_expirationDate);

    // Права
    m_permissionsCombo = new QComboBox();
    m_permissionsCombo->addItem("Только чтение", "read_only");
    m_permissionsCombo->addItem("Чтение и запись", "read_write");
    formLayout->addRow("Права:", m_permissionsCombo);

    mainLayout->addLayout(formLayout);

    // Кнопки
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_createButton = new QPushButton("Создать share");
    m_cancelButton = new QPushButton("Отмена");

    buttonLayout->addWidget(m_createButton);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    connect(m_createButton, &QPushButton::clicked, this, &ShareDialog::onCreateShare);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    onMethodChanged();
    loadContacts();
}

void ShareDialog::onMethodChanged()
{
    QString method = m_methodCombo->currentData().toString();
    bool isPassword = (method == "password");

    m_passwordEdit->setVisible(isPassword);
    m_contactCombo->setVisible(!isPassword);
    m_keyInfoLabel->setVisible(!isPassword);

    // Обновляем текст меток
    QFormLayout* form = qobject_cast<QFormLayout*>(layout()->itemAt(0)->layout());
    if (!form) return;

    QLabel* passwordLabel = qobject_cast<QLabel*>(form->labelForField(m_passwordEdit));
    QLabel* contactLabel = qobject_cast<QLabel*>(form->labelForField(m_contactCombo));

    if (passwordLabel) passwordLabel->setText(isPassword ? "Пароль:" : "");
    if (contactLabel) contactLabel->setText(!isPassword ? "Контакт:" : "");
}

void ShareDialog::loadContacts()
{
    m_contactCombo->clear();
    auto contacts = m_db->getAllContacts();
    for (const auto& contact : contacts) {
        m_contactCombo->addItem(QString::fromStdString(contact.second), contact.first);
    }
    if (m_contactCombo->count() > 0) {
        m_contactCombo->setCurrentIndex(0);
    } else {
        m_keyInfoLabel->setText("Нет контактов. Сначала импортируйте публичный ключ.");
        m_keyInfoLabel->setStyleSheet("color: orange;");
    }
}

void ShareDialog::onContactSelected(int index)
{
    if (index < 0) return;
    int contactId = m_contactCombo->itemData(index).toInt();
    std::vector<uint8_t> publicKey;
    if (m_db->getContactPublicKeyById(contactId, publicKey)) {
        m_selectedPublicKey = publicKey;
        m_keyInfoLabel->setText(QString::fromStdString("Ключ загружен: " + m_contactCombo->currentText().toStdString()));
        m_keyInfoLabel->setStyleSheet("color: green;");
    } else {
        m_keyInfoLabel->setText("Ошибка загрузки ключа");
        m_keyInfoLabel->setStyleSheet("color: red;");
    }
}

void ShareDialog::onCreateShare()
{
    // Проверяем имя отправителя
    QString sharer = m_sharerEdit->text().trimmed();
    if (sharer.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите ваше имя");
        return;
    }

    // Выбираем файл для сохранения
    QString filename = QFileDialog::getSaveFileName(this, "Сохранить share-файл",
                                                    QString::fromStdString(m_entry.title) + ".cryptoshare",
                                                    "CryptoSafe Share (*.cryptoshare)");
    if (filename.isEmpty()) return;

    // Параметры
    int days = QDate::currentDate().daysTo(m_expirationDate->date());
    if (days < 1) days = 1;
    if (days > 30) days = 30;

    std::string permissions = m_permissionsCombo->currentData().toString().toStdString();
    QString method = m_methodCombo->currentData().toString();

    try {
        SharingService& service = SharingService::getInstance();

        if (method == "password") {
            QString password = m_passwordEdit->text();
            if (password.isEmpty()) {
                QMessageBox::warning(this, "Ошибка", "Введите пароль");
                return;
            }

            service.shareWithPassword(m_entry, password.toStdString(),
                                      sharer.toStdString(), days,
                                      permissions, filename.toStdString());

            // Очищаем пароль
            volatile char* p = const_cast<char*>(password.toStdString().data());
            for (size_t i = 0; i < password.length(); ++i) p[i] = 0;

        } else {
            if (m_selectedPublicKey.empty()) {
                QMessageBox::warning(this, "Ошибка", "Выберите контакт с публичным ключом");
                return;
            }

            service.shareWithPublicKey(m_entry, m_selectedPublicKey,
                                       sharer.toStdString(), days,
                                       permissions, filename.toStdString());
        }

        QMessageBox::information(this, "Успех", "Share-файл успешно создан");
        accept();

    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Ошибка", e.what());
    }
}
