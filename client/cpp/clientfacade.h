#pragma once

#include "model/contactlistmodel.h"
#include "model/conversationlistmodel.h"
#include "model/currentusermodel.h"
#include "model/friendrequestlistmodel.h"
#include "model/grouplistmodel.h"
#include "model/messagelistmodel.h"
#include "model/tasklistmodel.h"

#include <QObject>
#include <QString>
#include <QVariantList>

class ClientSettings;
class ClientMessageRouter;
class AuthService;
class ChatService;
class ContactService;
class GroupService;
class GroupListModel;

class ClientFacade : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY isLoggedInChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString serverHost READ serverHost WRITE setServerHost NOTIFY serverHostChanged)
    Q_PROPERTY(int serverPort READ serverPort WRITE setServerPort NOTIFY serverPortChanged)
    Q_PROPERTY(int themeMode READ themeMode CONSTANT)

    Q_PROPERTY(CurrentUserModel *currentUser READ currentUser CONSTANT)
    Q_PROPERTY(ConversationListModel *conversations READ conversations CONSTANT)
    Q_PROPERTY(ContactListModel *contacts READ contacts CONSTANT)
    Q_PROPERTY(FriendRequestListModel *friendRequests READ friendRequests CONSTANT)
    Q_PROPERTY(int pendingFriendRequestCount READ pendingFriendRequestCount NOTIFY pendingFriendRequestCountChanged)
    Q_PROPERTY(int pendingGroupInviteCount READ pendingGroupInviteCount CONSTANT)
    Q_PROPERTY(int notificationCount READ notificationCount NOTIFY notificationCountChanged)
    Q_PROPERTY(int unreadMessageCount READ unreadMessageCount NOTIFY unreadMessageCountChanged)
    Q_PROPERTY(MessageListModel *messages READ messages CONSTANT)
    Q_PROPERTY(GroupListModel *groups READ groups CONSTANT)
    Q_PROPERTY(TaskListModel *tasks READ tasks CONSTANT)
    // [Feature 2] 智能标签开关：QML 读写 ClientFacade.smartTagEnabled
    Q_PROPERTY(bool smartTagEnabled READ smartTagEnabled WRITE setSmartTagEnabled NOTIFY smartTagEnabledChanged)

public:
    explicit ClientFacade(QObject *parent = nullptr);

    bool isLoggedIn() const;
    bool busy() const;
    QString serverHost() const;
    int serverPort() const;
    int themeMode() const { return 0; }

    CurrentUserModel *currentUser() const { return currentUser_; }
    ConversationListModel *conversations() const { return conversations_; }
    ContactListModel *contacts() const { return contacts_; }
    FriendRequestListModel *friendRequests() const { return friendRequests_; }
    int pendingFriendRequestCount() const;
    int pendingGroupInviteCount() const { return 0; }
    int notificationCount() const;
    int unreadMessageCount() const;
    MessageListModel *messages() const { return messages_; }
    GroupListModel *groups() const { return groups_; }
    TaskListModel *tasks() const { return tasks_; }

    void setServerHost(const QString &host);
    void setServerPort(int port);

    Q_INVOKABLE void saveServerSettings();
    Q_INVOKABLE void registerUser(const QString &nickname,
                                  const QString &account,
                                  const QString &password);
    Q_INVOKABLE void loginUser(const QString &account, const QString &password);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void searchUser(const QString &account);
    Q_INVOKABLE void sendFriendRequest(const QString &toAccount);
    Q_INVOKABLE void acceptFriendRequest(const QString &fromAccount);
    Q_INVOKABLE void rejectFriendRequest(const QString &fromAccount);
    Q_INVOKABLE void refreshFriendRequests();
    Q_INVOKABLE void refreshFriendList();
    Q_INVOKABLE void syncAfterLogin();
    /** 本地已登录时向服务端恢复会话（拉离线私聊）；无保存密码时仅同步好友 */
    Q_INVOKABLE void resumeSession();
    Q_INVOKABLE void openChat(const QString &peerAccount, const QString &peerTitle);
    Q_INVOKABLE void sendPrivateMessage(const QString &content);
    Q_INVOKABLE void sendGroupMessage(qint64 groupId, const QString &content);
    Q_INVOKABLE void createGroup(const QString &name, const QStringList &memberAccounts);
    Q_INVOKABLE void inviteGroupMembers(qint64 groupId, const QStringList &memberAccounts);
    Q_INVOKABLE void leaveGroup(qint64 groupId);
    Q_INVOKABLE void refreshMyGroups();
    Q_INVOKABLE void fetchGroupInfo(qint64 groupId);
    // [Feature 2] 智能标签开关
    Q_INVOKABLE bool smartTagEnabled() const;
    Q_INVOKABLE void setSmartTagEnabled(bool enabled);

    // [Feature 1] 个人事务管家
    Q_INVOKABLE void markAsTask(const QString &conversationId, const QString &peerAccount,
                                const QString &text, const QString &senderName = QString());
    Q_INVOKABLE void extractTasks();
    Q_INVOKABLE void reloadTasks();

signals:
    void isLoggedInChanged();
    void busyChanged();
    void serverHostChanged();
    void serverPortChanged();
    void registerFinished(bool success, const QString &message, qint64 userId);
    void loginFinished(bool success, const QString &message);
    void sessionResumeFailed(const QString &message);
    void searchUserFinished(bool success,
                            const QString &message,
                            qint64 userId,
                            const QString &account,
                            const QString &nickname);
    void friendRequestFinished(bool success, const QString &message);
    void acceptFriendFinished(bool success, const QString &message);
    void rejectFriendFinished(bool success, const QString &message);
    void friendNotify(const QString &message);
    void pendingFriendRequestCountChanged();
    void notificationCountChanged();
    void unreadMessageCountChanged();
    void conversationsUpdated();
    void createGroupFinished(bool success, const QString &message, qint64 groupId);
    void groupListUpdated();
    void groupInfoLoaded(bool success,
                         const QString &message,
                         qint64 groupId,
                         const QString &name,
                         const QVariantList &members);
    void sendMessageFinished(bool success, const QString &message);
    void chatHistoryLoaded();
    void smartTagEnabledChanged();

private:
    ClientSettings *settings_ = nullptr;
    ClientMessageRouter *router_ = nullptr;
    CurrentUserModel *currentUser_ = nullptr;
    ConversationListModel *conversations_ = nullptr;
    ContactListModel *contacts_ = nullptr;
    FriendRequestListModel *friendRequests_ = nullptr;
    MessageListModel *messages_ = nullptr;
    GroupListModel *groups_ = nullptr;
    TaskListModel *tasks_ = nullptr;
    AuthService *authService_ = nullptr;
    ChatService *chatService_ = nullptr;
    ContactService *contactService_ = nullptr;
    GroupService *groupService_ = nullptr;

    void onAuthSessionReady();
};
