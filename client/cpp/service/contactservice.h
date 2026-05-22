#pragma once

#include <QByteArray>
#include <QJsonArray>
#include <QObject>
#include <QString>

class ClientSettings;
class ClientMessageRouter;
class CurrentUserModel;
class ContactListModel;
class FriendRequestListModel;
class ConversationListModel;

class ContactService : public QObject
{
    Q_OBJECT
public:
    explicit ContactService(ClientSettings *settings,
                            CurrentUserModel *currentUser,
                            ClientMessageRouter *router,
                            ContactListModel *contacts,
                            FriendRequestListModel *friendRequests,
                            ConversationListModel *conversations,
                            QObject *parent = nullptr);

    Q_INVOKABLE void searchUser(const QString &account);
    Q_INVOKABLE void sendFriendRequest(const QString &toAccount);
    Q_INVOKABLE void refreshFriendRequests();
    Q_INVOKABLE void acceptFriendRequest(const QString &fromAccount);
    Q_INVOKABLE void rejectFriendRequest(const QString &fromAccount);
    Q_INVOKABLE void refreshFriendList();
    Q_INVOKABLE void syncAfterLogin();
    Q_INVOKABLE void refreshAllFriendData();

    /** 退出登录时清空本地好友/申请/会话缓存（非 signal） */
    void clearLocalData();

signals:
    void searchUserFinished(bool success,
                            const QString &message,
                            qint64 userId,
                            const QString &account,
                            const QString &nickname);
    void friendRequestFinished(bool success, const QString &message);
    void acceptFriendFinished(bool success, const QString &message);
    void rejectFriendFinished(bool success, const QString &message);
    void friendRequestsUpdated();
    void friendListUpdated();
    void conversationsUpdated();
    /** 需弹窗提示时发出（不含好友申请，申请仅用红点） */
    void friendNotify(const QString &message);

private:
    void handleFriendNotify(const QByteArray &body);
    void applyFriendRequestList(const QJsonArray &arr);
    void applyFriendList(const QJsonArray &arr);
    void syncConversationsFromContacts();
    bool ensureLoggedIn() const;

    ClientSettings *settings_ = nullptr;
    CurrentUserModel *currentUser_ = nullptr;
    ClientMessageRouter *router_ = nullptr;
    ContactListModel *contacts_ = nullptr;
    FriendRequestListModel *friendRequests_ = nullptr;
    ConversationListModel *conversations_ = nullptr;
};
