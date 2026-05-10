#include "import_dialog.h"
#include "import.h"
#include <QFile>
#include <QTextStream>
#include <QInputDialog>
#include <QComboBox>

ImportDialog::ImportDialog(VaultManager* vaultManager, Database* db, QWidget* parent)
    : QDialog(parent)
    , m_vaultManager(vaultManager)
    , m_db(db)
    , m_conflictAction(0)
    , m_applyToAllConflicts(false)
{
    setWindowTitle("Импорт хранилища");
    setMinimumSize(900, 700);
    setModal(true);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // ===== Выбор файла =====
    QGroupBox* fileGroup = new QGroupBox("Выбор файла");
    QHBoxLayout* fileLayout = new QHBoxLayout(fileGroup);

    m_fileEdit = new QLineEdit();
    m_fileEdit->setPlaceholderText("Путь к файлу для импорта...");
    m_fileEdit->setReadOnly(true);

    m_browseButton = new QPushButton("Обзор...");

    fileLayout->addWidget(m_fileEdit);
    fileLayout->addWidget(m_browseButton);

    mainLayout->addWidget(fileGroup);

    // ===== Информация о формате =====
    QHBoxLayout* infoLayout = new QHBoxLayout();
    infoLayout->addWidget(new QLabel("Определённый формат:"));
    m_formatLabel = new QLabel("—");
    m_formatLabel->setStyleSheet("font-weight: bold; color: #2c3e50;");
    infoLayout->addWidget(m_formatLabel);
    infoLayout->addStretch();
    mainLayout->addLayout(infoLayout);

    // ===== Предпросмотр записей =====
    QGroupBox* previewGroup = new QGroupBox("Предпросмотр импортируемых записей");
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);

    m_selectAllCheck = new QCheckBox("Выбрать все");
    connect(m_selectAllCheck, &QCheckBox::toggled, this, &ImportDialog::onSelectAll);

    m_previewTable = new QTableWidget();
    m_previewTable->setColumnCount(5);
    m_previewTable->setHorizontalHeaderLabels(QStringList()
                                              << "Импортировать" << "Название" << "Логин" << "Категория" << "Статус");
    m_previewTable->horizontalHeader()->setStretchLastSection(true);
    m_previewTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    previewLayout->addWidget(m_selectAllCheck);
    previewLayout->addWidget(m_previewTable);

    mainLayout->addWidget(previewGroup);

    // ===== Настройки конфликтов =====
    m_conflictGroup = new QGroupBox("Разрешение конфликтов (при дубликатах)");
    QVBoxLayout* conflictLayout = new QVBoxLayout(m_conflictGroup);

    m_conflictSkip = new QRadioButton("Пропустить (не импортировать)");
    m_conflictReplace = new QRadioButton("Заменить (удалить старую, добавить новую)");
    m_conflictAddNew = new QRadioButton("Добавить как новую (будет дубликат)");
    m_conflictUpdate = new QRadioButton("Обновить существующую запись");

    m_conflictSkip->setChecked(true);

    QButtonGroup* conflictGroup = new QButtonGroup(this);
    conflictGroup->addButton(m_conflictSkip, 0);
    conflictGroup->addButton(m_conflictReplace, 1);
    conflictGroup->addButton(m_conflictAddNew, 2);
    conflictGroup->addButton(m_conflictUpdate, 3);

    connect(conflictGroup, QOverload<int>::of(&QButtonGroup::buttonClicked),
            this, &ImportDialog::onConflictOptionChanged);

    m_applyToAll = new QCheckBox("Применить ко всем конфликтам");

    conflictLayout->addWidget(m_conflictSkip);
    conflictLayout->addWidget(m_conflictReplace);
    conflictLayout->addWidget(m_conflictAddNew);
    conflictLayout->addWidget(m_conflictUpdate);
    conflictLayout->addWidget(m_applyToAll);

    mainLayout->addWidget(m_conflictGroup);

    // ===== Статус и кнопки =====
    m_summaryLabel = new QLabel();
    m_summaryLabel->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 8px; border-radius: 4px; }");
    mainLayout->addWidget(m_summaryLabel);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_importButton = new QPushButton("Импортировать");
    m_cancelButton = new QPushButton("Отмена");

    m_importButton->setEnabled(false);

    buttonLayout->addWidget(m_importButton);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    // Подключаем сигналы
    connect(m_browseButton, &QPushButton::clicked, this, &ImportDialog::onBrowse);
    connect(m_importButton, &QPushButton::clicked, this, &ImportDialog::onImport);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    // Загружаем существующие записи для проверки дубликатов
    auto entries = m_vaultManager->getAllEntryMetadata();
    for (const auto& e : entries) {
        PlaintextEntry entry;
        entry.title = e.title;
        entry.username = e.username;
        m_existingEntries.push_back(entry);
    }
}

void ImportDialog::onBrowse()
{
    QString filter = "Все поддерживаемые файлы (*.cryptosafe *.json *.csv);;CryptoSafe Export (*.cryptosafe);;JSON Files (*.json);;CSV Files (*.csv)";
    QString filepath = QFileDialog::getOpenFileName(this, "Выберите файл для импорта", "", filter);

    if (!filepath.isEmpty()) {
        m_fileEdit->setText(filepath);
        onFileSelected(filepath);
    }
}

void ImportDialog::onFileSelected(const QString& filepath)
{
    m_currentFilepath = filepath;
    detectFormat(filepath);
    loadAndParseFile();
}

void ImportDialog::detectFormat(const QString& filepath)
{
    if (filepath.endsWith(".cryptosafe", Qt::CaseInsensitive)) {
        m_currentFormat = "Encrypted JSON (CryptoSafe)";
    } else if (filepath.endsWith(".json", Qt::CaseInsensitive)) {
        m_currentFormat = "JSON";
    } else if (filepath.endsWith(".csv", Qt::CaseInsensitive)) {
        m_currentFormat = "CSV";
    } else {
        // Пробуем определить по содержимому
        QFile file(filepath);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray firstLine = file.readLine(100);
            file.close();

            if (firstLine.contains("cryptosafe_export")) {
                m_currentFormat = "Encrypted JSON (CryptoSafe)";
            } else if (firstLine.contains("{")) {
                m_currentFormat = "JSON (предположительно)";
            } else if (firstLine.contains(",")) {
                m_currentFormat = "CSV (предположительно)";
            } else {
                m_currentFormat = "Неизвестный формат";
            }
        } else {
            m_currentFormat = "Ошибка чтения файла";
        }
    }

    m_formatLabel->setText(m_currentFormat);
}

void ImportDialog::loadAndParseFile()
{
    m_previewTable->setRowCount(0);
    m_importedEntries.clear();
    m_duplicateIndices.clear();

    Importer importer;
    ImportResult result;

    if (m_currentFormat.contains("Encrypted JSON") || m_currentFilepath.endsWith(".cryptosafe") || m_currentFilepath.endsWith(".json")) {
        // Запрашиваем пароль для расшифровки
        bool ok;
        QString password = QInputDialog::getText(this, "Пароль",
                                                 "Введите пароль для расшифровки:",
                                                 QLineEdit::Password,
                                                 "", &ok);
        if (!ok || password.isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Пароль не введён");
            return;
        }

        result = importer.importFromEncryptedJSON(m_currentFilepath, password.toStdString());

        // Очищаем пароль из памяти
        volatile char* p = const_cast<char*>(password.toStdString().data());
        for (size_t i = 0; i < password.length(); ++i) p[i] = 0;

    } else if (m_currentFormat.contains("CSV") || m_currentFilepath.endsWith(".csv")) {
        result = importer.importFromCSV(m_currentFilepath);

    } else {
        QMessageBox::warning(this, "Ошибка", "Неподдерживаемый формат файла");
        return;
    }

    if (!result.success) {
        QMessageBox::critical(this, "Ошибка импорта", QString::fromStdString(result.errorMessage));
        return;
    }

    m_importedEntries = result.entries;

    // Показываем сообщение о санитизации
    if (result.sanitizedCount > 0) {
        QMessageBox::information(this, "Санитизация",
                                 QString("Обнаружен потенциально вредоносный контент в %1 полях.\n"
                                         "Данные были очищены от опасных символов.")
                                     .arg(result.sanitizedCount));
    }

    showPreview();
}

void ImportDialog::showPreview()
{
    m_previewTable->clearContents();
    m_previewTable->setRowCount(static_cast<int>(m_importedEntries.size()));

    for (size_t i = 0; i < m_importedEntries.size(); ++i) {
        const auto& entry = m_importedEntries[i];

        // Чекбокс для выбора записи
        QTableWidgetItem* checkItem = new QTableWidgetItem();
        checkItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        checkItem->setCheckState(Qt::Checked);
        m_previewTable->setItem(static_cast<int>(i), 0, checkItem);

        // Название
        m_previewTable->setItem(static_cast<int>(i), 1,
                                new QTableWidgetItem(QString::fromStdString(entry.title)));

        // Логин
        m_previewTable->setItem(static_cast<int>(i), 2,
                                new QTableWidgetItem(QString::fromStdString(entry.username)));

        // Категория
        m_previewTable->setItem(static_cast<int>(i), 3,
                                new QTableWidgetItem(QString::fromStdString(entry.category)));

        // Статус (проверка дубликата)
        PlaintextEntry existing;
        if (isDuplicate(entry, &existing)) {
            m_previewTable->setItem(static_cast<int>(i), 4,
                                    new QTableWidgetItem("Дубликат"));
            m_duplicateIndices.push_back(static_cast<int>(i));
        } else {
            m_previewTable->setItem(static_cast<int>(i), 4,
                                    new QTableWidgetItem("Новая"));
        }
    }

    m_previewTable->resizeColumnsToContents();
    m_importButton->setEnabled(m_importedEntries.size() > 0);

    // Обновляем сводку
    int newCount = 0;
    int duplicateCount = 0;
    for (size_t i = 0; i < m_importedEntries.size(); ++i) {
        if (m_previewTable->item(static_cast<int>(i), 4)->text() == "Новая") {
            newCount++;
        } else {
            duplicateCount++;
        }
    }

    m_summaryLabel->setText(QString("Всего записей: %1 | Новых: %2 | Дубликатов: %3")
                                .arg(m_importedEntries.size()).arg(newCount).arg(duplicateCount));
}


bool ImportDialog::isDuplicate(const PlaintextEntry& entry, PlaintextEntry* existingEntry)
{
    for (const auto& existing : m_existingEntries) {
        if (existing.title == entry.title && existing.username == entry.username) {
            if (existingEntry) {
                *existingEntry = existing;
            }
            return true;
        }
    }
    return false;
}

std::vector<int> ImportDialog::getSelectedRows()
{
    std::vector<int> selected;
    for (int i = 0; i < m_previewTable->rowCount(); ++i) {
        QTableWidgetItem* item = m_previewTable->item(i, 0);
        if (item && item->checkState() == Qt::Checked) {
            selected.push_back(i);
        }
    }
    return selected;
}

void ImportDialog::onSelectAll(bool checked)
{
    for (int i = 0; i < m_previewTable->rowCount(); ++i) {
        QTableWidgetItem* item = m_previewTable->item(i, 0);
        if (item) {
            item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
        }
    }
}

void ImportDialog::onConflictOptionChanged()
{
    if (m_conflictSkip->isChecked()) m_conflictAction = 0;
    else if (m_conflictReplace->isChecked()) m_conflictAction = 1;
    else if (m_conflictAddNew->isChecked()) m_conflictAction = 2;
    else if (m_conflictUpdate->isChecked()) m_conflictAction = 3;
}

void ImportDialog::onImport()
{
    std::vector<int> selectedRows = getSelectedRows();

    if (selectedRows.empty()) {
        QMessageBox::warning(this, "Предупреждение", "Не выбрано ни одной записи для импорта");
        return;
    }

    int imported = 0;
    int skipped = 0;
    int updated = 0;
    int errors = 0;

    // Загружаем существующие записи для проверки дубликатов
    auto existingMetadata = m_vaultManager->getAllEntryMetadata();
    std::vector<PlaintextEntry> existingEntries;
    for (const auto& meta : existingMetadata) {
        PlaintextEntry entry;
        entry.title = meta.title;
        entry.username = meta.username;
        existingEntries.push_back(entry);
    }

    for (int row : selectedRows) {
        const auto& entry = m_importedEntries[row];

        // Проверяем дубликат
        bool isDuplicate = false;
        PlaintextEntry existingEntry;
        for (const auto& existing : existingEntries) {
            if (existing.title == entry.title && existing.username == entry.username) {
                isDuplicate = true;
                existingEntry = existing;
                break;
            }
        }

        int action = m_conflictAction;

        // Если конфликт и не применяем ко всем, показываем диалог
        if (isDuplicate && !m_applyToAll->isChecked()) {
            QDialog dialog(this);
            dialog.setWindowTitle("Конфликт при импорте");
            dialog.setMinimumWidth(400);

            QVBoxLayout* layout = new QVBoxLayout(&dialog);
            layout->addWidget(new QLabel(QString("Запись '%1' уже существует.\nЧто вы хотите сделать?")
                                             .arg(QString::fromStdString(entry.title))));

            QComboBox* combo = new QComboBox();
            combo->addItem("Пропустить (не импортировать)", 0);
            combo->addItem("Заменить (удалить старую)", 1);
            combo->addItem("Добавить как новую (дубликат)", 2);
            combo->addItem("Обновить существующую", 3);

            QCheckBox* applyToAll = new QCheckBox("Применить ко всем конфликтам");

            QHBoxLayout* btnLayout = new QHBoxLayout();
            QPushButton* okBtn = new QPushButton("OK");
            QPushButton* cancelBtn = new QPushButton("Отмена");
            btnLayout->addWidget(okBtn);
            btnLayout->addWidget(cancelBtn);

            layout->addWidget(combo);
            layout->addWidget(applyToAll);
            layout->addLayout(btnLayout);

            connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
            connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

            if (dialog.exec() != QDialog::Accepted) {
                skipped++;
                continue;
            }

            action = combo->currentData().toInt();
            if (applyToAll->isChecked()) {
                m_applyToAllConflicts = true;
                m_conflictAction = action;
            }
        }

        // Выполняем действие
        try {
            switch (action) {
            case 0: // Пропустить
                skipped++;
                break;

            case 1: // Заменить (удалить старую, добавить новую)
                // Находим ID существующей записи и удаляем
                for (const auto& meta : existingMetadata) {
                    if (meta.title == entry.title && meta.username == entry.username) {
                        m_vaultManager->deleteEntry(static_cast<int>(meta.id));
                        break;
                    }
                }
                m_vaultManager->createEntry(entry);
                imported++;
                break;

            case 2: // Добавить как новую
                m_vaultManager->createEntry(entry);
                imported++;
                break;

            case 3: // Обновить существующую
                for (const auto& meta : existingMetadata) {
                    if (meta.title == entry.title && meta.username == entry.username) {
                        m_vaultManager->updateEntry(static_cast<int>(meta.id), entry);
                        updated++;
                        break;
                    }
                }
                break;
            }
        } catch (const std::exception& e) {
            errors++;
            qWarning() << "Import error:" << e.what();
        }
    }

    // Логируем импорт в аудит
    // json logDetails = json::object();
    // logDetails["action"] = "import";
    // logDetails["format"] = m_currentFormat.toStdString();
    // logDetails["imported"] = imported;
    // logDetails["skipped"] = skipped;
    // logDetails["updated"] = updated;
    // logDetails["errors"] = errors;

    // EventBus::getInstance().publish(EventType::ImportCompleted, logDetails, "ImportDialog");

    showSummary(imported, skipped, updated, errors);

    // Обновляем список существующих записей для следующих импортов
    auto updatedMetadata = m_vaultManager->getAllEntryMetadata();
    m_existingEntries.clear();
    for (const auto& meta : updatedMetadata) {
        PlaintextEntry entry;
        entry.title = meta.title;
        entry.username = meta.username;
        m_existingEntries.push_back(entry);
    }
}
void ImportDialog::showSummary(int imported, int skipped, int updated, int errors)
{
    QString summary = QString("Импорт завершён!\n\n"
                              "Импортировано: %1\n"
                              "Пропущено: %2\n"
                              "Обновлено: %3\n"
                              "Ошибок: %4")
                          .arg(imported).arg(skipped).arg(updated).arg(errors);

    QMessageBox::information(this, "Результат импорта", summary);

    if (imported > 0 || updated > 0) {
        accept();
    }
}
