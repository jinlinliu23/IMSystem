#pragma once

// ============================================================================
// 智能消息标签（Feature 2）— 服务层
//
// 阅读顺序：第 3 步。
// ChatService 持有 MessageClassifier 实例，管理分类器的生命周期：
//   - 构造时创建分类器
//   - 首次使用时加载持久化模型，失败则用种子数据训练
//   - setSmartTagEnabled(true) → 触发全量分类
//   - 每条新消息到达时（appendChatLine）→ 增量分类最后一条
//   - 关闭时保持分类器，下次开启更快
// ============================================================================

#include <QObject>
#include <QString>

class ClientSettings;
class ClientMessageRouter;
class CurrentUserModel;
class ContactListModel;
class MessageListModel;
class ConversationListModel;
class ChatLocalStore;
class MessageClassifier;

class ChatService : public QObject
{
    Q_OBJECT
    // [Feature 2] 暴露给 QML：ClientFacade.smartTagEnabled
    Q_PROPERTY(bool smartTagEnabled READ smartTagEnabled NOTIFY smartTagEnabledChanged)
public:
    explicit ChatService(ClientSettings *settings,
                         CurrentUserModel *currentUser,
                         ClientMessageRouter *router,
                         ConversationListModel *conversations,
                         MessageListModel *messages,
                         ContactListModel *contacts,
                         QObject *parent = nullptr);

    void setOwnerAccount(const QString &account);
    void mergeLocalPreviewsIntoConversations();
    void syncConversationsFromLocalStore();

    Q_INVOKABLE void openChat(const QString &peerAccount, const QString &peerTitle);
    Q_INVOKABLE void sendMessage(const QString &content);
    Q_INVOKABLE void openGroupChat(qint64 groupId, const QString &groupName);
    Q_INVOKABLE void sendGroupMessage(qint64 groupId, const QString &content);
    Q_INVOKABLE void markGroupDissolved(qint64 groupId, const QString &groupName);
    void clearSession();

    ChatLocalStore *localStore() const { return localStore_; }
    MessageClassifier *classifier() const { return classifier_; }
    // MessageClassifier 前向声明在 ai/messageclassifier.h

    // [Feature 2] 智能标签开关
    bool smartTagEnabled() const { return smartTagEnabled_; }
    Q_INVOKABLE void setSmartTagEnabled(bool enabled);

public slots:
    void onOwnGroupMessageSent(qint64 groupId, const QString &content, qint64 createdAt, qint64 messageId);

signals:
    void sendMessageFinished(bool success, const QString &message);
    void chatHistoryLoaded();
    void conversationPreviewUpdated();
    void groupStructureChanged();
    void groupEventsChanged();
    // [Feature 2] 开关状态变化通知 QML
    void smartTagEnabledChanged();

private:
    void handlePrivateNotify(const QByteArray &body);
    bool persistMessage(const QString &peerAccount,
                        const QString &fromAccount,
                        const QString &content,
                        qint64 createdAt,
                        qint64 serverMessageId,
                        const QString &fromNickname = QString());
    void loadLocalMessagesToUi(const QString &peerAccount);
    void appendChatLine(const QString &who,
                        const QString &text,
                        const QString &senderName = QString(),
                        const QString &senderAccount = QString());
    QString nicknameForAccount(const QString &account) const;
    QString resolveSenderNickname(const QString &fromAccount, const QString &storedNickname) const;
    void updateConversationPreview(const QString &peerAccount,
                                   const QString &previewText,
                                   qint64 createdAt);
    void notifyIncomingMessage(const QString &conversationId,
                               const QString &previewText,
                               qint64 createdAt,
                               bool incrementUnread);
    void markConversationRead(const QString &conversationId);
    void handleGroupNotify(const QByteArray &body);

    // [Feature 2] 确保分类器就绪：加载模型或回退种子训练
    void ensureClassifierReady();
    // [Feature 2] 对当前聊天页所有消息重新分类
    void reclassifyCurrentMessages();

    static QString formatMessageTime(qint64 unixSec);
    static QString previewText(const QString &content);
    bool ensureLoggedIn() const;
    bool ensureLocalStoreReady();
    bool isGroupActive(qint64 groupId) const;
    qint64 effectiveMessageId(qint64 serverMessageId,
                              const QString &peerAccount,
                              const QString &fromAccount,
                              const QString &content,
                              qint64 createdAt) const;

    ClientSettings *settings_ = nullptr;
    CurrentUserModel *currentUser_ = nullptr;
    ClientMessageRouter *router_ = nullptr;
    ConversationListModel *conversations_ = nullptr;
    MessageListModel *messages_ = nullptr;
    ContactListModel *contacts_ = nullptr;
    ChatLocalStore *localStore_ = nullptr;

    // [Feature 2]
    MessageClassifier *classifier_ = nullptr;        // 朴素贝叶斯分类器
    QString classifierModelPath_;                   // 模型 JSON 文件路径
    bool smartTagEnabled_ = false;                  // 智能标签开关状态

    QString activePeerAccount_;
    QString activePeerTitle_;
};