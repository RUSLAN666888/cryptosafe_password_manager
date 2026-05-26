// audit_log_viewer.cpp
#include "../src/gui/widgets/audit_log/audit_logViewer.h"
#include "../src/database/DB_helper/db_helper.h"
#include "../src/core/audit/log_formatter/log_formatter.h"
#include <QHeaderView>
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>

AuditLogViewer::AuditLogViewer(QWidget* parent) : QWidget(parent) {

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // ========== Панель фильтров ==========
    filtersGroup = new QGroupBox("Фильтры", this);
    QGridLayout* filtersLayout = new QGridLayout(filtersGroup);

    // Тип события
    QLabel* eventTypeLabel = new QLabel("Тип события:", filtersGroup);
    eventTypeCombo = new QComboBox(filtersGroup);
    eventTypeCombo->addItem("Все", "");
    eventTypeCombo->addItem("Entry Added", "0");
    eventTypeCombo->addItem("Entry Updated", "1");
    eventTypeCombo->addItem("Entry Deleted", "2");
    eventTypeCombo->addItem("User Logged In", "4");
    eventTypeCombo->addItem("Login Failure", "9");
    connect(eventTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AuditLogViewer::onFilterChanged);

    // Дата с
    QLabel* dateFromLabel = new QLabel("Дата с:", filtersGroup);
    dateFromEdit = new QDateEdit(filtersGroup);
    dateFromEdit->setCalendarPopup(true);
    dateFromEdit->setDate(QDate::currentDate().addDays(-30));
    connect(dateFromEdit, &QDateEdit::dateChanged, this, &AuditLogViewer::onFilterChanged);

    // Дата по
    QLabel* dateToLabel = new QLabel("Дата по:", filtersGroup);
    dateToEdit = new QDateEdit(filtersGroup);
    dateToEdit->setCalendarPopup(true);
    dateToEdit->setDate(QDate::currentDate());
    connect(dateToEdit, &QDateEdit::dateChanged, this, &AuditLogViewer::onFilterChanged);

    clearFiltersButton = new QPushButton("Сбросить", filtersGroup);
    connect(clearFiltersButton, &QPushButton::clicked, this, &AuditLogViewer::onClearFilters);

    // Поиск
    QLabel* searchLabel = new QLabel("Поиск:", filtersGroup);
    searchLineEdit = new QLineEdit(filtersGroup);
    searchLineEdit->setPlaceholderText("поиск по деталям...");
    connect(searchLineEdit, &QLineEdit::textChanged, this, &AuditLogViewer::onFilterChanged);

    filtersLayout->addWidget(eventTypeLabel, 0, 0);
    filtersLayout->addWidget(eventTypeCombo, 0, 1);
    filtersLayout->addWidget(dateFromLabel, 0, 2);
    filtersLayout->addWidget(dateFromEdit, 0, 3);
    filtersLayout->addWidget(dateToLabel, 0, 4);
    filtersLayout->addWidget(dateToEdit, 0, 5);
    filtersLayout->addWidget(clearFiltersButton, 0, 6);

    filtersLayout->addWidget(searchLabel, 1, 0);
    filtersLayout->addWidget(searchLineEdit, 1, 1, 1, 6);

    mainLayout->addWidget(filtersGroup);

    // ========== Таблица ==========
    m_model = new AuditLogModel();
    tableView = new QTableView(this);
    tableView->setModel(m_model);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView->setSortingEnabled(false);
    tableView->horizontalHeader()->setStretchLastSection(true);
    tableView->setColumnWidth(0, 60);
    tableView->setColumnWidth(1, 150);
    tableView->setColumnWidth(2, 150);
    tableView->setColumnWidth(3, 70);
    tableView->setColumnWidth(4, 60);
    tableView->setColumnWidth(5, 150);
    tableView->setColumnWidth(6, 70);
    connect(tableView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &AuditLogViewer::onRowSelected);

    mainLayout->addWidget(tableView, 3);

    // ========== Пагинация ==========
    QHBoxLayout* paginationLayout = new QHBoxLayout();
    paginationLayout->addStretch();

    QLabel* pageLabel = new QLabel("Страница:", this);
    paginationLayout->addWidget(pageLabel);

    pageSpinBox = new QSpinBox(this);
    pageSpinBox->setMinimum(1);
    pageSpinBox->setMaximum(1);
    pageSpinBox->setValue(1);
    // connect(pageSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &AuditLogViewer::onGoToPage);
    paginationLayout->addWidget(pageSpinBox);

    totalPagesLabel = new QLabel("из 1", this);
    paginationLayout->addWidget(totalPagesLabel);

    goToPageButton = new QPushButton("Перейти", this);
    connect(goToPageButton, &QPushButton::clicked, this, &AuditLogViewer::onGoToPage);
    paginationLayout->addWidget(goToPageButton);

    paginationLayout->addStretch();
    mainLayout->addLayout(paginationLayout);

    // ========== Панель деталей ==========
    QHBoxLayout* detailsLayout = new QHBoxLayout();

    QGroupBox* jsonBox = new QGroupBox("Детали записи (JSON)");
    QVBoxLayout* jsonLayout = new QVBoxLayout(jsonBox);
    m_detailsTextEdit = new QTextEdit();
    m_detailsTextEdit->setReadOnly(true);
    m_detailsTextEdit->setFont(QFont("Monospace", 9));
    jsonLayout->addWidget(m_detailsTextEdit);
    detailsLayout->addWidget(jsonBox, 2);

    QGroupBox* hashBox = new QGroupBox("Хеши");
    QVBoxLayout* hashLayout = new QVBoxLayout(hashBox);
    m_previousHashLabel = new QLabel("Предыдущий хеш:");
    m_previousHashLabel->setWordWrap(true);
    m_previousHashLabel->setFont(QFont("Monospace", 8));
    m_currentHashLabel = new QLabel("Текущий хеш:");
    m_currentHashLabel->setWordWrap(true);
    m_currentHashLabel->setFont(QFont("Monospace", 8));
    hashLayout->addWidget(m_previousHashLabel);
    hashLayout->addWidget(m_currentHashLabel);
    hashLayout->addStretch();
    detailsLayout->addWidget(hashBox, 1);

    mainLayout->addLayout(detailsLayout, 2);

    // ========== Экспорт/Импорт и проверка логов ==========
    QHBoxLayout* utilLayout = new QHBoxLayout();
    m_exportJSONbtn = new QPushButton("Экспорт JSON");
    connect(m_exportJSONbtn, &QPushButton::clicked, this, &AuditLogViewer::onExportJSON);
    utilLayout->addStretch();
    utilLayout->addWidget(m_exportJSONbtn);

    m_verifyLogbtn = new QPushButton("Верифицировать лог");
    connect(m_verifyLogbtn, &QPushButton::clicked, this, &AuditLogViewer::onVerifyIntegrity);
    utilLayout->addWidget(m_verifyLogbtn);

    m_exportCSVbtn = new QPushButton("Экспорт CSV");
    connect(m_exportCSVbtn, &QPushButton::clicked, this, &AuditLogViewer::onExportCSV);
    utilLayout->addWidget(m_exportCSVbtn);

    mainLayout->addLayout(utilLayout);

}

void AuditLogViewer::setDatabase(Database* db) {
    m_db = db;

    // QTimer* refreshTimer = new QTimer(this);
    // connect(refreshTimer, &QTimer::timeout, this, &AuditLogViewer::refreshLog);
    // refreshTimer->start(2000);

    loadPage();
}

void AuditLogViewer::onFilterChanged() {
    m_currentPage = 0;
    pageSpinBox->setValue(1);
    loadPage();
}

void AuditLogViewer::onClearFilters() {
    searchLineEdit->clear();
    eventTypeCombo->setCurrentIndex(0);
    severityCombo->setCurrentIndex(0);
    userIdCombo->setCurrentIndex(0);
    dateFromEdit->setDate(QDate::currentDate().addDays(-30));
    dateToEdit->setDate(QDate::currentDate());
}

void AuditLogViewer::onGoToPage() {
    m_currentPage = pageSpinBox->value() - 1;
    loadPage();
}

void AuditLogViewer::loadPage() {
    if (!m_db) return;

    std::string eventTypeFilter = eventTypeCombo->currentData().toString().toStdString();
    std::string severityFilter = "";//severityCombo->currentData().toString().toStdString();
    int userId = 1;//userIdCombo->currentData().toInt();
    std::string dateFrom = dateFromEdit->date().toString("yyyy-MM-dd").toStdString();
    std::string dateTo = dateToEdit->date().toString("yyyy-MM-dd").toStdString();
    std::string searchText = searchLineEdit->text().toStdString();

    int offset = m_currentPage * m_pageSize;


    int totalCount = m_db->getLogEntryCount();
    m_totalPages = (totalCount + m_pageSize - 1) / m_pageSize;
    updatePaginationControls();

    std::vector<AuditEntryDisplay> entries = m_db->getAuditPage(
        offset, m_pageSize, m_sortColumn, m_sortOrder,
        eventTypeFilter, dateFrom, dateTo, searchText
        );

    m_model->loadPage(entries);
}

void AuditLogViewer::updatePaginationControls() {
    pageSpinBox->setMaximum(m_totalPages);
    pageSpinBox->setValue(m_currentPage + 1);
    totalPagesLabel->setText(QString("из %1").arg(m_totalPages));
}

void AuditLogViewer::onRowSelected(const QModelIndex& current, const QModelIndex& previous) {
    if (!current.isValid()) return;

    AuditEntryDisplay entry = m_model->getEntry(current.row());

    // Показываем JSON
    if (!entry.entry_data.empty()) {
        try {
            json j = json::parse(entry.entry_data);
            m_detailsTextEdit->setPlainText(QString::fromStdString(j.dump(2)));
        } catch (...) {
            m_detailsTextEdit->setPlainText(QString::fromStdString(entry.entry_data));
        }
    } else {
        m_detailsTextEdit->clear();
    }

    // Показываем хеши
    m_previousHashLabel->setText(QString("Предыдущий хеш:\n%1").arg(QString::fromStdString(entry.previous_hash)));
    m_currentHashLabel->setText(QString("Текущий хеш:\n%1").arg(QString::fromStdString(entry.current_hash)));
}

void AuditLogViewer::onImportJSON(){
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

void AuditLogViewer::onVerifyIntegrity() {
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

void AuditLogViewer::onExportCSV() {
    QString filename = QFileDialog::getSaveFileName(this,
                                                    "Экспорт аудит-лога",
                                                    "audit_log.csv",
                                                    "CSV Files (*.csv)");

    if (filename.isEmpty())
        return;

    LogFormatter::getInstance().exportCSV(filename.toStdString());

    QMessageBox::information(this, "Экспорт", "Лог успешно экспортирован в CSV");
}


void AuditLogViewer::onExportJSON(){
    QString filename = QFileDialog::getSaveFileName(this,
                                                    "Экспорт аудит-лога (JSON)",
                                                    "audit_log.jsonl",
                                                    "JSON Lines (*.jsonl);;JSON Files (*.json)");

    if (filename.isEmpty())
        return;

    LogFormatter::getInstance().exportJSON(filename.toStdString());

    QMessageBox::information(this, "Экспорт JSON", "Лог успешно экспортирован в JSON");
}
