// audit_log_dialog.h
#ifndef AUDIT_LOG_DIALOG_H
#define AUDIT_LOG_DIALOG_H

#include <QDialog>
#include <QTableView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDateEdit>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QGroupBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QHeaderView>
#include <QMenu>
#include <QAction>
#include <QProgressDialog>

#include "audit_log_model.h"
#include "../database/DB_helper/db_helper.h"
#include "../core/audit/log_verifier/log_verifier.h"

class AuditLogDialog : public QDialog {
    Q_OBJECT

public:
    explicit AuditLogDialog(Database& db, QWidget* parent = nullptr);

private slots:
    void onRefresh();
    void onFilterChanged();
    void onRowSelected(const QModelIndex& current, const QModelIndex& previous);
    void onVerifyIntegrity();
    void onExportCSV();
    void onNextPage();
    void onPrevPage();
    void onContextMenu(const QPoint& pos);
    void onGoToVaultEntry();
    void onShowFailedLoginDetails();
    void onExportSignedJSON();
    void onImportJSON();

private:
    void setupUI();
    void loadLogs();
    void showEntryDetails(const AuditEntryDisplay& entry);
    void updateStatusBar();

    Database& m_db;
    AuditLogModel* m_model;
    AuditLogSortFilterProxyModel* m_proxyModel;
    QTableView* m_tableView;

    // Фильтры
    QLineEdit* m_searchEdit;
    QDateEdit* m_dateFromEdit;
    QDateEdit* m_dateToEdit;
    QComboBox* m_eventTypeCombo;
    QComboBox* m_severityCombo;
    QComboBox* m_userCombo;

    // Детали
    QTextEdit* m_detailsTextEdit;
    QLabel* m_statusLabel;
    QLabel* m_hashChainLabel;

    // Пагинация
    QPushButton* m_prevButton;
    QPushButton* m_nextButton;
    QLabel* m_pageLabel;
    int m_currentPage;
    int m_pageSize;

    QLabel* m_previousHashLabel;
    QLabel* m_currentHashLabel;

    // Текущая выбранная запись
    AuditEntryDisplay m_currentEntry;
};

#endif
