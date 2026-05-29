// ============================================================================
// 智能消息标签（Feature 2）— 模型层实现
//
// 阅读顺序：接上面 .h。
// 给 MessageListModel 的 data/roleNames 加上 TodoScoreRole，
// 并实现 reclassifyAll / reclassifyLast 两个分类驱动方法。
// ============================================================================

#include "model/messagelistmodel.h"
#include "ai/messageclassifier.h"

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
    case SenderNameRole:
        return item.senderName;
    case SenderAccountRole:
        return item.senderAccount;
    case TodoScoreRole:              // [Feature 2] QML 通过 model.todoScore 读取
        return item.todoScore;
    default:
        return {};
    }
}

QHash<int, QByteArray> MessageListModel::roleNames() const
{
    return {
        {WhoRole, "who"},
        {TextRole, "text"},
        {SenderNameRole, "senderName"},
        {SenderAccountRole, "senderAccount"},
        {TodoScoreRole, "todoScore"},  // [Feature 2]
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

void MessageListModel::appendMessage(const QString &who,
                                     const QString &text,
                                     const QString &senderName,
                                     const QString &senderAccount)
{
    const int row = items_.size();
    beginInsertRows(QModelIndex(), row, row);
    items_.append(Item{who, text, senderName, senderAccount, 0.0});
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

// ---------------------------------------------------------------------------
// [Feature 2] 分类驱动
// ---------------------------------------------------------------------------

// 全量重新分类：用于开启智能标签、加载本地历史消息时
void MessageListModel::reclassifyAll(MessageClassifier *classifier)
{
    if (!classifier || !classifier->trained() || items_.isEmpty()) {
        return;
    }

    for (int i = 0; i < items_.size(); ++i) {
        const Classification result = classifier->classify(items_.at(i).text);
        if (!qFuzzyCompare(items_[i].todoScore, result.confidence)) {
            items_[i].todoScore = result.isTodo ? result.confidence : 0.0;
            emit dataChanged(index(i), index(i), {TodoScoreRole});
        }
    }
}

// 增量分类：用于新消息实时到达时，只分类最新一条
void MessageListModel::reclassifyLast(MessageClassifier *classifier)
{
    if (!classifier || !classifier->trained() || items_.isEmpty()) {
        return;
    }

    const int i = items_.size() - 1;
    const Classification result = classifier->classify(items_.at(i).text);
    items_[i].todoScore = result.isTodo ? result.confidence : 0.0;
    emit dataChanged(index(i), index(i), {TodoScoreRole});
}