#include "SecureTable.h"
#include <QHeaderView>

SecureTable::SecureTable(QWidget *parent)
    : QTableWidget(parent)
{
    initColumns();

    // Минимальные настройки
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void SecureTable::initColumns()
{
    setColumnCount(6);

    QStringList headers;
    headers << "ID" << "Title" << "Username" << "URL" << "Tags" << "Created";
    setHorizontalHeaderLabels(headers);

    // Устанавливаем ширину колонок как в wxWidgets
    setColumnWidth(0, 50);
    setColumnWidth(1, 200);
    setColumnWidth(2, 150);
    setColumnWidth(3, 200);
    setColumnWidth(4, 150);
    setColumnWidth(5, 150);
}

void SecureTable::addEntry(const VaultEntry &entry)
{
    int row = rowCount();
    insertRow(row);

    setItem(row, 0, new QTableWidgetItem(QString::number(entry.id)));
    setItem(row, 1, new QTableWidgetItem(QString::fromStdString(entry.title)));
    setItem(row, 2, new QTableWidgetItem(QString::fromStdString(entry.username)));
    setItem(row, 3, new QTableWidgetItem(QString::fromStdString(entry.url)));
    setItem(row, 4, new QTableWidgetItem(QString::fromStdString(entry.tags)));
    setItem(row, 5, new QTableWidgetItem(QString::fromStdString(entry.created_at)));
}

void SecureTable::addSampleData()
{
    VaultEntry e1;
    e1.id = 1;
    e1.title = "GitHub";
    e1.username = "user123";
    e1.url = "https://github.com";
    e1.tags = "[\"work\", \"dev\"]";
    e1.created_at = "2024-01-15";
    addEntry(e1);

    VaultEntry e2;
    e2.id = 2;
    e2.title = "Gmail";
    e2.username = "alice@gmail.com";
    e2.url = "https://gmail.com";
    e2.tags = "[\"personal\"]";
    e2.created_at = "2024-01-16";
    addEntry(e2);

    VaultEntry e3;
    e3.id = 3;
    e3.title = "Facebook";
    e3.username = "alice123";
    e3.url = "https://facebook.com";
    e3.tags = "[\"social\"]";
    e3.created_at = "2024-01-17";
    addEntry(e3);
}

void SecureTable::clearAll()
{
    clearContents();
    setRowCount(0);
}

long SecureTable::getSelectedId()
{
    QList<QTableWidgetItem*> selected = selectedItems();
    if (selected.isEmpty()) {
        return -1;
    }

    int row = selected.first()->row();
    return item(row, 0)->text().toLong();
}
