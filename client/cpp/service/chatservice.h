#pragma once

#include <QObject>
#include <QString>

class ClientSettings;
class ClientMessageRouter;
class CurrentUserModel;
class ContactListModel;
class MessageListModel;
class ConversationListModel;
class ChatLocalStore;

class ChatService : public QObject
{
    Q_OBJECT
public:
    explicit ChatService(ClientSettings *settings,
                         CurrentUserModel *currentUser,
                         ClientMessageRouter *router,
                         ConversationListModel *conversations,
                         MessageListModel *messages,
                         ContactListModel *contacts,
                         QObject *parent = nullptr);

    void setOwnerAccount(const QString &account);
    void mergeLocalPreviewsIntoConversations();
    void syncConversationsFromLocalStore();

    Q_INVOKABLE void openChat(const QString &peerAccount, const QString &peerTitle);
    Q_INVOKABLE void sendMessage(const QString &content);
    Q_INVOKABLE void openGroupChat(qint64 groupId, const QString &groupName);
    Q_INVOKABLE void sendGroupMessage(qint64 groupId, const QString &content);
    Q_INVOKABLE void markGroupDissolved(qint64 groupId, const QString &groupName);
    void clearSession();

public slots:
    void onOwnGroupMessageSent(qint64 groupId, const QString &content, qint64 createdAt, qint64 messageId);

signals:
    void sendMessageFinished(bool success, const QString &message);
    void chatHistoryLoaded();
    void conversationPreviewUpdated();
    void groupStructureChanged();
    void groupEventsChanged();

private:
    void handlePrivateNotify(const QByteArray &body);
    bool persistMessage(const QString &peerAccount,
                        const QString &fromAccount,
                        const QString &content,
                        qint64 createdAt,
                        qint64 serverMessageId,
                        const QString &fromNickname = QString());
    void loadLocalMessagesToUi(const QString &peerAccount);
    void appendChatLine(const QString &who,
                        const QString &text,
                        const QString &senderName = QString(),
                        const QString &senderAccount = QString());
    QString nicknameForAccount(const QString &account) const;
    QString resolveSenderNickname(const QString &fromAccount, const QString &storedNickname) const;
    void updateConversationPreview(const QString &peerAccount,
                                   const QString &previewText,
                                   qint64 createdAt);
    void notifyIncomingMessage(const QString &conversationId,
                               const QString &previewText,
                               qint64 createdAt,
                               bool incrementUnread);
    void markConversationRead(const QString &conversationId);
    void handleGroupNotify(const QByteArray &body);
    static QString formatMessageTime(qint64 unixSec);
    static QString previewText(const QString &content);
    bool ensureLoggedIn() const;
    bool ensureLocalStoreReady();
    bool isGroupActive(qint64 groupId) const;
    qint64 effectiveMessageId(qint64 serverMessageId,
                              const QString &peerAccount,
                              const QString &fromAccount,
                              const QString &content,
                              qint64 createdAt) const;

    ClientSettings *settings_ = nullptr;
    CurrentUserModel *currentUser_ = nullptr;
    ClientMessageRouter *router_ = nullptr;
    ConversationListModel *conversations_ = nullptr;
    MessageListModel *messages_ = nullptr;
    ContactListModel *contacts_ = nullptr;
    ChatLocalStore *localStore_ = nullptr;

    QString activePeerAccount_;
    QString activePeerTitle_;
};
