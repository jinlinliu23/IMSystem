#include "clientfacade.h"

#include "core/clientsettings.h"
#include "model/contactlistmodel.h"
#include "model/conversationlistmodel.h"
#include "model/currentusermodel.h"
#include "model/friendrequestlistmodel.h"
#include "model/messagelistmodel.h"
#include "model/grouplistmodel.h"
#include "network/clientmessagerouter.h"
#include "service/authservice.h"
#include "service/chatservice.h"
#include "service/contactservice.h"
#include "service/groupservice.h"

#include <QDebug>

ClientFacade::ClientFacade(QObject *parent)
    : QObject(parent)
    , settings_(new ClientSettings(this))
    , router_(new ClientMessageRouter(this))
    , currentUser_(new CurrentUserModel(this))
    , conversations_(new ConversationListModel(this))
    , contacts_(new ContactListModel(this))
    , friendRequests_(new FriendRequestListModel(this))
    , messages_(new MessageListModel(this))
    , groups_(new GroupListModel(this))
    , authService_(new AuthService(settings_, currentUser_, router_, this))
    , chatService_(new ChatService(settings_, currentUser_, router_, conversations_, messages_, contacts_, this))
    , contactService_(new ContactService(settings_,
                                         currentUser_,
                                         router_,
                                         contacts_,
                                         friendRequests_,
                                         conversations_,
                                         this))
    , groupService_(new GroupService(settings_,
                                     currentUser_,
                                     router_,
                                     groups_,
                                     conversations_,
                                     this))
{
    currentUser_->loadFromSettings(settings_);
    if (currentUser_->loggedIn() && chatService_) {
        chatService_->setOwnerAccount(currentUser_->account());
    }

    connect(settings_, &ClientSettings::serverHostChanged, this, &ClientFacade::serverHostChanged);
    connect(settings_, &ClientSettings::serverPortChanged, this, &ClientFacade::serverPortChanged);
    connect(settings_, &ClientSettings::authChanged, this, &ClientFacade::isLoggedInChanged);
    connect(currentUser_, &CurrentUserModel::userChanged, this, &ClientFacade::isLoggedInChanged);
    connect(router_, &ClientMessageRouter::busyChanged, this, &ClientFacade::busyChanged);

    connect(authService_, &AuthService::registerFinished, this, &ClientFacade::registerFinished);
    connect(authService_, &AuthService::loginFinished, this, [this](bool success, const QString &message) {
        if (success) {
            onAuthSessionReady();
        }
        emit loginFinished(success, message);
    });

    connect(authService_, &AuthService::sessionResumeFinished, this, [this](bool success, const QString &message) {
        if (success) {
            onAuthSessionReady();
        } else if (!message.isEmpty()) {
            emit sessionResumeFailed(message);
        }
    });

    connect(contactService_, &ContactService::searchUserFinished, this, &ClientFacade::searchUserFinished);
    connect(contactService_, &ContactService::friendRequestFinished, this, &ClientFacade::friendRequestFinished);
    connect(contactService_, &ContactService::acceptFriendFinished, this, &ClientFacade::acceptFriendFinished);
    connect(contactService_, &ContactService::rejectFriendFinished, this, &ClientFacade::rejectFriendFinished);
    connect(contactService_, &ContactService::friendNotify, this, &ClientFacade::friendNotify);
    connect(contactService_, &ContactService::friendRequestsUpdated, this, [this]() {
        emit pendingFriendRequestCountChanged();
        emit notificationCountChanged();
    });
    connect(friendRequests_, &FriendRequestListModel::pendingCountChanged, this, [this]() {
        emit pendingFriendRequestCountChanged();
        emit notificationCountChanged();
    });
    connect(groupService_, &GroupService::createGroupFinished, this, &ClientFacade::createGroupFinished);
    connect(groupService_, &GroupService::inviteMembersFinished, this, [this](bool success, const QString &message) {
        if (success && groupService_) {
            groupService_->refreshMyGroupsDeferred();
        }
        emit friendNotify(message);
    });
    connect(groupService_, &GroupService::leaveGroupFinished, this, [this](bool success, const QString &message, bool dissolved) {
        Q_UNUSED(dissolved)
        if (success && groupService_) {
            groupService_->refreshMyGroupsDeferred();
        }
        emit friendNotify(message);
    });
    connect(groupService_, &GroupService::sendGroupMessageFinished, this, &ClientFacade::sendMessageFinished);
    connect(groupService_, &GroupService::groupMessageSent, chatService_, &ChatService::onOwnGroupMessageSent);
    connect(groupService_, &GroupService::groupListUpdated, this, [this]() {
        if (chatService_) {
            chatService_->mergeLocalPreviewsIntoConversations();
            chatService_->syncConversationsFromLocalStore();
        }
        emit groupListUpdated();
        emit conversationsUpdated();
    });
    connect(groupService_, &GroupService::groupInfoLoaded, this, &ClientFacade::groupInfoLoaded);
    connect(contactService_, &ContactService::conversationsUpdated, this, [this]() {
        if (chatService_) {
            chatService_->mergeLocalPreviewsIntoConversations();
        }
        emit conversationsUpdated();
    });
    connect(chatService_, &ChatService::sendMessageFinished, this, &ClientFacade::sendMessageFinished);
    connect(chatService_, &ChatService::chatHistoryLoaded, this, &ClientFacade::chatHistoryLoaded);
    connect(chatService_, &ChatService::conversationPreviewUpdated, this, &ClientFacade::conversationsUpdated);
    connect(chatService_, &ChatService::groupEventsChanged, this, [this]() {
        if (groupService_) {
            groupService_->refreshMyGroupsDeferred();
        }
        if (contactService_) {
            contactService_->syncAfterLogin();
        }
        emit conversationsUpdated();
        emit groupListUpdated();
    });
    connect(conversations_, &ConversationListModel::totalUnreadChanged, this, &ClientFacade::unreadMessageCountChanged);
    // [Feature 2] ChatService 开关变化 → 通知 QML
    connect(chatService_, &ChatService::smartTagEnabledChanged, this, &ClientFacade::smartTagEnabledChanged);

    qInfo() << "ClientFacade ready"
            << settings_->serverHost() << settings_->serverPort()
            << "loggedIn" << isLoggedIn();
}

bool ClientFacade::isLoggedIn() const
{
    return currentUser_ && currentUser_->loggedIn();
}

bool ClientFacade::busy() const
{
    return router_ && router_->busy();
}

QString ClientFacade::serverHost() const
{
    return settings_ ? settings_->serverHost() : QString();
}

int ClientFacade::serverPort() const
{
    return settings_ ? settings_->serverPort() : 16701;
}

void ClientFacade::setServerHost(const QString &host)
{
    if (settings_) {
        settings_->setServerHost(host);
    }
}

void ClientFacade::setServerPort(int port)
{
    if (settings_) {
        settings_->setServerPort(port);
    }
}

void ClientFacade::saveServerSettings()
{
    if (settings_) {
        settings_->saveServer();
    }
}

void ClientFacade::registerUser(const QString &nickname,
                                const QString &account,
                                const QString &password)
{
    if (authService_) {
        authService_->registerUser(nickname, account, password);
    }
}

void ClientFacade::loginUser(const QString &account, const QString &password)
{
    if (authService_) {
        authService_->loginUser(account, password);
    }
}

void ClientFacade::searchUser(const QString &account)
{
    if (contactService_) {
        contactService_->searchUser(account);
    }
}

void ClientFacade::sendFriendRequest(const QString &toAccount)
{
    if (contactService_) {
        contactService_->sendFriendRequest(toAccount);
    }
}

void ClientFacade::acceptFriendRequest(const QString &fromAccount)
{
    if (contactService_) {
        contactService_->acceptFriendRequest(fromAccount);
    }
}

void ClientFacade::rejectFriendRequest(const QString &fromAccount)
{
    if (contactService_) {
        contactService_->rejectFriendRequest(fromAccount);
    }
}

void ClientFacade::refreshFriendRequests()
{
    if (contactService_) {
        contactService_->refreshFriendRequests();
    }
}

void ClientFacade::refreshFriendList()
{
    if (contactService_) {
        contactService_->refreshFriendList();
    }
}

void ClientFacade::syncAfterLogin()
{
    if (contactService_) {
        contactService_->syncAfterLogin();
    }
}

void ClientFacade::onAuthSessionReady()
{
    if (chatService_ && currentUser_) {
        chatService_->setOwnerAccount(currentUser_->account());
    }
    if (contactService_) {
        contactService_->syncAfterLogin();
    }
    if (groupService_) {
        groupService_->refreshMyGroupsDeferred();
    }
}

void ClientFacade::resumeSession()
{
    if (!isLoggedIn()) {
        return;
    }
    if (authService_ && settings_ && settings_->hasSessionPassword()) {
        authService_->resumeSession();
        return;
    }
    onAuthSessionReady();
}

void ClientFacade::openChat(const QString &peerAccount, const QString &peerTitle)
{
    if (chatService_) {
        chatService_->openChat(peerAccount, peerTitle);
    }
}

void ClientFacade::sendPrivateMessage(const QString &content)
{
    if (chatService_) {
        chatService_->sendMessage(content);
    }
}

void ClientFacade::sendGroupMessage(qint64 groupId, const QString &content)
{
    if (groupService_) {
        groupService_->sendGroupMessage(groupId, content);
    }
}

void ClientFacade::inviteGroupMembers(qint64 groupId, const QStringList &memberAccounts)
{
    if (groupService_) {
        groupService_->inviteMembers(groupId, memberAccounts);
    }
}

void ClientFacade::leaveGroup(qint64 groupId)
{
    if (groupService_) {
        groupService_->leaveGroup(groupId);
    }
}

void ClientFacade::logout()
{
    if (chatService_) {
        chatService_->clearSession();
    }
    if (groupService_) {
        groupService_->clearLocalData();
    }
    if (contactService_) {
        contactService_->clearLocalData();
    }
    if (authService_) {
        authService_->logout();
    }
}

int ClientFacade::pendingFriendRequestCount() const
{
    return friendRequests_ ? friendRequests_->pendingCount() : 0;
}

int ClientFacade::notificationCount() const
{
    return pendingFriendRequestCount() + pendingGroupInviteCount();
}

int ClientFacade::unreadMessageCount() const
{
    return conversations_ ? conversations_->totalUnreadCount() : 0;
}

void ClientFacade::createGroup(const QString &name, const QStringList &memberAccounts)
{
    if (groupService_) {
        groupService_->createGroup(name, memberAccounts);
    }
}

void ClientFacade::refreshMyGroups()
{
    if (groupService_) {
        groupService_->refreshMyGroups();
    }
}

void ClientFacade::fetchGroupInfo(qint64 groupId)
{
    if (groupService_) {
        groupService_->fetchGroupInfo(groupId);
    }
}

// [Feature 2] 智能标签开关：QML → ClientFacade → ChatService
bool ClientFacade::smartTagEnabled() const
{
    return chatService_ ? chatService_->smartTagEnabled() : false;
}

void ClientFacade::setSmartTagEnabled(bool enabled)
{
    if (chatService_) {
        chatService_->setSmartTagEnabled(enabled);
    }
}
