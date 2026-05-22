#pragma once

#include <QAbstractListModel>
#include <QVector>

class FriendRequestListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY pendingCountChanged)
public:
    enum Roles {
        RequestIdRole = Qt::UserRole + 1,
        FromUserIdRole,
        FromAccountRole,
        FromNicknameRole,
    };

    explicit FriendRequestListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int pendingCount() const { return items_.size(); }

    void setRequests(const QVector<QVariantMap> &rows);

signals:
    void pendingCountChanged();

private:
    struct Item {
        qint64 requestId = 0;
        qint64 fromUserId = 0;
        QString fromAccount;
        QString fromNickname;
    };

    QVector<Item> items_;
};
