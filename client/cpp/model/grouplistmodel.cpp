#include "model/grouplistmodel.h"

#include <QVariantMap>

GroupListModel::GroupListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int GroupListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return items_.size();
}

QVariant GroupListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= items_.size()) {
        return {};
    }

    const Item &item = items_.at(index.row());
    switch (role) {
    case GroupIdRole:
        return item.groupId;
    case NameRole:
        return item.name;
    case MemberCountRole:
        return item.memberCount;
    default:
        return {};
    }
}

QHash<int, QByteArray> GroupListModel::roleNames() const
{
    return {
        {GroupIdRole, "groupId"},
        {NameRole, "name"},
        {MemberCountRole, "memberCount"},
    };
}

void GroupListModel::setGroups(const QVector<QVariantMap> &rows)
{
    beginResetModel();
    items_.clear();
    items_.reserve(rows.size());
    for (const auto &row : rows) {
        Item item;
        item.groupId = row.value(QStringLiteral("groupId")).toLongLong();
        item.name = row.value(QStringLiteral("name")).toString();
        item.memberCount = row.value(QStringLiteral("memberCount")).toInt();
        items_.append(item);
    }
    endResetModel();
}

void GroupListModel::clear()
{
    if (items_.isEmpty()) {
        return;
    }
    beginResetModel();
    items_.clear();
    endResetModel();
}
