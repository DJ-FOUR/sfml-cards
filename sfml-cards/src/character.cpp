#include "character.hpp"

const std::array<CharacterDef, CHAR_COUNT>& getAllCharacters()
{
    static const std::array<CharacterDef, CHAR_COUNT> chars = {{
        { 0, L"争锋者", L"强袭", L"每回合首次出牌额外+1能量", 0, 1, false },
        { 1, L"谋略家", L"谋定", L"每关首次切换技能无冷却",   0, 0, true  },
        { 2, L"掌控者", L"储备", L"手牌上限+2（初始22张）",   2, 0, false },
    }};
    return chars;
}
