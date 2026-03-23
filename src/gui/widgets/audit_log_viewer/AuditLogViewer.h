#ifndef AUDITLOGVIEWER_H
#define AUDITLOGVIEWER_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "../src/database/DB_helper/db_helper.h"

class AuditLogViewer : public QDialog
{
    Q_OBJECT

private:
    Database &db;
    QTableWidget *logTable;
    QPushButton *refreshButton;
    QPushButton *closeButton;

private slots:
    void onRefresh();
    void onClose();

public:
    AuditLogViewer(QWidget *parent, Database &database);

private:
    void refreshLogs();
    void initUI();
};

#endif // AUDITLOGVIEWER_H
