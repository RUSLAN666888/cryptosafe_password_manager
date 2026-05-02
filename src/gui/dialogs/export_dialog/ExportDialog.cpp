#include "../src/gui/dialogs/export_dialog/ExportDialog.h"
#include "../src/database/DB_helper/db_helper.h"
#include <vector>
#include <sqlite3.h>

ExportDialog::ExportDialog(Database* db, QWidget *parent) : QDialog(parent){
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
    QGroupBox* previewGroup = new QGroupBox("Предпросмотр");
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);

    m_entriesCountLabel = new QLabel("Записей к экспорту: 0");
    m_previewLabel = new QLabel();
    m_previewLabel->setWordWrap(true);
    m_previewLabel->setStyleSheet("QLabel { background-color: #f5f5f5; padding: 10px; border-radius: 5px; }");

    previewLayout->addWidget(m_entriesCountLabel);
    previewLayout->addWidget(m_previewLabel);

    mainLayout->addWidget(previewGroup);

    // ===== Кнопки =====
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_exportButton = new QPushButton("Экспорт");
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

    // Обновляем счетчик
    m_entriesCountLabel->setText(QString("Записей к экспорту: %1").arg(count));

    // Подключаем сигнал изменения чекбокса для обновления счетчика
    connect(m_entriesTree, &QTreeWidget::itemChanged, this, &ExportDialog::onItemChanged);
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
