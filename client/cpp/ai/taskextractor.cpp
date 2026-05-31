// ============================================================================
// Feature 1：个人事务管家 — 本地信息提取器实现
// ============================================================================

#include "ai/taskextractor.h"
#include "cppjieba/Jieba.hpp"

#include <QRegularExpression>

TaskExtractor::TaskExtractor(cppjieba::Jieba *jieba)
    : jieba_(jieba)
{
}

TaskExtractResult TaskExtractor::extract(const QString &text) const
{
    TaskExtractResult result;
    result.time  = extractTime(text);
    result.place = extractPlace(text);
    return result;
}

// ============================================================================
// 时间提取：正则匹配数字日期、星期、相对时间、点钟、冒号时间
// ============================================================================

QString TaskExtractor::extractTime(const QString &text) const
{
    const QString cleaned = text.simplified();

    // 完整数字日期：2026年6月9日、2026年6月9号
    static const QRegularExpression fullDate(QStringLiteral(R"(\d{4}年\d{1,2}月\d{1,2}[日号])"));
    QRegularExpressionMatch m = fullDate.match(cleaned);
    if (m.hasMatch()) return m.captured(0);

    // 短数字日期：6月9日、12月31号（在之前/之内/以内之前）
    static const QRegularExpression shortDate(QStringLiteral(
        R"_(\d{1,2}月\d{1,2}[日号][\s]*(之前|之内|以内|前|截止|之前))_"));
    m = shortDate.match(cleaned);
    if (m.hasMatch()) return m.captured(0);

    // 短数字日期不带期限：6月9日
    static const QRegularExpression shortDateOnly(QStringLiteral(R"(\d{1,2}月\d{1,2}[日号])"));
    m = shortDateOnly.match(cleaned);
    if (m.hasMatch()) return m.captured(0);

    // 星期 + 方向 + 时段：下周一上午、本周三中午、下周三下午两点
    static const QRegularExpression weekdayTime(QStringLiteral(
        R"_(([下上本]?周[一二三四五六日天]|星期[一二三四五六日天])\s*([早中下晚]午|早上|晚上|早上)?\s*(\d{1,2}点)?)_"));
    m = weekdayTime.match(cleaned);
    if (m.hasMatch()) return m.captured(0).trimmed();

    // 相对日 + 时间：明天下午3点、今晚8点、后天上午
    static const QRegularExpression relativeDayTime(QStringLiteral(
        R"_(([今明后]天|[今明][早晚])\s*([早中下晚]午|早上|晚上)?\s*(\d{1,2}点)?)_"));
    m = relativeDayTime.match(cleaned);
    if (m.hasMatch()) return m.captured(0).trimmed();

    // 独立星期
    static const QRegularExpression standaloneWeekday(QStringLiteral(
        R"([下上本]?周[一二三四五六日天]|星期[一二三四五六日天])"));
    m = standaloneWeekday.match(cleaned);
    if (m.hasMatch()) return m.captured(0);

    // 相对日
    static const QRegularExpression standaloneRelDay(QStringLiteral(
        R"([今明后]天|[今明][早晚]|周末)"));
    m = standaloneRelDay.match(cleaned);
    if (m.hasMatch()) return m.captured(0);

    // 冒号时间：8:30、14:00
    static const QRegularExpression colonTime(QStringLiteral(R"(\d{1,2}[:：]\d{2})"));
    m = colonTime.match(cleaned);
    if (m.hasMatch()) return m.captured(0);

    // 点钟 + 场合：下午3点、上午9点、8点
    static const QRegularExpression pointTime(QStringLiteral(
        R"_(([早中下晚]午|早上|晚上)\s*)?(\d{1,2}点(\d{1,2}分)?([早中下晚]午|早上|晚上)?)_"));
    m = pointTime.match(cleaned);
    if (m.hasMatch()) return m.captured(0).trimmed();

    // 期限表达：之前、之内、以内 + 前面词作为上下文
    static const QRegularExpression deadline(QStringLiteral(R"(之前|之内|以内)"));
    m = deadline.match(cleaned);
    if (m.hasMatch()) return m.captured(0);

    // 课程节次：7-8节、3-4节
    static const QRegularExpression classP(QStringLiteral(R"(\d{1,2}\s*-\s*\d{1,2}\s*节)"));
    m = classP.match(cleaned);
    if (m.hasMatch()) return m.captured(0);

    // 单双周 + 星期
    static const QRegularExpression weekCycle(QStringLiteral(R"([单双]周)"));
    m = weekCycle.match(cleaned);
    if (m.hasMatch()) return m.captured(0);

    return {};
}

// ============================================================================
// 地点提取：字典 + 正则
// ============================================================================

QString TaskExtractor::extractPlace(const QString &text) const
{
    // 带编号的场所：302会议室、C101教室、2号楼、3号楼301室
    static const QRegularExpression numberedPlace(QStringLiteral(
        R"_([A-Za-z]*\d{3,}[号]?(会议室|教室|室|楼|房间)?)_"));
    QRegularExpressionMatch m = numberedPlace.match(text);
    if (m.hasMatch()) return m.captured(0);

    // 地点词典
    static const QRegularExpression placeDict(QStringLiteral(
        R"(会议室|教室|实验楼|实验室|办公室|办公楼|食堂|餐厅|图书馆|体育馆|操场|)"
        R"(地铁站|车站|火车站|机场|医院|超市|商场|电影院|剧场|公园|)"
        R"(公司|学校|宿舍|家里|银行|邮局|酒店|宾馆|咖啡厅|书店|)"
        R"(课堂派|讲台|学办|教务处|报告厅|大会堂)"));
    m = placeDict.match(text);
    if (m.hasMatch()) return m.captured(0);

    return {};
}