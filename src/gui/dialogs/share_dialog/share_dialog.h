#ifndef SHARE_DIALOG_H
#define SHARE_DIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPlainTextEdit>
#include "../../core/vault/VaultManager.h"
#include "../../core/sharing/sharing_service.h"
#include "../../database/DB_helper/db_helper.h"

class ShareDialog : public QDialog
{
    Q_OBJECT

public:
    // Передаём конкретную запись для шеринга
    ShareDialog(VaultManager* vaultManager, Database* db,
                const PlaintextEntry& entry, QWidget* parent = nullptr);

private slots:
    void onMethodChanged();
    void onBrowsePublicKey();
    void onCreateShare();
    void onContactSelected(int index);

private:
    void showPublicKeyInfo();
    void loadContacts();

    VaultManager* m_vaultManager;
    Database* m_db;
    PlaintextEntry m_entry;  // Конкретная запись для шеринга

    // UI элементы
    QLabel* m_entryInfoLabel;
    QLineEdit* m_sharerEdit;
    QComboBox* m_methodCombo;
    QLineEdit* m_passwordEdit;
    QPushButton* m_browseKeyBtn;
    QLabel* m_keyInfoLabel;
    QDateEdit* m_expirationDate;
    QComboBox* m_permissionsCombo;
    QPushButton* m_createButton;
    QPushButton* m_cancelButton;
    QComboBox* m_contactCombo;

    // Данные
    std::vector<uint8_t> m_selectedPublicKey;
};

#endif
