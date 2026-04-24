// audit_log_dialog.cpp
#include "audit_log_dialog.h"
#include <QDateTime>
#include <QDebug>
#include <QScrollBar>

#include <iostream>
#include <fstream>
#include <QString>

#include "../src/core/audit/log_formatter/log_formatter.h"

using namespace std;

AuditLogDialog::AuditLogDialog(Database& db, QWidget* parent)
    : QDialog(parent), m_db(db), m_currentPage(0), m_pageSize(50)
{
    setWindowTitle("Audit Log Viewer");
    resize(1200, 800);
    setupUI();
    loadLogs();
}

void AuditLogDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // ===== Панель фильтров =====
    QGroupBox* filterBox = new QGroupBox("Фильтры");
    QGridLayout* filterLayout = new QGridLayout(filterBox);

    int row = 0;
    filterLayout->addWidget(new QLabel("Поиск:"), row, 0);
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("Поиск по событиям, источникам, деталям...");
    connect(m_searchEdit, &QLineEdit::textChanged, this, &AuditLogDialog::onFilterChanged);
    filterLayout->addWidget(m_searchEdit, row, 1, 1, 3);

    row++;
    filterLayout->addWidget(new QLabel("Тип события:"), row, 0);
    m_eventTypeCombo = new QComboBox();
    m_eventTypeCombo->addItem("Все", "");
    m_eventTypeCombo->addItem("Добавление записи", "EntryAdded");
    m_eventTypeCombo->addItem("Обновление записи", "EntryUpdated");
    m_eventTypeCombo->addItem("Удаление записи", "EntryDeleted");
    m_eventTypeCombo->addItem("Просмотр записи", "EntryReaded");
    m_eventTypeCombo->addItem("Успешный вход", "LoginSuccess");
    m_eventTypeCombo->addItem("Ошибка входа", "LoginFailure");
    connect(m_eventTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AuditLogDialog::onFilterChanged);
    filterLayout->addWidget(m_eventTypeCombo, row, 1);

    filterLayout->addWidget(new QLabel("Важность:"), row, 2);
    m_severityCombo = new QComboBox();
    m_severityCombo->addItem("Все", "");
    m_severityCombo->addItem("INFO", "INFO");
    m_severityCombo->addItem("WARN", "WARN");
    m_severityCombo->addItem("ERROR", "ERROR");
    m_severityCombo->addItem("CRITICAL", "CRITICAL");
    connect(m_severityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AuditLogDialog::onFilterChanged);
    filterLayout->addWidget(m_severityCombo, row, 3);

    row++;
    filterLayout->addWidget(new QLabel("Дата с:"), row, 0);
    m_dateFromEdit = new QDateEdit();
    m_dateFromEdit->setCalendarPopup(true);
    m_dateFromEdit->setDate(QDate::currentDate().addDays(-30));
    connect(m_dateFromEdit, &QDateEdit::dateChanged, this, &AuditLogDialog::onFilterChanged);
    filterLayout->addWidget(m_dateFromEdit, row, 1);

    filterLayout->addWidget(new QLabel("Дата по:"), row, 2);
    m_dateToEdit = new QDateEdit();
    m_dateToEdit->setCalendarPopup(true);
    m_dateToEdit->setDate(QDate::currentDate());
    connect(m_dateToEdit, &QDateEdit::dateChanged, this, &AuditLogDialog::onFilterChanged);
    filterLayout->addWidget(m_dateToEdit, row, 3);

    row++;
    filterLayout->addWidget(new QLabel("ID пользователя:"), row, 0);
    m_userCombo = new QComboBox();
    m_userCombo->addItem("Все", -1);
    m_userCombo->addItem("Пользователь 1", 1);
    m_userCombo->addItem("Пользователь 2", 2);
    connect(m_userCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AuditLogDialog::onFilterChanged);
    filterLayout->addWidget(m_userCombo, row, 1);

    mainLayout->addWidget(filterBox);

    // ===== Таблица =====
    m_model = new AuditLogModel(m_db, this);
    m_proxyModel = new AuditLogSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);

    m_tableView = new QTableView();
    m_tableView->setModel(m_proxyModel);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setSortingEnabled(true);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->setColumnWidth(0, 60);
    m_tableView->setColumnWidth(1, 150);
    m_tableView->setColumnWidth(2, 150);
    m_tableView->setColumnWidth(3, 80);
    m_tableView->setColumnWidth(4, 60);
    m_tableView->setColumnWidth(5, 120);
    m_tableView->setColumnWidth(6, 70);

    m_tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tableView, &QTableView::customContextMenuRequested,
            this, &AuditLogDialog::onContextMenu);
    connect(m_tableView->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &AuditLogDialog::onRowSelected);

    mainLayout->addWidget(m_tableView, 3);

    // ===== Панель деталей =====
    QHBoxLayout* detailsLayout = new QHBoxLayout();

    // Левая панель - JSON
    QGroupBox* jsonBox = new QGroupBox("Детали записи (JSON)");
    QVBoxLayout* jsonLayout = new QVBoxLayout(jsonBox);
    m_detailsTextEdit = new QTextEdit();
    m_detailsTextEdit->setReadOnly(true);
    m_detailsTextEdit->setFont(QFont("Monospace", 9));
    jsonLayout->addWidget(m_detailsTextEdit);
    detailsLayout->addWidget(jsonBox, 2);

    // Правая панель - хеши
    QGroupBox* hashBox = new QGroupBox("Хеши");
    QVBoxLayout* hashLayout = new QVBoxLayout(hashBox);

    m_previousHashLabel = new QLabel();
    m_previousHashLabel->setWordWrap(true);
    m_previousHashLabel->setFont(QFont("Monospace", 8));
    hashLayout->addWidget(m_previousHashLabel);

    m_currentHashLabel = new QLabel();
    m_currentHashLabel->setWordWrap(true);
    m_currentHashLabel->setFont(QFont("Monospace", 8));
    hashLayout->addWidget(m_currentHashLabel);

    hashLayout->addStretch();

    detailsLayout->addWidget(hashBox, 1);

    mainLayout->addLayout(detailsLayout, 2);

    // ===== Панель пагинации =====
    QHBoxLayout* paginationLayout = new QHBoxLayout();

    m_prevButton = new QPushButton("Назад");
    connect(m_prevButton, &QPushButton::clicked, this, &AuditLogDialog::onPrevPage);
    paginationLayout->addWidget(m_prevButton);

    m_pageLabel = new QLabel();
    paginationLayout->addWidget(m_pageLabel);

    m_nextButton = new QPushButton("Вперед");
    connect(m_nextButton, &QPushButton::clicked, this, &AuditLogDialog::onNextPage);
    paginationLayout->addWidget(m_nextButton);

    paginationLayout->addStretch();

    QPushButton* verifyBtn = new QPushButton("Проверить целостность");
    connect(verifyBtn, &QPushButton::clicked, this, &AuditLogDialog::onVerifyIntegrity);
    paginationLayout->addWidget(verifyBtn);

    QPushButton* exportBtn = new QPushButton("Экспорт в CSV");
    connect(exportBtn, &QPushButton::clicked, this, &AuditLogDialog::onExportCSV);
    paginationLayout->addWidget(exportBtn);

    QPushButton* exportJSONBtn = new QPushButton("Экспорт JSON");
    connect(exportJSONBtn, &QPushButton::clicked, this, &AuditLogDialog::onExportSignedJSON);
    paginationLayout->addWidget(exportJSONBtn);

    QPushButton* importJSONBtn = new QPushButton("Импорт JSON");
    connect(importJSONBtn, &QPushButton::clicked, this, &AuditLogDialog::onImportJSON);
    paginationLayout->addWidget(importJSONBtn);

    QPushButton* closeBtn = new QPushButton("Закрыть");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    paginationLayout->addWidget(closeBtn);

    mainLayout->addLayout(paginationLayout);

    // Статус бар
    m_statusLabel = new QLabel();
    mainLayout->addWidget(m_statusLabel);
}

void AuditLogDialog::loadLogs() {
    QString eventType = m_eventTypeCombo->currentData().toString();
    QString severity = m_severityCombo->currentData().toString();
    QDate dateFrom = m_dateFromEdit->date();
    QDate dateTo = m_dateToEdit->date();
    QString searchText = m_searchEdit->text();
    int userId = m_userCombo->currentData().toInt();

    m_model->refresh(eventType, severity, dateFrom, dateTo, searchText, userId);

    m_proxyModel->setEventTypeFilter(eventType);
    m_proxyModel->setSeverityFilter(severity);
    m_proxyModel->setDateRangeFilter(dateFrom, dateTo);
    m_proxyModel->setSearchTextFilter(searchText);
    m_proxyModel->setUserIdFilter(userId);

    updateStatusBar();
}

void AuditLogDialog::updateStatusBar() {
    m_statusLabel->setText(QString("Showing %1 entries").arg(m_proxyModel->rowCount()));

    int totalCount = m_db.getLogEntryCount();
    int totalPages = (totalCount + m_pageSize - 1) / m_pageSize;
    m_pageLabel->setText(QString("Page %1 of %2").arg(m_currentPage + 1).arg(totalPages));
    m_prevButton->setEnabled(m_currentPage > 0);
    m_nextButton->setEnabled(m_currentPage < totalPages - 1);
}

void AuditLogDialog::onRefresh() {
    loadLogs();
}

void AuditLogDialog::onFilterChanged() {
    m_currentPage = 0;
    loadLogs();
}

void AuditLogDialog::onNextPage() {
    m_currentPage++;
    loadLogs();
}

void AuditLogDialog::onPrevPage() {
    if (m_currentPage > 0) {
        m_currentPage--;
        loadLogs();
    }
}

void AuditLogDialog::onRowSelected(const QModelIndex& current, const QModelIndex& previous) {
    if (!current.isValid()) return;

    QModelIndex sourceIndex = m_proxyModel->mapToSource(current);
    m_currentEntry = m_model->getEntry(sourceIndex.row());
    showEntryDetails(m_currentEntry);
}

void AuditLogDialog::showEntryDetails(const AuditEntryDisplay& entry) {
    // Форматируем JSON
    try {
        json j = json::parse(entry.entry_data.toStdString());
        QString formatted = QString::fromStdString(j.dump(2));
        m_detailsTextEdit->setPlainText(formatted);
    } catch (...) {
        m_detailsTextEdit->setPlainText(entry.entry_data);
    }

    // Показываем хеши
    m_previousHashLabel->setText(QString("Предыдущий хеш:\n%1").arg(entry.previous_hash));
    m_currentHashLabel->setText(QString("Текущий хеш:\n%1").arg(entry.current_hash));
}



void AuditLogDialog::onVerifyIntegrity() {
    auto& verifier = LogVerifier::getInstance();
    auto result = verifier.verifyAllLogs();

    if (result.isValid) {
        QMessageBox::information(this, "Проверка целостности",
                                 QString("Все %1 записей валидны!\n\n"
                                         "Цепочка хешей: валидна\n"
                                         "Подписи: валидны")
                                     .arg(result.verifiedCount));
    } else {
        QString tamperedList;
        if (!result.tamperedEntries.empty()) {
            QStringList entries;
            for (int seq : result.tamperedEntries) {
                entries << QString::number(seq);
            }
            tamperedList = QString("\n\nПовреждённые записи: %1").arg(entries.join(", "));
        }

        QString message = QString(
                              "ПРОВЕРКА ЦЕЛОСТНОСТИ НЕ ПРОЙДЕНА!\n\n"
                              "Статус: %1\n"
                              "Проверено записей: %2\n"
                              "Ошибка в записи: #%3\n"
                              "Ошибка: %4\n\n"
                              "┌─────────────────────────────────────┐\n"
                              "│ Цепочка хешей: %5\n"
                              "│ Подписи: %6\n"
                              "└─────────────────────────────────────┘%7"
                              )
                              .arg(result.isValid ? "Валиден" : "НЕ ВАЛИДЕН")
                              .arg(result.verifiedCount)
                              .arg(result.failedSequence)
                              .arg(QString::fromStdString(result.errorMessage))
                              .arg(result.hashChainValid ? "валидна" : "НЕ ВАЛИДНА")
                              .arg(result.signaturesValid ? "валидны" : "НЕ ВАЛИДНЫ")
                              .arg(tamperedList);

        QMessageBox::critical(this, "Проверка целостности", message);
    }
}

void AuditLogDialog::onExportCSV() {
    QString filename = QFileDialog::getSaveFileName(this,
                                                    "Экспорт аудит-лога",
                                                    "audit_log.csv",
                                                    "CSV Files (*.csv)");

    if (filename.isEmpty()) return;

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось создать файл");
        return;
    }

    QByteArray data;

    // Заголовки (преобразуем QString в UTF-8)
    data.append("#,Дата и время,Тип события,Важность,Пользователь,Источник,ID записи,Детали\n");

    for (int row = 0; row < m_proxyModel->rowCount(); ++row) {
        QModelIndex idx = m_proxyModel->index(row, 0);
        QModelIndex sourceIdx = m_proxyModel->mapToSource(idx);
        auto entry = m_model->getEntry(sourceIdx.row());

        QString line = QString("%1,%2,%3,%4,%5,%6,%7,\"%8\"\n")
                           .arg(entry.sequence_number)
                           .arg(entry.created_at)
                           .arg(entry.event_type)
                           .arg(entry.severity)
                           .arg(entry.user_id)
                           .arg(entry.source)
                           .arg(entry.entry_id == 0 ? "-" : QString::number(entry.entry_id))
                           .arg(entry.entry_data.replace("\"", "\"\""));

        // Преобразуем строку в UTF-8 байты
        data.append(line.toUtf8());
    }

    file.write(data);
    file.close();

    QMessageBox::information(this, "Экспорт", "Лог успешно экспортирован в CSV");
}

void AuditLogDialog::onContextMenu(const QPoint& pos) {
    QModelIndex index = m_tableView->indexAt(pos);
    if (!index.isValid()) return;

    QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
    m_currentEntry = m_model->getEntry(sourceIndex.row());

    QMenu* menu = new QMenu(this);

    // Проверяем тип события
    if (m_currentEntry.event_type.contains("Entry", Qt::CaseInsensitive) &&
        m_currentEntry.entry_id != 0) {
        QAction* goToVault = new QAction("Перейти к записи", menu);
        connect(goToVault, &QAction::triggered, this, &AuditLogDialog::onGoToVaultEntry);
        menu->addAction(goToVault);
    }

    if (m_currentEntry.event_type.contains("Login Failure", Qt::CaseInsensitive)) {
        QAction* showDetails = new QAction("Показать детали неудачного входа", menu);
        connect(showDetails, &QAction::triggered, this, &AuditLogDialog::onShowFailedLoginDetails);
        menu->addAction(showDetails);
    }

    menu->exec(m_tableView->viewport()->mapToGlobal(pos));
    delete menu;
}

void AuditLogDialog::onGoToVaultEntry() {
    // // Эмитируем сигнал или вызываем метод MainWindow для перехода к записи
    // emit goToVaultEntry(m_currentEntry.entry_id);
    // close();
}

void AuditLogDialog::onShowFailedLoginDetails() {
    try {
        json j = json::parse(m_currentEntry.entry_data.toStdString());
        QString details = QString("Failed Login Details:\n\n");

        if (j.contains("details")) {
            auto& d = j["details"];
            if (d.contains("ip")) details += QString("IP Address: %1\n").arg(QString::fromStdString(d["ip"]));
            if (d.contains("username")) details += QString("Username: %1\n").arg(QString::fromStdString(d["username"]));
            if (d.contains("attempts")) details += QString("Attempts: %1\n").arg(d["attempts"].get<int>());
        }

        details += QString("\nTimestamp: %1\n").arg(m_currentEntry.created_at);
        details += QString("Source: %1\n").arg(m_currentEntry.source);

        QMessageBox::information(this, "Failed Login Details", details);
    } catch (...) {
        QMessageBox::warning(this, "Failed Login Details", "Could not parse login details.");
    }
}

void AuditLogDialog::onExportSignedJSON() {
    QString filename = QFileDialog::getSaveFileName(this,
                                                    "Экспорт аудит-лога (JSON)",
                                                    "audit_log.jsonl",
                                                    "JSON Lines (*.jsonl);;JSON Files (*.json)");

    if (filename.isEmpty())
        return;

    ofstream out(filename.toStdString());

    stringstream ss;

    json metadata;
    metadata["export_timestamp"] = getUTCTimestamp();
    metadata["exporter"] = "CryptoSafe Manager";
    metadata["entry_count"] = m_proxyModel->rowCount();
    metadata["algorithm"] = "Ed25519";

    ss << metadata.dump() << "\n";
    out.write(ss.str().c_str(), ss.str().size());

    ss.str("");

    int totalCount = m_db.getLogEntryCount();

    for (int seq = 1; seq <= totalCount; ++seq){
        std::string previous_hash, current_hash, entry_data, created_at, event_type_str;
        std::vector<uint8_t> signature;
        int key_version;

        // Получаем запись из БД по sequence_number
        if (!m_db.getLogEntry(seq, previous_hash, current_hash, entry_data,
                              signature, key_version, created_at, event_type_str)) {
            qWarning() << "Failed to get log entry for sequence:" << seq;
            continue;
        }

        // Получаем публичный ключ для этой записи
        std::vector<uint8_t> publicKey;
        m_db.getPublicKeyForSequence(seq, publicKey, key_version);

        // Преобразуем публичный ключ в hex
        std::stringstream pubKeySs;
        for (uint8_t byte : publicKey) {
            pubKeySs << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
        }

        json entryJson = json::parse(entry_data);

        json exportEntry;

        exportEntry["user_id"] = entryJson["user_id"];

        exportEntry["event_type"] = entryJson["event_type"];

        exportEntry["source"] = entryJson["source"];

        exportEntry["timestamp"] = entryJson["timestamp"];
        exportEntry["severity"] = entryJson["severity"];

        exportEntry["entry_id"] = entryJson["entry_id"];

        exportEntry["details"] = json::parse(entryJson["details"].dump());

        exportEntry["previous_hash"] = previous_hash;
        exportEntry["current_hash"] = current_hash;

        std::stringstream sigSs;
        for (uint8_t byte : signature) {
            sigSs << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
        }

        exportEntry["signature"] = sigSs.str();

        exportEntry["public_key"] = pubKeySs.str();

        ss << exportEntry.dump() << "\n";
        out.write(ss.str().c_str(), ss.str().size());

        ss.str("");
    }


    out.close();

    QMessageBox::information(this, "Экспорт",
                             QString("Лог успешно экспортирован в JSON Lines\n"
                                     "Записей: %1\n"
                                     "Формат: каждая запись на отдельной строке")
                                 .arg(m_proxyModel->rowCount()));
}


void AuditLogDialog::onImportJSON(){
    auto& formatter = LogFormatter::getInstance();

    QString filePath = QFileDialog::getOpenFileName(this, "Выберите файл");

    if (filePath.isEmpty())
        return;

    LogFormatter::ImportResult r = formatter.importJSON(filePath.toStdString());

    if (r.isValid){
        QMessageBox::information(this, "Импорт", QString("Лог валиден"));
    }
    else{
        QMessageBox::information(this, "Импорт", QString("Лог невалиден.\nПричина: %1").arg(QString::fromStdString(r.msg)));
    }
}
