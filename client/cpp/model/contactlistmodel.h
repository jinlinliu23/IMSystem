#pragma once

#include <QAbstractListModel>
#include <QVector>

/**
 * @brief 联系人列表
 */
class ContactListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        UserIdRole = Qt::UserRole + 1,
        AccountRole,
        NicknameRole,
    };

    explicit ContactListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setFriends(const QVector<QVariantMap> &rows);

private:
    struct Item {
        qint64 userId = 0;
        QString account;
        QString nickname;
    };

    QVector<Item> items_;
};
