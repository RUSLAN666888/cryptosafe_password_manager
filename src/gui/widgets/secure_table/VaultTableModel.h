#ifndef VAULTTABLEMODEL_H
#define VAULTTABLEMODEL_H

#include <QAbstractTableModel>
#include <vector>
#include "../src/core/vault/VaultManager.h"

class VaultManager;  // forward declaration

class VaultTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    // Номера колонок
    enum Columns {
        COL_TITLE = 0,
        COL_USERNAME,
        COL_URL,
        COL_PASSWORD,
        COL_MODIFIED,
        COL_COUNT
    };

    long getId(int row) const;

    explicit VaultTableModel(VaultManager& vaultManager, QObject* parent = nullptr);

    // Обязательные методы QAbstractTableModel
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    // Обновление данных
    void refresh();


    // Управление видимостью паролей
    void setPasswordsVisible(bool visible);
    bool passwordsVisible() const { return m_passwordsVisible; }

    // Переключение видимости для конкретной строки (глаз в колонке)
    void togglePasswordVisibilityForRow(int row);

    // Получение реального пароля
    QString getRealPassword(int row) const;

    void updatePasswordInCache(long id, const std::string& newPassword);

private:
    VaultManager& m_vaultManager;
    std::vector<VaultManager::EntryMetadata> m_data;

    // Вспомогательные методы для форматирования
    QString extractDomain(const QString& url) const;
    QString formatDate(const QString& date) const;
    QString maskUsername(const QString& username);

    bool m_passwordsVisible = false;
    QString getPasswordForRow(int row) const;
    mutable QHash<long, QString> m_passwordCache;  // кэш расшифрованных паролей

    void loadPasswordForRow(int row) const;

    QHash<int, bool> m_rowPasswordVisible;     // индивидуальное состояние для каждой строки
};

#endif // VAULTTABLEMODEL_H
