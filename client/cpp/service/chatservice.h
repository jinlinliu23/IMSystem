#pragma once

#include <QObject>
#include <QString>

class ClientSettings;
class ClientMessageRouter;
class CurrentUserModel;
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
                         QObject *parent = nullptr);

    void setOwnerAccount(const QString &account);
    void mergeLocalPreviewsIntoConversations();

    Q_INVOKABLE void openChat(const QString &peerAccount, const QString &peerTitle);
    Q_INVOKABLE void sendMessage(const QString &content);
    void clearSession();

signals:
    void sendMessageFinished(bool success, const QString &message);
    void chatHistoryLoaded();
    void conversationPreviewUpdated();

private:
    void handlePrivateNotify(const QByteArray &body);
    bool persistMessage(const QString &peerAccount,
                        const QString &fromAccount,
                        const QString &content,
                        qint64 createdAt,
                        qint64 serverMessageId);
    void loadLocalMessagesToUi(const QString &peerAccount);
    void appendChatLine(const QString &who, const QString &text);
    void updateConversationPreview(const QString &peerAccount,
                                   const QString &previewText,
                                   qint64 createdAt);
    static QString formatMessageTime(qint64 unixSec);
    static QString previewText(const QString &content);
    bool ensureLoggedIn() const;
    bool ensureLocalStoreReady();
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
    ChatLocalStore *localStore_ = nullptr;

    QString activePeerAccount_;
    QString activePeerTitle_;
};
