#pragma once

#include <string>
#include <array>

struct CharacterDef
{
    int id;
    std::wstring name;
    std::wstring passiveName;
    std::wstring passiveDesc;
    int extraCards;
};

constexpr int CHAR_COUNT = 3;

const std::array<CharacterDef, CHAR_COUNT>& getAllCharacters();
