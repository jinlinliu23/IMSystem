#include "service/contactservice.h"

#include "core/clientsettings.h"
#include "model/contactlistmodel.h"
#include "model/conversationlistmodel.h"
#include "model/currentusermodel.h"
#include "model/friendrequestlistmodel.h"
#include "network/clientmessagerouter.h"
#include "msg_ids.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantMap>

namespace {

bool parseResponseObject(const QByteArray &body, QJsonObject *out, QString *errMsg)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errMsg) {
            *errMsg = QStringLiteral("服务器响应格式错误");
        }
        return false;
    }
    *out = doc.object();
    return true;
}

} // namespace

ContactService::ContactService(ClientSettings *settings,
                               CurrentUserModel *currentUser,
                               ClientMessageRouter *router,
                               ContactListModel *contacts,
                               FriendRequestListModel *friendRequests,
                               ConversationListModel *conversations,
                               QObject *parent)
    : QObject(parent)
    , settings_(settings)
    , currentUser_(currentUser)
    , router_(router)
    , contacts_(contacts)
    , friendRequests_(friendRequests)
    , conversations_(conversations)
{
    if (router_) {
        router_->registerPushHandler(static_cast<quint16>(MSG_IDS::MSG_NOTIFY_FRIEND),
                                     [this](const QByteArray &body) {
                                         handleFriendNotify(body);
                                     });
    }
}

void ContactService::searchUser(const QString &account)
{
    if (!settings_ || !router_ || !currentUser_) {
        emit searchUserFinished(false, QStringLiteral("内部错误"), 0, {}, {});
        return;
    }
    if (!currentUser_->loggedIn()) {
        emit searchUserFinished(false, QStringLiteral("请先登录"), 0, {}, {});
        return;
    }
    if (router_->busy()) {
        emit searchUserFinished(false, QStringLiteral("请稍候，正在处理上一请求"), 0, {}, {});
        return;
    }
    const QString targetAccount = account.trimmed();
    if (targetAccount.size() < 3) {
        emit searchUserFinished(false, QStringLiteral("请输入至少 3 位账号"), 0, {}, {});
        return;
    }
    if (settings_->serverHost().isEmpty()) {
        emit searchUserFinished(false, QStringLiteral("服务器地址未配置"), 0, {}, {});
        return;
    }

    QJsonObject obj;
    obj.insert(QStringLiteral("from_account"), currentUser_->account());
    obj.insert(QStringLiteral("account"), targetAccount);
    const QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);

    router_->request(
        settings_->serverHost(),
        static_cast<quint16>(settings_->serverPort()),
        static_cast<quint16>(MSG_IDS::MSG_SEARCH_USER),
        body,
        static_cast<quint16>(MSG_IDS::MSG_SEARCH_USER_RSP),
        [this](const QByteArray &rspBody) {
            QJsonObject root;
            QString err;
            if (!parseResponseObject(rspBody, &root, &err)) {
                emit searchUserFinished(false, err, 0, {}, {});
                return;
            }
            const int code = root.value(QStringLiteral("code")).toInt(-1);
            const QString msg = root.value(QStringLiteral("msg")).toString(QStringLiteral("未知错误"));
            if (code != static_cast<int>(API_CODE::OK)) {
                emit searchUserFinished(false, msg, 0, {}, {});
                return;
            }
            const QJsonObject user = root.value(QStringLiteral("user")).toObject();
            emit searchUserFinished(true,
                                    msg,
                                    user.value(QStringLiteral("user_id")).toInteger(0),
                                    user.value(QStringLiteral("account")).toString(),
                                    user.value(QStringLiteral("nickname")).toString());
        },
        [this](const QString &e) {
            emit searchUserFinished(false, e, 0, {}, {});
        });
}

void ContactService::sendFriendRequest(const QString &toAccount)
{
    if (!ensureLoggedIn()) {
        emit friendRequestFinished(false, QStringLiteral("请先登录"));
        return;
    }
    if (router_->busy()) {
        emit friendRequestFinished(false, QStringLiteral("请稍候，正在处理上一请求"));
        return;
    }

    const QString target = toAccount.trimmed();
    if (target.isEmpty()) {
        emit friendRequestFinished(false, QStringLiteral("请先搜索用户"));
        return;
    }

    QJsonObject obj;
    obj.insert(QStringLiteral("from_account"), currentUser_->account());
    obj.insert(QStringLiteral("to_account"), target);
    const QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);

    router_->request(
        settings_->serverHost(),
        static_cast<quint16>(settings_->serverPort()),
        static_cast<quint16>(MSG_IDS::MSG_FRIEND_REQUEST),
        body,
        static_cast<quint16>(MSG_IDS::MSG_FRIEND_REQUEST_RSP),
        [this](const QByteArray &rspBody) {
            QJsonObject root;
            QString err;
            if (!parseResponseObject(rspBody, &root, &err)) {
                emit friendRequestFinished(false, err);
                return;
            }
            const int code = root.value(QStringLiteral("code")).toInt(-1);
            const QString msg = root.value(QStringLiteral("msg")).toString(QStringLiteral("未知错误"));
            emit friendRequestFinished(code == static_cast<int>(API_CODE::OK), msg);
        },
        [this](const QString &e) {
            emit friendRequestFinished(false, e);
        });
}

void ContactService::refreshFriendRequests()
{
    if (!ensureLoggedIn() || router_->busy()) {
        return;
    }

    QJsonObject obj;
    obj.insert(QStringLiteral("account"), currentUser_->account());
    const QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);

    router_->request(
        settings_->serverHost(),
        static_cast<quint16>(settings_->serverPort()),
        static_cast<quint16>(MSG_IDS::MSG_FRIEND_REQUEST_LIST),
        body,
        static_cast<quint16>(MSG_IDS::MSG_FRIEND_REQUEST_LIST_RSP),
        [this](const QByteArray &rspBody) {
            QJsonObject root;
            QString err;
            if (!parseResponseObject(rspBody, &root, &err)) {
                emit friendNotify(err);
                return;
            }
            if (root.value(QStringLiteral("code")).toInt(-1) != static_cast<int>(API_CODE::OK)) {
                return;
            }
            applyFriendRequestList(root.value(QStringLiteral("requests")).toArray());
            emit friendRequestsUpdated();
        },
        [this](const QString &e) {
            emit friendNotify(e);
        });
}

void ContactService::acceptFriendRequest(const QString &fromAccount)
{
    if (!ensureLoggedIn()) {
        emit acceptFriendFinished(false, QStringLiteral("请先登录"));
        return;
    }
    if (router_->busy()) {
        emit acceptFriendFinished(false, QStringLiteral("请稍候，正在处理上一请求"));
        return;
    }

    QJsonObject obj;
    obj.insert(QStringLiteral("account"), currentUser_->account());
    obj.insert(QStringLiteral("from_account"), fromAccount.trimmed());
    const QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);

    router_->request(
        settings_->serverHost(),
        static_cast<quint16>(settings_->serverPort()),
        static_cast<quint16>(MSG_IDS::MSG_FRIEND_ACCEPT),
        body,
        static_cast<quint16>(MSG_IDS::MSG_FRIEND_ACCEPT_RSP),
        [this](const QByteArray &rspBody) {
            QJsonObject root;
            QString err;
            if (!parseResponseObject(rspBody, &root, &err)) {
                emit acceptFriendFinished(false, err);
                return;
            }
            const int code = root.value(QStringLiteral("code")).toInt(-1);
            const QString msg = root.value(QStringLiteral("msg")).toString(QStringLiteral("未知错误"));
            const bool ok = code == static_cast<int>(API_CODE::OK);
            emit acceptFriendFinished(ok, msg);
            if (ok) {
                refreshAllFriendData();
            }
        },
        [this](const QString &e) {
            emit acceptFriendFinished(false, e);
        });
}

void ContactService::rejectFriendRequest(const QString &fromAccount)
{
    if (!ensureLoggedIn()) {
        emit rejectFriendFinished(false, QStringLiteral("请先登录"));
        return;
    }
    if (router_->busy()) {
        emit rejectFriendFinished(false, QStringLiteral("请稍候，正在处理上一请求"));
        return;
    }

    QJsonObject obj;
    obj.insert(QStringLiteral("account"), currentUser_->account());
    obj.insert(QStringLiteral("from_account"), fromAccount.trimmed());
    const QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);

    router_->request(
        settings_->serverHost(),
        static_cast<quint16>(settings_->serverPort()),
        static_cast<quint16>(MSG_IDS::MSG_FRIEND_REJECT),
        body,
        static_cast<quint16>(MSG_IDS::MSG_FRIEND_REJECT_RSP),
        [this](const QByteArray &rspBody) {
            QJsonObject root;
            QString err;
            if (!parseResponseObject(rspBody, &root, &err)) {
                emit rejectFriendFinished(false, err);
                return;
            }
            const int code = root.value(QStringLiteral("code")).toInt(-1);
            const QString msg = root.value(QStringLiteral("msg")).toString(QStringLiteral("未知错误"));
            const bool ok = code == static_cast<int>(API_CODE::OK);
            emit rejectFriendFinished(ok, msg);
            if (ok) {
                refreshAllFriendData();
            }
        },
        [this](const QString &e) {
            emit rejectFriendFinished(false, e);
        });
}

void ContactService::refreshFriendList()
{
    if (!ensureLoggedIn() || router_->busy()) {
        return;
    }

    QJsonObject obj;
    obj.insert(QStringLiteral("account"), currentUser_->account());
    const QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);

    router_->request(
        settings_->serverHost(),
        static_cast<quint16>(settings_->serverPort()),
        static_cast<quint16>(MSG_IDS::MSG_FRIEND_LIST),
        body,
        static_cast<quint16>(MSG_IDS::MSG_FRIEND_LIST_RSP),
        [this](const QByteArray &rspBody) {
            QJsonObject root;
            QString err;
            if (!parseResponseObject(rspBody, &root, &err)) {
                emit friendNotify(err);
                return;
            }
            if (root.value(QStringLiteral("code")).toInt(-1) != static_cast<int>(API_CODE::OK)) {
                return;
            }
            applyFriendList(root.value(QStringLiteral("friends")).toArray());
            emit friendListUpdated();
        },
        [this](const QString &e) {
            emit friendNotify(e);
        });
}

void ContactService::syncAfterLogin()
{
    refreshAllFriendData();
}

void ContactService::refreshAllFriendData()
{
    if (!ensureLoggedIn() || router_->busy()) {
        return;
    }

    QJsonObject obj;
    obj.insert(QStringLiteral("account"), currentUser_->account());
    const QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);

    router_->request(
        settings_->serverHost(),
        static_cast<quint16>(settings_->serverPort()),
        static_cast<quint16>(MSG_IDS::MSG_FRIEND_REQUEST_LIST),
        body,
        static_cast<quint16>(MSG_IDS::MSG_FRIEND_REQUEST_LIST_RSP),
        [this](const QByteArray &rspBody) {
            QJsonObject root;
            QString err;
            if (parseResponseObject(rspBody, &root, &err)
                && root.value(QStringLiteral("code")).toInt(-1) == static_cast<int>(API_CODE::OK)) {
                applyFriendRequestList(root.value(QStringLiteral("requests")).toArray());
                emit friendRequestsUpdated();
            }

            QJsonObject listObj;
            listObj.insert(QStringLiteral("account"), currentUser_->account());
            const QByteArray listBody = QJsonDocument(listObj).toJson(QJsonDocument::Compact);
            router_->request(
                settings_->serverHost(),
                static_cast<quint16>(settings_->serverPort()),
                static_cast<quint16>(MSG_IDS::MSG_FRIEND_LIST),
                listBody,
                static_cast<quint16>(MSG_IDS::MSG_FRIEND_LIST_RSP),
                [this](const QByteArray &listRsp) {
                    QJsonObject listRoot;
                    QString listErr;
                    if (parseResponseObject(listRsp, &listRoot, &listErr)
                        && listRoot.value(QStringLiteral("code")).toInt(-1)
                               == static_cast<int>(API_CODE::OK)) {
                        applyFriendList(listRoot.value(QStringLiteral("friends")).toArray());
                        emit friendListUpdated();
                    }
                },
                [](const QString &) {});
        },
        [](const QString &) {});
}

void ContactService::handleFriendNotify(const QByteArray &body)
{
    QJsonObject root;
    QString err;
    if (!parseResponseObject(body, &root, &err)) {
        emit friendNotify(err);
        return;
    }

    const QString type = root.value(QStringLiteral("type")).toString();
    const QString fromAccount = root.value(QStringLiteral("from_account")).toString();
    const QString fromNick = root.value(QStringLiteral("from_nickname")).toString();

    if (type == QStringLiteral("friend_request")) {
        refreshAllFriendData();
    } else if (type == QStringLiteral("friend_accepted")) {
        refreshAllFriendData();
        emit friendNotify(QStringLiteral("%1 已同意你的好友申请").arg(fromNick.isEmpty() ? fromAccount : fromNick));
    } else if (type == QStringLiteral("friend_rejected")) {
        refreshAllFriendData();
        emit friendNotify(QStringLiteral("%1 拒绝了你的好友申请").arg(fromNick.isEmpty() ? fromAccount : fromNick));
    } else {
        refreshAllFriendData();
    }
}

void ContactService::applyFriendRequestList(const QJsonArray &arr)
{
    if (!friendRequests_) {
        return;
    }
    QVector<QVariantMap> rows;
    rows.reserve(arr.size());
    for (const auto &v : arr) {
        const QJsonObject o = v.toObject();
        QVariantMap row;
        row.insert(QStringLiteral("requestId"), o.value(QStringLiteral("request_id")).toVariant());
        row.insert(QStringLiteral("fromUserId"), o.value(QStringLiteral("from_user_id")).toVariant());
        row.insert(QStringLiteral("fromAccount"), o.value(QStringLiteral("from_account")).toString());
        row.insert(QStringLiteral("fromNickname"), o.value(QStringLiteral("from_nickname")).toString());
        rows.append(row);
    }
    friendRequests_->setRequests(rows);
}

void ContactService::applyFriendList(const QJsonArray &arr)
{
    if (!contacts_) {
        return;
    }
    QVector<QVariantMap> rows;
    rows.reserve(arr.size());
    for (const auto &v : arr) {
        const QJsonObject o = v.toObject();
        QVariantMap row;
        row.insert(QStringLiteral("userId"), o.value(QStringLiteral("user_id")).toVariant());
        row.insert(QStringLiteral("account"), o.value(QStringLiteral("account")).toString());
        row.insert(QStringLiteral("nickname"), o.value(QStringLiteral("nickname")).toString());
        rows.append(row);
    }
    contacts_->setFriends(rows);
    syncConversationsFromContacts();
    emit conversationsUpdated();
}

void ContactService::syncConversationsFromContacts()
{
    if (!conversations_ || !contacts_) {
        return;
    }

    const int count = contacts_->rowCount();
    for (int i = 0; i < count; ++i) {
        const QModelIndex idx = contacts_->index(i);
        const QString account = contacts_->data(idx, ContactListModel::AccountRole).toString();
        const QString nickname = contacts_->data(idx, ContactListModel::NicknameRole).toString();
        const QString oldLast = conversations_->lastMessageOf(account);
        const QString oldTime = conversations_->timeOf(account);
        const bool hasChat = !oldLast.isEmpty()
                             && oldLast != QStringLiteral("点击开始聊天");

        conversations_->upsertFriendConversation(
            account,
            nickname.isEmpty() ? account : nickname,
            hasChat ? oldLast : QStringLiteral("点击开始聊天"),
            hasChat ? oldTime : QString(),
            0,
            conversations_->unreadCountOf(account));
    }
}

void ContactService::clearLocalData()
{
    if (contacts_) {
        contacts_->setFriends({});
    }
    if (friendRequests_) {
        friendRequests_->setRequests({});
    }
    if (conversations_) {
        conversations_->clear();
    }
}

bool ContactService::ensureLoggedIn() const
{
    return currentUser_ && currentUser_->loggedIn() && settings_ && router_;
}
