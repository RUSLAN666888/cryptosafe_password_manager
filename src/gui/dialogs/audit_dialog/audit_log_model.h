// // audit_log_model.h
// #ifndef AUDIT_LOG_MODEL_H
// #define AUDIT_LOG_MODEL_H

// #include <QAbstractTableModel>
// #include <vector>
// #include <string>
// #include <QDateTime>
// #include <QSortFilterProxyModel>
// #include "../database/DB_helper/db_helper.h"

// struct AuditEntryDisplay {
//     int sequence_number;
//     QString created_at;
//     QString event_type;
//     QString severity;
//     int user_id;
//     QString source;
//     int entry_id;
//     QString entry_data;
//     std::vector<uint8_t> signature;
//     QString previous_hash;
//     QString current_hash;
//     bool signature_valid;
//     int key_version;
// };

// class AuditLogModel : public QAbstractTableModel {
//     Q_OBJECT

// public:
//     explicit AuditLogModel(Database& db, QObject* parent = nullptr);

//     void refresh(const QString& eventType = "", const QString& severity = "",
//                  const QDate& dateFrom = QDate(), const QDate& dateTo = QDate(),
//                  const QString& searchText = "", int userId = -1);

//     QString eventTypeToString(int eventType);
//     QString getSeverityFromEventType(int eventType);

//     int rowCount(const QModelIndex& parent = QModelIndex()) const override;
//     int columnCount(const QModelIndex& parent = QModelIndex()) const override;
//     QVariant data(const QModelIndex& index, int role) const override;
//     QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

//     AuditEntryDisplay getEntry(int row) const;
//     bool verifySignature(int row);

// private:
//     void parseSeverity(const QString& eventType, QString& severity) const;

//     std::vector<AuditEntryDisplay> m_entries;
//     Database& m_db;
//     int m_currentPage;
//     int m_pageSize;
// };

// // Прокси модель для сортировки и фильтрации
// class AuditLogSortFilterProxyModel : public QSortFilterProxyModel {
//     Q_OBJECT

// public:
//     explicit AuditLogSortFilterProxyModel(QObject* parent = nullptr);

//     void setEventTypeFilter(const QString& type);
//     void setSeverityFilter(const QString& severity);
//     void setDateRangeFilter(const QDate& from, const QDate& to);
//     void setSearchTextFilter(const QString& text);
//     void setUserIdFilter(int userId);

// protected:
//     bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
//     bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

// private:
//     QString m_eventTypeFilter;
//     QString m_severityFilter;
//     QDate m_dateFromFilter;
//     QDate m_dateToFilter;
//     QString m_searchTextFilter;
//     int m_userIdFilter;
// };

// #endif
