#pragma once

// ============================================================================
// 智能消息标签（Feature 2）— 模型层
//
// 阅读顺序：第 2 步。
// 消息列表模型新增 TodoScoreRole，存每条消息的分类置信度。
// QML 通过 model.todoScore 读取，> 0.55 的视为待办。
// ============================================================================

#include <QAbstractListModel>
#include <QPair>
#include <QVector>

class MessageClassifier;

class MessageListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        WhoRole = Qt::UserRole + 1,
        TextRole,
        SenderNameRole,
        SenderAccountRole,
        TodoScoreRole,           // [Feature 2] 待办置信度 0.0~1.0
    };

    explicit MessageListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void clear();
    Q_INVOKABLE void appendMessage(const QString &who,
                                   const QString &text,
                                   const QString &senderName = QString(),
                                   const QString &senderAccount = QString());
    void setMessages(const QVector<QPair<QString, QString>> &rows);

    // [Feature 2] 用分类器重新标注所有消息的 todoScore
    void reclassifyAll(MessageClassifier *classifier);
    // [Feature 2] 只重新标注最后一条（新消息到达时高效增量更新）
    void reclassifyLast(MessageClassifier *classifier);

private:
    struct Item {
        QString who;
        QString text;
        QString senderName;
        QString senderAccount;
        double todoScore = 0.0;  // [Feature 2] 默认 0 = 未分类
    };

    QVector<Item> items_;
};