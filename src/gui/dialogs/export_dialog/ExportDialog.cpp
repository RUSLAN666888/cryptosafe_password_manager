#include "../src/gui/dialogs/export_dialog/ExportDialog.h"
#include "../src/database/DB_helper/db_helper.h"
#include "export.h"
#include <vector>
#include <sqlite3.h>
#include "authentication.h"

ExportDialog::ExportDialog(Database* db, VaultManager* vm, QWidget *parent) : m_vaultManager(vm),  QDialog(parent){
    m_db = db;

    setWindowTitle("Экспорт хранилища");
    setMinimumSize(700, 600);
    setModal(true);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QGroupBox* formatGroup = new QGroupBox("Формат экспорта");
    QVBoxLayout* formatLayout = new QVBoxLayout(formatGroup);

    m_encryptedJsonRadio = new QRadioButton("Encrypted JSON (native) - зашифрованный формат с метаданными");
    m_csvRadio = new QRadioButton("CSV (plaintext) - для миграции в другие менеджеры паролей");
    m_bitwardenRadio = new QRadioButton("Bitwarden JSON - совместимый с Bitwarden");
    m_lastpassRadio = new QRadioButton("LastPass CSV - совместимый с LastPass");

    m_encryptedJsonRadio->setChecked(true);

    formatLayout->addWidget(m_encryptedJsonRadio);
    formatLayout->addWidget(m_csvRadio);
    formatLayout->addWidget(m_bitwardenRadio);
    formatLayout->addWidget(m_lastpassRadio);

    mainLayout->addWidget(formatGroup);

    // ===== Выбор записей =====
    QGroupBox* entriesGroup = new QGroupBox("Выбор записей");
    QVBoxLayout* entriesLayout = new QVBoxLayout(entriesGroup);

    m_selectAllCheck = new QCheckBox("Выбрать все");
    m_entriesTree = new QTreeWidget();
    m_entriesTree->setHeaderLabels(QStringList() << "Название" << "Логин");
    m_entriesTree->setRootIsDecorated(false);

    entriesLayout->addWidget(m_selectAllCheck);
    entriesLayout->addWidget(m_entriesTree);

    mainLayout->addWidget(entriesGroup);

    // ===== Настройки шифрования =====
    m_encryptionGroup = new QGroupBox("Настройки шифрования");
    QVBoxLayout* encryptionLayout = new QVBoxLayout(m_encryptionGroup);

    m_aes128Radio = new QRadioButton("AES-128-GCM");
    m_aes256Radio = new QRadioButton("AES-256-GCM");
    m_aes256Radio->setChecked(true);

    encryptionLayout->addWidget(m_aes128Radio);
    encryptionLayout->addWidget(m_aes256Radio);

    mainLayout->addWidget(m_encryptionGroup);

    // ===== Предпросмотр =====
    // QGroupBox* previewGroup = new QGroupBox("Предпросмотр");
    // QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);

    // m_entriesCountLabel = new QLabel("Записей к экспорту: 0");
    // m_previewLabel = new QLabel();
    // m_previewLabel->setWordWrap(true);
    // m_previewLabel->setStyleSheet("QLabel { background-color: #f5f5f5; padding: 10px; border-radius: 5px; }");

    // previewLayout->addWidget(m_entriesCountLabel);
    // previewLayout->addWidget(m_previewLabel);

    // mainLayout->addWidget(previewGroup);

    // ===== Кнопки =====
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_exportButton = new QPushButton("Экспорт");
    connect(m_exportButton, &QPushButton::clicked, this, &ExportDialog::onExport);

    m_cancelButton = new QPushButton("Отмена");

    buttonLayout->addWidget(m_exportButton);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    loadPreview();
}

void ExportDialog::loadPreview()
{
    // Очищаем дерево
    m_entriesTree->clear();

    // Получаем соединение из пула
    sqlite3* conn = m_db->getConnection();
    if (!conn) return;

    // Запрос на получение rowid, title, username
    const char* sql = "SELECT rowid, title, username FROM vault_entries ORDER BY title";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(conn) << std::endl;
        m_db->releaseConnection(conn);
        return;
    }

    int count = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char* title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

        QTreeWidgetItem* item = new QTreeWidgetItem(m_entriesTree);

        // Поле 0: Title
        item->setText(0, QString::fromStdString(title ? title : ""));

        // Поле 1: Username
        item->setText(1, QString::fromStdString(username ? username : ""));

        // Сохраняем id (используем rowid как id)
        item->setData(0, Qt::UserRole, id);

        // Чекбокс
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(0, Qt::Checked);

        count++;
    }

    sqlite3_finalize(stmt);

    // Возвращаем соединение в пул
    m_db->releaseConnection(conn);
}

void ExportDialog::onItemChanged(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);

    // Подсчитываем выбранные записи
    int selectedCount = 0;
    for (int i = 0; i < m_entriesTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* it = m_entriesTree->topLevelItem(i);
        if (it->checkState(0) == Qt::Checked) {
            selectedCount++;
        }
    }

    m_entriesCountLabel->setText(QString("Записей к экспорту: %1").arg(selectedCount));

    // Обновляем чекбокс "Выбрать все"
    if (m_selectAllCheck) {
        m_selectAllCheck->blockSignals(true);
        if (selectedCount == m_entriesTree->topLevelItemCount()) {
            m_selectAllCheck->setCheckState(Qt::Checked);
        } else if (selectedCount == 0) {
            m_selectAllCheck->setCheckState(Qt::Unchecked);
        } else {
            m_selectAllCheck->setCheckState(Qt::PartiallyChecked);
        }
        m_selectAllCheck->blockSignals(false);
    }
}

void ExportDialog::onExport()
{
    // Проверяем, что выбраны записи
    std::vector<int> selectedIds = getSelectedEntryIds();
    if (selectedIds.empty()) {
        QMessageBox::warning(this, "Предупреждение", "Не выбрано ни одной записи для экспорта");
        return;
    }

    // Подтверждение мастер-пароля
    if (!confirmMasterPassword()) {
        return;
    }

    // Получаем выбранные записи из БД через VaultManager
    std::vector<PlaintextEntry> entries;
    for (int id : selectedIds) {
        auto entry = m_vaultManager->getEntry(id);
        if (entry) {
            entries.push_back(*entry);
        }
    }

    if (entries.empty()) {
        QMessageBox::warning(this, "Ошибка", "Не удалось загрузить выбранные записи");
        return;
    }

    // Проверяем формат и запрашиваем пароль для шифрования (если нужно)
    std::string exportPassword;
    bool needPassword = false;

    if (m_encryptedJsonRadio->isChecked() || m_bitwardenRadio->isChecked()) {
        needPassword = true;
    } else if (m_csvRadio->isChecked() || m_lastpassRadio->isChecked()) {
        needPassword = false;
    }

    if (needPassword) {
        bool ok;
        QString pwd = QInputDialog::getText(this, "Пароль экспорта",
                                            "Введите пароль для шифрования:",
                                            QLineEdit::Password,
                                            "", &ok);
        if (!ok || pwd.isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Пароль не может быть пустым");
            return;
        }
        exportPassword = pwd.toStdString();
    }

    // Выбираем файл для сохранения
    QString filepath;
    Exporter::EncryptionStrength strength = m_aes256Radio->isChecked()
                                                ? Exporter::EncryptionStrength::AES_256
                                                : Exporter::EncryptionStrength::AES_128;

    Exporter exporter;

    if (m_encryptedJsonRadio->isChecked()) {
        filepath = QFileDialog::getSaveFileName(this, "Сохранить как",
                                                "export.cryptosafe",
                                                "CryptoSafe Export (*.cryptosafe)");
        if (filepath.isEmpty()) return;

        exporter.exportToEncryptedJSON(entries, filepath.toStdString(), exportPassword, strength);
    }
    else if (m_bitwardenRadio->isChecked()) {
        filepath = QFileDialog::getSaveFileName(this, "Сохранить как",
                                                "bitwarden_export.json",
                                                "Bitwarden JSON (*.json)");
        if (filepath.isEmpty()) return;

        // Bitwarden export ВСЕГДА использует AES-256-GCM, игнорируем выбор пользователя
        exporter.exportToBitwardenEncryptedJSON(entries, filepath.toStdString(), exportPassword);
    }
    else if (m_csvRadio->isChecked()) {
        filepath = QFileDialog::getSaveFileName(this, "Сохранить как",
                                                "export.csv",
                                                "CSV Files (*.csv)");
        if (filepath.isEmpty()) return;

        exporter.exportToCSV(entries, filepath.toStdString());
    }
    else if (m_lastpassRadio->isChecked()) {
        filepath = QFileDialog::getSaveFileName(this, "Сохранить как",
                                                "lastpass_export.csv",
                                                "LastPass CSV (*.csv)");
        if (filepath.isEmpty()) return;

        exporter.exportToLastPassCSV(entries, filepath.toStdString());
    }

    QString formatName;
    if (m_encryptedJsonRadio->isChecked()) formatName = "Encrypted JSON";
    else if (m_bitwardenRadio->isChecked()) formatName = "Bitwarden JSON";
    else if (m_csvRadio->isChecked()) formatName = "CSV";
    else if (m_lastpassRadio->isChecked()) formatName = "LastPass CSV";

    QMessageBox::information(this, "Успех",
                             QString("Экспортировано %1 записей в формат %2")
                                 .arg(entries.size())
                                 .arg(formatName));
    accept();
}

std::vector<int> ExportDialog::getSelectedEntryIds()
{
    std::vector<int> ids;
    for (int i = 0; i < m_entriesTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_entriesTree->topLevelItem(i);
        if (item->checkState(0) == Qt::Checked) {
            int id = item->data(0, Qt::UserRole).toInt();
            ids.push_back(id);
        }
    }
    return ids;
}

bool ExportDialog::confirmMasterPassword()
{
    // Загружаем данные аутентификации из БД
    std::vector<uint8_t> hash, salt;
    uint32_t time_cost, memory_cost, parallelism, hash_len;

    if (!m_db->getAuthData(hash, salt, time_cost, memory_cost, parallelism, hash_len)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось загрузить данные аутентификации");
        return false;
    }

    Argon2Data authData(time_cost, memory_cost, parallelism, hash_len);
    authData.hash = std::move(hash);
    authData.salt = std::move(salt);

    // Создаём диалог подтверждения пароля
    LoginDialog confirmDialog(this, [authData](const std::string& password) -> bool {
        return verify_password(password, authData);
    });

    std::string masterPassword;
    if (!confirmDialog.exec(masterPassword)) {
        return false;
    }

    // Очищаем пароль
    volatile char* p = const_cast<char*>(masterPassword.data());
    for (size_t i = 0; i < masterPassword.size(); ++i) {
        p[i] = 0;
    }

    return true;
}
