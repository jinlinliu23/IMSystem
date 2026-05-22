#include "model/conversationlistmodel.h"

ConversationListModel::ConversationListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ConversationListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return items_.size();
}

QVariant ConversationListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= items_.size()) {
        return {};
    }

    const Item &item = items_.at(index.row());
    switch (role) {
    case ConversationIdRole:
        return item.conversationId;
    case PeerAccountRole:
        return item.peerAccount;
    case TitleRole:
        return item.title;
    case LastMessageRole:
        return item.lastMessage;
    case TimeRole:
        return item.time;
    default:
        return {};
    }
}

QHash<int, QByteArray> ConversationListModel::roleNames() const
{
    return {
        {ConversationIdRole, "conversationId"},
        {PeerAccountRole, "peerAccount"},
        {TitleRole, "name"},
        {LastMessageRole, "last"},
        {TimeRole, "time"},
    };
}

void ConversationListModel::setConversations(const QVector<QVariantMap> &rows)
{
    beginResetModel();
    items_.clear();
    items_.reserve(rows.size());
    for (const auto &row : rows) {
        Item item;
        item.conversationId = row.value(QStringLiteral("conversationId")).toString();
        item.peerAccount = row.value(QStringLiteral("peerAccount")).toString();
        item.title = row.value(QStringLiteral("title")).toString();
        item.lastMessage = row.value(QStringLiteral("lastMessage")).toString();
        item.time = row.value(QStringLiteral("time")).toString();
        items_.append(item);
    }
    endResetModel();
}

void ConversationListModel::updatePreview(const QString &peerAccount,
                                          const QString &lastMessage,
                                          const QString &time)
{
    for (int i = 0; i < items_.size(); ++i) {
        if (items_.at(i).peerAccount != peerAccount) {
            continue;
        }
        items_[i].lastMessage = lastMessage;
        items_[i].time = time;
        const QModelIndex idx = index(i);
        emit dataChanged(idx, idx, {LastMessageRole, TimeRole});
        return;
    }
}

QString ConversationListModel::lastMessageOf(const QString &peerAccount) const
{
    for (const auto &item : items_) {
        if (item.peerAccount == peerAccount) {
            return item.lastMessage;
        }
    }
    return {};
}

QString ConversationListModel::timeOf(const QString &peerAccount) const
{
    for (const auto &item : items_) {
        if (item.peerAccount == peerAccount) {
            return item.time;
        }
    }
    return {};
}

void ConversationListModel::clear()
{
    if (items_.isEmpty()) {
        return;
    }
    beginResetModel();
    items_.clear();
    endResetModel();
}
