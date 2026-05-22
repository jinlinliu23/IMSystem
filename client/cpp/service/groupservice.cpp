#include "service/groupservice.h"

#include "core/clientsettings.h"
#include "model/conversationlistmodel.h"
#include "model/currentusermodel.h"
#include "model/grouplistmodel.h"
#include "network/clientmessagerouter.h"
#include "msg_ids.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
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

QString apiMessage(const QJsonObject &obj, const QString &fallback)
{
    return obj.value(QStringLiteral("msg")).toString(fallback);
}

} // namespace

GroupService::GroupService(ClientSettings *settings,
                           CurrentUserModel *currentUser,
                           ClientMessageRouter *router,
                           GroupListModel *groups,
                           ConversationListModel *conversations,
                           QObject *parent)
    : QObject(parent)
    , settings_(settings)
    , currentUser_(currentUser)
    , router_(router)
    , groups_(groups)
    , conversations_(conversations)
{
}

bool GroupService::ensureLoggedIn() const
{
    return currentUser_ && currentUser_->loggedIn();
}

void GroupService::clearLocalData()
{
    if (groups_) {
        groups_->clear();
    }
}


void GroupService::applyGroupList(const QJsonArray &arr)
{
    QVector<QVariantMap> rows;
    rows.reserve(arr.size());
    for (const QJsonValue &v : arr) {
        if (!v.isObject()) {
            continue;
        }
        const QJsonObject o = v.toObject();
        QVariantMap row;
        const qint64 groupId = static_cast<qint64>(o.value(QStringLiteral("group_id")).toDouble());
        const QString name = o.value(QStringLiteral("name")).toString();
        row.insert(QStringLiteral("groupId"), groupId);
        row.insert(QStringLiteral("name"), name);
        row.insert(QStringLiteral("memberCount"), o.value(QStringLiteral("member_count")).toInt());
        rows.append(row);

        if (conversations_) {
            conversations_->upsertGroupConversation(groupId, name);
        }
    }

    if (groups_) {
        groups_->setGroups(rows);
    }
    emit groupListUpdated();
}

void GroupService::createGroup(const QString &name, const QStringList &memberAccounts)
{
    if (!settings_ || !router_ || !currentUser_) {
        emit createGroupFinished(false, QStringLiteral("内部错误"), 0);
        return;
    }
    if (!ensureLoggedIn()) {
        emit createGroupFinished(false, QStringLiteral("请先登录"), 0);
        return;
    }
    if (router_->busy()) {
        emit createGroupFinished(false, QStringLiteral("请稍候，正在处理上一请求"), 0);
        return;
    }

    const QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty()) {
        emit createGroupFinished(false, QStringLiteral("请输入群名称"), 0);
        return;
    }

    QJsonObject req;
    req.insert(QStringLiteral("creator_account"), currentUser_->account());
    req.insert(QStringLiteral("name"), trimmedName);

    QJsonArray members;
    for (const QString &acc : memberAccounts) {
        const QString trimmed = acc.trimmed();
        if (!trimmed.isEmpty() && trimmed != currentUser_->account()) {
            members.append(trimmed);
        }
    }
    req.insert(QStringLiteral("member_accounts"), members);

    const QByteArray body = QJsonDocument(req).toJson(QJsonDocument::Compact);
    const QString host = settings_->serverHost();
    const quint16 port = static_cast<quint16>(settings_->serverPort());

    router_->request(
        host,
        port,
        static_cast<quint16>(MSG_IDS::MSG_CREATE_GROUP),
        body,
        static_cast<quint16>(MSG_IDS::MSG_CREATE_GROUP_RSP),
        [this, trimmedName](const QByteArray &rspBody) {
            QJsonObject obj;
            QString err;
            if (!parseResponseObject(rspBody, &obj, &err)) {
                emit createGroupFinished(false, err, 0);
                return;
            }
            const int code = obj.value(QStringLiteral("code")).toInt(-1);
            if (code != static_cast<int>(API_CODE::OK)) {
                emit createGroupFinished(false, apiMessage(obj, QStringLiteral("创建失败")), 0);
                return;
            }
            const qint64 groupId =
                static_cast<qint64>(obj.value(QStringLiteral("group_id")).toDouble());
            const QString groupName = obj.value(QStringLiteral("name")).toString(trimmedName);
            if (conversations_) {
                conversations_->upsertGroupConversation(groupId, groupName);
            }
            refreshMyGroups();
            emit createGroupFinished(true, apiMessage(obj, QStringLiteral("创建成功")), groupId);
        },
        [this](const QString &message) {
            emit createGroupFinished(false, message, 0);
        });
}

void GroupService::refreshMyGroups()
{
    if (!settings_ || !router_ || !currentUser_ || !ensureLoggedIn()) {
        return;
    }
    if (router_->busy()) {
        QTimer::singleShot(150, this, [this]() { refreshMyGroups(); });
        return;
    }

    QJsonObject req;
    req.insert(QStringLiteral("account"), currentUser_->account());
    const QByteArray body = QJsonDocument(req).toJson(QJsonDocument::Compact);

    router_->request(
        settings_->serverHost(),
        static_cast<quint16>(settings_->serverPort()),
        static_cast<quint16>(MSG_IDS::MSG_GROUP_LIST),
        body,
        static_cast<quint16>(MSG_IDS::MSG_GROUP_LIST_RSP),
        [this](const QByteArray &rspBody) {
            QJsonObject obj;
            QString err;
            if (!parseResponseObject(rspBody, &obj, &err)) {
                return;
            }
            if (obj.value(QStringLiteral("code")).toInt(-1) != static_cast<int>(API_CODE::OK)) {
                return;
            }
            applyGroupList(obj.value(QStringLiteral("groups")).toArray());
        },
        [](const QString &) {});
}

void GroupService::fetchGroupInfo(qint64 groupId)
{
    if (!settings_ || !router_ || !currentUser_) {
        emit groupInfoLoaded(false, QStringLiteral("内部错误"), groupId, {}, {});
        return;
    }
    if (!ensureLoggedIn()) {
        emit groupInfoLoaded(false, QStringLiteral("请先登录"), groupId, {}, {});
        return;
    }
    if (router_->busy()) {
        emit groupInfoLoaded(false, QStringLiteral("请稍候，正在处理上一请求"), groupId, {}, {});
        return;
    }
    if (groupId <= 0) {
        emit groupInfoLoaded(false, QStringLiteral("群 ID 无效"), groupId, {}, {});
        return;
    }

    QJsonObject req;
    req.insert(QStringLiteral("group_id"), groupId);
    req.insert(QStringLiteral("account"), currentUser_->account());
    const QByteArray body = QJsonDocument(req).toJson(QJsonDocument::Compact);

    router_->request(
        settings_->serverHost(),
        static_cast<quint16>(settings_->serverPort()),
        static_cast<quint16>(MSG_IDS::MSG_GROUP_INFO),
        body,
        static_cast<quint16>(MSG_IDS::MSG_GROUP_INFO_RSP),
        [this, groupId](const QByteArray &rspBody) {
            QJsonObject obj;
            QString err;
            if (!parseResponseObject(rspBody, &obj, &err)) {
                emit groupInfoLoaded(false, err, groupId, {}, {});
                return;
            }
            const int code = obj.value(QStringLiteral("code")).toInt(-1);
            if (code != static_cast<int>(API_CODE::OK)) {
                emit groupInfoLoaded(false, apiMessage(obj, QStringLiteral("加载失败")), groupId, {},
                                       {});
                return;
            }

            const QString name = obj.value(QStringLiteral("name")).toString();
            QVariantList members;
            const QJsonArray arr = obj.value(QStringLiteral("members")).toArray();
            for (const QJsonValue &v : arr) {
                if (!v.isObject()) {
                    continue;
                }
                const QJsonObject m = v.toObject();
                QVariantMap row;
                row.insert(QStringLiteral("account"), m.value(QStringLiteral("account")).toString());
                row.insert(QStringLiteral("nickname"),
                           m.value(QStringLiteral("nickname")).toString());
                row.insert(QStringLiteral("role"), m.value(QStringLiteral("role")).toString());
                members.append(row);
            }
            emit groupInfoLoaded(true, apiMessage(obj, QStringLiteral("ok")), groupId, name,
                                 members);
        },
        [this, groupId](const QString &message) {
            emit groupInfoLoaded(false, message, groupId, {}, {});
        });
}

void GroupService::sendGroupMessage(qint64 groupId, const QString &content)
{
    if (!settings_ || !router_ || !currentUser_) {
        return;
    }
    if (!ensureLoggedIn()) {
        return;
    }
    if (groupId <= 0 || content.trimmed().isEmpty()) {
        return;
    }
    if (router_->busy()) {
        QTimer::singleShot(120, this, [this, groupId, content]() { sendGroupMessage(groupId, content); });
        return;
    }

    QJsonObject req;
    req.insert(QStringLiteral("group_id"), groupId);
    req.insert(QStringLiteral("from_account"), currentUser_->account());
    req.insert(QStringLiteral("content"), content.trimmed());
    const QByteArray body = QJsonDocument(req).toJson(QJsonDocument::Compact);

    router_->request(
        settings_->serverHost(),
        static_cast<quint16>(settings_->serverPort()),
        static_cast<quint16>(MSG_IDS::MSG_SEND_GROUP),
        body,
        static_cast<quint16>(MSG_IDS::MSG_SEND_GROUP_RSP),
        [this, groupId, content](const QByteArray &rspBody) {
            QJsonObject obj;
            QString err;
            if (!parseResponseObject(rspBody, &obj, &err)) {
                emit sendGroupMessageFinished(false, err);
                return;
            }
            const int code = obj.value(QStringLiteral("code")).toInt(-1);
            const QString msg = obj.value(QStringLiteral("msg")).toString(QStringLiteral("发送失败"));
            if (code != static_cast<int>(API_CODE::OK)) {
                emit sendGroupMessageFinished(false, msg);
                return;
            }
            const qint64 createdAt = obj.value(QStringLiteral("created_at")).toInteger(
                QDateTime::currentSecsSinceEpoch());
            const qint64 messageId = obj.value(QStringLiteral("message_id")).toInteger(0);
            emit groupMessageSent(groupId, content.trimmed(), createdAt, messageId);
            emit sendGroupMessageFinished(true, msg);
        },
        [this](const QString &message) {
            emit sendGroupMessageFinished(false, message);
        });
}

void GroupService::refreshMyGroupsDeferred()
{
    QTimer::singleShot(0, this, [this]() { refreshMyGroups(); });
}
