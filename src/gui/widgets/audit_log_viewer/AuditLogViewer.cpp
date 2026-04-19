#include "AuditLogViewer.h"
#include <QHeaderView>

AuditLogViewer::AuditLogViewer(QWidget *parent, Database &database)
    : QDialog(parent)
    , db(database)
{
    initUI();
    refreshLogs();

    // Центрируем окно
    setWindowTitle("Audit Logs");
    resize(700, 500);
}

void AuditLogViewer::initUI()
{
    // Основной вертикальный layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Создаем таблицу для логов
    logTable = new QTableWidget(this);
    logTable->setColumnCount(5);

    // Устанавливаем заголовки колонок
    QStringList headers;
    headers << "ID" << "Action" << "Timestamp" << "Entry ID" << "Details";
    logTable->setHorizontalHeaderLabels(headers);

    // Настройки таблицы
    logTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    logTable->setSelectionMode(QAbstractItemView::SingleSelection);
    logTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    logTable->setAlternatingRowColors(true);

    // Устанавливаем ширину колонок
    logTable->setColumnWidth(0, 50);   // ID
    logTable->setColumnWidth(1, 150);  // Action
    logTable->setColumnWidth(2, 180);  // Timestamp
    logTable->setColumnWidth(3, 80);   // Entry ID
    logTable->setColumnWidth(4, 200);  // Details

    mainLayout->addWidget(logTable, 1);

    // Панель с кнопками
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    refreshButton = new QPushButton("Refresh", this);
    closeButton = new QPushButton("Close", this);

    buttonLayout->addWidget(refreshButton);
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);

    // Подключаем сигналы
    connect(refreshButton, &QPushButton::clicked, this, &AuditLogViewer::onRefresh);
    connect(closeButton, &QPushButton::clicked, this, &AuditLogViewer::onClose);
}

void AuditLogViewer::refreshLogs()
{
    // // Очищаем таблицу
    // logTable->clearContents();
    // logTable->setRowCount(0);

    // // Получаем логи из базы данных (последние 100 записей)
    // auto logs = db.getAuditLogs(100);

    // int row = 0;
    // for (const auto &log : logs)
    // {
    //     logTable->insertRow(row);

    //     // Конвертируем данные в QString (предполагаем, что поля - std::string или wxString)
    //     logTable->setItem(row, 0, new QTableWidgetItem(QString::number(log.id)));
    //     logTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(log.action)));
    //     logTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(log.timestamp)));

    //     // Entry ID
    //     if (log.entry_id >= 0)
    //     {
    //         logTable->setItem(row, 3, new QTableWidgetItem(QString::number(log.entry_id)));
    //     }
    //     else
    //     {
    //         logTable->setItem(row, 3, new QTableWidgetItem("-"));
    //     }

    //     logTable->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(log.details)));

    //     row++;
    // }

    // // Если нет логов, показываем сообщение
    // if (logs.empty())
    // {
    //     logTable->insertRow(0);
    //     logTable->setItem(0, 0, new QTableWidgetItem("-"));
    //     logTable->setItem(0, 1, new QTableWidgetItem("No audit logs found"));
    //     logTable->setItem(0, 2, new QTableWidgetItem(""));
    //     logTable->setItem(0, 3, new QTableWidgetItem(""));
    //     logTable->setItem(0, 4, new QTableWidgetItem(""));
    // }

    // // Автоматически подгоняем ширину колонок
    // for (int i = 0; i < 5; i++)
    // {
    //     logTable->resizeColumnToContents(i);
    //     // Минимальная ширина для колонки Details
    //     if (i == 4 && logTable->columnWidth(i) < 200)
    //     {
    //         logTable->setColumnWidth(i, 200);
    //     }
    // }
}

void AuditLogViewer::onRefresh()
{
    refreshLogs();
}

void AuditLogViewer::onClose()
{
    accept();  // Закрываем диалог
}
