#include "skill.hpp"

const std::array<SkillDef, SKILL_COUNT>& getAllSkills()
{
    static const std::array<SkillDef, SKILL_COUNT> skills = {{
        { 0, L"连击之势", L"敌人连续2回合不出牌时触发；选择1张手牌打出",
          SkillType::PASSIVE },
        { 1, L"顺子大师", L"顺子最低长度-1（4张即可出顺子）",
          SkillType::PASSIVE },
        { 2, L"王牌意志", L"你的小王不能被大王以外的牌压制（炸弹也不行）",
          SkillType::PASSIVE },
    }};
    return skills;
}
