#include "character.hpp"

const std::array<CharacterDef, CHAR_COUNT>& getAllCharacters()
{
    static const std::array<CharacterDef, CHAR_COUNT> chars = {{
        { 0, L"炸弹收藏家", L"收藏", L"场上每炸一次获得1个印记；3印记自动生成1个炸弹", 1 },
        { 1, L"谋略家", L"谋定", L"可选择指定点数为万能牌，每局游戏开始时固定抽取两张及以上", 1 },
        { 2, L"掌控者", L"擢升", L"每2回合可弃至多3张牌，获得牌型一致的更高点数牌组", 1 },
    }};
    return chars;
}
