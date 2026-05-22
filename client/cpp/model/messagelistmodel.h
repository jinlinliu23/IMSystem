#pragma once

#include <QAbstractListModel>
#include <QPair>
#include <QVector>

/**
 * @brief 当前会话消息列表（聊天页）
 */
class MessageListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        WhoRole = Qt::UserRole + 1,
        TextRole,
    };

    explicit MessageListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void clear();
    Q_INVOKABLE void appendMessage(const QString &who, const QString &text);
    void setMessages(const QVector<QPair<QString, QString>> &rows);

private:
    struct Item {
        QString who;
        QString text;
    };

    QVector<Item> items_;
};
