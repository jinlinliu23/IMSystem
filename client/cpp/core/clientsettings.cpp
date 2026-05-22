#include "clientsettings.h"

#include <QSettings>

ClientSettings::ClientSettings(QObject *parent)
    : QObject(parent)
{
    load();
}

QString ClientSettings::defaultServerHost()
{
#if defined(Q_OS_ANDROID)
    return QStringLiteral("10.0.2.2");
#else
    return QStringLiteral("127.0.0.1");
#endif
}

void ClientSettings::load()
{
    QSettings settings;
    serverHost_ = settings.value(QStringLiteral("server/host"), defaultServerHost()).toString().trimmed();
    serverPort_ = settings.value(QStringLiteral("server/port"), 16701).toInt();
    if (serverPort_ <= 0 || serverPort_ > 65535) {
        serverPort_ = 16701;
    }

    loggedIn_ = settings.value(QStringLiteral("auth/logged_in"), false).toBool();
    userId_ = settings.value(QStringLiteral("auth/user_id"), 0).toLongLong();
    account_ = settings.value(QStringLiteral("auth/account")).toString();
    if (account_.isEmpty()) {
        account_ = settings.value(QStringLiteral("auth/username")).toString();
    }
    nickname_ = settings.value(QStringLiteral("auth/nickname")).toString();
    sessionPassword_ = settings.value(QStringLiteral("auth/session_password")).toString();

    emit serverHostChanged();
    emit serverPortChanged();
    emit authChanged();
}

void ClientSettings::saveServer()
{
    QSettings settings;
    settings.setValue(QStringLiteral("server/host"), serverHost_);
    settings.setValue(QStringLiteral("server/port"), serverPort_);
}

void ClientSettings::saveAuth()
{
    QSettings settings;
    settings.setValue(QStringLiteral("auth/logged_in"), loggedIn_);
    settings.setValue(QStringLiteral("auth/user_id"), userId_);
    settings.setValue(QStringLiteral("auth/account"), account_);
    settings.setValue(QStringLiteral("auth/nickname"), nickname_);
    settings.setValue(QStringLiteral("auth/session_password"), sessionPassword_);
}

void ClientSettings::setSessionPassword(const QString &password)
{
    sessionPassword_ = password;
    saveAuth();
}

void ClientSettings::setServerHost(const QString &host)
{
    const QString trimmed = host.trimmed();
    if (serverHost_ == trimmed) {
        return;
    }
    serverHost_ = trimmed;
    emit serverHostChanged();
}

void ClientSettings::setServerPort(int port)
{
    if (serverPort_ == port) {
        return;
    }
    serverPort_ = port;
    emit serverPortChanged();
}

void ClientSettings::setLoggedIn(bool loggedIn)
{
    if (loggedIn_ == loggedIn) {
        return;
    }
    loggedIn_ = loggedIn;
    saveAuth();
    emit authChanged();
}

void ClientSettings::setUserInfo(qint64 userId, const QString &account, const QString &nickname)
{
    userId_ = userId;
    account_ = account;
    nickname_ = nickname;
    saveAuth();
    emit authChanged();
}

void ClientSettings::clearAuth()
{
    userId_ = 0;
    account_.clear();
    nickname_.clear();
    sessionPassword_.clear();
    loggedIn_ = false;
    saveAuth();
    emit authChanged();
}
