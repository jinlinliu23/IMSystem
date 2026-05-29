#pragma once

// ============================================================================
// 智能消息标签（Feature 2）— 核心算法层
//
// 阅读顺序：第 1 步，先看这里。
// 这是一个朴素贝叶斯二分类器，中文消息 → 待办/非待办。
// 不依赖任何外部库，纯 C++ 实现。
// ============================================================================

#include <QHash>
#include <QPair>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

// 分类结果：一条消息被判定为"待办"的结论
struct Classification {
    bool isTodo = false;       // 是否待办（置信度 >= 阈值时为 true）
    double confidence = 0.0;   // P(待办|消息)，0.0~1.0
};

class MessageClassifier
{
public:
    MessageClassifier();

    // 训练：输入 (文本, 标签) 列表，标签为 "todo" 或 "normal"
    void train(const QVector<QPair<QString, QString>> &samples);
    // 分类：对单条文本做预测
    Classification classify(const QString &text) const;

    // 模型持久化：保存为 JSON 文件，下次启动直接加载，避免重复训练
    bool save(const QString &path) const;
    bool load(const QString &path);

    bool trained() const { return trained_; }
    int todoSampleCount() const { return classDocCount_.value(QStringLiteral("todo"), 0); }
    int normalSampleCount() const { return classDocCount_.value(QStringLiteral("normal"), 0); }
    int vocabularySize() const { return vocabulary_.size(); }

    void clear();
    // 种子训练数据：40 条中文例句，确保冷启动即可用
    static QVector<QPair<QString, QString>> seedSamples();

private:
    // 中文字符二元组提取："明天开会" → ["明天","天开","开会"]
    QStringList extractBigrams(const QString &text) const;

    void ensureBuilt();
    void trainSingle(const QString &text, const QString &label);
    // 核心公式：log P(label) + Σ log P(bigram|label)，加 Laplace 平滑
    double classLogProb(const QString &label, const QStringList &bigrams) const;

    // 训练状态
    QHash<QString, int> classDocCount_;                         // 每类文档数 只有两类S
    QHash<QString, int> classFeatureTotal_;                     // 每类特征总数 每个分词属于 两类 中的累计计数
    QHash<QPair<QString, QString>, int> featureCount_;          // (类, bigram) 出现次数
    QSet<QString> vocabulary_;                                  // 所有见过的 bigram

    bool trained_ = false;
    static constexpr double kSmoothing = 1.0;                   // Laplace 平滑系数
    static constexpr double kTodoThreshold = 0.55;              // 判定为待办的置信度阈值
};