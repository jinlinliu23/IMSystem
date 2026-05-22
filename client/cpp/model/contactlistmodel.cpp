#include "model/contactlistmodel.h"

#include <QVariantMap>

ContactListModel::ContactListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ContactListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return items_.size();
}

QVariant ContactListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= items_.size()) {
        return {};
    }

    const Item &item = items_.at(index.row());
    switch (role) {
    case UserIdRole:
        return item.userId;
    case AccountRole:
        return item.account;
    case NicknameRole:
        return item.nickname;
    default:
        return {};
    }
}

QHash<int, QByteArray> ContactListModel::roleNames() const
{
    return {
        {UserIdRole, "userId"},
        {AccountRole, "account"},
        {NicknameRole, "nickname"},
    };
}

void ContactListModel::setFriends(const QVector<QVariantMap> &rows)
{
    beginResetModel();
    items_.clear();
    items_.reserve(rows.size());
    for (const auto &row : rows) {
        Item item;
        item.userId = row.value(QStringLiteral("userId")).toLongLong();
        item.account = row.value(QStringLiteral("account")).toString();
        item.nickname = row.value(QStringLiteral("nickname")).toString();
        items_.append(item);
    }
    endResetModel();
}
