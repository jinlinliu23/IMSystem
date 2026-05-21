#include "tcpclient.h"
#include "protocolframe.h"

#include <QDebug>
#include <qendian.h>

TcpClient::TcpClient(QObject *parent)
    : QObject(parent)
{
    connect(&socket_, &QTcpSocket::connected, this, &TcpClient::connected);
    connect(&socket_, &QTcpSocket::disconnected, this, &TcpClient::disconnected);
    connect(&socket_, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    connect(&socket_, &QTcpSocket::errorOccurred, this, &TcpClient::onSocketError);
    connect(&socket_, &QTcpSocket::stateChanged, this, &TcpClient::onStateChanged);
}

bool TcpClient::isConnected() const
{
    return socket_.state() == QAbstractSocket::ConnectedState;
}

QString TcpClient::peerDescription() const
{
    if (peerHost_.isEmpty()) {
        return QStringLiteral("(未设置)");
    }
    return QStringLiteral("%1:%2").arg(peerHost_).arg(peerPort_);
}

void TcpClient::connectToServer(const QString &host, quint16 port)
{
    peerHost_ = host.trimmed();
    peerPort_ = port;

    if (peerHost_.isEmpty()) {
        emit connectError(QStringLiteral("服务器地址不能为空"));
        return;
    }

    abortConnection();
    readBuffer_.clear();

    qInfo() << "TcpClient connecting to" << peerDescription();
    socket_.connectToHost(peerHost_, peerPort_);
}

void TcpClient::abortConnection()
{
    if (socket_.state() != QAbstractSocket::UnconnectedState) {
        socket_.abort();
    }
    readBuffer_.clear();
}

void TcpClient::disconnectFromServer()
{
    socket_.disconnectFromHost();
}

void TcpClient::sendFrame(quint16 msgId, const QByteArray &body)
{
    if (!isConnected()) {
        qWarning() << "TcpClient::sendFrame skipped, not connected to" << peerDescription();
        return;
    }

    QByteArray packet;
    packet.resize(Protocol::HEAD_TOTAL_LEN + body.size());

    // 帧头使用网络字节序（大端），与服务端 SendNode 一致
    const quint16 msgIdNet = qToBigEndian(msgId);
    const quint32 bodyLenNet = qToBigEndian(static_cast<quint32>(body.size()));

    memcpy(packet.data(), &msgIdNet, Protocol::HEAD_ID_LEN);
    memcpy(packet.data() + Protocol::HEAD_ID_LEN, &bodyLenNet, Protocol::HEAD_DATA_LEN);
    memcpy(packet.data() + Protocol::HEAD_TOTAL_LEN, body.data(), body.size());

    const qint64 written = socket_.write(packet);
    socket_.flush();
    qInfo() << "TcpClient sent frame msgId=" << msgId << "bytes=" << written;
}

void TcpClient::onReadyRead()
{
    readBuffer_.append(socket_.readAll());
    processBuffer();
}

void TcpClient::onSocketError(QAbstractSocket::SocketError error)
{
    // 已连接后的收发错误由业务层另行处理，这里只上报「连接阶段」失败
    if (socket_.state() == QAbstractSocket::ConnectedState) {
        return;
    }

    qWarning() << "TcpClient error" << error << socket_.errorString()
               << "peer=" << peerDescription();
    emit connectError(QStringLiteral("%1 (%2)").arg(socket_.errorString(), peerDescription()));
}

void TcpClient::onStateChanged(QAbstractSocket::SocketState state)
{
    qInfo() << "TcpClient state" << state << "peer=" << peerDescription();
}

void TcpClient::processBuffer()
{
    // 循环解析：缓冲区里可能有多帧（粘包），也可能只有半帧（拆包）
    while (readBuffer_.size() >= Protocol::HEAD_TOTAL_LEN) {
        quint16 msgId = 0;
        memcpy(&msgId, readBuffer_.constData(), Protocol::HEAD_ID_LEN);
        msgId = qFromBigEndian(msgId);

        quint32 bodyLen = 0;
        memcpy(&bodyLen, readBuffer_.constData() + Protocol::HEAD_ID_LEN, Protocol::HEAD_DATA_LEN);
        bodyLen = qFromBigEndian(bodyLen);

        const int frameLen = Protocol::HEAD_TOTAL_LEN + static_cast<int>(bodyLen);
        if (readBuffer_.size() < frameLen) {
            break; // body 未收齐，等待下次 readyRead
        }

        const QByteArray body = readBuffer_.mid(Protocol::HEAD_TOTAL_LEN, static_cast<int>(bodyLen));
        readBuffer_.remove(0, frameLen);
        emit frameReceived(msgId, body);
    }
}
