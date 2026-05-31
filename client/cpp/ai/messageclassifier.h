#pragma once

// ============================================================================
// 智能消息标签（Feature 2）— 核心算法层
//
// 阅读顺序：第 1 步，先看这里。
// 这是一个朴素贝叶斯二分类器，中文消息 → 待办/非待办。
//
// 特征提取（三步）：
//   1. cppjieba 分词 → 词级语义特征
//   2. 正则检测 → __EXACT_DATE__（具体日期，强信号）/ __RELATIVE_TIME__（模糊时间，弱信号）
//   3. 正则检测 → __TIME__（时间表达，含课程节次）
//
// 分词使用 cppjieba（header-only），CMake 自动引入。
// ============================================================================

#include <QHash>
#include <QPair>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

namespace cppjieba {
class Jieba;
}

struct Classification {
    bool isTodo = false;
    double confidence = 0.0;
};

class MessageClassifier
{
public:
    MessageClassifier();
    ~MessageClassifier();

    void train(const QVector<QPair<QString, QString>> &samples);
    Classification classify(const QString &text) const;

    bool save(const QString &path) const;
    bool load(const QString &path);

    bool trained() const { return trained_; }
    int todoSampleCount() const { return classDocCount_.value(QStringLiteral("todo"), 0); }
    int normalSampleCount() const { return classDocCount_.value(QStringLiteral("normal"), 0); }
    int vocabularySize() const { return vocabulary_.size(); }

    // 暴露 jieba 实例供 TaskExtractor 复用
    cppjieba::Jieba *jieba() const { return jieba_; }

    void clear();
    static QVector<QPair<QString, QString>> seedSamples();

private:
    // 特征提取：jieba 分词 + 日期/时间伪特征注入
    // "6月9日下午3点开会" → ["下午","3","点","开会","__EXACT_DATE__","__TIME__"]
    // "今天去超市"       → ["今天","去","超市","__RELATIVE_TIME__"]
    QStringList extractFeatures(const QString &text) const;

    void ensureBuilt();
    void trainSingle(const QString &text, const QString &label);
    double classLogProb(const QString &label, const QStringList &features) const;

    QHash<QString, int> classDocCount_;
    QHash<QString, int> classFeatureTotal_;
    QHash<QPair<QString, QString>, int> featureCount_;
    QSet<QString> vocabulary_;

    bool trained_ = false;
    static constexpr double kSmoothing = 1.0;
    static constexpr double kTodoThreshold = 0.55;
    static constexpr double kRelativeTimeFactor = 0.3;  // 弱补偿系数：__RELATIVE_TIME__只有全量的30%

    cppjieba::Jieba *jieba_ = nullptr;
};