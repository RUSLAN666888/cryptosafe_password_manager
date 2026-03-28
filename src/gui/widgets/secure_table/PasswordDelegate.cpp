#include "PasswordDelegate.h"
#include <QApplication>
#include <QStyle>
#include <QDebug>

PasswordDelegate::PasswordDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void PasswordDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                             const QModelIndex& index) const
{
    // Получаем текст пароля (маскированный или реальный)
    QString password = index.data(Qt::DisplayRole).toString();

    // Создаем копию опции для рисования текста
    QStyleOptionViewItem textOption = option;

    // Рисуем фон
    QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, nullptr);

    // Рисуем текст пароля
    painter->save();

    // Отступ для текста (оставляем место для иконки)
    const int eyeIconWidth = 20;
    QRect textRect = option.rect;
    textRect.setWidth(textRect.width() - eyeIconWidth - 4);

    painter->setPen(option.palette.text().color());
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, password);

    // Рисуем иконку глаза
    QRect eyeRect = option.rect;
    eyeRect.setLeft(option.rect.right() - eyeIconWidth - 2);
    eyeRect.setWidth(eyeIconWidth);
    m_eyeRect = eyeRect;

    // Выбираем иконку в зависимости от состояния
    bool passwordsVisible = index.data(Qt::UserRole + 1).toBool();  // передаем состояние
    QString iconText = passwordsVisible ? "👁‍🗨" : "👁";

    painter->setFont(QFont("Segoe UI Emoji", 12));
    painter->drawText(eyeRect, Qt::AlignCenter, iconText);

    painter->restore();

    // Рисуем рамку фокуса если нужно
    if (option.state & QStyle::State_Selected) {
        painter->save();
        painter->setPen(QPen(option.palette.highlight().color(), 1));
        painter->drawRect(option.rect.adjusted(0, 0, -1, -1));
        painter->restore();
    }
}

bool PasswordDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
                                   const QStyleOptionViewItem& option,
                                   const QModelIndex& index)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

        // Проверяем, попали ли в область иконки глаза
        if (m_eyeRect.contains(mouseEvent->pos())) {
            // Испускаем сигнал для переключения видимости
            emit togglePasswordVisibility(index);
            return true;
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
