#include "ai_memory.hpp"
#include <algorithm>
#include <cstdlib>

// ----- 距离函数 -----

float AIMemory::computeDistance(const PlayFeatures& a, const PlayFeatures& b) const
{
    float d = 0.f;

    // 手牌数 → 粗粒度分桶比较
    auto bucket = [](int sz) {
        if (sz <= 2)  return 0;
        if (sz <= 5)  return 1;
        if (sz <= 9)  return 2;
        if (sz <= 14) return 3;
        return 4;
    };
    if (bucket(a.handSize) != bucket(b.handSize))
        d += 6.f;

    // 新一轮 vs 跟牌: 最重要
    if (a.isNewRound != b.isNewRound)
        d += 12.f;

    // 跟牌时匹配对手牌型
    if (!a.isNewRound && !b.isNewRound) {
        if (a.lastPlayType != b.lastPlayType)
            d += 8.f;
        else
            d += std::abs(a.lastPlayRank - b.lastPlayRank) * 0.8f;
    }

    // 是否有炸弹/火箭
    if (a.hasBomb   != b.hasBomb)   d += 4.f;
    if (a.hasRocket != b.hasRocket) d += 3.f;

    // 关卡数影响 (后期更激进)
    d += std::abs(a.level - b.level) * 1.5f;

    return d;
}

// ----- 录制 -----

void AIMemory::recordPlay(const PlayFeatures& feat, const PlayAction& act)
{
    if (m_playRecords.size() >= MAX_PLAY_RECORDS)
        m_playRecords.pop_front();
    m_playRecords.emplace_back(feat, act);
}

void AIMemory::recordSkillUse(int skillId)
{
    m_skillUses[skillId]++;
}

// ----- 查询: 出牌 -----

std::vector<AIMemory::WeightedAction> AIMemory::queryPlays(const PlayFeatures& feat, int topK) const
{
    if (m_playRecords.empty())
        return {};

    // 计算所有记录的距离
    std::vector<std::pair<float, size_t>> ranked;
    ranked.reserve(m_playRecords.size());
    for (size_t i = 0; i < m_playRecords.size(); ++i) {
        float dist = computeDistance(feat, m_playRecords[i].first);
        ranked.emplace_back(dist, i);
    }
    std::sort(ranked.begin(), ranked.end(),
              [](auto& a, auto& b) { return a.first < b.first; });

    // 取 TopK, 按距离反比加权
    int k = std::min(topK, (int)ranked.size());
    float maxWeight = 0.f;
    std::vector<WeightedAction> result;
    result.reserve(k);

    for (int i = 0; i < k; ++i) {
        auto& rec = m_playRecords[ranked[i].second];
        float w = 0.f;
        if (ranked[i].first < 0.001f)
            w = 10.f;
        else
            w = 1.f / (1.f + ranked[i].first);

        // 合并相同动作的权重
        bool merged = false;
        for (auto& r : result) {
            if (r.handType  == rec.second.handType &&
                r.mainRank  == rec.second.mainRank &&
                r.cardCount == rec.second.cardCount &&
                r.passed    == rec.second.passed)
            {
                r.weight += w;
                merged = true;
                break;
            }
        }
        if (!merged) {
            result.push_back({rec.second.handType, rec.second.mainRank,
                             rec.second.cardCount, rec.second.passed, w});
        }
        if (w > maxWeight) maxWeight = w;
    }

    // 归一化权重
    if (maxWeight > 0.f) {
        for (auto& r : result)
            r.weight /= maxWeight;
    }

    // 按权重降序
    std::sort(result.begin(), result.end(),
              [](auto& a, auto& b) { return a.weight > b.weight; });

    return result;
}

// ----- 查询: 技能 -----

float AIMemory::querySkillProb(int skillId) const
{
    // 无任何技能数据 → 玩家从不使用技能 → AI 也不用
    if (m_skillUses.empty())
        return 0.0f;

    auto it = m_skillUses.find(skillId);
    int uses = (it != m_skillUses.end()) ? it->second : 0;
    if (uses == 0)
        return 0.0f;  // 玩家没用过这个技能 → AI 也不用

    // 找最大使用次数作为基准
    int maxUses = 0;
    for (auto& [sid, cnt] : m_skillUses)
        if (cnt > maxUses) maxUses = cnt;
    if (maxUses == 0) return 0.0f;

    // 概率 = 使用次数 / 最大使用次数
    float prob = (float)uses / (float)maxUses;
    if (prob > 1.0f) prob = 1.0f;
    return prob;
}

// ----- 清空 -----

void AIMemory::clear()
{
    m_playRecords.clear();
    m_skillUses.clear();
}
