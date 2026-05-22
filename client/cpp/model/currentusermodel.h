#pragma once

#include <QObject>
#include <QString>

class ClientSettings;

/**
 * @brief 当前登录用户（供 QML 绑定）
 */
class CurrentUserModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY userChanged)
    Q_PROPERTY(qint64 userId READ userId NOTIFY userChanged)
    Q_PROPERTY(QString account READ account NOTIFY userChanged)
    Q_PROPERTY(QString nickname READ nickname NOTIFY userChanged)

public:
    explicit CurrentUserModel(QObject *parent = nullptr);

    bool loggedIn() const { return loggedIn_; }
    qint64 userId() const { return userId_; }
    QString account() const { return account_; }
    QString nickname() const { return nickname_; }

    void loadFromSettings(ClientSettings *settings);
    void applyToSettings(ClientSettings *settings) const;

    void setLoggedIn(bool loggedIn);
    void setUserInfo(qint64 userId, const QString &account, const QString &nickname);
    void clear();

signals:
    void userChanged();

private:
    bool loggedIn_ = false;
    qint64 userId_ = 0;
    QString account_;
    QString nickname_;
};
