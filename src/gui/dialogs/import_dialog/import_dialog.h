#ifndef IMPORT_DIALOG_H
#define IMPORT_DIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QGroupBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <nlohmann/json.hpp>

#include "../../database/DB_helper/db_helper.h"
#include "../../core/vault/VaultManager.h"

#include "sharing_service.h"

class ImportDialog : public QDialog
{
    Q_OBJECT

public:
    ImportDialog(VaultManager* vaultManager, Database* db, QWidget* parent = nullptr);

private slots:
    void onBrowse();
    void onFileSelected(const QString& filepath);
    void onImport();
    void onSelectAll(bool checked);
    void onConflictOptionChanged();
    bool isLastPassCSV(const QString& filepath);

private:
    void detectFormat(const QString& filepath);
    void loadAndParseFile();
    void showPreview();
    void checkDuplicates();
    void showSummary(int imported, int skipped, int updated, int errors);
    bool isDuplicate(const PlaintextEntry& entry, PlaintextEntry* existingEntry = nullptr);
    std::vector<int> getSelectedRows();
    void importSharedEntry();

    VaultManager* m_vaultManager;
    Database* m_db;

    // UI элементы
    QLineEdit* m_fileEdit;
    QPushButton* m_browseButton;
    QLabel* m_formatLabel;

    QTableWidget* m_previewTable;
    QCheckBox* m_selectAllCheck;

    QGroupBox* m_conflictGroup;
    QRadioButton* m_conflictSkip;
    QRadioButton* m_conflictReplace;
    QRadioButton* m_conflictAddNew;
    QRadioButton* m_conflictUpdate;
    QCheckBox* m_applyToAll;

    QLabel* m_summaryLabel;
    QPushButton* m_importButton;
    QPushButton* m_cancelButton;

    // Данные
    QString m_currentFilepath;
    QString m_currentFormat;
    std::vector<PlaintextEntry> m_importedEntries;
    std::vector<PlaintextEntry> m_existingEntries;
    std::vector<int> m_duplicateIndices;
    int m_conflictAction; // 0=skip, 1=replace, 2=addNew, 3=update
    bool m_applyToAllConflicts;

    bool m_isShareImport = false;
    ShareMetadata m_shareMetadata;
};

#endif
