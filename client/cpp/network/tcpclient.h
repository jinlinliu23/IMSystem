#pragma once

#include <QByteArray>
#include <QObject>
#include <QTcpSocket>

/**
 * @brief 底层 TCP 单例：连接、粘包拆帧、组帧发送
 */
class TcpClient : public QObject
{
    Q_OBJECT
public:
    static TcpClient &instance();

    TcpClient(const TcpClient &) = delete;
    TcpClient &operator=(const TcpClient &) = delete;

    bool isConnected() const;
    QString peerDescription() const;

    void connectToServer(const QString &host, quint16 port);
    void abortConnection();
    void disconnectFromServer();
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
    explicit TcpClient(QObject *parent = nullptr);
    void processBuffer();

    QTcpSocket socket_;
    QByteArray readBuffer_;
    QString peerHost_;
    quint16 peerPort_ = 0;
};
