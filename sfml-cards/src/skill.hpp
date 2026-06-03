#pragma once

#include <string>
#include <array>
#include <cstdint>

enum class SkillType : uint8_t { BUFF, TRIGGER, PASSIVE };

struct SkillDef
{
    int id;
    std::wstring name;
    std::wstring desc;
    SkillType type = SkillType::BUFF;
};

constexpr int SKILL_COUNT = 3;
constexpr int MAX_SKILL_SLOTS = 3;

const std::array<SkillDef, SKILL_COUNT>& getAllSkills();

// 当前回合技能效果
struct SkillBuffs
{
    int  wildcardRank      = -1;    // 癞子点数 (-1=无), 角色的被动技
    bool straightExtended  = false; // S02 顺子大师: 顺子最低长度4
    bool jokerWill         = false; // S03 王牌意志: 小王免疫炸弹

    void clear() {
        int saved = wildcardRank;
        *this = SkillBuffs{};
        wildcardRank = saved;
    }
};
