#pragma once

// ============================================================================
// Feature 1：个人事务管家 — 待办列表模型
//
// 暴露给 QML，支持待办列表展示和操作。
// ============================================================================

#include <QAbstractListModel>
#include <QVector>

struct UserTask;
class ChatLocalStore;
class TaskExtractor;

class TaskListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        OriginalTextRole,
        FromNicknameRole,
        ExtractedActionRole,
        ExtractedTimeRole,
        ExtractedPlaceRole,
        IsCompletedRole,
        AiProcessedRole,
        CreatedAtRole,
        PeerAccountRole,
        ConversationIdRole,
    };

    explicit TaskListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setStore(ChatLocalStore *store);
    void setExtractor(TaskExtractor *extractor);

    Q_INVOKABLE void reload();
    Q_INVOKABLE void toggleCompleted(int taskId, bool completed);
    Q_INVOKABLE void removeTask(int taskId);
    Q_INVOKABLE void extractAll();

private:
    QVector<UserTask> items_;
    ChatLocalStore *store_ = nullptr;
    TaskExtractor *extractor_ = nullptr;
};