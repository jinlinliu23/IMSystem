#include "service/chatservice.h"

#include "core/clientsettings.h"
#include "model/contactlistmodel.h"
#include "model/conversationlistmodel.h"
#include "model/currentusermodel.h"
#include "model/messagelistmodel.h"
#include "network/clientmessagerouter.h"
#include "storage/chatlocalstore.h"
#include "msg_ids.h"

#include <QDateTime>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

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

ChatService::ChatService(ClientSettings *settings,
                         CurrentUserModel *currentUser,
                         ClientMessageRouter *router,
                         ConversationListModel *conversations,
                         MessageListModel *messages,
                         ContactListModel *contacts,
                         QObject *parent)
    : QObject(parent)
    , settings_(settings)
    , currentUser_(currentUser)
    , router_(router)
    , conversations_(conversations)
    , messages_(messages)
    , contacts_(contacts)
    , localStore_(new ChatLocalStore(this))
{
    if (router_) {
        router_->registerPushHandler(static_cast<quint16>(MSG_IDS::MSG_NOTIFY_PRIVATE),
                                     [this](const QByteArray &body) {
                                         handlePrivateNotify(body);
                                     });
        router_->registerPushHandler(static_cast<quint16>(MSG_IDS::MSG_NOTIFY_GROUP),
                                     [this](const QByteArray &body) {
                                         handleGroupNotify(body);
                                     });
        router_->registerPushHandler(static_cast<quint16>(MSG_IDS::MSG_NOTIFY_GROUP_EVENT),
                                     [this](const QByteArray &body) {
                                         handleGroupNotify(body);
                                     });
    }

    if (currentUser_ && currentUser_->loggedIn()) {
        setOwnerAccount(currentUser_->account());
    }
}

void ChatService::setOwnerAccount(const QString &account)
{
    if (localStore_) {
        localStore_->openForAccount(account);
        mergeLocalPreviewsIntoConversations();
        syncConversationsFromLocalStore();
    }
}

void ChatService::mergeLocalPreviewsIntoConversations()
{
    if (!localStore_ || !conversations_ || !currentUser_) {
        return;
    }

    const auto previews = localStore_->allPeerPreviews();
    for (auto it = previews.cbegin(); it != previews.cend(); ++it) {
        const QString preview = previewText(it.value().lastMessage);
        conversations_->updatePreview(it.key(), preview, it.value().timeText, it.value().createdAt);
    }
    if (!previews.isEmpty()) {
        emit conversationPreviewUpdated();
    }
}

void ChatService::openChat(const QString &peerAccount, const QString &peerTitle)
{
    if (!ensureLoggedIn()) {
        return;
    }
    ensureLocalStoreReady();

    activePeerAccount_ = peerAccount.trimmed();
    activePeerTitle_ = peerTitle.trimmed();
    if (activePeerTitle_.isEmpty()) {
        activePeerTitle_ = activePeerAccount_;
    }

    markConversationRead(activePeerAccount_);
    loadLocalMessagesToUi(activePeerAccount_);
    emit chatHistoryLoaded();
}

void ChatService::openGroupChat(qint64 groupId, const QString &groupName)
{
    if (!ensureLoggedIn()) {
        return;
    }
    ensureLocalStoreReady();
    activePeerAccount_ = QStringLiteral("g:%1").arg(groupId);
    activePeerTitle_ = groupName.trimmed().isEmpty() ? activePeerAccount_ : groupName.trimmed();
    markConversationRead(activePeerAccount_);
    loadLocalMessagesToUi(activePeerAccount_);
    emit chatHistoryLoaded();
}

void ChatService::sendMessage(const QString &content)
{
    if (!ensureLoggedIn()) {
        emit sendMessageFinished(false, QStringLiteral("请先登录"));
        return;
    }
    if (!ensureLocalStoreReady()) {
        emit sendMessageFinished(false, QStringLiteral("本地聊天记录未就绪"));
        return;
    }
    if (activePeerAccount_.isEmpty() || activePeerAccount_.startsWith(QStringLiteral("g:"))) {
        emit sendMessageFinished(false, QStringLiteral("当前不是私聊会话"));
        return;
    }
    if (router_->busy()) {
        emit sendMessageFinished(false, QStringLiteral("请稍候，正在处理上一请求"));
        return;
    }

    const QString text = content.trimmed();
    if (text.isEmpty()) {
        emit sendMessageFinished(false, QStringLiteral("消息不能为空"));
        return;
    }

    QJsonObject obj;
    obj.insert(QStringLiteral("from_account"), currentUser_->account());
    obj.insert(QStringLiteral("to_account"), activePeerAccount_);
    obj.insert(QStringLiteral("content"), text);
    const QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);

    router_->request(
        settings_->serverHost(),
        static_cast<quint16>(settings_->serverPort()),
        static_cast<quint16>(MSG_IDS::MSG_SEND_PRIVATE),
        body,
        static_cast<quint16>(MSG_IDS::MSG_SEND_PRIVATE_RSP),
        [this, text](const QByteArray &rspBody) {
            QJsonObject root;
            QString err;
            if (!parseResponseObject(rspBody, &root, &err)) {
                emit sendMessageFinished(false, err);
                return;
            }
            const int code = root.value(QStringLiteral("code")).toInt(-1);
            const QString msg = root.value(QStringLiteral("msg")).toString(QStringLiteral("未知错误"));
            if (code != static_cast<int>(API_CODE::OK)) {
                emit sendMessageFinished(false, msg);
                return;
            }

            const qint64 createdAt = root.value(QStringLiteral("created_at")).toInteger(
                QDateTime::currentSecsSinceEpoch());
            const qint64 messageId = root.value(QStringLiteral("message_id")).toInteger(0);

            const QString myNick = currentUser_->nickname();
            if (!persistMessage(activePeerAccount_,
                                currentUser_->account(),
                                text,
                                createdAt,
                                messageId,
                                myNick)) {
                emit sendMessageFinished(false, QStringLiteral("消息已发送但本地保存失败"));
                return;
            }
            appendChatLine(QStringLiteral("me"), text, myNick, currentUser_->account());
            updateConversationPreview(activePeerAccount_, previewText(text), createdAt);
            emit sendMessageFinished(true, msg);
        },
        [this](const QString &e) {
            emit sendMessageFinished(false, e);
        });
}

void ChatService::clearSession()
{
    activePeerAccount_.clear();
    activePeerTitle_.clear();
    if (messages_) {
        messages_->clear();
    }
}

void ChatService::syncConversationsFromLocalStore()
{
    if (!localStore_ || !conversations_) {
        return;
    }

    const auto infos = localStore_->listConversations();
    for (const auto &info : infos) {
        if (info.isGroup) {
            conversations_->upsertGroupConversation(info.groupId, info.title, info.lastMessageAt);
            conversations_->setUnreadCount(info.conversationId, info.unreadCount);
            if (info.dissolved) {
                conversations_->markDissolved(info.groupId);
                conversations_->updatePreview(info.conversationId, QStringLiteral("[已解散]"), QString());
            }
        }
    }
    if (!infos.isEmpty()) {
        emit conversationPreviewUpdated();
    }
}

void ChatService::handlePrivateNotify(const QByteArray &body)
{
    QJsonObject root;
    QString err;
    if (!parseResponseObject(body, &root, &err) || !currentUser_) {
        return;
    }

    const QString fromAccount = root.value(QStringLiteral("from_account")).toString().trimmed();
    const QString fromNickname = root.value(QStringLiteral("from_nickname")).toString().trimmed();
    const QString toAccount = root.value(QStringLiteral("to_account")).toString().trimmed();
    const QString content = root.value(QStringLiteral("content")).toString();
    const qint64 createdAt = root.value(QStringLiteral("created_at")).toInteger(
        QDateTime::currentSecsSinceEpoch());
    const qint64 messageId = root.value(QStringLiteral("message_id")).toInteger(0);

    if (toAccount != currentUser_->account().trimmed() || fromAccount.isEmpty() || content.isEmpty()) {
        return;
    }

    const QString nick = resolveSenderNickname(fromAccount, fromNickname);
    persistMessage(fromAccount, fromAccount, content, createdAt, messageId, nick);
    if (localStore_) {
        localStore_->upsertConversationMeta(fromAccount, fromAccount, nick, false, 0, QString(), false, createdAt);
    }
    if (conversations_) {
        conversations_->upsertFriendConversation(fromAccount,
                                                 nick,
                                                 previewText(content),
                                                 formatMessageTime(createdAt),
                                                 createdAt);
    }
    const bool isActive = fromAccount == activePeerAccount_;
    notifyIncomingMessage(fromAccount, previewText(content), createdAt, !isActive);

    if (isActive) {
        appendChatLine(QStringLiteral("peer"), content, nick, fromAccount);
    }
}

void ChatService::handleGroupNotify(const QByteArray &body)
{
    QJsonObject root;
    QString err;
    if (!parseResponseObject(body, &root, &err) || !currentUser_) {
        return;
    }

    const qint64 groupId = root.value(QStringLiteral("group_id")).toInteger(0);
    const QString groupName = root.value(QStringLiteral("group_name")).toString();
    const QString fromAccount = root.value(QStringLiteral("from_account")).toString().trimmed();
    const QString fromNickname = root.value(QStringLiteral("from_nickname")).toString(fromAccount);
    const QString content = root.value(QStringLiteral("content")).toString();
    const QString type = root.value(QStringLiteral("type")).toString();
    const qint64 createdAt = root.value(QStringLiteral("created_at")).toInteger(
        QDateTime::currentSecsSinceEpoch());
    const qint64 messageId = root.value(QStringLiteral("message_id")).toInteger(0);

    if (type == QStringLiteral("group_dissolved")) {
        if (localStore_) {
            localStore_->upsertConversationMeta(QStringLiteral("g:%1").arg(groupId),
                                               QStringLiteral("g:%1").arg(groupId),
                                               groupName,
                                               true,
                                               groupId,
                                               groupName,
                                               true,
                                               createdAt);
        }
        if (conversations_) {
            conversations_->upsertGroupConversation(groupId, groupName, createdAt);
            conversations_->markDissolved(groupId);
            conversations_->clearUnread(QStringLiteral("g:%1").arg(groupId));
            conversations_->updatePreview(QStringLiteral("g:%1").arg(groupId),
                                          QStringLiteral("[已解散]"),
                                          QString());
        }
        if (localStore_) {
            localStore_->setUnreadCount(QStringLiteral("g:%1").arg(groupId), 0);
        }
        emit conversationPreviewUpdated();
        return;
    }

    if (groupId <= 0 || fromAccount.isEmpty() || content.isEmpty()) {
        return;
    }

    const QString conversationId = QStringLiteral("g:%1").arg(groupId);
    const QString nick = resolveSenderNickname(fromAccount, fromNickname);
    persistMessage(conversationId, fromAccount, content, createdAt, messageId, nick);
    if (localStore_) {
        localStore_->upsertConversationMeta(conversationId,
                                            conversationId,
                                            groupName.isEmpty() ? conversationId : groupName,
                                            true,
                                            groupId,
                                            groupName,
                                            false,
                                            createdAt);
    }
    const QString title = groupName.isEmpty() ? conversationId : groupName;
    if (conversations_) {
        conversations_->upsertGroupConversation(groupId, title, createdAt);
    }
    notifyIncomingMessage(conversationId, previewText(content), createdAt, !isGroupActive(groupId));

    if (isGroupActive(groupId)) {
        const bool isMine = fromAccount == currentUser_->account();
        const QString senderName = isMine ? currentUser_->nickname() : nick;
        appendChatLine(isMine ? QStringLiteral("me") : QStringLiteral("peer"),
                       content,
                       senderName,
                       fromAccount);
    }
}

bool ChatService::persistMessage(const QString &peerAccount,
                                 const QString &fromAccount,
                                 const QString &content,
                                 qint64 createdAt,
                                 qint64 serverMessageId,
                                 const QString &fromNickname)
{
    if (!localStore_ || !currentUser_) {
        return false;
    }
    if (!ensureLocalStoreReady()) {
        qWarning() << "ChatService: local store not ready for" << currentUser_->account();
        return false;
    }

    const QString peer = peerAccount.trimmed();
    const QString from = fromAccount.trimmed();
    if (peer.isEmpty() || content.isEmpty()) {
        return false;
    }

    const qint64 effectiveId = effectiveMessageId(serverMessageId, peer, from, content, createdAt);
    const QString nick = resolveSenderNickname(from, fromNickname);
    if (!localStore_->insertMessage(peer, from, content, createdAt, effectiveId, nick)) {
        qWarning() << "ChatService: insertMessage failed peer=" << peer << "msgId=" << effectiveId;
        return false;
    }
    return true;
}

void ChatService::loadLocalMessagesToUi(const QString &peerAccount)
{
    if (!messages_ || !currentUser_) {
        return;
    }

    messages_->clear();
    if (!localStore_ || peerAccount.isEmpty()) {
        return;
    }

    const auto rows = localStore_->listMessages(peerAccount);
    const bool isGroupConv = peerAccount.startsWith(QStringLiteral("g:"));
    QString lastPreview;
    qint64 lastTime = 0;

    for (const auto &row : rows) {
        const bool isMine = row.fromAccount == currentUser_->account();
        const QString who = isMine ? QStringLiteral("me") : QStringLiteral("peer");
        QString senderName;
        if (isGroupConv && !isMine) {
            senderName = resolveSenderNickname(row.fromAccount, row.fromNickname);
        } else if (!isMine) {
            senderName = resolveSenderNickname(row.fromAccount, row.fromNickname);
        } else {
            senderName = currentUser_->nickname();
        }
        messages_->appendMessage(who, row.content, senderName, row.fromAccount);
        lastPreview = previewText(row.content);
        lastTime = row.createdAt;
    }
    if (!lastPreview.isEmpty()) {
        updateConversationPreview(peerAccount, lastPreview, lastTime);
    }
}

void ChatService::appendChatLine(const QString &who,
                                 const QString &text,
                                 const QString &senderName,
                                 const QString &senderAccount)
{
    if (messages_) {
        messages_->appendMessage(who, text, senderName, senderAccount);
    }
}

void ChatService::sendGroupMessage(qint64 groupId, const QString &content)
{
    Q_UNUSED(groupId)
    Q_UNUSED(content)
}

void ChatService::onOwnGroupMessageSent(qint64 groupId, const QString &content, qint64 createdAt, qint64 messageId)
{
    if (!currentUser_ || groupId <= 0 || content.isEmpty()) {
        return;
    }

    const QString conversationId = QStringLiteral("g:%1").arg(groupId);
    const QString fromAccount = currentUser_->account();

    const QString myNick = currentUser_->nickname();
    persistMessage(conversationId, fromAccount, content, createdAt, messageId, myNick);

    if (localStore_) {
        const QString groupName = activePeerTitle_;
        localStore_->upsertConversationMeta(conversationId, conversationId,
                                            groupName.isEmpty() ? conversationId : groupName,
                                            true, groupId, groupName, false, createdAt);
    }

    if (conversations_) {
        const QString groupName = activePeerTitle_;
        conversations_->upsertGroupConversation(groupId, groupName.isEmpty() ? conversationId : groupName, createdAt);
        conversations_->updatePreview(conversationId, previewText(content),
                                      formatMessageTime(createdAt), createdAt);
    }

    if (isGroupActive(groupId)) {
        appendChatLine(QStringLiteral("me"), content, myNick, fromAccount);
    }

    emit conversationPreviewUpdated();
}

QString ChatService::nicknameForAccount(const QString &account) const
{
    const QString acc = account.trimmed();
    if (acc.isEmpty()) {
        return acc;
    }
    if (currentUser_ && acc == currentUser_->account()) {
        const QString selfNick = currentUser_->nickname().trimmed();
        return selfNick.isEmpty() ? acc : selfNick;
    }
    if (!contacts_) {
        return acc;
    }
    for (int i = 0; i < contacts_->rowCount(); ++i) {
        const QModelIndex idx = contacts_->index(i);
        if (contacts_->data(idx, ContactListModel::AccountRole).toString() == acc) {
            const QString nick = contacts_->data(idx, ContactListModel::NicknameRole).toString().trimmed();
            return nick.isEmpty() ? acc : nick;
        }
    }
    return acc;
}

QString ChatService::resolveSenderNickname(const QString &fromAccount,
                                           const QString &storedNickname) const
{
    const QString stored = storedNickname.trimmed();
    if (!stored.isEmpty() && stored != fromAccount.trimmed()) {
        return stored;
    }
    return nicknameForAccount(fromAccount);
}

void ChatService::markGroupDissolved(qint64 groupId, const QString &groupName)
{
    if (!localStore_ || groupId <= 0) {
        return;
    }
    const QString cid = QStringLiteral("g:%1").arg(groupId);
    localStore_->upsertConversationMeta(cid, cid, groupName, true, groupId, groupName, true, 0);
    syncConversationsFromLocalStore();
}

void ChatService::updateConversationPreview(const QString &peerAccount,
                                            const QString &previewText,
                                            qint64 createdAt)
{
    if (!conversations_ || peerAccount.isEmpty()) {
        return;
    }
    conversations_->updatePreview(peerAccount, previewText, formatMessageTime(createdAt), createdAt);
    emit conversationPreviewUpdated();
}

void ChatService::notifyIncomingMessage(const QString &conversationId,
                                        const QString &previewText,
                                        qint64 createdAt,
                                        bool incrementUnread)
{
    if (conversationId.isEmpty()) {
        return;
    }
    updateConversationPreview(conversationId, previewText, createdAt);
    if (!incrementUnread) {
        return;
    }
    if (conversations_) {
        conversations_->incrementUnread(conversationId);
    }
    if (localStore_) {
        localStore_->incrementUnread(conversationId);
    }
}

void ChatService::markConversationRead(const QString &conversationId)
{
    if (conversationId.isEmpty()) {
        return;
    }
    if (conversations_) {
        conversations_->clearUnread(conversationId);
    }
    if (localStore_) {
        localStore_->setUnreadCount(conversationId, 0);
    }
}

QString ChatService::formatMessageTime(qint64 unixSec)
{
    if (unixSec <= 0) {
        return QString();
    }
    const QDateTime dt = QDateTime::fromSecsSinceEpoch(unixSec);
    const QDateTime now = QDateTime::currentDateTime();
    if (dt.date() == now.date()) {
        return dt.toString(QStringLiteral("HH:mm"));
    }
    return dt.toString(QStringLiteral("MM/dd HH:mm"));
}

QString ChatService::previewText(const QString &content)
{
    QString preview = content;
    if (preview.size() > 40) {
        preview = preview.left(40) + QStringLiteral("...");
    }
    return preview;
}

bool ChatService::isGroupActive(qint64 groupId) const
{
    return groupId > 0 && activePeerAccount_ == QStringLiteral("g:%1").arg(groupId);
}

bool ChatService::ensureLoggedIn() const
{
    return currentUser_ && currentUser_->loggedIn() && settings_ && router_;
}

bool ChatService::ensureLocalStoreReady()
{
    if (!localStore_ || !currentUser_ || !currentUser_->loggedIn()) {
        return false;
    }
    const QString account = currentUser_->account().trimmed();
    if (account.isEmpty()) {
        return false;
    }
    if (localStore_->isOpen() && localStore_->ownerAccount() == account) {
        return true;
    }
    return localStore_->openForAccount(account);
}

qint64 ChatService::effectiveMessageId(qint64 serverMessageId,
                                       const QString &peerAccount,
                                       const QString &fromAccount,
                                       const QString &content,
                                       qint64 createdAt) const
{
    if (serverMessageId > 0) {
        return serverMessageId;
    }
    const qint64 ts = createdAt > 0 ? createdAt : QDateTime::currentSecsSinceEpoch();
    const QString key = peerAccount + QLatin1Char('|') + fromAccount + QLatin1Char('|') + content
                        + QLatin1Char('|') + QString::number(ts);
    const quint64 hash = qHash(key);
    return ts * 1000000LL + static_cast<qint64>(hash % 1000000ULL);
}
