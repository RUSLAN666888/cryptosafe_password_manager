#ifndef EXPORT_PUBLIC_KEY_DIALOG_H
#define EXPORT_PUBLIC_KEY_DIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QMessageBox>
#include <QFileDialog>
#include <QClipboard>
#include <QApplication>
#include "key_manager.h"

class ExportPublicKeyDialog : public QDialog
{
    Q_OBJECT

public:
    ExportPublicKeyDialog(QWidget* parent = nullptr);

private slots:
    void onCopyToClipboard();
    void onSaveToFile();

private:
    void loadPublicKey();

    std::vector<uint8_t> m_publicKey;
    QPlainTextEdit* m_textEdit;
};

#endif
