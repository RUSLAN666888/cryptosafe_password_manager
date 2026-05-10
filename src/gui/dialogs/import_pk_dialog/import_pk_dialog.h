#ifndef IMPORT_PUBLIC_KEY_DIALOG_H
#define IMPORT_PUBLIC_KEY_DIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QRadioButton>
#include <QButtonGroup>
#include <QFileDialog>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <vector>
#include <cstdint>

#include "db_helper.h"

class ImportPublicKeyDialog : public QDialog
{
    Q_OBJECT

public:
    ImportPublicKeyDialog(Database* db, QWidget* parent = nullptr);

    std::vector<uint8_t> getImportedKey() const { return m_importedKey; }
    QString getContactName() const { return m_contactNameEdit->text().trimmed(); }
    bool isKeyImported() const { return !m_importedKey.empty(); }

private slots:
    void onMethodChanged();
    void onBrowseFile();
    void onPasteFromClipboard();
    void onImport();

private:
    void importKey(const std::string& pemData);
    std::string cleanPEM(const std::string& pemData);

    std::vector<uint8_t> m_importedKey;

    QRadioButton* m_fileRadio;
    QRadioButton* m_textRadio;
    QLineEdit* m_fileEdit;
    QPushButton* m_browseBtn;
    QPlainTextEdit* m_textEdit;
    QLineEdit* m_contactNameEdit;
    QPushButton* m_importButton;
    QPushButton* m_cancelButton;

    Database* m_db;
};

#endif
