#ifndef SEARCHPROXYMODEL_H
#define SEARCHPROXYMODEL_H

#include <QSortFilterProxyModel>

class SearchProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit SearchProxyModel(QObject* parent = nullptr);

    void setSearchText(const QString& text);
    void setSearchColumn(int column);  // -1 = все колонки

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

private:
    QString m_searchText;
    int m_searchColumn = -1;
};

#endif // SEARCHPROXYMODEL_H
