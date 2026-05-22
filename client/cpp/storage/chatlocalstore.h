#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QVector>

#include <QSqlDatabase>

struct ChatPreview {
    QString lastMessage;
    QString timeText;
    qint64 createdAt = 0;
};

struct ChatLocalRecord {
    QString fromAccount;
    QString content;
    qint64 createdAt = 0;
    qint64 serverMessageId = 0;
};

struct ChatConversationInfo {
    QString conversationId;
    QString peerAccount;
    QString title;
    bool isGroup = false;
    qint64 groupId = 0;
    QString groupName;
    bool dissolved = false;
    qint64 lastMessageAt = 0;
};

/**
 * @brief 客户端本地聊天记录（Qt Sql + QSQLITE，按登录账号分库）
 */
class ChatLocalStore : public QObject
{
    Q_OBJECT
public:
    explicit ChatLocalStore(QObject *parent = nullptr);
    ~ChatLocalStore();

    bool openForAccount(const QString &ownerAccount);
    void close();

    bool isOpen() const;
    QString ownerAccount() const { return ownerAccount_; }

    bool insertMessage(const QString &peerAccount,
                       const QString &fromAccount,
                       const QString &content,
                       qint64 createdAt,
                       qint64 serverMessageId);

    QVector<ChatLocalRecord> listMessages(const QString &peerAccount, int limit = 500) const;

    ChatPreview previewForPeer(const QString &peerAccount) const;

    QHash<QString, ChatPreview> allPeerPreviews() const;
    bool upsertConversationMeta(const QString &conversationId,
                                const QString &peerAccount,
                                const QString &title,
                                bool isGroup,
                                qint64 groupId,
                                const QString &groupName,
                                bool dissolved,
                                qint64 lastMessageAt);
    QVector<ChatConversationInfo> listConversations() const;

private:
    bool ensureSchema();
    QSqlDatabase database() const;

    QString ownerAccount_;
    QString dbPath_;
    QString connectionName_;
};
