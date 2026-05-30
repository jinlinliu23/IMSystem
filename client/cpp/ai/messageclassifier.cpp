// ============================================================================
// 智能消息标签（Feature 2）— 核心算法实现
//
// 阅读顺序：看完 .h 后接着看这里。
//
// 算法要点：
// 1. 特征 = cppjieba 分词 token + 伪特征（__EXACT_DATE__ / __RELATIVE_TIME__ / __TIME__）
// 2. 日期分两级：__EXACT_DATE__（具体日期/指定星期）→ classify() 补偿；
//    __RELATIVE_TIME__（今天/明天/周末）→ 无补偿，靠贝叶斯正常判断
// 3. 时间表达：冒号时间、点钟、时段、课程节次(X-X节)、单双周
// 4. 训练 = 统计 P(token|label)，种子数据平衡 todo/normal 数量
// 5. 分类 = 对数概率 + Laplace 平滑 + softmax
// 6. 模型 JSON 持久化，启动时加载，避免每次重新训练
// ============================================================================

#include "ai/messageclassifier.h"
#include "cppjieba/Jieba.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QtMath>

// ============================================================================
// 词库路径（桌面 / Android 兼容）
// ============================================================================

static QString jiebaDictDir()
{
#ifdef Q_OS_ANDROID
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                        + QStringLiteral("/jieba_dict");

    static const QStringList files = {
        QStringLiteral("jieba.dict.utf8"),
        QStringLiteral("hmm_model.utf8"),
        QStringLiteral("user.dict.utf8"),
        QStringLiteral("idf.utf8"),
        QStringLiteral("stop_words.utf8"),
    };

    bool needCopy = false;
    for (const QString &f : files) {
        if (!QFile::exists(dir + QStringLiteral("/") + f)) {
            needCopy = true;
            break;
        }
    }

    if (needCopy) {
        QDir().mkpath(dir);
        for (const QString &f : files) {
            const QString dest = dir + QStringLiteral("/") + f;
            if (!QFile::exists(dest)) {
                QFile src(QStringLiteral("assets:/dict/") + f);
                if (src.open(QIODevice::ReadOnly)) {
                    QFile dst(dest);
                    if (dst.open(QIODevice::WriteOnly)) {
                        dst.write(src.readAll());
                    }
                }
            }
        }
    }

    return dir + QStringLiteral("/");
#else
    return QCoreApplication::applicationDirPath() + QStringLiteral("/dict/");
#endif
}

// ============================================================================
// 构造 / 析构
// ============================================================================

MessageClassifier::MessageClassifier()
{
    const std::string dictDir = jiebaDictDir().toStdString();
    jieba_ = new cppjieba::Jieba(
        dictDir + "jieba.dict.utf8",
        dictDir + "hmm_model.utf8",
        dictDir + "user.dict.utf8",
        dictDir + "idf.utf8",
        dictDir + "stop_words.utf8"
    );
}

MessageClassifier::~MessageClassifier()
{
    delete jieba_;
}

// ============================================================================
// 训练
// ============================================================================

void MessageClassifier::ensureBuilt()
{
    if (trained_) return;
    const auto seeds = seedSamples();
    train(seeds);
}

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
    samples.append(qMakePair(QStringLiteral("1月15日之前完成项目文档"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("12月31日晚上跨年聚会"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("这周五下班前提交周报"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("下周二上午面试"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("本周三中午部门聚餐"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("3月5日下午3点开会讨论方案"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("6月20日之前处理完报销"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("明早9点前到公司"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("11月11日晚上8点抢购"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("9月1日上午开学典礼"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("2月9日下午3点和客户见面"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("4月12日之前提交毕业论文"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("7月23日上午医院体检"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("10月1号国庆节出去玩"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("8月15号之前交房租"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("1月1号元旦快乐"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("下周六晚上朋友生日聚会"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("这周日早上八点公园跑步"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("明天记得买牛奶"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("后天下午去银行办卡"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("明晚七点体育馆看球赛"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("周五下午到会议室开会"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("请明天中午之前把报告发我"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("麻烦帮我带一杯咖啡上来"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("务必周五下班前把方案给我"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("尽快处理这个问题"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("马上联系客户确认一下"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("明天早上九点会议室讨论需求"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("下周三下午两点开发评审"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("别忘了明天带身份证"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("明天要下雨记得带伞"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("下月初交物业费"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("月底结账"), QStringLiteral("todo")));
    // -- 通知/公告类（换教室、调课、停课等） --
    samples.append(qMakePair(QStringLiteral("教室调整通知"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("明天换教室到实验楼302"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("本周三停课一次"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("请各位今天下午交实验报告"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("注意系统设计课换教室了"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("明天上午的课调到周五下午"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("下周一期末考试别忘了"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("周五交大作业截止"), QStringLiteral("todo")));
    // -- 含课程节次的 --
    samples.append(qMakePair(QStringLiteral("明天3-4节实验课带实验报告"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("下周三7-8节补课"), QStringLiteral("todo")));
    samples.append(qMakePair(QStringLiteral("双周周五下午实验课"), QStringLiteral("todo")));

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
    samples.append(qMakePair(QStringLiteral("晚上吃什么"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("明天再聊"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("今天好累啊"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("下周有空吗"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("下午茶时间到了"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("你几点下班"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("周末有什么安排"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("昨天看到你了"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("今天中午吃什么"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("明天放假吗"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("下午是不是要下雨"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("晚上一起吃饭吗就问问"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("这个视频挺搞笑的"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("有时间帮我看看这个问题"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("那我们现在开始吧"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("我晚点回复你"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("你先忙吧不用管我"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("不急慢慢来"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("周末去哪玩了"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("刚刚开的会有什么问题吗"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("今天晚上挺凉的"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("好的那就先这样"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("这个月挺忙的"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("你长胖了"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("今天早上堵车了吗"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("最近过得怎么样"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("明天去不去健身"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("有人一起去食堂吗"), QStringLiteral("normal")));
    // -- 平衡数据：更多日常闲聊场景 --
    samples.append(qMakePair(QStringLiteral("今天是我生日大家来吃蛋糕"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("晚上有约吗"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("明天有课吗"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("昨天睡得真晚"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("周末又要加班了"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("今天心情不太好"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("新买的手机不错"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("困死了想睡觉"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("你们在聊什么"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("太搞笑了哈哈哈"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("作业写完了吗"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("这个老师讲得好无聊"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("放学一起走"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("食堂人太多了"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("今天好冷啊"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("明天周末终于可以休息了"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("这个剧真的好看推荐给你"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("今晚吃什么好纠结"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("帮我看看这道题怎么做"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("明天见不到你我会想你的"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("今天下午没什么事好无聊"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("我刚刚在路上看到一个猫"), QStringLiteral("normal")));
    samples.append(qMakePair(QStringLiteral("老师今天点名了你没来"), QStringLiteral("normal")));

    return samples;
}

void MessageClassifier::train(const QVector<QPair<QString, QString>> &samples)
{
    for (const auto &sample : samples) {
        trainSingle(sample.first, sample.second);
    }
    trained_ = true;
}

void MessageClassifier::trainSingle(const QString &text, const QString &label)
{
    const QStringList features = extractFeatures(text);

    classDocCount_[label] += 1;

    for (const QString &f : features) {
        vocabulary_.insert(f);
        featureCount_[qMakePair(label, f)] += 1;
        classFeatureTotal_[label] += 1;
    }
}

// ============================================================================
// 特征提取：jieba 分词 + 日期/时间伪特征注入
//
// 两步：
//   1. jieba.Cut() → 词级语义特征
//   2. 正则检测日期/时间模式 → 注入伪特征
//
// 伪特征分两级：
//   __EXACT_DATE__   : 具体数字日期、指定星期 → 强信号 → classify() 中做补偿
//   __RELATIVE_TIME__ : 相对时间(今天/明天)、模糊时段(周末/之前) → 弱信号 → 不做补偿
//   __TIME__          : 冒号时间、点钟、时段、课程节次
//
// "2月9日下午3点开会" → ["下午","3","点","开会","__EXACT_DATE__","__TIME__"]
// "今天是我们生日"   → ["今天","是","我们","生日","__RELATIVE_TIME__"]
// ============================================================================

QStringList MessageClassifier::extractFeatures(const QString &text) const
{
    QStringList features;

    const QString cleaned = text.simplified();

    // ----------------------------------------------------------------
    // 第1步：jieba 分词
    // ----------------------------------------------------------------
    if (jieba_) {
        std::vector<std::string> words;
        jieba_->Cut(cleaned.toStdString(), words, true);
        for (const auto &word : words) {
            features.append(QString::fromStdString(word));
        }
    }

    // ----------------------------------------------------------------
    // 第2步：日期/时间模式检测 → 注入伪特征
    // ----------------------------------------------------------------
    bool hasExactDate = false;
    bool hasRelativeTime = false;
    bool hasTime = false;

    // -- __EXACT_DATE__ : 具体数字日期 / 指定星期 --
    static const QRegularExpression fullDate(QStringLiteral(R"(\d{4}年\d{1,2}月\d{1,2}[日号])"));
    if (fullDate.match(cleaned).hasMatch()) hasExactDate = true;

    static const QRegularExpression shortDate(QStringLiteral(R"(\d{1,2}月\d{1,2}[日号])"));
    if (shortDate.match(cleaned).hasMatch()) hasExactDate = true;

    // 指定星期（含方向）：下周一、上周五、周三、星期一
    static const QRegularExpression weekday(QStringLiteral(R"([下上本]?周[一二三四五六日天]|星期[一二三四五六日天])"));
    if (weekday.match(cleaned).hasMatch()) hasExactDate = true;

    // 指定月份方向：下个月、上个月（相对但指向具体月份）
    static const QRegularExpression relativeMonth(QStringLiteral(R"([上下]个月)"));
    if (relativeMonth.match(cleaned).hasMatch()) hasExactDate = true;

    // -- __RELATIVE_TIME__ : 模糊相对时间 --
    static const QRegularExpression relativeDay(QStringLiteral(R"([今明后]天|周末)"));
    if (relativeDay.match(cleaned).hasMatch()) hasRelativeTime = true;

    static const QRegularExpression relativeNight(QStringLiteral(R"([今明]晚|[今明]早)"));
    if (relativeNight.match(cleaned).hasMatch()) hasRelativeTime = true;

    static const QRegularExpression deadline(QStringLiteral(R"((之前|之内|以内))"));
    if (deadline.match(cleaned).hasMatch()) hasRelativeTime = true;

    // -- __TIME__ : 时间表达 --
    // 冒号时间：8:30、14:00、8：30
    static const QRegularExpression colonTime(QStringLiteral(R"(\d{1,2}[:：]\d{2})"));
    if (colonTime.match(cleaned).hasMatch()) hasTime = true;

    // 点钟时间：8点、8点30分
    static const QRegularExpression pointTime(QStringLiteral(R"(\d{1,2}点)"));
    if (pointTime.match(cleaned).hasMatch()) hasTime = true;

    // 时段
    static const QRegularExpression period(QStringLiteral(R"([早中下晚]午|早上|晚上|傍晚|凌晨)"));
    if (period.match(cleaned).hasMatch()) hasTime = true;

    // 课程节次：7-8节、3-4节、第3-4节
    static const QRegularExpression classPeriod(QStringLiteral(R"(\d{1,2}\s*-\s*\d{1,2}\s*节)"));
    if (classPeriod.match(cleaned).hasMatch()) hasTime = true;

    // 单双周
    static const QRegularExpression weekType(QStringLiteral(R"([单双]周)"));
    if (weekType.match(cleaned).hasMatch()) hasTime = true;

    if (hasExactDate) features.append(QStringLiteral("__EXACT_DATE__"));
    if (hasRelativeTime) features.append(QStringLiteral("__RELATIVE_TIME__"));
    if (hasTime) features.append(QStringLiteral("__TIME__"));

    return features;
}

// ============================================================================
// 分类（核心数学）
//
// P(label|text) ∝ P(label) × Π P(token|label)
//
// 用 log 空间计算避免浮点下溢：
//   logP(label|text) = logP(label) + Σ logP(token|label)
//
// P(token|label) = (count(token,label) + α) / (total_features(label) + α × vocab_size)
// α = 1 (Laplace 平滑)
//
// 最后 softmax 归一化到 [0,1]
// ============================================================================

double MessageClassifier::classLogProb(const QString &label, const QStringList &features) const
{
    const int totalDocs = classDocCount_.value(QStringLiteral("todo"), 0)
                          + classDocCount_.value(QStringLiteral("normal"), 0);
    if (totalDocs <= 0) {
        return -1e9;
    }

    const int docCount = classDocCount_.value(label, 0);
    const double logPrior = qLn(static_cast<double>(docCount) / totalDocs);

    const int featuresInClass = classFeatureTotal_.value(label, 0);
    const int vocabSize = vocabulary_.size();
    if (vocabSize <= 0 || featuresInClass <= 0) {
        return logPrior;
    }

    double logLikelihood = 0.0;
    for (const QString &f : features) {
        const int count = featureCount_.value(qMakePair(label, f), 0);
        const double prob = (count + kSmoothing) / (featuresInClass + kSmoothing * vocabSize);
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

    const QStringList features = extractFeatures(text);
    if (features.isEmpty()) {
        return result;
    }

    double logTodo = classLogProb(QStringLiteral("todo"), features);
    const double logNormal = classLogProb(QStringLiteral("normal"), features);

    // 未见词补偿：补偿长消息中大量未见中性词的累积稀释效应。
    //
    // 每个未见 token 因 Laplace 平滑在两类的条件概率比为：
    //   P(unseen|todo)   Fn + αV
    //   ────────────── = ──────    (Ft/Fn = 两类特征总数，V = 词表大小)
    //   P(unseen|normal)  Ft + αV
    //
    // normal 类文档更短 → Fn < Ft → 比值 < 1 → 每个未见 token 微偏 normal。
    // 长消息中数十个未见 token 会累积出巨大偏差，淹没日期信号。
    //
    // 补偿分两级：
    //   __EXACT_DATE__   : 具体数字日期、指定星期 → 全量补偿
    //   __RELATIVE_TIME__ : 今天/明天/周末/之前   → 30% 弱补偿
    //      避免"今天是我生日"被过度加持，但"今天中午之前交报告"仍能识别
    const int Ft = classFeatureTotal_.value(QStringLiteral("todo"), 0);
    const int Fn = classFeatureTotal_.value(QStringLiteral("normal"), 0);
    const int V  = vocabulary_.size();

    if (Ft > 0 && Fn > 0 && V > 0) {
        int unseenCount = 0;
        for (const QString &f : features) {
            if (f.startsWith(QStringLiteral("__"))) continue;
            if (!vocabulary_.contains(f)) {
                ++unseenCount;
            }
        }

        if (unseenCount > 0) {
            const double perUnseenBias = qLn(
                static_cast<double>(Fn + kSmoothing * V)
                / (Ft + kSmoothing * V));

            if (features.contains(QStringLiteral("__EXACT_DATE__"))) {
                logTodo -= perUnseenBias * unseenCount;
            } else if (features.contains(QStringLiteral("__RELATIVE_TIME__"))) {
                logTodo -= perUnseenBias * unseenCount * kRelativeTimeFactor;
            }
        }
    }

    const double maxLog = qMax(logTodo, logNormal);
    const double expTodo = qExp(logTodo - maxLog);
    const double expNormal = qExp(logNormal - maxLog);
    const double sum = expTodo + expNormal;

    const double todoProb = sum > 0.0 ? expTodo / sum : 0.5;

    result.isTodo = todoProb >= kTodoThreshold;
    result.confidence = todoProb;

    return result;
}

// ============================================================================
// 模型持久化（JSON）
// ============================================================================

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
