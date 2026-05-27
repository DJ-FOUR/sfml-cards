#include "character.hpp"

const std::array<CharacterDef, CHAR_COUNT>& getAllCharacters()
{
    static const std::array<CharacterDef, CHAR_COUNT> chars = {{
        { 0, L"争锋者", L"强袭", L"手牌上限+1（初始16张）", 1 },
        { 1, L"谋略家", L"谋定", L"洞察牌局，手牌上限不变",   0 },
        { 2, L"掌控者", L"储备", L"手牌上限+2（初始17张）", 2 },
    }};
    return chars;
}
