#pragma once

#include <vector>
#include <deque>
#include <unordered_map>

// 出牌决策的特征向量
struct PlayFeatures
{
    int  handSize = 0;
    bool isNewRound = true;
    int  lastPlayType = -1;   // HandType, -1 = 无
    int  lastPlayRank = -1;
    bool hasBomb = false;
    bool hasRocket = false;
    int  level = 1;
};

// 出牌决策的动作
struct PlayAction
{
    int  handType = -1;       // HandType, -2 = pass
    int  mainRank = -1;
    int  cardCount = 0;
    bool passed = false;
};

class AIMemory
{
public:
    // 记录玩家的一次出牌/不出决策
    void recordPlay(const PlayFeatures& feat, const PlayAction& act);

    // 记录玩家激活了某个技能
    void recordSkillUse(int skillId);

    // 查询: 返回与当前状态最相似的K个历史动作及其权重
    struct WeightedAction
    {
        int   handType  = -1;
        int   mainRank  = -1;
        int   cardCount = 0;
        bool  passed    = false;
        float weight    = 0.f;
    };
    std::vector<WeightedAction> queryPlays(const PlayFeatures& feat, int topK = 5) const;

    // 查询: 玩家使用某个技能的概率 (0.0 ~ 1.0)
    float querySkillProb(int skillId) const;

    // 清空所有记忆 (玩家死亡/退出时)
    void clear();

    int playRecordCount()  const { return (int)m_playRecords.size(); }
    int skillRecordCount() const { return (int)m_skillUses.size(); }

private:
    static constexpr size_t MAX_PLAY_RECORDS = 2000;

    // 出牌记忆: (特征, 动作) 对
    std::deque<std::pair<PlayFeatures, PlayAction>> m_playRecords;

    // 技能记忆: skillId -> 使用次数
    std::unordered_map<int, int> m_skillUses;

    float computeDistance(const PlayFeatures& a, const PlayFeatures& b) const;
};
