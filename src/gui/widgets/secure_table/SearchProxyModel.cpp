// SearchProxyModel.cpp
#include "SearchProxyModel.h"

SearchProxyModel::SearchProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
}

void SearchProxyModel::setSearchText(const QString& text)
{
    m_searchText = text;
    invalidateFilter();  // пересчитать фильтрацию
}

void SearchProxyModel::setSearchColumn(int column)
{
    m_searchColumn = column;
    invalidateFilter();
}

bool SearchProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    if (m_searchText.isEmpty()) {
        return true;  // пустой поиск — показываем всё
    }

    QModelIndex index;
    if (m_searchColumn == -1) {
        // Ищем по всем колонкам
        for (int col = 0; col < sourceModel()->columnCount(); col++) {
            index = sourceModel()->index(sourceRow, col, sourceParent);
            QString data = sourceModel()->data(index, Qt::DisplayRole).toString();
            if (data.contains(m_searchText, Qt::CaseInsensitive)) {
                return true;
            }
        }
        return false;
    } else {
        // Ищем только в одной колонке
        index = sourceModel()->index(sourceRow, m_searchColumn, sourceParent);
        QString data = sourceModel()->data(index, Qt::DisplayRole).toString();
        return data.contains(m_searchText, Qt::CaseInsensitive);
    }
}

bool SearchProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    // Используем Qt::UserRole для сортировки (оригинальные данные)
    QVariant leftData = sourceModel()->data(left, Qt::UserRole);
    QVariant rightData = sourceModel()->data(right, Qt::UserRole);

    if (leftData.type() == QVariant::String) {
        return leftData.toString().localeAwareCompare(rightData.toString()) < 0;
    }

    return QSortFilterProxyModel::lessThan(left, right);
}
