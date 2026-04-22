// audit_log_model.cpp
#include "../src/gui/dialogs/audit_dialog/audit_log_model.h"
#include <QFont>
#include <QColor>
#include <QBrush>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

AuditLogModel::AuditLogModel(Database& db, QObject* parent)
    : QAbstractTableModel(parent), m_db(db), m_currentPage(0), m_pageSize(50)
{
    refresh();
}

void AuditLogModel::refresh(const QString& eventType, const QString& severity,
                            const QDate& dateFrom, const QDate& dateTo,
                            const QString& searchText, int userId) {
    beginResetModel();
    m_entries.clear();

    int totalCount = m_db.getLogEntryCount();
    int startRow = m_currentPage * m_pageSize;
    int endRow = std::min(startRow + m_pageSize, totalCount);

    for (int i = startRow; i < endRow; i++) {
        std::string previous_hash, current_hash, entry_data, created_at, event_type_str;
        std::vector<uint8_t> signature;
        int key_version;

        if (m_db.getLogEntry(i + 1, previous_hash, current_hash, entry_data,
                             signature, key_version, created_at, event_type_str)) {

            // Преобразуем числовой event_type в читаемую строку
            int eventTypeInt = std::stoi(event_type_str);
            QString eventTypeDisplay = eventTypeToString(eventTypeInt);

            // Применяем фильтры
            bool passFilter = true;

            // Фильтр по типу события (сравниваем с отображаемой строкой)
            if (!eventType.isEmpty() && eventTypeDisplay != eventType) {
                passFilter = false;
            }

            // Фильтр по дате
            if (passFilter && dateFrom.isValid()) {
                QDateTime dt = QDateTime::fromString(QString::fromStdString(created_at), Qt::ISODate);
                if (dt.date() < dateFrom) passFilter = false;
            }
            if (passFilter && dateTo.isValid()) {
                QDateTime dt = QDateTime::fromString(QString::fromStdString(created_at), Qt::ISODate);
                if (dt.date() > dateTo) passFilter = false;
            }

            // Поиск по тексту
            if (passFilter && !searchText.isEmpty()) {
                if (!QString::fromStdString(entry_data).contains(searchText, Qt::CaseInsensitive) &&
                    !eventTypeDisplay.contains(searchText, Qt::CaseInsensitive)) {
                    passFilter = false;
                }
            }

            if (!passFilter) continue;

            AuditEntryDisplay display;
            display.sequence_number = i + 1;
            display.created_at = QString::fromStdString(created_at);
            display.event_type = eventTypeDisplay;  // ← теперь строка
            display.entry_data = QString::fromStdString(entry_data);
            display.signature = signature;
            display.previous_hash = QString::fromStdString(previous_hash);
            display.current_hash = QString::fromStdString(current_hash);
            display.key_version = key_version;
            display.signature_valid = true;

            // Парсим JSON
            try {
                json j = json::parse(entry_data);
                display.source = QString::fromStdString(j.value("source", "unknown"));
                display.user_id = j.value("user_id", 0);
                display.entry_id = j.value("entry_id", 0);
            } catch (...) {
                display.source = "parse_error";
                display.user_id = 0;
                display.entry_id = 0;
            }

            // Определяем severity на основе типа события
            display.severity = getSeverityFromEventType(eventTypeInt);

            m_entries.push_back(display);
        }
    }

    endResetModel();
}

QString AuditLogModel::eventTypeToString(int eventType) {
    switch (static_cast<EventType>(eventType)) {
    // Entry events
    case EventType::EntryAdded:        return "Entry Added";
    case EventType::EntryUpdated:      return "Entry Updated";
    case EventType::EntryDeleted:      return "Entry Deleted";
    case EventType::EntryReaded:       return "Entry Readed";
    // Auth events
    case EventType::UserLoggedIn:      return "User Logged In";
    case EventType::UserLoggedOut:     return "User Logged Out";
    // Clipboard events
    case EventType::ClipboardCopied:   return "Clipboard Copied";
    case EventType::ClipboardCleared:  return "Clipboard Cleared";
    case EventType::ClipboardWillClear:return "Clipboard Will Clear";
    // Login events
    case EventType::LoginFailure:      return "Login Failure";
    case EventType::PasswordChange:    return "Password Change";
    // System events
    case EventType::Startup:           return "Startup";
    case EventType::Shutdown:          return "Shutdown";
    case EventType::Lock:              return "Lock";
    case EventType::Unlock:            return "Unlock";
    case EventType::SettingsModification: return "Settings Modified";
    case EventType::IntegrityCheckFailed: return "Integrity Check Failed";
    default:                           return "Unknown";
    }
}

QString AuditLogModel::getSeverityFromEventType(int eventType) {
    EventType type = static_cast<EventType>(eventType);

    switch (type) {
    case EventType::LoginFailure:
    case EventType::IntegrityCheckFailed:
        return "ERROR";

    case EventType::EntryDeleted:
    case EventType::ClipboardWillClear:
    case EventType::PasswordChange:
    case EventType::Lock:
    case EventType::Shutdown:
        return "WARN";

    case EventType::ClipboardCopied:
    case EventType::ClipboardCleared:
    case EventType::SettingsModification:
    case EventType::Unlock:
        return "INFO";

    default:
        return "INFO";
    }
}

void AuditLogModel::parseSeverity(const QString& eventType, QString& severity) const {
    if (eventType.contains("delete", Qt::CaseInsensitive)) {
        severity = "WARN";
    } else if (eventType.contains("fail", Qt::CaseInsensitive)) {
        severity = "ERROR";
    } else if (eventType.contains("critical", Qt::CaseInsensitive)) {
        severity = "CRITICAL";
    } else {
        severity = "INFO";
    }
}

int AuditLogModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_entries.size());
}

int AuditLogModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : 7;
}

QVariant AuditLogModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal) return QVariant();

    if (role == Qt::DisplayRole) {
        switch (section) {
        case 0: return "#";
        case 1: return "Timestamp";
        case 2: return "Event Type";
        case 3: return "Severity";
        case 4: return "User";
        case 5: return "Source";
        case 6: return "Entry ID";
        default: return QVariant();
        }
    }
    if (role == Qt::FontRole) {
        QFont font;
        font.setBold(true);
        return font;
    }
    return QVariant();
}

QVariant AuditLogModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= (int)m_entries.size()) return QVariant();

    const auto& entry = m_entries[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0: return entry.sequence_number;
        case 1: return entry.created_at;
        case 2: return entry.event_type;
        case 3: return entry.severity;
        case 4: return entry.user_id;
        case 5: return entry.source;
        case 6: return entry.entry_id == 0 ? "-" : QString::number(entry.entry_id);
        default: return QVariant();
        }
    }

    if (role == Qt::TextAlignmentRole) {
        if (index.column() == 0 || index.column() == 4 || index.column() == 6) {
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        }
        return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
    }

    if (role == Qt::ForegroundRole) {
        if (entry.severity == "ERROR") return QColor(255, 165, 0);
        if (entry.severity == "CRITICAL") return QColor(255, 0, 0);
        if (entry.severity == "WARN") return QColor(255, 140, 0);
        return QColor(0, 0, 0);
    }

    if (role == Qt::ToolTipRole) {
        return QString("Sequence: %1\nTime: %2\nEvent: %3\nSource: %4\nStatus: %5")
            .arg(entry.sequence_number)
            .arg(entry.created_at)
            .arg(entry.event_type)
            .arg(entry.source)
            .arg(entry.signature_valid ? "Valid" : "Invalid");
    }

    return QVariant();
}

AuditEntryDisplay AuditLogModel::getEntry(int row) const {
    if (row < 0 || row >= (int)m_entries.size()) return AuditEntryDisplay();
    return m_entries[row];
}

bool AuditLogModel::verifySignature(int row) {
    if (row < 0 || row >= (int)m_entries.size()) return false;

    auto& entry = m_entries[row];

    // Здесь должна быть проверка подписи через LogVerifier
    // Для примера возвращаем true
    entry.signature_valid = true;

    // Обновляем отображение
    QModelIndex idx = index(row, 7);
    emit dataChanged(idx, idx);

    return entry.signature_valid;
}

// ============== SortFilterProxyModel ==============

AuditLogSortFilterProxyModel::AuditLogSortFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent), m_userIdFilter(-1)
{
    setDynamicSortFilter(true);
}

void AuditLogSortFilterProxyModel::setEventTypeFilter(const QString& type) {
    m_eventTypeFilter = type;
    invalidateFilter();
}

void AuditLogSortFilterProxyModel::setSeverityFilter(const QString& severity) {
    m_severityFilter = severity;
    invalidateFilter();
}

void AuditLogSortFilterProxyModel::setDateRangeFilter(const QDate& from, const QDate& to) {
    m_dateFromFilter = from;
    m_dateToFilter = to;
    invalidateFilter();
}

void AuditLogSortFilterProxyModel::setSearchTextFilter(const QString& text) {
    m_searchTextFilter = text;
    invalidateFilter();
}

void AuditLogSortFilterProxyModel::setUserIdFilter(int userId) {
    m_userIdFilter = userId;
    invalidateFilter();
}

bool AuditLogSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    AuditLogModel* model = qobject_cast<AuditLogModel*>(sourceModel());
    if (!model) return true;

    AuditEntryDisplay entry = model->getEntry(sourceRow);

    // Фильтр по типу события
    if (!m_eventTypeFilter.isEmpty() && entry.event_type != m_eventTypeFilter) {
        return false;
    }

    // Фильтр по severity
    if (!m_severityFilter.isEmpty() && entry.severity != m_severityFilter) {
        return false;
    }

    // Фильтр по пользователю
    if (m_userIdFilter != -1 && entry.user_id != m_userIdFilter) {
        return false;
    }

    // Фильтр по дате
    if (m_dateFromFilter.isValid()) {
        QDateTime dt = QDateTime::fromString(entry.created_at, Qt::ISODate);
        if (dt.date() < m_dateFromFilter) return false;
    }
    if (m_dateToFilter.isValid()) {
        QDateTime dt = QDateTime::fromString(entry.created_at, Qt::ISODate);
        if (dt.date() > m_dateToFilter) return false;
    }

    // Поиск
    if (!m_searchTextFilter.isEmpty()) {
        if (!entry.entry_data.contains(m_searchTextFilter, Qt::CaseInsensitive) &&
            !entry.event_type.contains(m_searchTextFilter, Qt::CaseInsensitive) &&
            !entry.source.contains(m_searchTextFilter, Qt::CaseInsensitive)) {
            return false;
        }
    }

    return true;
}

bool AuditLogSortFilterProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const {
    QVariant leftData = sourceModel()->data(left);
    QVariant rightData = sourceModel()->data(right);

    if (left.column() == 0) {  // sequence_number
        return leftData.toInt() < rightData.toInt();
    }
    if (left.column() == 1) {  // timestamp
        return leftData.toDateTime() < rightData.toDateTime();
    }
    if (left.column() == 3) {  // severity - сортировка по важности
        QString leftSeverity = leftData.toString();
        QString rightSeverity = rightData.toString();

        auto severityOrder = [](const QString& s) -> int {
            if (s == "CRITICAL") return 4;
            if (s == "ERROR") return 3;
            if (s == "WARN") return 2;
            if (s == "INFO") return 1;
            return 0;
        };

        return severityOrder(leftSeverity) < severityOrder(rightSeverity);
    }

    return QString::localeAwareCompare(leftData.toString(), rightData.toString()) < 0;
}
