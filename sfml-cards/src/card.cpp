#include "card.hpp"
#include <algorithm>
#include <random>

namespace {

struct CardInfo { Suit suit; Rank rank; };

// 图片按斗地主点数顺序排列, 每个点数4张对应4种花色: 黑桃,红桃,梅花,方片
// 可直接在文件资源管理器中查看图片验证和调整花色顺序
constexpr CardInfo CARD_MAP[DECK_SIZE] = {
    // 0-3:   3
    {Suit::Spade, Rank::Three}, {Suit::Heart, Rank::Three},
    {Suit::Club, Rank::Three},  {Suit::Diamond, Rank::Three},
    // 4-7:   4
    {Suit::Spade, Rank::Four},  {Suit::Heart, Rank::Four},
    {Suit::Club, Rank::Four},   {Suit::Diamond, Rank::Four},
    // 8-11:  5
    {Suit::Spade, Rank::Five},  {Suit::Heart, Rank::Five},
    {Suit::Club, Rank::Five},   {Suit::Diamond, Rank::Five},
    // 12-15: 6
    {Suit::Spade, Rank::Six},   {Suit::Heart, Rank::Six},
    {Suit::Club, Rank::Six},    {Suit::Diamond, Rank::Six},
    // 16-19: 7
    {Suit::Spade, Rank::Seven}, {Suit::Heart, Rank::Seven},
    {Suit::Club, Rank::Seven},  {Suit::Diamond, Rank::Seven},
    // 20-23: 8
    {Suit::Spade, Rank::Eight}, {Suit::Heart, Rank::Eight},
    {Suit::Club, Rank::Eight},  {Suit::Diamond, Rank::Eight},
    // 24-27: 9
    {Suit::Spade, Rank::Nine},  {Suit::Heart, Rank::Nine},
    {Suit::Club, Rank::Nine},   {Suit::Diamond, Rank::Nine},
    // 28-31: 10
    {Suit::Spade, Rank::Ten},   {Suit::Heart, Rank::Ten},
    {Suit::Club, Rank::Ten},    {Suit::Diamond, Rank::Ten},
    // 32-35: J
    {Suit::Spade, Rank::Jack},  {Suit::Heart, Rank::Jack},
    {Suit::Club, Rank::Jack},   {Suit::Diamond, Rank::Jack},
    // 36-39: Q
    {Suit::Spade, Rank::Queen}, {Suit::Heart, Rank::Queen},
    {Suit::Club, Rank::Queen},  {Suit::Diamond, Rank::Queen},
    // 40-43: K
    {Suit::Spade, Rank::King},  {Suit::Heart, Rank::King},
    {Suit::Club, Rank::King},   {Suit::Diamond, Rank::King},
    // 44-47: A
    {Suit::Spade, Rank::Ace},   {Suit::Heart, Rank::Ace},
    {Suit::Club, Rank::Ace},    {Suit::Diamond, Rank::Ace},
    // 48-51: 2
    {Suit::Spade, Rank::Two},   {Suit::Heart, Rank::Two},
    {Suit::Club, Rank::Two},    {Suit::Diamond, Rank::Two},
    // 52-53: 小王, 大王
    {Suit::None, Rank::SmallJoker},
    {Suit::None, Rank::BigJoker},
};

} // namespace

Card cardFromImageIndex(int idx)
{
    auto& info = CARD_MAP[idx];
    return {info.suit, info.rank, idx};
}

std::vector<Card> createDeck()
{
    std::vector<Card> deck;
    deck.reserve(DECK_SIZE);
    for (int i = 0; i < DECK_SIZE; ++i)
        deck.push_back(cardFromImageIndex(i));

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(deck.begin(), deck.end(), g);
    return deck;
}
