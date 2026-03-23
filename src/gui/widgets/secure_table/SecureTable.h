#ifndef SECURETABLE_H
#define SECURETABLE_H

#include <QTableWidget>
#include "../src/database/DB_helper/db_helper.h"

class SecureTable : public QTableWidget
{
    Q_OBJECT

public:
    explicit SecureTable(QWidget *parent = nullptr);

    void addEntry(const VaultEntry &entry);
    void addSampleData();
    void clearAll();
    long getSelectedId();

private:
    void initColumns();
};

#endif // SECURETABLE_H
