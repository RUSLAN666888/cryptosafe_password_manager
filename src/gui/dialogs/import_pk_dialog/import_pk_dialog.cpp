#include "import_pk_dialog.h"

#include <QGroupBox>

ImportPublicKeyDialog::ImportPublicKeyDialog(Database* db, QWidget* parent)
    : m_db(db), QDialog(parent)
{
    setWindowTitle("Импорт публичного ключа");
    setMinimumSize(550, 450);
    setModal(true);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QLabel* infoLabel = new QLabel(
        "Введите публичный ключ другого пользователя.\n"
        "После импорта вы сможете шифровать записи для этого пользователя."
        );
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #555;");
    mainLayout->addWidget(infoLabel);

    // Выбор способа ввода
    QGroupBox* methodGroup = new QGroupBox("Способ ввода");
    QVBoxLayout* methodLayout = new QVBoxLayout(methodGroup);

    m_fileRadio = new QRadioButton("Загрузить из файла (.pem)");
    m_textRadio = new QRadioButton("Вставить текст (PEM)");
    m_fileRadio->setChecked(true);

    methodLayout->addWidget(m_fileRadio);
    methodLayout->addWidget(m_textRadio);

    mainLayout->addWidget(methodGroup);

    // Панель для файла
    QWidget* fileWidget = new QWidget();
    QHBoxLayout* fileLayout = new QHBoxLayout(fileWidget);
    m_fileEdit = new QLineEdit();
    m_fileEdit->setPlaceholderText("Путь к файлу с публичным ключом");
    m_fileEdit->setReadOnly(true);
    m_browseBtn = new QPushButton("Обзор...");
    fileLayout->addWidget(m_fileEdit);
    fileLayout->addWidget(m_browseBtn);
    mainLayout->addWidget(fileWidget);

    // Панель для текста
    QWidget* textWidget = new QWidget();
    QVBoxLayout* textLayout = new QVBoxLayout(textWidget);
    m_textEdit = new QPlainTextEdit();
    m_textEdit->setPlaceholderText(
        "Вставьте PEM ключ в формате:\n"
        "-----BEGIN PUBLIC KEY-----\n"
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA...\n"
        "-----END PUBLIC KEY-----"
        );
    m_textEdit->setFont(QFont("Monospace", 9));
    textLayout->addWidget(m_textEdit);

    QPushButton* pasteBtn = new QPushButton("Вставить из буфера");
    textLayout->addWidget(pasteBtn);

    textWidget->setLayout(textLayout);
    mainLayout->addWidget(textWidget);

    // Имя контакта
    QHBoxLayout* nameLayout = new QHBoxLayout();
    nameLayout->addWidget(new QLabel("Имя контакта:"));
    m_contactNameEdit = new QLineEdit();
    m_contactNameEdit->setPlaceholderText("Например: Иван Петров");
    nameLayout->addWidget(m_contactNameEdit);
    mainLayout->addLayout(nameLayout);

    // Кнопки
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_importButton = new QPushButton("Импортировать");
    m_cancelButton = new QPushButton("Отмена");

    buttonLayout->addWidget(m_importButton);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    // Подключения
    connect(m_fileRadio, &QRadioButton::toggled, this, &ImportPublicKeyDialog::onMethodChanged);
    connect(m_browseBtn, &QPushButton::clicked, this, &ImportPublicKeyDialog::onBrowseFile);
    connect(pasteBtn, &QPushButton::clicked, this, &ImportPublicKeyDialog::onPasteFromClipboard);
    connect(m_importButton, &QPushButton::clicked, this, &ImportPublicKeyDialog::onImport);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    onMethodChanged();
}

void ImportPublicKeyDialog::onMethodChanged()
{
    bool isFile = m_fileRadio->isChecked();

    // Находим панели по индексам
    QWidget* fileWidget = qobject_cast<QWidget*>(layout()->itemAt(2)->widget());
    QWidget* textWidget = qobject_cast<QWidget*>(layout()->itemAt(3)->widget());

    if (fileWidget) fileWidget->setVisible(isFile);
    if (textWidget) textWidget->setVisible(!isFile);
}

void ImportPublicKeyDialog::onBrowseFile()
{
    QString filepath = QFileDialog::getOpenFileName(this, "Выберите публичный ключ",
                                                    "", "PEM Files (*.pem);;All Files (*)");
    if (filepath.isEmpty()) return;

    m_fileEdit->setText(filepath);

    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл");
        return;
    }

    QByteArray keyData = file.readAll();
    file.close();

    importKey(keyData.toStdString());
}

void ImportPublicKeyDialog::onPasteFromClipboard()
{
    QClipboard* clipboard = QApplication::clipboard();
    QString text = clipboard->text();
    if (!text.isEmpty()) {
        m_textEdit->setPlainText(text);
        importKey(text.toStdString());
    }
}

std::string ImportPublicKeyDialog::cleanPEM(const std::string& pemData)
{
    // Убираем лишние пробелы и переносы строк в конце
    std::string result = pemData;

    // Убираем нулевые байты
    result.erase(std::remove(result.begin(), result.end(), '\0'), result.end());

    // Убираем пробелы и переносы в конце
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r' || result.back() == ' ')) {
        result.pop_back();
    }

    // Добавляем перевод строки в конце если нужно
    if (!result.empty() && result.back() != '\n') {
        result += '\n';
    }

    return result;
}

void ImportPublicKeyDialog::importKey(const std::string& pemData)
{
    if (pemData.find("-----BEGIN PUBLIC KEY-----") == std::string::npos) {
        QMessageBox::warning(this, "Ошибка",
                             "Неверный формат публичного ключа.\n\n"
                             "Файл должен содержать PEM ключ в формате:\n"
                             "-----BEGIN PUBLIC KEY-----\n"
                             "...\n"
                             "-----END PUBLIC KEY-----");
        return;
    }

    // Очищаем и сохраняем
    std::string cleaned = cleanPEM(pemData);
    m_importedKey.assign(cleaned.begin(), cleaned.end());
}

void ImportPublicKeyDialog::onImport()
{
    if (m_importedKey.empty()) {
        QMessageBox::warning(this, "Ошибка", "Публичный ключ не загружен");
        return;
    }

    QString contactName = m_contactNameEdit->text().trimmed();
    if (contactName.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите имя контакта");
        return;
    }

    // Сохраняем контакт в БД
    std::string pemText(m_importedKey.begin(), m_importedKey.end());
    if (!m_db->addContact(contactName.toStdString(), pemText)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось сохранить контакт");
        return;
    }

    QMessageBox::information(this, "Успех", QString("Контакт '%1' успешно добавлен").arg(contactName));
    accept();
}
