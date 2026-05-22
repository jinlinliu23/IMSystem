#include "model/conversationlistmodel.h"

#include <algorithm>

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
    case IsGroupRole:
        return item.isGroup;
    case GroupIdRole:
        return item.groupId;
    case DissolvedRole:
        return item.dissolved;
    case UnreadCountRole:
        return item.unreadCount;
    case HasUnreadRole:
        return item.unreadCount > 0;
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
        {IsGroupRole, "isGroup"},
        {GroupIdRole, "groupId"},
        {DissolvedRole, "dissolved"},
        {UnreadCountRole, "unreadCount"},
        {HasUnreadRole, "hasUnread"},
    };
}

int ConversationListModel::indexOfPeer(const QString &peerAccount) const
{
    for (int i = 0; i < items_.size(); ++i) {
        if (items_.at(i).peerAccount == peerAccount) {
            return i;
        }
    }
    return -1;
}

void ConversationListModel::emitTotalUnreadIfChanged(int previousTotal)
{
    const int now = totalUnreadCount();
    if (now != previousTotal) {
        emit totalUnreadChanged();
    }
}

void ConversationListModel::setConversations(const QVector<QVariantMap> &rows)
{
    const int prevTotal = totalUnreadCount();
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
        item.isGroup = row.value(QStringLiteral("isGroup")).toBool();
        item.groupId = row.value(QStringLiteral("groupId")).toLongLong();
        item.unreadCount = row.value(QStringLiteral("unreadCount")).toInt();
        items_.append(item);
    }
    endResetModel();
    emitTotalUnreadIfChanged(prevTotal);
}

void ConversationListModel::updatePreview(const QString &peerAccount,
                                          const QString &lastMessage,
                                          const QString &time,
                                          qint64 sortKey)
{
    int idx = indexOfPeer(peerAccount);
    if (idx < 0) {
        if (peerAccount.startsWith(QStringLiteral("g:"))) {
            const qint64 groupId = peerAccount.mid(2).toLongLong();
            if (groupId > 0) {
                upsertGroupConversation(groupId, peerAccount, sortKey);
            }
        } else {
            upsertFriendConversation(peerAccount, peerAccount, lastMessage, time, sortKey);
        }
        idx = indexOfPeer(peerAccount);
        if (idx < 0) {
            return;
        }
    }

    items_[idx].lastMessage = lastMessage;
    items_[idx].time = time;
    if (sortKey > 0) {
        items_[idx].sortKey = sortKey;
    }

    if (sortKey > 0) {
        sortItems();
    } else {
        const QModelIndex modelIdx = index(idx);
        emit dataChanged(modelIdx, modelIdx, {LastMessageRole, TimeRole});
    }
}

void ConversationListModel::upsertFriendConversation(const QString &peerAccount,
                                                     const QString &title,
                                                     const QString &lastMessage,
                                                     const QString &time,
                                                     qint64 sortKey,
                                                     int unreadCount)
{
    const int idx = indexOfPeer(peerAccount);
    if (idx >= 0) {
        items_[idx].title = title;
        if (!lastMessage.isEmpty()) {
            items_[idx].lastMessage = lastMessage;
        }
        if (!time.isEmpty()) {
            items_[idx].time = time;
        }
        if (sortKey > 0) {
            items_[idx].sortKey = sortKey;
        }
        const QModelIndex modelIdx = index(idx);
        if (unreadCount >= 0) {
            const int prevTotal = totalUnreadCount();
            items_[idx].unreadCount = unreadCount;
            emit dataChanged(modelIdx, modelIdx);
            emitTotalUnreadIfChanged(prevTotal);
        } else {
            emit dataChanged(modelIdx, modelIdx, {TitleRole, LastMessageRole, TimeRole});
        }
        if (sortKey > 0) {
            sortItems();
        }
        return;
    }

    const int prevTotal = totalUnreadCount();
    beginInsertRows(QModelIndex(), items_.size(), items_.size());
    Item item;
    item.conversationId = peerAccount;
    item.peerAccount = peerAccount;
    item.title = title;
    item.lastMessage = lastMessage;
    item.time = time;
    item.sortKey = sortKey;
    item.unreadCount = unreadCount > 0 ? unreadCount : 0;
    items_.append(item);
    endInsertRows();
    emitTotalUnreadIfChanged(prevTotal);
    if (sortKey > 0) {
        sortItems();
    }
}

void ConversationListModel::upsertGroupConversation(qint64 groupId, const QString &title, qint64 sortKey)
{
    const QString convId = QStringLiteral("g:%1").arg(groupId);
    const int idx = indexOfPeer(convId);
    if (idx >= 0) {
        items_[idx].title = title;
        if (sortKey > 0) {
            items_[idx].sortKey = sortKey;
        }
        if (sortKey > 0) {
            sortItems();
        } else {
            const QModelIndex modelIdx = index(idx);
            emit dataChanged(modelIdx, modelIdx, {TitleRole});
        }
        return;
    }

    beginInsertRows(QModelIndex(), items_.size(), items_.size());
    Item item;
    item.conversationId = convId;
    item.peerAccount = convId;
    item.title = title;
    item.isGroup = true;
    item.groupId = groupId;
    item.sortKey = sortKey;
    items_.append(item);
    endInsertRows();
    if (sortKey > 0) {
        sortItems();
    }
}

void ConversationListModel::incrementUnread(const QString &peerAccount)
{
    const int idx = indexOfPeer(peerAccount);
    if (idx < 0) {
        return;
    }
    const int prevTotal = totalUnreadCount();
    items_[idx].unreadCount += 1;
    const QModelIndex modelIdx = index(idx);
    emit dataChanged(modelIdx, modelIdx, {UnreadCountRole, HasUnreadRole});
    emitTotalUnreadIfChanged(prevTotal);
}

void ConversationListModel::setUnreadCount(const QString &peerAccount, int count)
{
    const int idx = indexOfPeer(peerAccount);
    if (idx < 0) {
        return;
    }
    const int normalized = qMax(0, count);
    if (items_[idx].unreadCount == normalized) {
        return;
    }
    const int prevTotal = totalUnreadCount();
    items_[idx].unreadCount = normalized;
    const QModelIndex modelIdx = index(idx);
    emit dataChanged(modelIdx, modelIdx, {UnreadCountRole, HasUnreadRole});
    emitTotalUnreadIfChanged(prevTotal);
}

void ConversationListModel::clearUnread(const QString &peerAccount)
{
    setUnreadCount(peerAccount, 0);
}

int ConversationListModel::unreadCountOf(const QString &peerAccount) const
{
    const int idx = indexOfPeer(peerAccount);
    return idx >= 0 ? items_.at(idx).unreadCount : 0;
}

int ConversationListModel::totalUnreadCount() const
{
    int total = 0;
    for (const auto &item : items_) {
        total += item.unreadCount;
    }
    return total;
}

QString ConversationListModel::lastMessageOf(const QString &peerAccount) const
{
    const int idx = indexOfPeer(peerAccount);
    return idx >= 0 ? items_.at(idx).lastMessage : QString();
}

QString ConversationListModel::timeOf(const QString &peerAccount) const
{
    const int idx = indexOfPeer(peerAccount);
    return idx >= 0 ? items_.at(idx).time : QString();
}

void ConversationListModel::markDissolved(qint64 groupId)
{
    for (int i = 0; i < items_.size(); ++i) {
        if (items_.at(i).groupId == groupId) {
            items_[i].dissolved = true;
            const QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {DissolvedRole});
            return;
        }
    }
}

void ConversationListModel::sortItems()
{
    if (items_.size() <= 1) {
        return;
    }
    std::sort(items_.begin(), items_.end(),
              [](const Item &a, const Item &b) { return a.sortKey > b.sortKey; });
    beginResetModel();
    endResetModel();
}

void ConversationListModel::clear()
{
    if (items_.isEmpty()) {
        return;
    }
    const int prevTotal = totalUnreadCount();
    beginResetModel();
    items_.clear();
    endResetModel();
    emitTotalUnreadIfChanged(prevTotal);
}
