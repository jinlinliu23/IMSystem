#pragma once

#include <QObject>
#include <QString>

/**
 * @brief 客户端本地配置（QSettings：服务器地址、登录态）
 */
class ClientSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString serverHost READ serverHost WRITE setServerHost NOTIFY serverHostChanged)
    Q_PROPERTY(int serverPort READ serverPort WRITE setServerPort NOTIFY serverPortChanged)

public:
    explicit ClientSettings(QObject *parent = nullptr);

    QString serverHost() const { return serverHost_; }
    int serverPort() const { return serverPort_; }

    void setServerHost(const QString &host);
    void setServerPort(int port);

    bool loggedIn() const { return loggedIn_; }
    qint64 userId() const { return userId_; }
    QString account() const { return account_; }
    QString nickname() const { return nickname_; }
    QString sessionPassword() const { return sessionPassword_; }
    bool hasSessionPassword() const { return !sessionPassword_.isEmpty(); }

    void setLoggedIn(bool loggedIn);
    void setUserInfo(qint64 userId, const QString &account, const QString &nickname);
    void setSessionPassword(const QString &password);
    void clearAuth();

    Q_INVOKABLE void load();
    Q_INVOKABLE void saveServer();
    Q_INVOKABLE void saveAuth();

signals:
    void serverHostChanged();
    void serverPortChanged();
    void authChanged();

private:
    static QString defaultServerHost();

    QString serverHost_;
    int serverPort_ = 16701;
    bool loggedIn_ = false;
    qint64 userId_ = 0;
    QString account_;
    QString nickname_;
    QString sessionPassword_;
};
