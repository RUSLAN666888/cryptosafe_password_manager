// export_dialog.h
#ifndef EXPORT_DIALOG_H
#define EXPORT_DIALOG_H

#include <QDialog>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRadioButton>
#include <QCheckBox>
#include <QTreeWidget>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>

#include "../../database/DB_helper/db_helper.h"
#include "../login_dialog/LoginDialog.h"
#include "../../core/import_export/export/export.h"
#include "../../core/vault/VaultManager.h"

class ExportDialog : public QDialog
{
    Q_OBJECT

public:
    ExportDialog(Database* db, VaultManager* vm, QWidget *parent = nullptr);

private slots:
    void onExport();
    void onItemChanged(QTreeWidgetItem* item, int column);

private:
    void loadPreview();
    std::vector<int> getSelectedEntryIds();
    bool confirmMasterPassword();

    Database* m_db;
    VaultManager* m_vaultManager;

    QRadioButton* m_encryptedJsonRadio;
    QRadioButton* m_csvRadio;
    QRadioButton* m_bitwardenRadio;
    QRadioButton* m_lastpassRadio;

    QCheckBox* m_selectAllCheck;
    QTreeWidget* m_entriesTree;

    QGroupBox* m_encryptionGroup;
    QRadioButton* m_aes128Radio;
    QRadioButton* m_aes256Radio;

    QLabel* m_entriesCountLabel;

    QPushButton* m_exportButton;
    QPushButton* m_cancelButton;
};

#endif
