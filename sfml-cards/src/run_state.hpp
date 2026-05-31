#pragma once

#include "skill.hpp"
#include "character.hpp"
#include <vector>
#include <array>
#include <random>

class RunState
{
public:
    RunState();

    void startNewRun(int characterId);
    void advanceToNextLevel();

    int  currentLevel()    const { return m_level; }
    int  currentCharId()   const { return m_charId; }
    int  extraCards()      const;

    // 已获得的所有技能 (永久技能池)
    const std::vector<int>& acquiredSkills() const { return m_acquired; }
    bool hasSkill(int id) const;
    void addSkill(int id);

    // 装备的技能 (3槽, -1=空)
    const std::array<int, MAX_SKILL_SLOTS>& equippedSkills() const { return m_equipped; }
    void equipSkill(int slotIdx, int skillId);
    void unequipSlot(int slotIdx);
    int  equippedCount() const;
    int  equippedSlotOf(int skillId) const;
    void swapSlots(int a, int b);

    // 镜像给下一关敌人的技能 (上一关战斗时的快照)
    std::array<int, MAX_SKILL_SLOTS> mirroredSkills() const { return m_mirroredSkills; }

    // 随机获取3个可选技能 (用于奖励界面)
    std::vector<int> rollRewardSkills();

    // 癞子点数 (谋略家被动技, -1=无)
    int  wildcardRank() const { return m_wildcardRank; }
    void setWildcardRank(int r) { m_wildcardRank = r; }

private:
    int  m_level = 1;
    int  m_charId = 0;

    std::vector<int>          m_acquired;   // 已获得技能id列表
    std::array<int, MAX_SKILL_SLOTS> m_equipped = {-1, -1, -1};
    std::array<int, MAX_SKILL_SLOTS> m_mirroredSkills = {-1, -1, -1};

    int m_wildcardRank = -1;  // 癞子点数, -1=无 (谋略家被动)

    std::mt19937 m_rng;
};
