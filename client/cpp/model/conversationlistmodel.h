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
    };

    explicit ConversationListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setConversations(const QVector<QVariantMap> &rows);
    void updatePreview(const QString &peerAccount, const QString &lastMessage, const QString &time);
    void upsertGroupConversation(qint64 groupId, const QString &title);
    QString lastMessageOf(const QString &peerAccount) const;
    QString timeOf(const QString &peerAccount) const;
    void clear();

private:
    struct Item {
        QString conversationId;
        QString peerAccount;
        QString title;
        QString lastMessage;
        QString time;
        bool isGroup = false;
        qint64 groupId = 0;
    };

    QVector<Item> items_;
};
