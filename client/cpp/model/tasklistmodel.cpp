#include "model/tasklistmodel.h"
#include "ai/taskextractor.h"
#include "storage/chatlocalstore.h"

TaskListModel::TaskListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void TaskListModel::setStore(ChatLocalStore *store)
{
    store_ = store;
}

void TaskListModel::setExtractor(TaskExtractor *extractor)
{
    extractor_ = extractor;
}

int TaskListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return items_.size();
}

QVariant TaskListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= items_.size())
        return {};

    const UserTask &t = items_.at(index.row());
    switch (role) {
    case IdRole:               return t.id;
    case OriginalTextRole:     return t.originalText;
    case FromNicknameRole:     return t.fromNickname;
    case ExtractedActionRole:  return t.extractedAction;
    case ExtractedTimeRole:    return t.extractedTime;
    case ExtractedPlaceRole:   return t.extractedPlace;
    case IsCompletedRole:      return t.isCompleted;
    case AiProcessedRole:      return t.aiProcessed;
    case CreatedAtRole:        return t.createdAt;
    case PeerAccountRole:      return t.peerAccount;
    case ConversationIdRole:   return t.conversationId;
    default: break;
    }
    return {};
}

QHash<int, QByteArray> TaskListModel::roleNames() const
{
    return {
        {IdRole,               "taskId"},
        {OriginalTextRole,     "originalText"},
        {FromNicknameRole,     "fromNickname"},
        {ExtractedActionRole,  "extractedAction"},
        {ExtractedTimeRole,    "extractedTime"},
        {ExtractedPlaceRole,   "extractedPlace"},
        {IsCompletedRole,      "isCompleted"},
        {AiProcessedRole,      "aiProcessed"},
        {CreatedAtRole,        "createdAt"},
        {PeerAccountRole,      "peerAccount"},
        {ConversationIdRole,   "conversationId"},
    };
}

void TaskListModel::reload()
{
    if (!store_) return;

    beginResetModel();
    items_ = store_->listTasks();
    endResetModel();
}

void TaskListModel::toggleCompleted(int taskId, bool completed)
{
    if (!store_) return;
    store_->setTaskCompleted(taskId, completed);

    for (int i = 0; i < items_.size(); ++i) {
        if (items_[i].id == taskId) {
            items_[i].isCompleted = completed;
            emit dataChanged(index(i), index(i), {IsCompletedRole});
            break;
        }
    }
}

void TaskListModel::removeTask(int taskId)
{
    if (!store_) return;
    store_->removeTask(taskId);

    for (int i = 0; i < items_.size(); ++i) {
        if (items_[i].id == taskId) {
            beginRemoveRows(QModelIndex(), i, i);
            items_.removeAt(i);
            endRemoveRows();
            break;
        }
    }
}

void TaskListModel::extractAll()
{
    if (!store_ || !extractor_) return;

    for (int i = 0; i < items_.size(); ++i) {
        UserTask &t = items_[i];
        if (t.aiProcessed) continue;

        const TaskExtractResult r = extractor_->extract(t.originalText);
        t.extractedTime   = r.time;
        t.extractedPlace  = r.place;
        t.aiProcessed     = true;

        store_->updateTaskExtracted(t.id, QString(), r.time, r.place);
        emit dataChanged(index(i), index(i),
                         {ExtractedActionRole, ExtractedTimeRole, ExtractedPlaceRole, AiProcessedRole});
    }
}