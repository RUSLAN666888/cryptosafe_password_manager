#ifndef AUDIT_LOG_H
#define AUDIT_LOG_H

#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QTextEdit>
#include <QDateEdit>
#include <QLineEdit>
#include <QTableView>
#include <QSplitter>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <vector>
#include <QSpinBox>
#include <QAbstractTableModel>

#include "../src/core/LogEntry.h"
#include "../src/database/DB_helper/db_helper.h"


class AuditLogModel : public QAbstractTableModel {
    Q_OBJECT
public:


    void loadPage(const std::vector<AuditEntryDisplay>& entries) {
        beginResetModel();
        m_entries = entries;
        endResetModel();
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        return parent.isValid() ? 0 : m_entries.size();
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const override {
        return parent.isValid() ? 0 : 7;
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (!index.isValid() || index.row() >= m_entries.size())
            return QVariant();

        const auto& entry = m_entries[index.row()];

        if (role == Qt::DisplayRole) {
            switch (index.column()) {
            case 0: return entry.sequence_number;
            case 1: return QString::fromStdString(entry.created_at);
            case 2: return QString::fromStdString(entry.event_type);
            case 3: return QString::fromStdString(entry.severity);
            case 4: return entry.user_id;
            case 5: return QString::fromStdString(entry.source);
            case 6: return entry.entry_id;
            default: return QVariant();
            }
        }

        if (role == Qt::ForegroundRole) {
            if (entry.severity == "ERROR") return QColor(255, 165, 0);
            if (entry.severity == "CRITICAL") return QColor(255, 0, 0);
            if (entry.severity == "WARN") return QColor(255, 140, 0);
        }

        return QVariant();
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
            return QVariant();

        switch (section) {
        case 0: return "#";
        case 1: return "Дата/Время";
        case 2: return "Тип события";
        case 3: return "Уровень";
        case 4: return "User ID";
        case 5: return "Источник";
        case 6: return "Entry ID";
        default: return QVariant();
        }
    }

    AuditEntryDisplay getEntry(int row) const {
        if (row >= 0 && row < m_entries.size())
            return m_entries[row];
        return AuditEntryDisplay{};
    }

private:
    std::vector<AuditEntryDisplay> m_entries;
};

class AuditLogViewer : public QWidget{

    Q_OBJECT

private slots:
    void onFilterChanged();
    void onClearFilters();
    void onGoToPage();
    void onRowSelected(const QModelIndex& current, const QModelIndex& previous);
    void onImportJSON();
    void onVerifyIntegrity();
    void onExportCSV();
    void onExportJSON();

private:
    void loadPage();
    void updatePaginationControls();

    // Фильтры
    QGroupBox* filtersGroup;
    QLineEdit* searchLineEdit;
    QComboBox* eventTypeCombo;
    QComboBox* severityCombo;
    QComboBox* userIdCombo;
    QDateEdit* dateFromEdit;
    QDateEdit* dateToEdit;
    QPushButton* clearFiltersButton;

    // Таблица
    QTableView* tableView;
    AuditLogModel* m_model;

    // Пагинация
    QSpinBox* pageSpinBox;
    QLabel* totalPagesLabel;
    QPushButton* goToPageButton;
    int m_currentPage = 0;
    int m_pageSize = 50;
    int m_totalPages = 1;

    // Детали
    QTextEdit* m_detailsTextEdit;
    QLabel* m_previousHashLabel;
    QLabel* m_currentHashLabel;

    // Сортировка
    std::string m_sortColumn = "sequence_number";
    bool m_sortOrder = false;  // DESC


    QPushButton* m_exportJSONbtn;
    QPushButton* m_verifyLogbtn;
    QPushButton* m_exportCSVbtn;

    Database* m_db = nullptr;

public:
    AuditLogViewer(QWidget* parent = nullptr);
    void setDatabase(Database* db);
};




#endif // AUDIT_LOG_H
