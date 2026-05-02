#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialog>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QTreeWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

#include "../src/database/DB_helper/db_helper.h"

class ExportDialog : public QDialog{
    Q_OBJECT
public:
    ExportDialog(Database* db, QWidget *parent = nullptr);

private slots:
    void onItemChanged(QTreeWidgetItem* item, int column);

private:
    struct EntryPreview{
        std::string title;
        std::string login;
    };

    // Форматы экспорта
    QRadioButton* m_encryptedJsonRadio;
    QRadioButton* m_csvRadio;
    QRadioButton* m_bitwardenRadio;
    QRadioButton* m_lastpassRadio;

    // Выбор записей
    QCheckBox* m_selectAllCheck;
    QTreeWidget* m_entriesTree;

    // Настройки шифрования
    QGroupBox* m_encryptionGroup;
    QRadioButton* m_aes128Radio;
    QRadioButton* m_aes256Radio;

    // Предпросмотр
    QLabel* m_previewLabel;

    // Счетчик
    QLabel* m_entriesCountLabel;

    // Кнопки
    QPushButton* m_exportButton;
    QPushButton* m_cancelButton;

    Database* m_db;

    void loadPreview();
};

#endif // EXPORTDIALOG_H
