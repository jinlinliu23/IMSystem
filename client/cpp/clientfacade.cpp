#include "clientfacade.h"

#include "msg_ids.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QDebug>

namespace {

QString defaultServerHost()
{
#if defined(Q_OS_ANDROID)
    return QStringLiteral("10.0.2.2");
#else
    return QStringLiteral("127.0.0.1");
#endif
}

QString endpointLabel(const QString &host, int port)
{
    return QStringLiteral("%1:%2").arg(host).arg(port);
}

QString connectFailHint(const QString &endpoint, const QString &detail)
{
    return QStringLiteral("无法连接 %1\n%2\n\n请确认：\n"
                          "1. 电脑已运行 server\n"
                          "2. 手机与电脑网络互通\n"
                          "3. 真机填写电脑 IP（非 10.0.2.2）\n"
                          "4. 防火墙放行 16701")
        .arg(endpoint, detail);
}

} // namespace

ClientFacade::ClientFacade(QObject *parent)
    : QObject(parent)
{
    loadSettings();

    connect(&tcp_, &TcpClient::connected, this, &ClientFacade::onConnected);
    connect(&tcp_, &TcpClient::connectError, this, &ClientFacade::onConnectError);
    connect(&tcp_, &TcpClient::frameReceived, this, &ClientFacade::onFrameReceived);

    connectTimer_.setSingleShot(true);
    connectTimer_.setInterval(10000);
    connect(&connectTimer_, &QTimer::timeout, this, &ClientFacade::onConnectTimeout);

    responseTimer_.setSingleShot(true);
    responseTimer_.setInterval(10000);
    connect(&responseTimer_, &QTimer::timeout, this, &ClientFacade::onResponseTimeout);

    qInfo() << "ClientFacade server" << endpointLabel(serverHost_, serverPort_)
            << "loggedIn" << isLoggedIn_;
}

void ClientFacade::loadSettings()
{
    QSettings settings;
    serverHost_ = settings.value(QStringLiteral("server/host"), defaultServerHost()).toString().trimmed();
    serverPort_ = settings.value(QStringLiteral("server/port"), 16701).toInt();
    if (serverPort_ <= 0 || serverPort_ > 65535) {
        serverPort_ = 16701;
    }

    isLoggedIn_ = settings.value(QStringLiteral("auth/logged_in"), false).toBool();
    userId_ = settings.value(QStringLiteral("auth/user_id"), 0).toLongLong();
    username_ = settings.value(QStringLiteral("auth/username")).toString();
    nickname_ = settings.value(QStringLiteral("auth/nickname")).toString();
}

void ClientFacade::persistServerSettings() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("server/host"), serverHost_);
    settings.setValue(QStringLiteral("server/port"), serverPort_);
}

void ClientFacade::persistAuthState() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("auth/logged_in"), isLoggedIn_);
    settings.setValue(QStringLiteral("auth/user_id"), userId_);
    settings.setValue(QStringLiteral("auth/username"), username_);
    settings.setValue(QStringLiteral("auth/nickname"), nickname_);
}

void ClientFacade::clearAuthState()
{
    userId_ = 0;
    username_.clear();
    nickname_.clear();
    persistAuthState();
    emit userInfoChanged();
}

void ClientFacade::setLoggedIn(bool loggedIn)
{
    if (isLoggedIn_ == loggedIn) {
        return;
    }
    isLoggedIn_ = loggedIn;
    persistAuthState();
    emit isLoggedInChanged();
}

void ClientFacade::setUserInfo(qint64 userId, const QString &username, const QString &nickname)
{
    userId_ = userId;
    username_ = username;
    nickname_ = nickname;
    persistAuthState();
    emit userInfoChanged();
}

void ClientFacade::setServerHost(const QString &host)
{
    const QString trimmed = host.trimmed();
    if (serverHost_ == trimmed) {
        return;
    }
    serverHost_ = trimmed;
    emit serverHostChanged();
}

void ClientFacade::setServerPort(int port)
{
    if (serverPort_ == port) {
        return;
    }
    serverPort_ = port;
    emit serverPortChanged();
}

void ClientFacade::saveServerSettings()
{
    persistServerSettings();
}

void ClientFacade::logout()
{
    setLoggedIn(false);
    clearAuthState();
    tcp_.disconnectFromServer();
}

void ClientFacade::registerUser(const QString &nickname,
                                const QString &username,
                                const QString &password)
{
    if (busy_) {
        emit registerFinished(false, QStringLiteral("请稍候，正在处理上一请求"), 0);
        return;
    }
    if (serverHost_.isEmpty()) {
        emit registerFinished(false, QStringLiteral("请填写服务器 IP 地址"), 0);
        return;
    }
    if (nickname.trimmed().isEmpty()) {
        emit registerFinished(false, QStringLiteral("请输入昵称"), 0);
        return;
    }
    if (username.trimmed().size() < 3) {
        emit registerFinished(false, QStringLiteral("账号至少 3 个字符"), 0);
        return;
    }
    if (password.size() < 6) {
        emit registerFinished(false, QStringLiteral("密码至少 6 位"), 0);
        return;
    }

    persistServerSettings();
    pendingNickname_ = nickname.trimmed();
    pendingUsername_ = username.trimmed();
    pendingPassword_ = password;
    startPendingRequest(PendingRequest::Register);
}

void ClientFacade::loginUser(const QString &username, const QString &password)
{
    if (busy_) {
        emit loginFinished(false, QStringLiteral("请稍候，正在处理上一请求"));
        return;
    }
    if (serverHost_.isEmpty()) {
        emit loginFinished(false, QStringLiteral("请填写服务器 IP 地址"));
        return;
    }
    if (username.trimmed().isEmpty()) {
        emit loginFinished(false, QStringLiteral("请输入账号"));
        return;
    }
    if (password.isEmpty()) {
        emit loginFinished(false, QStringLiteral("请输入密码"));
        return;
    }

    persistServerSettings();
    pendingUsername_ = username.trimmed();
    pendingPassword_ = password;
    startPendingRequest(PendingRequest::Login);
}

void ClientFacade::startPendingRequest(PendingRequest request)
{
    pendingRequest_ = request;
    setBusy(true);
    clearPendingTimers();

    if (tcp_.isConnected()) {
        sendPendingRequest();
        return;
    }
    beginConnect();
}

void ClientFacade::beginConnect()
{
    qInfo() << "Connect to" << endpointLabel(serverHost_, serverPort_)
            << "request" << static_cast<int>(pendingRequest_);
    tcp_.connectToServer(serverHost_, static_cast<quint16>(serverPort_));
    connectTimer_.start();
}

void ClientFacade::onConnected()
{
    connectTimer_.stop();
    if (pendingRequest_ != PendingRequest::None) {
        sendPendingRequest();
    }
}

void ClientFacade::sendPendingRequest()
{
    switch (pendingRequest_) {
    case PendingRequest::Register:
        sendRegisterRequest();
        break;
    case PendingRequest::Login:
        sendLoginRequest();
        break;
    default:
        break;
    }
}

void ClientFacade::onConnectError(const QString &message)
{
    if (pendingRequest_ == PendingRequest::None) {
        return;
    }
    failConnect(message);
}

void ClientFacade::onConnectTimeout()
{
    if (pendingRequest_ == PendingRequest::None || tcp_.isConnected()) {
        return;
    }
    tcp_.abortConnection();
    failConnect(QStringLiteral("连接超时"));
}

void ClientFacade::onResponseTimeout()
{
    if (pendingRequest_ == PendingRequest::None) {
        return;
    }
    failResponse(QStringLiteral("服务器无响应，请确认服务端正在运行"));
}

void ClientFacade::failConnect(const QString &message)
{
    const QString endpoint = endpointLabel(serverHost_, serverPort_);
    const QString hint = connectFailHint(endpoint, message);

    if (pendingRequest_ == PendingRequest::Register) {
        finishRegister(false, hint);
    } else if (pendingRequest_ == PendingRequest::Login) {
        finishLogin(false, hint);
    }
}

void ClientFacade::failResponse(const QString &message)
{
    if (pendingRequest_ == PendingRequest::Register) {
        finishRegister(false, message);
    } else if (pendingRequest_ == PendingRequest::Login) {
        finishLogin(false, message);
    }
}

void ClientFacade::sendRegisterRequest()
{
    QJsonObject obj;
    obj.insert(QStringLiteral("nickname"), pendingNickname_);
    obj.insert(QStringLiteral("username"), pendingUsername_);
    obj.insert(QStringLiteral("password"), pendingPassword_);
    const QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    tcp_.sendFrame(static_cast<quint16>(MSG_IDS::MSG_REGISTER), body);
    responseTimer_.start();
}

void ClientFacade::sendLoginRequest()
{
    QJsonObject obj;
    obj.insert(QStringLiteral("username"), pendingUsername_);
    obj.insert(QStringLiteral("password"), pendingPassword_);
    const QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    tcp_.sendFrame(static_cast<quint16>(MSG_IDS::MSG_LOGIN), body);
    responseTimer_.start();
}

void ClientFacade::onFrameReceived(quint16 msgId, const QByteArray &body)
{
    if (pendingRequest_ == PendingRequest::Register
        && msgId == static_cast<quint16>(MSG_IDS::MSG_REGISTER_RSP)) {
        responseTimer_.stop();

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            finishRegister(false, QStringLiteral("服务器响应格式错误"));
            return;
        }

        const QJsonObject obj = doc.object();
        const int code = obj.value(QStringLiteral("code")).toInt(-1);
        const QString msg = obj.value(QStringLiteral("msg")).toString(QStringLiteral("未知错误"));
        const qint64 userId = obj.value(QStringLiteral("user_id")).toInteger(0);

        if (code == static_cast<int>(API_CODE::OK)) {
            finishRegister(true, msg, userId);
        } else {
            finishRegister(false, msg);
        }
        return;
    }

    if (pendingRequest_ == PendingRequest::Login
        && msgId == static_cast<quint16>(MSG_IDS::MSG_LOGIN_RSP)) {
        responseTimer_.stop();

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            finishLogin(false, QStringLiteral("服务器响应格式错误"));
            return;
        }

        const QJsonObject obj = doc.object();
        const int code = obj.value(QStringLiteral("code")).toInt(-1);
        const QString msg = obj.value(QStringLiteral("msg")).toString(QStringLiteral("未知错误"));

        if (code == static_cast<int>(API_CODE::OK)) {
            const qint64 userId = obj.value(QStringLiteral("user_id")).toInteger(0);
            const QString uname = obj.value(QStringLiteral("username")).toString();
            const QString nick = obj.value(QStringLiteral("nickname")).toString();
            setUserInfo(userId, uname, nick);
            setLoggedIn(true);
            finishLogin(true, msg);
        } else {
            finishLogin(false, msg);
        }
    }
}

void ClientFacade::finishRegister(bool success, const QString &message, qint64 userId)
{
    pendingRequest_ = PendingRequest::None;
    clearPendingTimers();
    setBusy(false);
    emit registerFinished(success, message, userId);
}

void ClientFacade::finishLogin(bool success, const QString &message)
{
    pendingRequest_ = PendingRequest::None;
    clearPendingTimers();
    setBusy(false);
    emit loginFinished(success, message);
}

void ClientFacade::clearPendingTimers()
{
    connectTimer_.stop();
    responseTimer_.stop();
}

void ClientFacade::setBusy(bool busy)
{
    if (busy_ == busy) {
        return;
    }
    busy_ = busy;
    emit busyChanged();
}
