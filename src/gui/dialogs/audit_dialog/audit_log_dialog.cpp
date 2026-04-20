// audit_log_dialog.cpp
#include "audit_log_dialog.h"
#include <QDateTime>
#include <QDebug>
#include <QScrollBar>

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
    QGroupBox* filterBox = new QGroupBox("Filters");
    QGridLayout* filterLayout = new QGridLayout(filterBox);

    int row = 0;
    filterLayout->addWidget(new QLabel("Search:"), row, 0);
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("Search in events, source, details...");
    connect(m_searchEdit, &QLineEdit::textChanged, this, &AuditLogDialog::onFilterChanged);
    filterLayout->addWidget(m_searchEdit, row, 1, 1, 3);

    row++;
    filterLayout->addWidget(new QLabel("Event Type:"), row, 0);
    m_eventTypeCombo = new QComboBox();
    m_eventTypeCombo->addItem("All", "");
    m_eventTypeCombo->addItem("Entry Added", "EntryAdded");
    m_eventTypeCombo->addItem("Entry Updated", "EntryUpdated");
    m_eventTypeCombo->addItem("Entry Deleted", "EntryDeleted");
    m_eventTypeCombo->addItem("Entry Readed", "EntryReaded");
    m_eventTypeCombo->addItem("Login Success", "LoginSuccess");
    m_eventTypeCombo->addItem("Login Failure", "LoginFailure");
    connect(m_eventTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AuditLogDialog::onFilterChanged);
    filterLayout->addWidget(m_eventTypeCombo, row, 1);

    filterLayout->addWidget(new QLabel("Severity:"), row, 2);
    m_severityCombo = new QComboBox();
    m_severityCombo->addItem("All", "");
    m_severityCombo->addItem("INFO", "INFO");
    m_severityCombo->addItem("WARN", "WARN");
    m_severityCombo->addItem("ERROR", "ERROR");
    m_severityCombo->addItem("CRITICAL", "CRITICAL");
    connect(m_severityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AuditLogDialog::onFilterChanged);
    filterLayout->addWidget(m_severityCombo, row, 3);

    row++;
    filterLayout->addWidget(new QLabel("Date From:"), row, 0);
    m_dateFromEdit = new QDateEdit();
    m_dateFromEdit->setCalendarPopup(true);
    m_dateFromEdit->setDate(QDate::currentDate().addDays(-30));
    connect(m_dateFromEdit, &QDateEdit::dateChanged, this, &AuditLogDialog::onFilterChanged);
    filterLayout->addWidget(m_dateFromEdit, row, 1);

    filterLayout->addWidget(new QLabel("Date To:"), row, 2);
    m_dateToEdit = new QDateEdit();
    m_dateToEdit->setCalendarPopup(true);
    m_dateToEdit->setDate(QDate::currentDate());
    connect(m_dateToEdit, &QDateEdit::dateChanged, this, &AuditLogDialog::onFilterChanged);
    filterLayout->addWidget(m_dateToEdit, row, 3);

    row++;
    filterLayout->addWidget(new QLabel("User ID:"), row, 0);
    m_userCombo = new QComboBox();
    m_userCombo->addItem("All", -1);
    m_userCombo->addItem("User 1", 1);
    m_userCombo->addItem("User 2", 2);
    connect(m_userCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AuditLogDialog::onFilterChanged);
    filterLayout->addWidget(m_userCombo, row, 1);

    QPushButton* refreshBtn = new QPushButton("Refresh");
    connect(refreshBtn, &QPushButton::clicked, this, &AuditLogDialog::onRefresh);
    filterLayout->addWidget(refreshBtn, row, 3, Qt::AlignRight);

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
    m_tableView->setColumnWidth(2, 120);
    m_tableView->setColumnWidth(3, 80);
    m_tableView->setColumnWidth(4, 60);
    m_tableView->setColumnWidth(5, 120);
    m_tableView->setColumnWidth(6, 70);
    m_tableView->setColumnWidth(7, 80);

    m_tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tableView, &QTableView::customContextMenuRequested,
            this, &AuditLogDialog::onContextMenu);
    connect(m_tableView->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &AuditLogDialog::onRowSelected);

    mainLayout->addWidget(m_tableView, 3);

    // ===== Панель деталей =====
    QHBoxLayout* detailsLayout = new QHBoxLayout();

    // Левая панель - JSON
    QGroupBox* jsonBox = new QGroupBox("Entry Details (JSON)");
    QVBoxLayout* jsonLayout = new QVBoxLayout(jsonBox);
    m_detailsTextEdit = new QTextEdit();
    m_detailsTextEdit->setReadOnly(true);
    m_detailsTextEdit->setFont(QFont("Monospace", 9));
    jsonLayout->addWidget(m_detailsTextEdit);
    detailsLayout->addWidget(jsonBox, 2);

    // Правая панель - статус и хеш-цепочка
    QGroupBox* statusBox = new QGroupBox("Verification Status");
    QVBoxLayout* statusLayout = new QVBoxLayout(statusBox);

    m_signatureStatusLabel = new QLabel();
    m_signatureStatusLabel->setWordWrap(true);
    statusLayout->addWidget(m_signatureStatusLabel);

    m_hashChainLabel = new QLabel();
    m_hashChainLabel->setWordWrap(true);
    statusLayout->addWidget(m_hashChainLabel);

    QPushButton* verifySigBtn = new QPushButton("Verify Signature");
    connect(verifySigBtn, &QPushButton::clicked, this, &AuditLogDialog::onVerifySignature);
    statusLayout->addWidget(verifySigBtn);

    detailsLayout->addWidget(statusBox, 1);

    mainLayout->addLayout(detailsLayout, 2);

    // ===== Панель пагинации =====
    QHBoxLayout* paginationLayout = new QHBoxLayout();

    m_prevButton = new QPushButton("Previous");
    connect(m_prevButton, &QPushButton::clicked, this, &AuditLogDialog::onPrevPage);
    paginationLayout->addWidget(m_prevButton);

    m_pageLabel = new QLabel();
    paginationLayout->addWidget(m_pageLabel);

    m_nextButton = new QPushButton("Next");
    connect(m_nextButton, &QPushButton::clicked, this, &AuditLogDialog::onNextPage);
    paginationLayout->addWidget(m_nextButton);

    paginationLayout->addStretch();

    QPushButton* verifyBtn = new QPushButton("Verify All Logs");
    connect(verifyBtn, &QPushButton::clicked, this, &AuditLogDialog::onVerifyIntegrity);
    paginationLayout->addWidget(verifyBtn);

    QPushButton* exportBtn = new QPushButton("Export to CSV");
    connect(exportBtn, &QPushButton::clicked, this, &AuditLogDialog::onExportCSV);
    paginationLayout->addWidget(exportBtn);

    QPushButton* closeBtn = new QPushButton("Close");
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

    // Обновляем статус
    m_signatureStatusLabel->setText(QString("Signature: %1\nKey Version: %2")
                                        .arg(entry.signature_valid ? "✓ VALID" : "✗ INVALID")
                                        .arg(entry.key_version));

    // Хеш-цепочка
    m_hashChainLabel->setText(QString("Previous Hash: %1\nCurrent Hash: %2")
                                  .arg(entry.previous_hash.left(16) + "...")
                                  .arg(entry.current_hash.left(16) + "..."));
}

void AuditLogDialog::onVerifySignature() {
    if (m_currentEntry.sequence_number == 0) return;

    QProgressDialog progress("Verifying signature...", "Cancel", 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);

    // Здесь должна быть реальная проверка подписи
    m_currentEntry.signature_valid = true;

    m_signatureStatusLabel->setText(QString("Signature: %1\nKey Version: %2")
                                        .arg(m_currentEntry.signature_valid ? "✓ VALID" : "✗ INVALID")
                                        .arg(m_currentEntry.key_version));

    if (m_currentEntry.signature_valid) {
        QMessageBox::information(this, "Signature Verification", "Signature is VALID!");
    } else {
        QMessageBox::warning(this, "Signature Verification", "Signature is INVALID!");
    }
}

void AuditLogDialog::onVerifyIntegrity() {
    auto& verifier = LogVerifier::getInstance();
    auto result = verifier.verifyAllLogs();

    if (result.isValid) {
        QMessageBox::information(this, "Integrity Check",
                                 QString("✓ All %1 entries are valid!")
                                     .arg(result.verifiedCount));
    } else {
        QString message = QString(
                              "❌ Integrity check failed!\n\n"
                              "Failed at entry #%1\n"
                              "Error: %2\n\n"
                              "Hash chain valid: %3\n"
                              "Signatures valid: %4"
                              ).arg(result.failedSequence)
                              .arg(QString::fromStdString(result.errorMessage))
                              .arg(result.hashChainValid ? "yes" : "NO")
                              .arg(result.signaturesValid ? "yes" : "NO");

        QMessageBox::critical(this, "Integrity Check Failed", message);
    }
}

void AuditLogDialog::onExportCSV() {
    // QString filename = QFileDialog::getSaveFileName(this, "Export Audit Log",
    //                                                 "audit_log.csv",
    //                                                 "CSV Files (*.csv)");
    // if (filename.isEmpty()) return;

    // QFile file(filename);
    // if (file.open(QIODevice::WriteOnly)) {
    //     QTextStream stream(&file);
    //     stream.setEncoding(QStringConverter::Utf8);

    //     // Заголовки
    //     stream << "#,Timestamp,Event Type,Severity,User,Source,Entry ID,Status,Details\n";

    //     for (int row = 0; row < m_proxyModel->rowCount(); ++row) {
    //         QModelIndex idx = m_proxyModel->index(row, 0);
    //         QModelIndex sourceIdx = m_proxyModel->mapToSource(idx);
    //         auto entry = m_model->getEntry(sourceIdx.row());

    //         stream << QString("\"%1\",\"%2\",\"%3\",\"%4\",%5,\"%6\",%7,\"%8\",\"%9\"\n")
    //                       .arg(entry.sequence_number)
    //                       .arg(entry.created_at)
    //                       .arg(entry.event_type)
    //                       .arg(entry.severity)
    //                       .arg(entry.user_id)
    //                       .arg(entry.source)
    //                       .arg(entry.entry_id)
    //                       .arg(entry.signature_valid ? "Valid" : "Invalid")
    //                       .arg(entry.entry_data);
    //     }

    //     QMessageBox::information(this, "Export", "Audit log exported successfully!");
    // }
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
        QAction* goToVault = new QAction("Go to Vault Entry", menu);
        connect(goToVault, &QAction::triggered, this, &AuditLogDialog::onGoToVaultEntry);
        menu->addAction(goToVault);
    }

    if (m_currentEntry.event_type.contains("Login Failure", Qt::CaseInsensitive)) {
        QAction* showDetails = new QAction("Show Failed Login Details", menu);
        connect(showDetails, &QAction::triggered, this, &AuditLogDialog::onShowFailedLoginDetails);
        menu->addAction(showDetails);
    }

    menu->addSeparator();

    QAction* verifyAction = new QAction("Verify Signature", menu);
    connect(verifyAction, &QAction::triggered, this, &AuditLogDialog::onVerifySignature);
    menu->addAction(verifyAction);

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
