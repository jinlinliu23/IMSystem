// ============================================================================
// 智能消息标签（Feature 2）— 核心算法实现
//
// 阅读顺序：看完 .h 后接着看这里。
//
// 算法要点：
// 1. 特征 = 中文字符二元组（bigram），不需要分词器
// 2. 训练 = 统计 P(bigram|label) 和 P(label)
// 3. 分类 = 用对数概率避免浮点下溢，softmax 归一化到 [0,1]
// 4. Laplace 平滑 (α=1) 处理未见过的 bigram
// 5. 模型 JSON 持久化，启动时加载，避免每次重新训练
// ============================================================================

#include "ai/messageclassifier.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtMath>

MessageClassifier::MessageClassifier()
{
}

// ---------------------------------------------------------------------------
// 训练流程
// ---------------------------------------------------------------------------

void MessageClassifier::ensureBuilt()
{
    if (trained_) {
        return;
    }

    const auto seeds = seedSamples();
    train(seeds);
}

// 种子数据：20 条待办 + 20 条普通消息，覆盖常见的含时间/动作/地点表述
QVector<QPair<QString, QString>> MessageClassifier::seedSamples()
{
    QVector<QPair<QString, QString>> samples;

    samples.append(qMakePair(QStringLiteral("明天下午3点开会"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("明天下午三点开会"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("下周一之前交报告"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("今天下午要交作业"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("明天上午10点见客户"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("今晚8点记得开会"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("后天去机场接人"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("周五之前完成前端页面"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("下午去买火车票"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("记得带身份证"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("明天体检别忘了空腹"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("下周三答辩准备PPT"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("这周末要搬家"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("晚上7点聚餐"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("明早8:30地铁站集合"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("今天之内把代码review完"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("下午4点面试候选人"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("周末去超市买东西"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("下个月1号之前提交报销"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("明晚电影院见"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("2026年8月10日出去玩"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("2026年7月10日上交作业到课堂派"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("2月8日下午2点参加讲座"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("帮我把资料放到实验楼"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("10月24日上午9点参加团日活动"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("帮我把材料放到办公桌上"), QStringLiteral("todo")));

    samples.append(qMakePair(QStringLiteral("你好"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("今天天气不错"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("你吃饭了吗"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("哈哈好的"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("没问题"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("收到"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("谢谢"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("知道了"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("好的好的"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("OK我看看"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("这个方案怎么样"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("我觉得可以"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("稍等一下我看看"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("辛苦了"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("这是测试数据"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("文档已经发你了"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("在吗"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("是的已经完成了"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("分组情况如文件所示"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("我来处理这个bug"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("笑死我了"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("那个教室"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("我很想你"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("我本将心照明月"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("没有人觉得很神圣吗"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("坐吧"), QStringLiteral("normal")));


    return samples;
}

// 中文特征提取：2-gram（二元字符组）
// "明天开会" → ["明天", "天开", "开会"]
// 不需要分词器，中文相邻的两个字本身就是强特征
QStringList MessageClassifier::extractBigrams(const QString &text) const
{
    QStringList bigrams;
    const QString cleaned = text.simplified();//去除空白字符
    if (cleaned.length() < 2) {
        if (cleaned.length() == 1) {
            bigrams.append(cleaned);
        }
        return bigrams;
    }

    for (int i = 0; i < cleaned.length() - 1; ++i) {
        bigrams.append(cleaned.mid(i, 2));
    }

    return bigrams;
}

void MessageClassifier::train(const QVector<QPair<QString, QString>> &samples)
{
    for (const auto &sample : samples) {
        trainSingle(sample.first, sample.second);
    }
    trained_ = true;
}

// 逐条训练：统计每个类的文档数、每个 bigram 在各类中出现的次数
void MessageClassifier::trainSingle(const QString &text, const QString &label)
{
    const QStringList bigrams = extractBigrams(text);

    classDocCount_[label] += 1;

    for (const QString &bg : bigrams) {
        vocabulary_.insert(bg);
        featureCount_[qMakePair(label, bg)] += 1;//标签与词组
        classFeatureTotal_[label] += 1;
    }
}

// ---------------------------------------------------------------------------
// 分类流程（核心数学）
//
// P(label|text) ∝ P(label) × Π P(bigram|label)
//
// 用 log 空间计算避免连乘出的浮点下溢：
//   logP(label|text) = logP(label) + Σ logP(bigram|label)
//
// P(bigram|label) = (count(bigram,label) + α) / (total_features(label) + α × vocab_size)
// 其中 α = 1 (Laplace 平滑)，避免未见 bigram 概率为 0
//
// 最后 softmax 归一化到 [0,1] 区间
// ---------------------------------------------------------------------------

double MessageClassifier::classLogProb(const QString &label, const QStringList &bigrams) const
{
    const int totalDocs = classDocCount_.value(QStringLiteral("todo"), 0)
                          + classDocCount_.value(QStringLiteral("normal"), 0);
    if (totalDocs <= 0) {
        return -1e9;
    }

    const int docCount = classDocCount_.value(label, 0);//属于lable标签的总语句（原始数据集）数
    const double logPrior = qLn(static_cast<double>(docCount) / totalDocs);//totalDocs总语句数 //先验概率的自然对数 P（lable）

    const int featuresInClass = classFeatureTotal_.value(label, 0);
    const int vocabSize = vocabulary_.size();
    if (vocabSize <= 0 || featuresInClass <= 0) {
        return logPrior;
    }

    double logLikelihood = 0.0;
    for (const QString &bg : bigrams) {
        const int count = featureCount_.value(qMakePair(label, bg), 0);
        const double prob = (count + kSmoothing) / (featuresInClass + kSmoothing * vocabSize);// P(A，Bi)/P(lable) 这里用的词频，因为数据总量一样。 条件概率
        logLikelihood += qLn(prob);
    }

    return logPrior + logLikelihood;
}

Classification MessageClassifier::classify(const QString &text) const
{
    Classification result;

    if (!trained_) {
        return result;
    }

    const QStringList bigrams = extractBigrams(text);
    if (bigrams.isEmpty()) {
        return result;
    }

    // 计算两个类的对数概率 后验概率
    const double logTodo = classLogProb(QStringLiteral("todo"), bigrams);
    const double logNormal = classLogProb(QStringLiteral("normal"), bigrams);

    // softmax 归一化：避免数值溢出，用 maxLog 做稳定化
    const double maxLog = qMax(logTodo, logNormal);
    const double expTodo = qExp(logTodo - maxLog);
    const double expNormal = qExp(logNormal - maxLog);
    const double sum = expTodo + expNormal;

    const double todoProb = sum > 0.0 ? expTodo / sum : 0.5;

    result.isTodo = todoProb >= kTodoThreshold;
    result.confidence = todoProb;

    return result;
}

// ---------------------------------------------------------------------------
// 模型持久化（JSON 格式）
// ---------------------------------------------------------------------------

bool MessageClassifier::save(const QString &path) const
{
    QJsonObject root;

    QJsonArray classDocArr;
    for (auto it = classDocCount_.cbegin(); it != classDocCount_.cend(); ++it) {
        QJsonObject entry;
        entry[QStringLiteral("class")] = it.key();
        entry[QStringLiteral("count")] = it.value();
        classDocArr.append(entry);
    }
    root[QStringLiteral("class_doc_count")] = classDocArr;

    QJsonArray classFeatArr;
    for (auto it = classFeatureTotal_.cbegin(); it != classFeatureTotal_.cend(); ++it) {
        QJsonObject entry;
        entry[QStringLiteral("class")] = it.key();
        entry[QStringLiteral("total")] = it.value();
        classFeatArr.append(entry);
    }
    root[QStringLiteral("class_feature_total")] = classFeatArr;

    QJsonArray featArr;
    for (auto it = featureCount_.cbegin(); it != featureCount_.cend(); ++it) {
        QJsonObject entry;
        entry[QStringLiteral("class")] = it.key().first;
        entry[QStringLiteral("feature")] = it.key().second;
        entry[QStringLiteral("count")] = it.value();
        featArr.append(entry);
    }
    root[QStringLiteral("feature_counts")] = featArr;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Compact);
    file.write(data);
    file.close();
    return true;
}

bool MessageClassifier::load(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }

    clear();

    const QJsonObject root = doc.object();

    const QJsonArray classDocArr = root.value(QStringLiteral("class_doc_count")).toArray();
    for (const auto &val : classDocArr) {
        const QJsonObject entry = val.toObject();
        classDocCount_[entry.value(QStringLiteral("class")).toString()] =
            entry.value(QStringLiteral("count")).toInt();
    }

    const QJsonArray classFeatArr = root.value(QStringLiteral("class_feature_total")).toArray();
    for (const auto &val : classFeatArr) {
        const QJsonObject entry = val.toObject();
        classFeatureTotal_[entry.value(QStringLiteral("class")).toString()] =
            entry.value(QStringLiteral("total")).toInt();
    }

    const QJsonArray featArr = root.value(QStringLiteral("feature_counts")).toArray();
    for (const auto &val : featArr) {
        const QJsonObject entry = val.toObject();
        const QString cls = entry.value(QStringLiteral("class")).toString();
        const QString feat = entry.value(QStringLiteral("feature")).toString();
        const int cnt = entry.value(QStringLiteral("count")).toInt();
        featureCount_[qMakePair(cls, feat)] = cnt;
        vocabulary_.insert(feat);
    }

    trained_ = !classDocCount_.isEmpty();
    return trained_;
}

void MessageClassifier::clear()
{
    classDocCount_.clear();
    classFeatureTotal_.clear();
    featureCount_.clear();
    vocabulary_.clear();
    trained_ = false;
}