#include "service/chatservice.h"

#include "core/clientsettings.h"
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
                         QObject *parent)
    : QObject(parent)
    , settings_(settings)
    , currentUser_(currentUser)
    , router_(router)
    , conversations_(conversations)
    , messages_(messages)
    , localStore_(new ChatLocalStore(this))
{
    if (router_) {
        router_->registerPushHandler(static_cast<quint16>(MSG_IDS::MSG_NOTIFY_PRIVATE),
                                     [this](const QByteArray &body) {
                                         handlePrivateNotify(body);
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
        conversations_->updatePreview(it.key(), preview, it.value().timeText);
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
    if (activePeerAccount_.isEmpty()) {
        emit sendMessageFinished(false, QStringLiteral("未选择聊天对象"));
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

            if (!persistMessage(activePeerAccount_,
                                currentUser_->account(),
                                text,
                                createdAt,
                                messageId)) {
                emit sendMessageFinished(false, QStringLiteral("消息已发送但本地保存失败"));
                return;
            }
            appendChatLine(QStringLiteral("me"), text);
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

void ChatService::handlePrivateNotify(const QByteArray &body)
{
    QJsonObject root;
    QString err;
    if (!parseResponseObject(body, &root, &err) || !currentUser_) {
        return;
    }

    const QString fromAccount = root.value(QStringLiteral("from_account")).toString().trimmed();
    const QString toAccount = root.value(QStringLiteral("to_account")).toString().trimmed();
    const QString content = root.value(QStringLiteral("content")).toString();
    const qint64 createdAt = root.value(QStringLiteral("created_at")).toInteger(
        QDateTime::currentSecsSinceEpoch());
    const qint64 messageId = root.value(QStringLiteral("message_id")).toInteger(0);

    if (toAccount != currentUser_->account().trimmed() || fromAccount.isEmpty() || content.isEmpty()) {
        return;
    }

    persistMessage(fromAccount, fromAccount, content, createdAt, messageId);
    updateConversationPreview(fromAccount, previewText(content), createdAt);

    if (fromAccount == activePeerAccount_) {
        appendChatLine(QStringLiteral("peer"), content);
    }
}

bool ChatService::persistMessage(const QString &peerAccount,
                                 const QString &fromAccount,
                                 const QString &content,
                                 qint64 createdAt,
                                 qint64 serverMessageId)
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
    if (!localStore_->insertMessage(peer, from, content, createdAt, effectiveId)) {
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
    QVector<QPair<QString, QString>> uiRows;
    uiRows.reserve(rows.size());
    QString lastPreview;
    qint64 lastTime = 0;

    for (const auto &row : rows) {
        const bool isMine = row.fromAccount == currentUser_->account();
        uiRows.append(qMakePair(isMine ? QStringLiteral("me") : QStringLiteral("peer"), row.content));
        lastPreview = previewText(row.content);
        lastTime = row.createdAt;
    }

    messages_->setMessages(uiRows);
    if (!lastPreview.isEmpty()) {
        updateConversationPreview(peerAccount, lastPreview, lastTime);
    }
}

void ChatService::appendChatLine(const QString &who, const QString &text)
{
    if (messages_) {
        messages_->appendMessage(who, text);
    }
}

void ChatService::updateConversationPreview(const QString &peerAccount,
                                            const QString &previewText,
                                            qint64 createdAt)
{
    if (!conversations_ || peerAccount.isEmpty()) {
        return;
    }
    conversations_->updatePreview(peerAccount, previewText, formatMessageTime(createdAt));
    emit conversationPreviewUpdated();
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
