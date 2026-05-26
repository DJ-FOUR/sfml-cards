#pragma once

#include <string>
#include <array>

struct SkillDef
{
    int id;
    std::wstring name;
    std::wstring desc;
    int energyCost;
    int cooldown;   // 0 = 无冷却
};

constexpr int SKILL_COUNT = 8;
constexpr int MAX_SKILL_SLOTS = 3;
constexpr int MAX_ENERGY = 10;
constexpr int START_ENERGY = 3;

const std::array<SkillDef, SKILL_COUNT>& getAllSkills();

// 当前回合技能效果
struct SkillBuffs
{
    bool bombBoosted       = false;  // S01
    int  bombRankBonus     = 3;
    bool straightExtended  = false;  // S03
    bool pairsExtended     = false;  // S04
    bool tripleExtraKicker = false;  // S05
    bool airplaneExtended  = false;  // S06

    void clear() { *this = SkillBuffs{}; }
};
