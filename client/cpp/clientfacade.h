#ifndef CLIENTFACADE_H
#define CLIENTFACADE_H

#include "tcpclient.h"

#include <QObject>
#include <QString>
#include <QTimer>

/**
 * @brief 客户端业务门面：注册、登录、会话状态（供 QML 调用）
 */
class ClientFacade : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY isLoggedInChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString serverHost READ serverHost WRITE setServerHost NOTIFY serverHostChanged)
    Q_PROPERTY(int serverPort READ serverPort WRITE setServerPort NOTIFY serverPortChanged)
    Q_PROPERTY(qint64 userId READ userId NOTIFY userInfoChanged)
    Q_PROPERTY(QString username READ username NOTIFY userInfoChanged) ///< 账号（唯一标识）
    Q_PROPERTY(QString nickname READ nickname NOTIFY userInfoChanged) ///< 昵称（展示名）
    Q_PROPERTY(int themeMode READ themeMode CONSTANT)

public:
    explicit ClientFacade(QObject *parent = nullptr);

    bool isLoggedIn() const { return isLoggedIn_; }
    bool busy() const { return busy_; }
    QString serverHost() const { return serverHost_; }
    int serverPort() const { return serverPort_; }
    qint64 userId() const { return userId_; }
    QString username() const { return username_; }
    QString nickname() const { return nickname_; }
    int themeMode() const { return 0; }

    void setServerHost(const QString &host);
    void setServerPort(int port);

    Q_INVOKABLE void saveServerSettings();
    Q_INVOKABLE void registerUser(const QString &nickname,
                                  const QString &username,
                                  const QString &password);
    Q_INVOKABLE void loginUser(const QString &username, const QString &password);
    Q_INVOKABLE void logout();

signals:
    void isLoggedInChanged();
    void busyChanged();
    void serverHostChanged();
    void serverPortChanged();
    void userInfoChanged();
    void registerFinished(bool success, const QString &message, qint64 userId);
    void loginFinished(bool success, const QString &message);

private slots:
    void onConnected();
    void onConnectError(const QString &message);
    void onFrameReceived(quint16 msgId, const QByteArray &body);
    void onConnectTimeout();
    void onResponseTimeout();

private:
    enum class PendingRequest { None, Register, Login };

    void loadSettings();
    void persistServerSettings() const;
    void persistAuthState() const;
    void clearAuthState();
    void setLoggedIn(bool loggedIn);
    void setUserInfo(qint64 userId, const QString &username, const QString &nickname);

    void setBusy(bool busy);
    void clearPendingTimers();
    void startPendingRequest(PendingRequest request);
    void beginConnect();
    void sendPendingRequest();
    void sendRegisterRequest();
    void sendLoginRequest();
    void failConnect(const QString &message);
    void failResponse(const QString &message);
    void finishRegister(bool success, const QString &message, qint64 userId = 0);
    void finishLogin(bool success, const QString &message);

    TcpClient tcp_;
    QTimer connectTimer_;
    QTimer responseTimer_;

    PendingRequest pendingRequest_ = PendingRequest::None;
    bool isLoggedIn_ = false;
    bool busy_ = false;
    QString serverHost_;
    int serverPort_ = 16701;

    qint64 userId_ = 0;
    QString username_;
    QString nickname_;

    QString pendingNickname_;
    QString pendingUsername_;
    QString pendingPassword_;
};

#endif // CLIENTFACADE_H
