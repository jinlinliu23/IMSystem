#pragma once

#include <QAbstractListModel>
#include <QVector>

/**
 * @brief 群聊列表（联系人页「群聊」分组）
 */
class GroupListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        GroupIdRole = Qt::UserRole + 1,
        NameRole,
        MemberCountRole,
    };

    explicit GroupListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setGroups(const QVector<QVariantMap> &rows);
    void clear();

private:
    struct Item {
        qint64 groupId = 0;
        QString name;
        int memberCount = 0;
    };

    QVector<Item> items_;
};
