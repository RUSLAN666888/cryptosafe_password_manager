#include "export_pk_dialog.h"
#include <iostream>
#include <QString>
#include <QByteArray>

ExportPublicKeyDialog::ExportPublicKeyDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Мой публичный ключ (RSA)");
    setMinimumSize(550, 400);
    setModal(true);


    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QLabel* infoLabel = new QLabel(
        "Поделитесь этим ключом с другими пользователями.\n"
        "Они смогут зашифровать для вас записи.\n\n"
        "Скопируйте текст или сохраните в файл:"
        );
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #555;");
    mainLayout->addWidget(infoLabel);

    m_textEdit = new QPlainTextEdit();
    m_textEdit->setReadOnly(true);
    m_textEdit->setFont(QFont("Monospace", 9));
    m_textEdit->setMinimumHeight(200);
    mainLayout->addWidget(m_textEdit);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    QPushButton* copyBtn = new QPushButton("Копировать в буфер");
    QPushButton* saveBtn = new QPushButton("Сохранить в файл");
    QPushButton* closeBtn = new QPushButton("Закрыть");

    buttonLayout->addWidget(copyBtn);
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(closeBtn);

    mainLayout->addLayout(buttonLayout);

    connect(copyBtn, &QPushButton::clicked, this, &ExportPublicKeyDialog::onCopyToClipboard);
    connect(saveBtn, &QPushButton::clicked, this, &ExportPublicKeyDialog::onSaveToFile);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    loadPublicKey();
}

void ExportPublicKeyDialog::loadPublicKey()
{
    KeyData keyData;
    KeyManager::getInstance().getPublicRSAKey(keyData);

    if (keyData.data && keyData.size > 0) {
        m_publicKey.assign(keyData.data, keyData.data + keyData.size);
        std::string pemText(m_publicKey.begin(), m_publicKey.end());
        m_textEdit->setPlainText(QString::fromStdString(pemText));
    } else {
        m_textEdit->setPlainText("Публичный ключ не найден.\n\n"
                                 "Убедитесь, что ключи RSA были сгенерированы при входе в систему.");
    }
}

void ExportPublicKeyDialog::onCopyToClipboard()
{
    if (m_publicKey.empty()) {
        QMessageBox::warning(this, "Ошибка", "Публичный ключ не найден");
        return;
    }

    std::string pemText(m_publicKey.begin(), m_publicKey.end());
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(QString::fromStdString(pemText));

    QMessageBox::information(this, "Успех", "Публичный ключ скопирован в буфер обмена");
}

void ExportPublicKeyDialog::onSaveToFile()
{
    if (m_publicKey.empty()) {
        QMessageBox::warning(this, "Ошибка", "Публичный ключ не найден");
        return;
    }

    std::string pemText(m_publicKey.begin(), m_publicKey.end());

    QString filepath = QFileDialog::getSaveFileName(this, "Сохранить публичный ключ",
                                                    "public_key.pem",
                                                    "PEM Files (*.pem);;All Files (*)");
    if (filepath.isEmpty()) return;

    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось создать файл");
        return;
    }

    file.write(pemText.c_str());
    file.close();

    QMessageBox::information(this, "Успех", "Публичный ключ сохранён в файл");
}
