#include "VaultTableModel.h"
#include "../src/core/vault/VaultManager.h"
#include <QUrl>
#include <QDateTime>

VaultTableModel::VaultTableModel(VaultManager& vaultManager, QObject* parent)
    : QAbstractTableModel(parent)
    , m_vaultManager(vaultManager)
{
}

int VaultTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return static_cast<int>(m_data.size());
}

int VaultTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return COL_COUNT;
}

QVariant VaultTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= static_cast<int>(m_data.size()))
        return QVariant();

    const VaultManager::EntryMetadata& entry = m_data[index.row()];

    // Qt::DisplayRole — что показывать в ячейке
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case COL_TITLE:
            return QString::fromStdString(entry.title);
        case COL_USERNAME:
            return QString::fromStdString(entry.username);
        case COL_URL:
            return extractDomain(QString::fromStdString(entry.url));
        case COL_PASSWORD:
            // Показываем маскировку, если пароли скрыты
            if (m_passwordsVisible) {
                // Нужно загрузить реальный пароль
                return getPasswordForRow(index.row());
            }
            return "********";
        case COL_MODIFIED:
            return formatDate(QString::fromStdString(entry.updated_at));
        default:
            return QVariant();
        }
    }

    // Qt::UserRole — для сортировки (оригинальные данные без форматирования)
    if (role == Qt::UserRole && index.column() != COL_PASSWORD) {
        switch (index.column()) {
        case COL_TITLE:
            return QString::fromStdString(entry.title);
        case COL_USERNAME:
            return QString::fromStdString(entry.username);
        case COL_URL:
            return QString::fromStdString(entry.url);
        case COL_MODIFIED:
            return QString::fromStdString(entry.updated_at);
        default:
            return QVariant();
        }
    }

    return QVariant();
}

QVariant VaultTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case COL_TITLE: return "Название";
        case COL_USERNAME: return "Логин";
        case COL_URL: return "Сайт";
        case COL_PASSWORD: return "Пароль";
        case COL_MODIFIED: return "Изменено";
        default: return QVariant();
        }
    }
    return QVariant();
}

void VaultTableModel::refresh()
{
    beginResetModel();
    m_data = m_vaultManager.getAllEntryMetadata();
    endResetModel();
}

QString VaultTableModel::extractDomain(const QString& url) const
{
    QUrl qurl(url);
    QString host = qurl.host();
    if (!host.isEmpty()) {
        return host;
    }
    return url;
}

QString VaultTableModel::formatDate(const QString& date) const
{
    QDateTime dt = QDateTime::fromString(date, Qt::ISODate);
    if (dt.isValid()) {
        return dt.toString("dd.MM.yyyy");
    }
    return date.left(10);
}

long VaultTableModel::getId(int row) const
{
    if (row < 0 || row >= static_cast<int>(m_data.size())) return -1;
    return m_data[row].id;
}

void VaultTableModel::setPasswordsVisible(bool visible)
{
    if (m_passwordsVisible == visible) return;

    m_passwordsVisible = visible;

    if (!visible) {
        // Очищаем кэш, когда скрываем пароли (SEC-1)
        m_passwordCache.clear();
    }

    // Обновляем только колонку пароля
    emit dataChanged(index(0, COL_PASSWORD),
                     index(rowCount() - 1, COL_PASSWORD));
}

QString VaultTableModel::getPasswordForRow(int row) const
{
    long id = m_data[row].id;

    // Проверяем кэш
    if (m_passwordCache.contains(id)) {
        return m_passwordCache[id];
    }

    // Загружаем пароль из VaultManager

    auto entry = m_vaultManager.getEntry(static_cast<int>(id));
    if (entry)
    {
        QString password = QString::fromStdString(entry->password);
        m_passwordCache[id] = password;
        return password;
    }

    return "Ошибка";
}
