#pragma once

#include <QAbstractListModel>
#include <QVector>

/**
 * @brief 会话列表（消息页）
 */
class ConversationListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        ConversationIdRole = Qt::UserRole + 1,
        PeerAccountRole,
        TitleRole,
        LastMessageRole,
        TimeRole,
        IsGroupRole,
        GroupIdRole,
        DissolvedRole,
        UnreadCountRole,
        HasUnreadRole,
    };

    explicit ConversationListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setConversations(const QVector<QVariantMap> &rows);
    void updatePreview(const QString &peerAccount, const QString &lastMessage, const QString &time, qint64 sortKey = 0);
    void upsertFriendConversation(const QString &peerAccount,
                                  const QString &title,
                                  const QString &lastMessage = QString(),
                                  const QString &time = QString(),
                                  qint64 sortKey = 0,
                                  int unreadCount = -1);
    void upsertGroupConversation(qint64 groupId, const QString &title, qint64 sortKey = 0);
    void markDissolved(qint64 groupId);
    void incrementUnread(const QString &peerAccount);
    void setUnreadCount(const QString &peerAccount, int count);
    void clearUnread(const QString &peerAccount);
    int unreadCountOf(const QString &peerAccount) const;
    int totalUnreadCount() const;
    QString lastMessageOf(const QString &peerAccount) const;
    QString timeOf(const QString &peerAccount) const;
    void clear();

signals:
    void totalUnreadChanged();

private:
    struct Item {
        QString conversationId;
        QString peerAccount;
        QString title;
        QString lastMessage;
        QString time;
        bool isGroup = false;
        qint64 groupId = 0;
        qint64 sortKey = 0;
        bool dissolved = false;
        int unreadCount = 0;
    };

    QVector<Item> items_;

    int indexOfPeer(const QString &peerAccount) const;
    void emitTotalUnreadIfChanged(int previousTotal);
    void sortItems();
};
