#include "model/messagelistmodel.h"

MessageListModel::MessageListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int MessageListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return items_.size();
}

QVariant MessageListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= items_.size()) {
        return {};
    }

    const Item &item = items_.at(index.row());
    switch (role) {
    case WhoRole:
        return item.who;
    case TextRole:
        return item.text;
    default:
        return {};
    }
}

QHash<int, QByteArray> MessageListModel::roleNames() const
{
    return {
        {WhoRole, "who"},
        {TextRole, "text"},
    };
}

void MessageListModel::clear()
{
    if (items_.isEmpty()) {
        return;
    }
    beginResetModel();
    items_.clear();
    endResetModel();
}

void MessageListModel::appendMessage(const QString &who, const QString &text)
{
    const int row = items_.size();
    beginInsertRows(QModelIndex(), row, row);
    items_.append(Item{who, text});
    endInsertRows();
}

void MessageListModel::setMessages(const QVector<QPair<QString, QString>> &rows)
{
    beginResetModel();
    items_.clear();
    items_.reserve(rows.size());
    for (const auto &row : rows) {
        items_.append(Item{row.first, row.second});
    }
    endResetModel();
}
