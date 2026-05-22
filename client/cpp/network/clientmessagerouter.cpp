#include "network/clientmessagerouter.h"
#include "network/tcpclient.h"

#include <QHash>
#include <QDebug>

namespace {

QString endpointLabel(const QString &host, int port)
{
    return QStringLiteral("%1:%2").arg(host).arg(port);
}

constexpr int kTimeoutMs = 10000;

} // namespace

ClientMessageRouter::ClientMessageRouter(QObject *parent)
    : QObject(parent)
{
    auto &tcp = TcpClient::instance();
    connect(&tcp, &TcpClient::connected, this, &ClientMessageRouter::onConnected);
    connect(&tcp, &TcpClient::connectError, this, &ClientMessageRouter::onConnectError);
    connect(&tcp, &TcpClient::frameReceived, this, &ClientMessageRouter::onFrameReceived);

    connectTimer_.setSingleShot(true);
    connectTimer_.setInterval(kTimeoutMs);
    connect(&connectTimer_, &QTimer::timeout, this, &ClientMessageRouter::onConnectTimeout);

    responseTimer_.setSingleShot(true);
    responseTimer_.setInterval(kTimeoutMs);
    connect(&responseTimer_, &QTimer::timeout, this, &ClientMessageRouter::onResponseTimeout);
}

void ClientMessageRouter::registerPushHandler(quint16 msgId, ResponseHandler handler)
{
    pushHandlers_.insert(msgId, std::move(handler));
}

void ClientMessageRouter::request(const QString &host,
                                  quint16 port,
                                  quint16 reqMsgId,
                                  const QByteArray &body,
                                  quint16 rspMsgId,
                                  ResponseHandler onResponse,
                                  ErrorHandler onError)
{
    if (busy_) {
        if (onError) {
            onError(QStringLiteral("请稍候，正在处理上一请求"));
        }
        return;
    }

    setBusy(true);
    connectTimer_.stop();
    responseTimer_.stop();

    pendingHost_ = host.trimmed();
    pendingPort_ = port;
    pendingReqMsgId_ = reqMsgId;
    pendingBody_ = body;
    pendingRspMsgId_ = rspMsgId;
    onResponse_ = std::move(onResponse);
    onError_ = std::move(onError);
    awaitingConnect_ = false;

    auto &tcp = TcpClient::instance();
    if (tcp.isConnected()) {
        sendPendingFrame();
        return;
    }

    awaitingConnect_ = true;
    qInfo() << "ClientMessageRouter connect" << endpointLabel(pendingHost_, pendingPort_)
            << "req" << reqMsgId << "rsp" << rspMsgId;
    tcp.connectToServer(pendingHost_, pendingPort_);
    connectTimer_.start();
}

void ClientMessageRouter::onConnected()
{
    connectTimer_.stop();
    if (!busy_ || !awaitingConnect_) {
        return;
    }
    awaitingConnect_ = false;
    sendPendingFrame();
}

void ClientMessageRouter::sendPendingFrame()
{
    TcpClient::instance().sendFrame(pendingReqMsgId_, pendingBody_);
    responseTimer_.start();
}

void ClientMessageRouter::onConnectError(const QString &message)
{
    if (!busy_ || !awaitingConnect_) {
        return;
    }
    fail(connectFailHint(endpointLabel(pendingHost_, pendingPort_), message));
}

void ClientMessageRouter::onConnectTimeout()
{
    if (!busy_ || !awaitingConnect_ || TcpClient::instance().isConnected()) {
        return;
    }
    TcpClient::instance().abortConnection();
    fail(connectFailHint(endpointLabel(pendingHost_, pendingPort_),
                         QStringLiteral("连接超时")));
}

void ClientMessageRouter::onResponseTimeout()
{
    if (!busy_) {
        return;
    }
    fail(QStringLiteral("服务器无响应，请确认服务端正在运行"));
}

void ClientMessageRouter::onFrameReceived(quint16 msgId, const QByteArray &body)
{
    if (busy_ && msgId == pendingRspMsgId_) {
        responseTimer_.stop();
        ResponseHandler ok = onResponse_;
        clearPending();
        if (ok) {
            ok(body);
        }
        return;
    }

    const auto it = pushHandlers_.constFind(msgId);
    if (it != pushHandlers_.cend() && it.value()) {
        it.value()(body);
    }
}

void ClientMessageRouter::fail(const QString &message)
{
    ErrorHandler err = onError_;
    clearPending();
    if (err) {
        err(message);
    }
}

void ClientMessageRouter::clearPending()
{
    connectTimer_.stop();
    responseTimer_.stop();
    awaitingConnect_ = false;
    pendingReqMsgId_ = 0;
    pendingRspMsgId_ = 0;
    pendingBody_.clear();
    onResponse_ = nullptr;
    onError_ = nullptr;
    setBusy(false);
}

void ClientMessageRouter::setBusy(bool busy)
{
    if (busy_ == busy) {
        return;
    }
    busy_ = busy;
    emit busyChanged();
}

QString ClientMessageRouter::connectFailHint(const QString &endpoint, const QString &detail)
{
    return QStringLiteral("无法连接 %1\n%2\n\n请确认：\n"
                          "1. 电脑已运行 server\n"
                          "2. 手机与电脑网络互通\n"
                          "3. 真机填写电脑 IP（非 10.0.2.2）\n"
                          "4. 防火墙放行 16701")
        .arg(endpoint, detail);
}
