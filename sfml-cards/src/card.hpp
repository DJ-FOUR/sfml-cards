#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class Suit : uint8_t { Spade, Heart, Club, Diamond, None };

enum class Rank : uint8_t {
    Three = 3, Four = 4, Five = 5, Six = 6,
    Seven = 7, Eight = 8, Nine = 9, Ten = 10,
    Jack = 11, Queen = 12, King = 13, Ace = 14,
    Two = 15,
    SmallJoker = 16,
    BigJoker = 17
};

// 斗地主牌力排序: 3(0) < 4(1) < ... < A(11) < 2(12) < 小王(13) < 大王(14)
constexpr int doudizhuOrder(Rank r)
{
    return (r == Rank::Two) ? 12
         : (r == Rank::SmallJoker) ? 13
         : (r == Rank::BigJoker) ? 14
         : static_cast<int>(r) - 3;
}

struct Card
{
    Suit suit; // 花色 (对于大小王为 None)
    Rank rank; // 点数
    int  imageIndex = 0; // 0-53 对应 card{idx}.png

    bool operator<(const Card& other) const
    {
        return doudizhuOrder(rank) < doudizhuOrder(other.rank);
    }
    bool operator==(const Card& other) const
    {
        return rank == other.rank && suit == other.suit;
    }
    bool operator!=(const Card& other) const { return !(*this == other); }
};

constexpr int DECK_SIZE = 54;

Card cardFromImageIndex(int idx);
std::vector<Card> createDeck();
