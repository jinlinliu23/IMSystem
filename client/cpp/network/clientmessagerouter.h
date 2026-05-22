#pragma once

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QTimer>
#include <functional>

/**
 * @brief 消息路由：连接/超时、发帧、等待指定 rsp，并将下行帧分发给业务
 */
class ClientMessageRouter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    using ResponseHandler = std::function<void(const QByteArray &body)>;
    using ErrorHandler = std::function<void(const QString &message)>;

    explicit ClientMessageRouter(QObject *parent = nullptr);

    bool busy() const { return busy_; }

    /**
     * @brief 若未连接则先连接，再发送 req 并等待 rsp（含连接/响应超时）
     */
    void request(const QString &host,
                 quint16 port,
                 quint16 reqMsgId,
                 const QByteArray &body,
                 quint16 rspMsgId,
                 ResponseHandler onResponse,
                 ErrorHandler onError);

    /** 注册服务端推送类消息处理（后续单聊等） */
    void registerPushHandler(quint16 msgId, ResponseHandler handler);

signals:
    void busyChanged();

private slots:
    void onConnected();
    void onConnectError(const QString &message);
    void onFrameReceived(quint16 msgId, const QByteArray &body);
    void onConnectTimeout();
    void onResponseTimeout();

private:
    void setBusy(bool busy);
    void clearPending();
    void fail(const QString &message);
    void sendPendingFrame();
    static QString connectFailHint(const QString &endpoint, const QString &detail);

    QTimer connectTimer_;
    QTimer responseTimer_;

    bool busy_ = false;
    bool awaitingConnect_ = false;

    QString pendingHost_;
    quint16 pendingPort_ = 0;
    quint16 pendingReqMsgId_ = 0;
    QByteArray pendingBody_;
    quint16 pendingRspMsgId_ = 0;
    ResponseHandler onResponse_;
    ErrorHandler onError_;

    QHash<quint16, ResponseHandler> pushHandlers_;
};
