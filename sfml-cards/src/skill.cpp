#include "skill.hpp"

const std::array<SkillDef, SKILL_COUNT>& getAllSkills()
{
    static const std::array<SkillDef, SKILL_COUNT> skills = {{
        { 0, L"炸弹强化", L"本回合你的炸弹视为更大炸弹，可压过同级炸弹" },
        { 1, L"火箭冲刺", L"打出火箭时，额外抽3张牌" },
        { 2, L"顺子延长", L"本回合顺子最低长度-1（4张即可出顺子）" },
        { 3, L"连对增幅", L"本回合连对最低组数-1（2组即可出连对）" },
        { 4, L"三带一强化",L"本回合三带一/三带二可额外多带一张单牌" },
        { 5, L"飞机连射", L"本回合飞机最低组数-1（1组即可出飞机）" },
        { 6, L"炸弹馈赠", L"打出炸弹后额外抽1张牌" },
        { 7, L"连环炸弹", L"本回合出牌后若手中有炸弹，自动打出（限1次）" },
    }};
    return skills;
}
