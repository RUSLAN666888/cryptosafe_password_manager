#include "PasswordEntry.h"
#include <QClipboard>
#include <QApplication>
#include <QMessageBox>
#include <QStyle>

PasswordEntry::PasswordEntry(QWidget *parent,
                             const QString &value,
                             const QSize &size)
    : QWidget(parent)
    , passwordVisible(false)
{
    // Создаем горизонтальный layout
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);

    // Поле ввода пароля
    passwordInput = new QLineEdit(this);
    passwordInput->setEchoMode(QLineEdit::Password);
    passwordInput->setText(value);
    passwordInput->setMinimumSize(size);

    // Настройка размера поля
    if (size.width() > 0) {
        passwordInput->setFixedWidth(size.width());
    }

    // Чекбокс "Показать пароль"
    showPasswordCheck = new QCheckBox(tr("Show"), this);

    // Добавляем виджеты в layout
    layout->addWidget(passwordInput, 1);
    layout->addWidget(showPasswordCheck, 0, Qt::AlignVCenter);

    // Устанавливаем layout для виджета
    setLayout(layout);

    // Подключаем сигнал от чекбокса
    connect(showPasswordCheck, &QCheckBox::toggled,
            this, &PasswordEntry::onShowPassword);

    // Пробрасываем сигнал textChanged от QLineEdit
    connect(passwordInput, &QLineEdit::textChanged,
            this, &PasswordEntry::textChanged);
}

PasswordEntry::~PasswordEntry()
{
    // Безопасное затирание памяти (будет улучшено в будущих спринтах)
    if (passwordInput) {
        // Затираем содержимое QLineEdit
        passwordInput->setText(QString(passwordInput->text().size(), QChar(0)));
        passwordInput->clear();
    }

    // Qt автоматически удалит дочерние объекты
}

QString PasswordEntry::getValue() const
{
    return passwordInput->text();
}

void PasswordEntry::setValue(const QString &value)
{
    passwordInput->setText(value);
}

void PasswordEntry::setEditable(bool editable)
{
    passwordInput->setReadOnly(!editable);
}

void PasswordEntry::setPlaceholderText(const QString &text)
{
    passwordInput->setPlaceholderText(text);
}

void PasswordEntry::onShowPassword(bool checked)
{
    passwordVisible = checked;

    if (passwordVisible) {
        // Показываем пароль в открытом виде
        passwordInput->setEchoMode(QLineEdit::Normal);
    } else {
        // Скрываем пароль
        passwordInput->setEchoMode(QLineEdit::Password);
    }

    // Сохраняем позицию курсора
    int cursorPos = passwordInput->cursorPosition();

    // Принудительно обновляем виджет
    passwordInput->update();

    // Восстанавливаем позицию курсора
    passwordInput->setCursorPosition(cursorPos);
}
