#pragma once

#include <QObject>
#include <QJsonArray>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QTimer>

class ClientSettings;
class ClientMessageRouter;
class CurrentUserModel;
class GroupListModel;
class ConversationListModel;

class GroupService : public QObject
{
    Q_OBJECT
public:
    explicit GroupService(ClientSettings *settings,
                          CurrentUserModel *currentUser,
                          ClientMessageRouter *router,
                          GroupListModel *groups,
                          ConversationListModel *conversations,
                          QObject *parent = nullptr);

    void createGroup(const QString &name, const QStringList &memberAccounts);
    void refreshMyGroups();
    void refreshMyGroupsDeferred();
    void fetchGroupInfo(qint64 groupId);
    void sendGroupMessage(qint64 groupId, const QString &content);
    void clearLocalData();

signals:
    void createGroupFinished(bool success, const QString &message, qint64 groupId);
    void sendGroupMessageFinished(bool success, const QString &message);
    void groupMessageSent(qint64 groupId, const QString &content, qint64 createdAt, qint64 messageId);
    void groupListUpdated();
    void groupInfoLoaded(bool success,
                         const QString &message,
                         qint64 groupId,
                         const QString &name,
                         const QVariantList &members);

private:
    void applyGroupList(const QJsonArray &arr);
    bool ensureLoggedIn() const;

    ClientSettings *settings_ = nullptr;
    CurrentUserModel *currentUser_ = nullptr;
    ClientMessageRouter *router_ = nullptr;
    GroupListModel *groups_ = nullptr;
    ConversationListModel *conversations_ = nullptr;
};
