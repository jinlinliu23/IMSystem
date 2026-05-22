#pragma once

#include <QObject>
#include <QString>

class ClientSettings;
class ClientMessageRouter;
class CurrentUserModel;

/**
 * @brief 注册、登录业务
 */
class AuthService : public QObject
{
    Q_OBJECT
public:
    explicit AuthService(ClientSettings *settings,
                         CurrentUserModel *currentUser,
                         ClientMessageRouter *router,
                         QObject *parent = nullptr);

    Q_INVOKABLE void registerUser(const QString &nickname,
                                  const QString &account,
                                  const QString &password);
    Q_INVOKABLE void loginUser(const QString &account, const QString &password);
    /** 已本地登录时向服务端重新登录，绑定在线并拉取离线私聊 */
    Q_INVOKABLE void resumeSession();
    Q_INVOKABLE void logout();

signals:
    void registerFinished(bool success, const QString &message, qint64 userId);
    void loginFinished(bool success, const QString &message);
    void sessionResumeFinished(bool success, const QString &message);

private:
    void performLogin(const QString &account, const QString &password, bool isResume);

    ClientSettings *settings_ = nullptr;
    CurrentUserModel *currentUser_ = nullptr;
    ClientMessageRouter *router_ = nullptr;

    QString pendingNickname_;
    QString pendingAccount_;
    QString pendingPassword_;
};
