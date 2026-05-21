#pragma once

#include <QByteArray>
#include <QObject>
#include <QTcpSocket>

/**
 * @brief 底层 TCP 客户端：负责连接、按帧拆包/组包，不包含具体业务逻辑
 *
 * 收到完整一帧后通过 frameReceived(msgId, body) 交给 ClientFacade 处理。
 */
class TcpClient : public QObject
{
    Q_OBJECT
public:
    explicit TcpClient(QObject *parent = nullptr);

    bool isConnected() const;
    QString peerDescription() const;

    void connectToServer(const QString &host, quint16 port);
    void abortConnection();
    void disconnectFromServer();

    /** 发送一帧：自动加上 6 字节帧头，body 一般为 JSON */
    void sendFrame(quint16 msgId, const QByteArray &body);

signals:
    void connected();
    void disconnected();
    void connectError(const QString &message);
    void frameReceived(quint16 msgId, const QByteArray &body);

private slots:
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);
    void onStateChanged(QAbstractSocket::SocketState state);

private:
    /** 从 readBuffer_ 中解析尽可能多的完整帧（处理 TCP 粘包） */
    void processBuffer();

    QTcpSocket socket_;
    QByteArray readBuffer_; ///< 尚未解析完的接收缓冲区
    QString peerHost_;
    quint16 peerPort_ = 0;
};
