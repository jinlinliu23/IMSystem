#include "model/currentusermodel.h"
#include "core/clientsettings.h"

CurrentUserModel::CurrentUserModel(QObject *parent)
    : QObject(parent)
{
}

void CurrentUserModel::loadFromSettings(ClientSettings *settings)
{
    if (!settings) {
        return;
    }
    loggedIn_ = settings->loggedIn();
    userId_ = settings->userId();
    account_ = settings->account();
    nickname_ = settings->nickname();
    emit userChanged();
}

void CurrentUserModel::applyToSettings(ClientSettings *settings) const
{
    if (!settings) {
        return;
    }
    settings->setLoggedIn(loggedIn_);
    settings->setUserInfo(userId_, account_, nickname_);
}

void CurrentUserModel::setLoggedIn(bool loggedIn)
{
    if (loggedIn_ == loggedIn) {
        return;
    }
    loggedIn_ = loggedIn;
    emit userChanged();
}

void CurrentUserModel::setUserInfo(qint64 userId, const QString &account, const QString &nickname)
{
    userId_ = userId;
    account_ = account;
    nickname_ = nickname;
    emit userChanged();
}

void CurrentUserModel::clear()
{
    userId_ = 0;
    account_.clear();
    nickname_.clear();
    loggedIn_ = false;
    emit userChanged();
}
