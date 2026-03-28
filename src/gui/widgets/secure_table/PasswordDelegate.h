#ifndef PASSWORDDELEGATE_H
#define PASSWORDDELEGATE_H

#include <QStyledItemDelegate>
#include <QModelIndex>
#include <QStyleOptionViewItem>
#include <QPainter>
#include <QApplication>
#include <QMouseEvent>

class PasswordDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit PasswordDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    bool editorEvent(QEvent* event, QAbstractItemModel* model,
                     const QStyleOptionViewItem& option,
                     const QModelIndex& index) override;

signals:
    void togglePasswordVisibility(const QModelIndex& index) const;

private:
    mutable QRect m_eyeRect;  // область, где находится иконка глаза
};

#endif // PASSWORDDELEGATE_H
