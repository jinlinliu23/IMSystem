#include "model/friendrequestlistmodel.h"

#include <QVariantMap>

FriendRequestListModel::FriendRequestListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int FriendRequestListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return items_.size();
}

QVariant FriendRequestListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= items_.size()) {
        return {};
    }

    const Item &item = items_.at(index.row());
    switch (role) {
    case RequestIdRole:
        return item.requestId;
    case FromUserIdRole:
        return item.fromUserId;
    case FromAccountRole:
        return item.fromAccount;
    case FromNicknameRole:
        return item.fromNickname;
    default:
        return {};
    }
}

QHash<int, QByteArray> FriendRequestListModel::roleNames() const
{
    return {
        {RequestIdRole, "requestId"},
        {FromUserIdRole, "fromUserId"},
        {FromAccountRole, "fromAccount"},
        {FromNicknameRole, "fromNickname"},
    };
}

void FriendRequestListModel::setRequests(const QVector<QVariantMap> &rows)
{
    beginResetModel();
    items_.clear();
    items_.reserve(rows.size());
    for (const auto &row : rows) {
        Item item;
        item.requestId = row.value(QStringLiteral("requestId")).toLongLong();
        item.fromUserId = row.value(QStringLiteral("fromUserId")).toLongLong();
        item.fromAccount = row.value(QStringLiteral("fromAccount")).toString();
        item.fromNickname = row.value(QStringLiteral("fromNickname")).toString();
        items_.append(item);
    }
    endResetModel();
    emit pendingCountChanged();
}
