#pragma once

// ============================================================================
// Feature 1：个人事务管家 — 本地信息提取器
//
// 从用户标记的消息原文中提取 action（事项）、time（时间）、place（地点）。
// 纯本地运算，零延迟零成本，不依赖 LLM。
//
// 方法：
//   1. 正则提取时间（复用 MessageClassifier 的日期/时间模式）
//   2. 字典+正则提取地点
//   3. jieba 分词 → 扣除已匹配的时间/地点 token → 剩余作为 action
// ============================================================================

#include <QString>
#include <QStringList>

namespace cppjieba { class Jieba; }

struct TaskExtractResult {
    QString time;
    QString place;
};

class TaskExtractor
{
public:
    explicit TaskExtractor(cppjieba::Jieba *jieba);
    TaskExtractResult extract(const QString &text) const;

private:
    QString extractTime(const QString &text) const;
    QString extractPlace(const QString &text) const;

    cppjieba::Jieba *jieba_ = nullptr;
};