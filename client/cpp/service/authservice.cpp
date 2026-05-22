#include "service/authservice.h"

#include "core/clientsettings.h"
#include "model/currentusermodel.h"
#include "network/clientmessagerouter.h"
#include "network/tcpclient.h"
#include "msg_ids.h"

#include <QJsonDocument>
#include <QJsonObject>

AuthService::AuthService(ClientSettings *settings,
                         CurrentUserModel *currentUser,
                         ClientMessageRouter *router,
                         QObject *parent)
    : QObject(parent)
    , settings_(settings)
    , currentUser_(currentUser)
    , router_(router)
{
}

void AuthService::registerUser(const QString &nickname,
                               const QString &account,
                               const QString &password)
{
    if (!settings_ || !router_) {
        emit registerFinished(false, QStringLiteral("内部错误"), 0);
        return;
    }
    if (router_->busy()) {
        emit registerFinished(false, QStringLiteral("请稍候，正在处理上一请求"), 0);
        return;
    }
    if (settings_->serverHost().isEmpty()) {
        emit registerFinished(false, QStringLiteral("请填写服务器 IP 地址"), 0);
        return;
    }
    if (nickname.trimmed().isEmpty()) {
        emit registerFinished(false, QStringLiteral("请输入昵称"), 0);
        return;
    }
    if (account.trimmed().size() < 3) {
        emit registerFinished(false, QStringLiteral("账号至少 3 个字符"), 0);
        return;
    }
    if (password.size() < 6) {
        emit registerFinished(false, QStringLiteral("密码至少 6 位"), 0);
        return;
    }

    settings_->saveServer();
    pendingNickname_ = nickname.trimmed();
    pendingAccount_ = account.trimmed();
    pendingPassword_ = password;

    QJsonObject obj;
    obj.insert(QStringLiteral("nickname"), pendingNickname_);
    obj.insert(QStringLiteral("account"), pendingAccount_);
    obj.insert(QStringLiteral("password"), pendingPassword_);
    const QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);

    router_->request(
        settings_->serverHost(),
        static_cast<quint16>(settings_->serverPort()),
        static_cast<quint16>(MSG_IDS::MSG_REGISTER),
        body,
        static_cast<quint16>(MSG_IDS::MSG_REGISTER_RSP),
        [this](const QByteArray &rspBody) {
            QJsonParseError parseError;
            const QJsonDocument doc = QJsonDocument::fromJson(rspBody, &parseError);
            if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
                emit registerFinished(false, QStringLiteral("服务器响应格式错误"), 0);
                return;
            }
            const QJsonObject obj = doc.object();
            const int code = obj.value(QStringLiteral("code")).toInt(-1);
            const QString msg = obj.value(QStringLiteral("msg")).toString(QStringLiteral("未知错误"));
            const qint64 userId = obj.value(QStringLiteral("user_id")).toInteger(0);
            if (code == static_cast<int>(API_CODE::OK)) {
                emit registerFinished(true, msg, userId);
            } else {
                emit registerFinished(false, msg, 0);
            }
        },
        [this](const QString &err) {
            emit registerFinished(false, err, 0);
        });
}

void AuthService::loginUser(const QString &account, const QString &password)
{
    if (!settings_ || !router_ || !currentUser_) {
        emit loginFinished(false, QStringLiteral("内部错误"));
        return;
    }
    if (router_->busy()) {
        emit loginFinished(false, QStringLiteral("请稍候，正在处理上一请求"));
        return;
    }
    if (settings_->serverHost().isEmpty()) {
        emit loginFinished(false, QStringLiteral("请填写服务器 IP 地址"));
        return;
    }
    if (account.trimmed().isEmpty()) {
        emit loginFinished(false, QStringLiteral("请输入账号"));
        return;
    }
    if (password.isEmpty()) {
        emit loginFinished(false, QStringLiteral("请输入密码"));
        return;
    }

    settings_->saveServer();
    performLogin(account.trimmed(), password, false);
}

void AuthService::resumeSession()
{
    if (!settings_ || !router_ || !currentUser_) {
        emit sessionResumeFinished(false, QStringLiteral("内部错误"));
        return;
    }
    if (!settings_->loggedIn() || settings_->account().isEmpty()) {
        emit sessionResumeFinished(false, QStringLiteral("未登录"));
        return;
    }
    if (!settings_->hasSessionPassword()) {
        emit sessionResumeFinished(false, QString());
        return;
    }
    if (router_->busy()) {
        emit sessionResumeFinished(false, QStringLiteral("请稍候，正在处理上一请求"));
        return;
    }
    if (settings_->serverHost().isEmpty()) {
        emit sessionResumeFinished(false, QStringLiteral("请填写服务器 IP 地址"));
        return;
    }

    performLogin(settings_->account(), settings_->sessionPassword(), true);
}

void AuthService::performLogin(const QString &account, const QString &password, bool isResume)
{
    pendingAccount_ = account;
    pendingPassword_ = password;

    QJsonObject obj;
    obj.insert(QStringLiteral("account"), pendingAccount_);
    obj.insert(QStringLiteral("password"), pendingPassword_);
    const QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);

    router_->request(
        settings_->serverHost(),
        static_cast<quint16>(settings_->serverPort()),
        static_cast<quint16>(MSG_IDS::MSG_LOGIN),
        body,
        static_cast<quint16>(MSG_IDS::MSG_LOGIN_RSP),
        [this, isResume](const QByteArray &rspBody) {
            QJsonParseError parseError;
            const QJsonDocument doc = QJsonDocument::fromJson(rspBody, &parseError);
            if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
                const QString err = QStringLiteral("服务器响应格式错误");
                if (isResume) {
                    emit sessionResumeFinished(false, err);
                } else {
                    emit loginFinished(false, err);
                }
                return;
            }
            const QJsonObject obj = doc.object();
            const int code = obj.value(QStringLiteral("code")).toInt(-1);
            const QString msg = obj.value(QStringLiteral("msg")).toString(QStringLiteral("未知错误"));
            if (code == static_cast<int>(API_CODE::OK)) {
                const qint64 userId = obj.value(QStringLiteral("user_id")).toInteger(0);
                const QString acc = obj.value(QStringLiteral("account")).toString().trimmed();
                const QString nick = obj.value(QStringLiteral("nickname")).toString();
                currentUser_->setUserInfo(userId, acc, nick);
                currentUser_->setLoggedIn(true);
                currentUser_->applyToSettings(settings_);
                settings_->setSessionPassword(pendingPassword_);
                if (isResume) {
                    emit sessionResumeFinished(true, msg);
                } else {
                    emit loginFinished(true, msg);
                }
            } else if (isResume) {
                emit sessionResumeFinished(false, msg);
            } else {
                emit loginFinished(false, msg);
            }
        },
        [this, isResume](const QString &err) {
            if (isResume) {
                emit sessionResumeFinished(false, err);
            } else {
                emit loginFinished(false, err);
            }
        });
}

void AuthService::logout()
{
    if (currentUser_) {
        currentUser_->clear();
    }
    if (settings_) {
        settings_->clearAuth();
        if (currentUser_) {
            currentUser_->loadFromSettings(settings_);
        }
    }
    TcpClient::instance().disconnectFromServer();
}
