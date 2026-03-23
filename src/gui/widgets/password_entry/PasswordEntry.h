#ifndef PASSWORDENTRY_H
#define PASSWORDENTRY_H

#include <QWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QHBoxLayout>

class PasswordEntry : public QWidget
{
    Q_OBJECT

private:
    QLineEdit *passwordInput;
    QCheckBox *showPasswordCheck;
    bool passwordVisible;

private slots:
    void onShowPassword(bool checked);

public:
    explicit PasswordEntry(QWidget *parent = nullptr,
                           const QString &value = "",
                           const QSize &size = QSize(200, -1));

    ~PasswordEntry();

    QString getValue() const;
    void setValue(const QString &value);
    void setEditable(bool editable);
    void setPlaceholderText(const QString &text);

    // Для доступа к QLineEdit (для сигналов)
    QLineEdit* getLineEdit() const { return passwordInput; }

    // Для совместимости с wxWidgets API
    void SetValue(const QString &value) { setValue(value); }
    QString GetValue() const { return getValue(); }
    void SetEditable(bool editable) { setEditable(editable); }

signals:
    void textChanged(const QString &text);  // Добавляем сигнал

};

#endif // PASSWORDENTRY_H
