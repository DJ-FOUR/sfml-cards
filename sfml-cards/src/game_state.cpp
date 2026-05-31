#include "game_state.hpp"
#include <algorithm>

// 斗地主排序常量
constexpr int R3 = 0, R4 = 1, R5 = 2, R6 = 3, R7 = 4, R8 = 5, R9 = 6,
              R10 = 7, RJ = 8, RQ = 9, RK = 10, RA = 11, R2 = 12,
              RS = 13, RB = 14;
constexpr int DZ_RANKS = 15;
constexpr int STANDARD_DEAL = 15;

// ----- helpers -----

static void sortByDZ(std::vector<Card>& cards, int wildcardRank = -1)
{
    std::sort(cards.begin(), cards.end(), [wildcardRank](const Card& a, const Card& b) {
        if (wildcardRank >= 0) {
            bool aWild = (doudizhuOrder(a.rank) == wildcardRank);
            bool bWild = (doudizhuOrder(b.rank) == wildcardRank);
            if (aWild != bWild) return aWild;
        }
        return doudizhuOrder(a.rank) > doudizhuOrder(b.rank);
    });
}

static int findCardOfRank(const std::vector<Card>& hand, int dzRank,
                          const std::vector<bool>& used)
{
    for (size_t i = 0; i < hand.size(); ++i) {
        if (i < used.size() && used[i]) continue;
        if (doudizhuOrder(hand[i].rank) == dzRank) return (int)i;
    }
    return -1;
}

// ----- classifyHand (支持技能buff + 癞子) -----

// 检查能否用 freq + wildcards 组成指定长度的顺子(每rank≥1张)
static bool canFormStraight(int startRank, int length, const std::array<int,15>& freq, int wilds)
{
    int need = 0;
    for (int r = startRank; r < startRank + length; ++r) {
        if (r > RA) return false;
        if (freq[r] == 0) need++;
        else if (freq[r] > 1) return false;
    }
    return need <= wilds;
}

// 检查能否用 freq + wildcards 组成指定长度的连对(每rank≥2张)
static bool canFormPairs(int startRank, int length, const std::array<int,15>& freq, int wilds)
{
    int need = 0;
    for (int r = startRank; r < startRank + length; ++r) {
        if (r > RA) return false;
        if (freq[r] > 2) return false;
        need += std::max(0, 2 - freq[r]);
    }
    return need <= wilds;
}

// 检查能否用 freq + wildcards 组成指定长度的飞机(每rank≥3张)
static bool canFormAirplane(int startRank, int length, const std::array<int,15>& freq, int wilds)
{
    int need = 0;
    for (int r = startRank; r < startRank + length; ++r) {
        if (r > R2) return false;
        if (freq[r] > 3) { need += freq[r] - 3; }
        need += std::max(0, 3 - freq[r]);
    }
    return need <= wilds;
}

static void setHasBombRocket(PlayFeatures& feat, const std::vector<Card>& hand)
{
    std::array<int, 15> freq{};
    for (auto& c : hand)
        freq[doudizhuOrder(c.rank)]++;
    feat.hasBomb = false;
    for (int r = 0; r < 13; ++r)
        if (freq[r] >= 4) { feat.hasBomb = true; break; }
    feat.hasRocket = (freq[13] >= 1 && freq[14] >= 1);
}

std::optional<HandPattern> GameState::classifyHand(const std::vector<Card>& cards,
                                                     const SkillBuffs* buffs)
{
    int n = (int)cards.size();
    if (n == 0) return std::nullopt;

    // 分离癞子牌
    int wildRank = (buffs && buffs->wildcardRank >= 0) ? buffs->wildcardRank : -1;
    int wildCount = 0;
    std::array<int, DZ_RANKS> freq{};
    for (auto& c : cards) {
        int r = doudizhuOrder(c.rank);
        if (wildRank >= 0 && r == wildRank)
            wildCount++;
        else
            freq[r]++;
    }
    // 无癞子时走快速路径（原逻辑）
    if (wildCount == 0) {
        return classifyHandNoWild(cards, buffs);
    }

    // Rocket (大小王不能由癞子替代)
    if (n == 2 && freq[RS] == 1 && freq[RB] == 1)
        return HandPattern{HandType::Rocket, RB, 0, 0};

    // Single
    if (n == 1) {
        if (wildCount == 1) return HandPattern{HandType::Single, wildRank, 0, 0};
        for (int r = 0; r < DZ_RANKS; ++r)
            if (freq[r]) return HandPattern{HandType::Single, r, 0, 0};
    }

    // Pair: freq[r] + wildCount >= 2
    if (n == 2) {
        if (wildCount == 2) return HandPattern{HandType::Pair, wildRank, 0, 0};
        for (int r = 0; r < DZ_RANKS; ++r) {
            if (freq[r] + wildCount >= 2)
                return HandPattern{HandType::Pair, r, 0, 0};
        }
        return std::nullopt;
    }

    // Bomb: freq[r] + wildCount >= 4
    if (n == 4) {
        if (wildCount == 4) return HandPattern{HandType::Bomb, wildRank, 0, 0};
        for (int r = 0; r < DZ_RANKS; ++r) {
            if (freq[r] + wildCount >= 4)
                return HandPattern{HandType::Bomb, r, 0, 0};
        }
    }

    // Triple / Triple+1 / Triple+2
    bool extra = buffs && buffs->tripleExtraKicker;
    if (n == 3 || n == 4 || n == 5 || (extra && n == 6)) {
        if (n == 3 && wildCount == 3)
            return HandPattern{HandType::Triple, wildRank, 0, 0};
        for (int r = 0; r < DZ_RANKS; ++r) {
            int wUsed = std::max(0, 3 - freq[r]);
            if (freq[r] + wildCount < 3) continue;
            int restW = wildCount - wUsed;
            int rest = n - 3;
            if (rest == 0)
                return HandPattern{HandType::Triple, r, 0, 0};
            // 有剩余癞子时可作 kicker
            if (restW > 0 || extra) {
                if (rest == 1)
                    return HandPattern{HandType::TriplePlusOne, r, 0, 1};
                if (rest == 2) {
                    bool hasPair = false;
                    for (int k = 0; k < DZ_RANKS; ++k)
                        if (k != r && freq[k] + (k == wildRank ? restW : 0) >= 2)
                            { hasPair = true; break; }
                    if (hasPair)
                        return HandPattern{HandType::TriplePlusTwo, r, 0, 2};
                    if (extra)
                        return HandPattern{HandType::TriplePlusOne, r, 0, 2};
                }
                if (rest == 3 && extra)
                    return HandPattern{HandType::TriplePlusOne, r, 0, 3};
            }
            return std::nullopt;
        }
    }

    // Straight (顺子): 使用癞子填补缺口
    int minSLen = (buffs && buffs->straightExtended) ? 4 : 5;
    if (n >= minSLen && n <= (RA - R3 + 1)) {
        for (int start = R3; start + n - 1 <= RA; ++start) {
            if (canFormStraight(start, n, freq, wildCount))
                return HandPattern{HandType::Straight, start + n - 1, n, 0};
        }
    }

    // ConsecutivePairs (连对): 使用癞子填补
    int minPLen = (buffs && buffs->pairsExtended) ? 2 : 3;
    if (n >= minPLen * 2 && n % 2 == 0) {
        int pairLen = n / 2;
        for (int start = R3; start + pairLen - 1 <= RA; ++start) {
            if (canFormPairs(start, pairLen, freq, wildCount))
                return HandPattern{HandType::ConsecutivePairs, start + pairLen - 1, pairLen, 0};
        }
    }

    // Airplane (飞机): 使用癞子填补
    int minALen = (buffs && buffs->airplaneExtended) ? 1 : 2;
    if (n >= minALen * 3) {
        for (int use = n / 3; use >= minALen; --use) {
            int tripleCards = use * 3;
            int remain = n - tripleCards;
            for (int start = R3; start + use - 1 <= R2; ++start) {
                if (!canFormAirplane(start, use, freq, wildCount))
                    continue;

                // 计算使用癞子后剩余的癞子数
                int wUsed = 0;
                for (int t = 0; t < use; ++t)
                    wUsed += std::max(0, 3 - freq[start + t]);
                int restW = wildCount - wUsed;

                if (remain == 0)
                    return HandPattern{HandType::Airplane, start + use - 1, use, 0};
                if (remain == use)  // 带单
                    return HandPattern{HandType::Airplane, start + use - 1, use, use};
                if (remain == use * 2) {  // 带对
                    auto f = freq;
                    for (int t = 0; t < use; ++t)
                        f[start + t] = std::max(0, f[start + t] - 3);
                    f[wildRank] += restW;
                    int pairCnt = 0;
                    bool bad = false;
                    for (int r = 0; r < DZ_RANKS; ++r) {
                        if (f[r] == 2) pairCnt++;
                        else if (f[r] != 0) { bad = true; break; }
                    }
                    if (!bad && pairCnt == use)
                        return HandPattern{HandType::Airplane, start + use - 1, use, use * 2};
                }
            }
        }
    }

    return std::nullopt;
}

// 无癞子快速路径（保持原逻辑）
std::optional<HandPattern> GameState::classifyHandNoWild(const std::vector<Card>& cards,
                                                           const SkillBuffs* buffs)
{
    int n = (int)cards.size();
    if (n == 0) return std::nullopt;

    std::array<int, DZ_RANKS> freq{};
    for (auto& c : cards)
        freq[doudizhuOrder(c.rank)]++;

    // Rocket
    if (n == 2 && freq[RS] == 1 && freq[RB] == 1)
        return HandPattern{HandType::Rocket, RB, 0, 0};

    // Single
    if (n == 1) {
        for (int r = 0; r < DZ_RANKS; ++r)
            if (freq[r]) return HandPattern{HandType::Single, r, 0, 0};
    }

    // Pair
    if (n == 2) {
        for (int r = 0; r < DZ_RANKS; ++r)
            if (freq[r] == 2) return HandPattern{HandType::Pair, r, 0, 0};
        return std::nullopt;
    }

    // Bomb (check before Triple)
    if (n == 4) {
        for (int r = 0; r < DZ_RANKS; ++r)
            if (freq[r] == 4) return HandPattern{HandType::Bomb, r, 0, 0};
    }

    // Triple / Triple+1 / Triple+2
    bool extra = buffs && buffs->tripleExtraKicker;

    if (n == 3 || n == 4 || n == 5 || (extra && n == 6)) {
        for (int r = 0; r < DZ_RANKS; ++r) {
            if (freq[r] != 3) continue;
            int rest = n - 3;
            if (rest == 0)
                return HandPattern{HandType::Triple, r, 0, 0};
            if (rest == 1)
                return HandPattern{HandType::TriplePlusOne, r, 0, 1};
            if (rest == 2) {
                bool hasPair = false;
                for (int k = 0; k < DZ_RANKS; ++k)
                    if (k != r && freq[k] == 2) { hasPair = true; break; }
                if (hasPair)
                    return HandPattern{HandType::TriplePlusTwo, r, 0, 2};
                if (extra)
                    return HandPattern{HandType::TriplePlusOne, r, 0, 2};
            }
            if (rest == 3 && extra)
                return HandPattern{HandType::TriplePlusOne, r, 0, 3};
            return std::nullopt;
        }
    }

    // Straight
    int minSLen = (buffs && buffs->straightExtended) ? 4 : 5;
    if (n >= minSLen) {
        bool ok = true;
        int minR = DZ_RANKS, maxR = -1;
        for (int r = R3; r <= RA; ++r) {
            if (freq[r] > 1) { ok = false; break; }
            if (freq[r] == 1) { minR = std::min(minR, r); maxR = std::max(maxR, r); }
        }
        if (ok && maxR - minR + 1 == n && maxR >= minR
            && freq[R2] == 0 && freq[RS] == 0 && freq[RB] == 0)
            return HandPattern{HandType::Straight, maxR, n, 0};
    }

    // ConsecutivePairs
    int minPLen = (buffs && buffs->pairsExtended) ? 2 : 3;
    if (n >= minPLen * 2 && n % 2 == 0) {
        std::vector<int> pairRanks;
        bool ok = true;
        for (int r = R3; r <= RA; ++r) {
            if (freq[r] == 2) pairRanks.push_back(r);
            else if (freq[r] != 0) { ok = false; break; }
        }
        if (ok && (int)pairRanks.size() >= minPLen && (int)pairRanks.size() == n / 2) {
            bool consec = true;
            for (size_t i = 1; i < pairRanks.size(); ++i)
                if (pairRanks[i] != pairRanks[i-1] + 1) { consec = false; break; }
            if (consec && freq[R2]==0 && freq[RS]==0 && freq[RB]==0)
                return HandPattern{HandType::ConsecutivePairs,
                                   pairRanks.back(), (int)pairRanks.size(), 0};
        }
    }

    // Airplane
    int minALen = (buffs && buffs->airplaneExtended) ? 1 : 2;
    if (n >= minALen * 3) {
        std::vector<int> tripleRanks;
        for (int r = R3; r <= R2; ++r)
            if (freq[r] >= 3) tripleRanks.push_back(r);

        for (size_t i = 0; i < tripleRanks.size(); ) {
            size_t j = i + 1;
            while (j < tripleRanks.size() && tripleRanks[j] == tripleRanks[j-1] + 1)
                ++j;
            int k = (int)(j - i);
            if (k >= minALen) {
                for (int use = k; use >= minALen; --use) {
                    int tripleCards = use * 3;
                    int remain = n - tripleCards;
                    if (remain == 0)
                        return HandPattern{HandType::Airplane,
                                           tripleRanks[i + use - 1], use, 0};
                    if (remain == use)
                        return HandPattern{HandType::Airplane,
                                           tripleRanks[i + use - 1], use, use};
                    if (remain == use * 2) {
                        auto f = freq;
                        for (int t = 0; t < use; ++t)
                            f[tripleRanks[i + t]] -= 3;
                        int pairCnt = 0;
                        bool bad = false;
                        for (int r = 0; r < DZ_RANKS; ++r) {
                            if (f[r] == 2) pairCnt++;
                            else if (f[r] != 0) { bad = true; break; }
                        }
                        if (!bad && pairCnt == use)
                            return HandPattern{HandType::Airplane,
                                               tripleRanks[i + use - 1], use, use * 2};
                    }
                }
            }
            i = j;
        }
    }

    return std::nullopt;
}

// ----- beats (支持技能buff) -----

bool GameState::beats(const PlayedCards& play, const PlayedCards& lastPlay,
                      const SkillBuffs* buffs)
{
    auto& p = play.pattern;
    auto& lp = lastPlay.pattern;

    if (p.type == HandType::Rocket) return true;
    if (lp.type == HandType::Rocket) return false;

    // S01 bomb boost: bomb beats non-bomb, bombs compare with bonus
    if (p.type == HandType::Bomb && lp.type != HandType::Bomb) return true;
    if (p.type != HandType::Bomb && lp.type == HandType::Bomb) return false;

    if (p.type != lp.type) return false;

    if (p.type == HandType::Straight || p.type == HandType::ConsecutivePairs
        || p.type == HandType::Airplane) {
        if (p.length != lp.length) return false;
        if (p.type == HandType::Airplane && p.kickerCount != lp.kickerCount)
            return false;
    }

    if ((p.type == HandType::TriplePlusOne || p.type == HandType::TriplePlusTwo)
        && p.kickerCount != lp.kickerCount)
        return false;

    int bonus = 0;
    if (buffs && buffs->bombBoosted && p.type == HandType::Bomb)
        bonus = buffs->bombRankBonus;

    return p.mainRank + bonus > lp.mainRank;
}

// ----- GameState: setup / turn logic -----

GameState::GameState() = default;

void GameState::dealCards(int extraCards)
{
    auto deck = createDeck();
    int deal = STANDARD_DEAL + extraCards;
    m_playerHand.assign(deck.begin(), deck.begin() + deal);
    m_computerHand.assign(deck.begin() + deal, deck.begin() + deal + STANDARD_DEAL);
    // 剩余牌放入抽牌堆
    m_drawPile.assign(deck.begin() + deal + STANDARD_DEAL, deck.end());
    sortByDZ(m_playerHand, m_playerBuffs.wildcardRank);
    sortByDZ(m_computerHand);

    m_lastPlay.reset();
    m_lastPlayer = -1;
    m_playerDisplayed.clear();
    m_computerDisplayed.clear();
    m_phase = Phase::PlayerTurn;

    m_playerBuffs.clear();
    m_chainBombUsed = false;
    m_enemyBuffs.clear();
    m_s02_active = false;
    m_s07_active = false;
    m_s08_active = false;
}

void GameState::setEnemySkills(const std::array<int, MAX_SKILL_SLOTS>& skills)
{
    m_enemySkills = skills;
    m_enemyBuffs.clear();
}

void GameState::startNewRound()
{
    m_lastPlay.reset();
    m_lastPlayer = -1;
}

std::vector<Card> GameState::extractCards(const std::vector<Card>& hand,
                                          const std::vector<int>& indices)
{
    std::vector<Card> out;
    out.reserve(indices.size());
    for (int i : indices)
        out.push_back(hand[i]);
    sortByDZ(out);
    return out;
}

void GameState::removeIndices(std::vector<Card>& hand,
                              const std::vector<int>& indices)
{
    auto sorted = indices;
    std::sort(sorted.begin(), sorted.end(), std::greater<int>());
    for (int i : sorted)
        hand.erase(hand.begin() + i);
}

bool GameState::playerPlay(const std::vector<int>& handIndices)
{
    if (m_phase != Phase::PlayerTurn) return false;
    if (handIndices.empty()) return false;

    auto cards = extractCards(m_playerHand, handIndices);
    auto pattern = classifyHand(cards, &m_playerBuffs);
    if (!pattern) return false;

    PlayedCards play{*pattern, cards};

    if (m_lastPlay && !beats(play, *m_lastPlay, &m_playerBuffs))
        return false;

    // 在修改状态前捕获特征 (用于AI学习)
    bool wasNewRound = !m_lastPlay.has_value();
    int handSizeBefore = (int)m_playerHand.size();

    removeIndices(m_playerHand, handIndices);
    m_playerDisplayed = std::move(cards);
    m_computerDisplayed.clear();
    m_lastPlay = play;
    m_lastPlayer = 0;

    if (m_aiMemory) {
        PlayFeatures feat;
        feat.handSize   = handSizeBefore;
        feat.isNewRound = wasNewRound;
        feat.level      = 1;
        feat.lastPlayType = -1;
        feat.lastPlayRank = -1;

        // 使用出牌前的手牌状态检查炸弹/火箭
        auto preHand = m_playerHand;
        preHand.insert(preHand.end(), cards.begin(), cards.end());
        setHasBombRocket(feat, preHand);

        PlayAction act;
        act.handType  = (int)pattern->type;
        act.mainRank  = pattern->mainRank;
        act.cardCount = (int)cards.size();
        act.passed    = false;

        m_aiMemory->recordPlay(feat, act);
    }

    applyPostPlayEffects();

    if (m_playerHand.empty()) {
        m_phase = Phase::PlayerWins;
        return true;
    }
    endPlayerTurnCleanup();
    m_phase = Phase::ComputerTurn;
    return true;
}

void GameState::playerPass()
{
    if (m_phase != Phase::PlayerTurn) return;
    if (!m_lastPlay) return;

    // 在修改状态前捕获特征
    int handSizeBefore = (int)m_playerHand.size();

    startNewRound();
    m_playerDisplayed.clear();
    m_computerDisplayed.clear();

    if (m_aiMemory) {
        PlayFeatures feat;
        feat.handSize   = handSizeBefore;
        feat.isNewRound = false;
        feat.level      = 1;
        feat.lastPlayType = m_lastPlay ? (int)m_lastPlay->pattern.type : -1;
        feat.lastPlayRank = m_lastPlay ? m_lastPlay->pattern.mainRank : -1;

        setHasBombRocket(feat, m_playerHand);

        PlayAction act;
        act.passed    = true;
        act.handType  = -2;
        act.mainRank  = -1;
        act.cardCount = 0;

        m_aiMemory->recordPlay(feat, act);
    }

    endPlayerTurnCleanup();
    m_phase = Phase::ComputerTurn;
}

bool GameState::canPlay(const std::vector<int>& handIndices) const
{
    if (m_phase != Phase::PlayerTurn) return false;
    if (handIndices.empty()) return false;

    auto cards = extractCards(m_playerHand, handIndices);
    auto pattern = classifyHand(cards, &m_playerBuffs);
    if (!pattern) return false;

    PlayedCards play{*pattern, cards};
    if (m_lastPlay && !beats(play, *m_lastPlay, &m_playerBuffs)) return false;
    return true;
}

// ----- 技能系统 -----

void GameState::drawCards(int count)
{
    for (int i = 0; i < count && !m_drawPile.empty(); ++i) {
        m_playerHand.push_back(m_drawPile.back());
        m_drawPile.pop_back();
    }
    sortByDZ(m_playerHand, m_playerBuffs.wildcardRank);
}

std::vector<int> GameState::findBombInHand() const
{
    std::array<int, DZ_RANKS> freq{};
    for (auto& c : m_playerHand)
        freq[doudizhuOrder(c.rank)]++;

    for (int r = 0; r < DZ_RANKS; ++r) {
        if (freq[r] >= 4) {
            std::vector<int> indices;
            for (int i = 0; i < (int)m_playerHand.size() && (int)indices.size() < 4; ++i)
                if (doudizhuOrder(m_playerHand[i].rank) == r)
                    indices.push_back(i);
            return indices;
        }
    }
    return {};
}

void GameState::applyPostPlayEffects()
{
    auto& lastCards = m_playerDisplayed;
    if (lastCards.empty()) return;

    auto pattern = classifyHand(lastCards);
    if (!pattern) return;

    // S02: 火箭冲刺 - 打出火箭额外抽3张
    if (m_s02_active && pattern->type == HandType::Rocket) {
        drawCards(3);
        m_s02_active = false;
    }

    // S07: 炸弹馈赠 - 打出炸弹抽1张牌
    if (m_s07_active && pattern->type == HandType::Bomb) {
        drawCards(1);
        m_s07_active = false;
    }

    // S08: 连环炸弹 - 出牌后若手中有炸弹则自动打出 (限1次)
    if (m_s08_active && !m_chainBombUsed) {
        auto bombIdx = findBombInHand();
        if (!bombIdx.empty()) {
            m_chainBombUsed = true;
            auto bombCards = extractCards(m_playerHand, bombIdx);
            auto bombPattern = classifyHand(bombCards);
            if (bombPattern && bombPattern->type == HandType::Bomb) {
                removeIndices(m_playerHand, bombIdx);
                m_playerDisplayed.insert(m_playerDisplayed.end(),
                                         bombCards.begin(), bombCards.end());
                PlayedCards bombPlay{*bombPattern, bombCards};
                m_lastPlay = bombPlay;
                m_lastPlayer = 0;

                if (m_s07_active) {
                    drawCards(1);
                    m_s07_active = false;
                }

                if (m_playerHand.empty()) {
                    m_phase = Phase::PlayerWins;
                }
            }
        }
    }
}

void GameState::endPlayerTurnCleanup()
{
    m_playerBuffs.clear();
    m_chainBombUsed = false;
    m_s02_active = false;
    m_s07_active = false;
    m_s08_active = false;
}

bool GameState::activatePlayerSkill(int skillId)
{
    if (m_phase != Phase::PlayerTurn) return false;
    if (skillId < 0 || skillId >= SKILL_COUNT) return false;

    switch (skillId) {
    case 0: m_playerBuffs.bombBoosted = true;      break;
    case 1: m_s02_active = true;                   break;
    case 2: m_playerBuffs.straightExtended = true;  break;
    case 3: m_playerBuffs.pairsExtended = true;     break;
    case 4: m_playerBuffs.tripleExtraKicker = true; break;
    case 5: m_playerBuffs.airplaneExtended = true;  break;
    case 6: m_s07_active = true;                   break;
    case 7: m_s08_active = true;                   break;
    }

    if (m_aiMemory)
        m_aiMemory->recordSkillUse(skillId);

    return true;
}

// ----- 敌人技能激活 -----

void GameState::enemyActivateSkills()
{
    if (m_phase != Phase::ComputerTurn) return;

    for (int i = 0; i < MAX_SKILL_SLOTS; ++i) {
        int sid = m_enemySkills[i];
        if (sid < 0) continue;

        // 查询AI记忆: 玩家使用这个技能的概率
        float prob = 0.0f;
        if (m_aiMemory)
            prob = m_aiMemory->querySkillProb(sid);

        if (prob >= 0.4f) {
            switch (sid) {
            case 0: m_enemyBuffs.bombBoosted = true;      break;
            case 2: m_enemyBuffs.straightExtended = true;  break;
            case 3: m_enemyBuffs.pairsExtended = true;     break;
            case 4: m_enemyBuffs.tripleExtraKicker = true; break;
            case 5: m_enemyBuffs.airplaneExtended = true;  break;
            default: break;
            }
        }
    }
}

// ----- Computer AI -----

// 评估出牌代价: 拆炸弹/三条/对子的惩罚越高越不应选
static int evaluatePlayCost(const std::vector<int>& indices,
                            const std::vector<Card>& hand)
{
    std::array<int, DZ_RANKS> freq{};
    for (auto& c : hand)
        freq[doudizhuOrder(c.rank)]++;

    int cost = 0;
    for (int idx : indices) {
        int r = doudizhuOrder(hand[idx].rank);
        if (freq[r] >= 4)      cost += 50;   // 拆炸弹
        else if (freq[r] >= 3) cost += 15;   // 拆三条
        else if (freq[r] >= 2) cost += 3;    // 拆对子
    }
    return cost;
}

GameState::PlayOption GameState::findLowestPlay(const std::vector<Card>& hand) const
{
    int n = (int)hand.size();
    std::array<int, DZ_RANKS> freq{};
    for (auto& c : hand)
        freq[doudizhuOrder(c.rank)]++;

    // 辅助: 从手牌中找指定rank的count张牌
    auto findN = [&](int r, int count, std::vector<bool>& used) {
        std::vector<int> idx;
        for (int i = 0; i < n && (int)idx.size() < count; ++i)
            if (!used[i] && doudizhuOrder(hand[i].rank) == r) {
                idx.push_back(i);
                used[i] = true;
            }
        return idx;
    };

    auto findAnySingle = [&](const std::vector<bool>& used) -> int {
        for (int i = n - 1; i >= 0; --i)
            if (!used[i]) return i;
        return -1;
    };

    // 1. 顺子 — 优先最长、最低rank的
    for (int len = std::min(n, 12); len >= 5; --len) {
        for (int top = RA; top >= R3 + len - 1; --top) {
            int bottom = top - len + 1;
            std::vector<int> indices;
            std::vector<bool> used(n);
            bool ok = true;
            for (int r = bottom; r <= top; ++r) {
                int ix = findCardOfRank(hand, r, used);
                if (ix < 0) { ok = false; break; }
                indices.push_back(ix);
                used[ix] = true;
            }
            if (ok) {
                std::sort(indices.begin(), indices.end());
                return {HandPattern{HandType::Straight, top, len, 0}, indices};
            }
        }
    }

    // 2. 三带二 — 最低三条 + 最低对子
    for (int r = R3; r <= R2; ++r) {
        if (freq[r] < 3) continue;
        for (int kr = R3; kr <= R2; ++kr) {
            if (kr == r || freq[kr] < 2) continue;
            std::vector<bool> used(n);
            auto triple = findN(r, 3, used);
            auto pair   = findN(kr, 2, used);
            triple.insert(triple.end(), pair.begin(), pair.end());
            std::sort(triple.begin(), triple.end());
            return {HandPattern{HandType::TriplePlusTwo, r, 0, 2}, triple};
        }
    }

    // 3. 三带一 — 最低三条 + 最低单牌
    for (int r = R3; r <= R2; ++r) {
        if (freq[r] < 3) continue;
        std::vector<bool> used(n);
        auto triple = findN(r, 3, used);
        int kicker = findAnySingle(used);
        if (kicker >= 0) {
            triple.push_back(kicker);
            std::sort(triple.begin(), triple.end());
            return {HandPattern{HandType::TriplePlusOne, r, 0, 1}, triple};
        }
    }

    // 4. 三条
    for (int r = R3; r <= R2; ++r) {
        if (freq[r] >= 3) {
            std::vector<bool> used(n);
            auto triple = findN(r, 3, used);
            std::sort(triple.begin(), triple.end());
            return {HandPattern{HandType::Triple, r, 0, 0}, triple};
        }
    }

    // 5. 对子 — 最低的
    for (int r = R3; r <= R2; ++r) {
        if (freq[r] >= 2) {
            std::vector<bool> used(n);
            auto pair = findN(r, 2, used);
            std::sort(pair.begin(), pair.end());
            return {HandPattern{HandType::Pair, r, 0, 0}, pair};
        }
    }

    // 兜底: 最低单牌
    int last = n - 1;
    return {{HandPattern{HandType::Single,
                         doudizhuOrder(hand[last].rank), 0, 0}}, {last}};
}

std::vector<GameState::PlayOption> GameState::findBeatingPlays(
    const PlayedCards& target, const std::vector<Card>& hand,
    const SkillBuffs* buffs) const
{
    std::vector<PlayOption> result;
    auto& tp = target.pattern;
    int n = (int)hand.size();

    const SkillBuffs* b = buffs ? buffs : &m_enemyBuffs;

    // 火箭可以压任何牌
    {
        int si = findCardOfRank(hand, RS, {});
        int bi = findCardOfRank(hand, RB, {});
        if (si >= 0 && bi >= 0) {
            result.push_back({HandPattern{HandType::Rocket, RB, 0, 0},
                              {std::min(si, bi), std::max(si, bi)}});
        }
    }

    switch (tp.type) {
    case HandType::Single:
        for (int i = 0; i < n; ++i) {
            int r = doudizhuOrder(hand[i].rank);
            if (r > tp.mainRank)
                result.push_back({HandPattern{HandType::Single, r, 0, 0}, {i}});
        }
        break;

    case HandType::Pair:
        for (int r = tp.mainRank + 1; r < DZ_RANKS; ++r) {
            int i0 = findCardOfRank(hand, r, {});
            if (i0 < 0) continue;
            std::vector<bool> used(n); used[i0] = true;
            int i1 = findCardOfRank(hand, r, used);
            if (i1 >= 0)
                result.push_back({HandPattern{HandType::Pair, r, 0, 0},
                                  {std::min(i0, i1), std::max(i0, i1)}});
        }
        break;

    case HandType::Triple:
        for (int r = tp.mainRank + 1; r < DZ_RANKS; ++r) {
            std::vector<int> idx;
            std::vector<bool> used(n);
            for (int t = 0; t < 3; ++t) {
                int ix = findCardOfRank(hand, r, used);
                if (ix < 0) break;
                idx.push_back(ix);
                used[ix] = true;
            }
            if ((int)idx.size() == 3) {
                std::sort(idx.begin(), idx.end());
                result.push_back({HandPattern{HandType::Triple, r, 0, 0}, idx});
            }
        }
        break;

    case HandType::TriplePlusOne:
        for (int r = tp.mainRank + 1; r < DZ_RANKS; ++r) {
            std::vector<int> idx;
            std::vector<bool> used(n);
            for (int t = 0; t < 3; ++t) {
                int ix = findCardOfRank(hand, r, used);
                if (ix < 0) break;
                idx.push_back(ix);
                used[ix] = true;
            }
            if ((int)idx.size() != 3) continue;
            int kickersNeeded = tp.kickerCount;
            std::vector<int> kickers;
            for (int i = 0; i < n && (int)kickers.size() < kickersNeeded; ++i) {
                if (!used[i]) {
                    kickers.push_back(i);
                    used[i] = true;
                }
            }
            if ((int)kickers.size() == kickersNeeded) {
                auto full = idx;
                full.insert(full.end(), kickers.begin(), kickers.end());
                std::sort(full.begin(), full.end());
                result.push_back({HandPattern{HandType::TriplePlusOne, r, 0, tp.kickerCount}, full});
            }
        }
        break;

    case HandType::TriplePlusTwo:
        for (int r = tp.mainRank + 1; r < DZ_RANKS; ++r) {
            std::vector<int> idx;
            std::vector<bool> used(n);
            for (int t = 0; t < 3; ++t) {
                int ix = findCardOfRank(hand, r, used);
                if (ix < 0) break;
                idx.push_back(ix);
                used[ix] = true;
            }
            if ((int)idx.size() != 3) continue;
            int pairsNeeded = tp.kickerCount / 2;
            std::vector<int> kickerPairs;
            std::vector<bool> used2 = used;
            for (int kr = 0; kr < DZ_RANKS && (int)kickerPairs.size() < pairsNeeded * 2; ++kr) {
                if (kr == r) continue;
                int i0 = findCardOfRank(hand, kr, used2);
                if (i0 < 0) continue;
                used2[i0] = true;
                int i1 = findCardOfRank(hand, kr, used2);
                if (i1 < 0) { used2[i0] = false; continue; }
                used2[i1] = true;
                kickerPairs.push_back(i0);
                kickerPairs.push_back(i1);
            }
            if ((int)kickerPairs.size() == pairsNeeded * 2) {
                auto full = idx;
                full.insert(full.end(), kickerPairs.begin(), kickerPairs.end());
                std::sort(full.begin(), full.end());
                result.push_back({HandPattern{HandType::TriplePlusTwo, r, 0, tp.kickerCount}, full});
            }
        }
        break;

    case HandType::Straight:
        {
            int minLen = (b && b->straightExtended) ? 4 : 5;
            if (tp.length < minLen) break;
            for (int top = tp.mainRank + 1; top <= RA; ++top) {
                int bottom = top - tp.length + 1;
                if (bottom < R3) continue;
                std::vector<int> idx;
                std::vector<bool> used(n);
                bool ok = true;
                for (int rr = bottom; rr <= top; ++rr) {
                    int ix = findCardOfRank(hand, rr, used);
                    if (ix < 0) { ok = false; break; }
                    idx.push_back(ix);
                    used[ix] = true;
                }
                if (ok) {
                    std::sort(idx.begin(), idx.end());
                    result.push_back({HandPattern{HandType::Straight, top, tp.length, 0}, idx});
                }
            }
        }
        break;

    case HandType::ConsecutivePairs:
        {
            int minLen = (b && b->pairsExtended) ? 2 : 3;
            if (tp.length < minLen) break;
            for (int top = tp.mainRank + 1; top <= RA; ++top) {
                int bottom = top - tp.length + 1;
                if (bottom < R3) continue;
                std::vector<int> idx;
                std::vector<bool> used(n);
                bool ok = true;
                for (int rr = bottom; rr <= top; ++rr) {
                    int i0 = findCardOfRank(hand, rr, used);
                    if (i0 < 0) { ok = false; break; }
                    used[i0] = true;
                    int i1 = findCardOfRank(hand, rr, used);
                    if (i1 < 0) { ok = false; break; }
                    used[i1] = true;
                    idx.push_back(i0);
                    idx.push_back(i1);
                }
                if (ok) {
                    std::sort(idx.begin(), idx.end());
                    result.push_back({HandPattern{HandType::ConsecutivePairs,
                                                  top, tp.length, 0}, idx});
                }
            }
        }
        break;

    case HandType::Airplane:
        {
            int minLen = (b && b->airplaneExtended) ? 1 : 2;
            if (tp.length < minLen) break;
            for (int top = tp.mainRank + 1; top <= R2; ++top) {
                int bottom = top - tp.length + 1;
                if (bottom < R3) continue;
                std::vector<int> idx;
                std::vector<bool> used(n);
                bool ok = true;
                for (int rr = bottom; rr <= top; ++rr) {
                    for (int t = 0; t < 3; ++t) {
                        int ix = findCardOfRank(hand, rr, used);
                        if (ix < 0) { ok = false; break; }
                        idx.push_back(ix);
                        used[ix] = true;
                    }
                    if (!ok) break;
                }
                if (!ok) continue;

                if (tp.kickerCount == 0) {
                    std::sort(idx.begin(), idx.end());
                    result.push_back({HandPattern{HandType::Airplane, top, tp.length, 0}, idx});
                } else if (tp.kickerCount == tp.length) {
                    std::vector<int> kickers;
                    for (int i = 0; i < n && (int)kickers.size() < tp.length; ++i) {
                        if (!used[i]) {
                            kickers.push_back(i);
                            used[i] = true;
                        }
                    }
                    if ((int)kickers.size() == tp.length) {
                        auto full = idx;
                        full.insert(full.end(), kickers.begin(), kickers.end());
                        std::sort(full.begin(), full.end());
                        result.push_back({HandPattern{HandType::Airplane, top,
                                                      tp.length, tp.length}, full});
                    }
                } else if (tp.kickerCount == tp.length * 2) {
                    std::vector<int> kickerPairs;
                    std::vector<bool> used2 = used;
                    for (int kr = 0; kr < DZ_RANKS && (int)kickerPairs.size() < tp.length * 2; ++kr) {
                        bool inTriple = false;
                        for (int rr = bottom; rr <= top; ++rr)
                            if (kr == rr) { inTriple = true; break; }
                        if (inTriple) continue;
                        int i0 = findCardOfRank(hand, kr, used2);
                        if (i0 < 0) continue;
                        used2[i0] = true;
                        int i1 = findCardOfRank(hand, kr, used2);
                        if (i1 < 0) { used2[i0]=false; continue; }
                        used2[i1] = true;
                        kickerPairs.push_back(i0);
                        kickerPairs.push_back(i1);
                    }
                    if ((int)kickerPairs.size() == tp.length * 2) {
                        auto full = idx;
                        full.insert(full.end(), kickerPairs.begin(), kickerPairs.end());
                        std::sort(full.begin(), full.end());
                        result.push_back({HandPattern{HandType::Airplane, top,
                                                      tp.length, tp.length * 2}, full});
                    }
                }
            }
        }
        break;

    case HandType::Bomb:
        {
            int threshold = tp.mainRank;
            if (b && b->bombBoosted) threshold -= b->bombRankBonus;
            for (int r = threshold + 1; r < DZ_RANKS; ++r) {
                std::vector<int> idx;
                std::vector<bool> used(n);
                for (int t = 0; t < 4; ++t) {
                    int ix = findCardOfRank(hand, r, used);
                    if (ix < 0) break;
                    idx.push_back(ix);
                    used[ix] = true;
                }
                if ((int)idx.size() == 4) {
                    std::sort(idx.begin(), idx.end());
                    result.push_back({HandPattern{HandType::Bomb, r, 0, 0}, idx});
                }
            }
        }
        break;

    case HandType::Rocket:
        break;

    case HandType::None:
        break;
    }

    return result;
}

std::vector<Card> GameState::computerTakeTurn()
{
    if (m_phase != Phase::ComputerTurn) return {};

    // 敌人回合开始: 激活可用技能
    enemyActivateSkills();

    std::vector<Card> played;

    if (m_lastPlay && m_lastPlayer == 0) {
        auto options = findBeatingPlays(*m_lastPlay, m_computerHand, &m_enemyBuffs);

        if (options.empty()) {
            startNewRound();
            m_computerDisplayed.clear();
            m_playerDisplayed.clear();
            // 清除敌人回合buff
            m_enemyBuffs.clear();
            m_phase = Phase::PlayerTurn;
            return {};
        }

        PlayOption* best = nullptr;
        float bestScore = 999999.f;
        int handSize = (int)m_computerHand.size();

        // 查询AI记忆: 玩家在类似情况下怎么做
        std::vector<AIMemory::WeightedAction> memPrefs;
        bool useMemory = false;
        if (m_aiMemory && m_aiMemory->playRecordCount() > 0) {
            PlayFeatures feats;
            feats.handSize   = (int)m_computerHand.size();
            feats.isNewRound = false;
            feats.level      = 1;
            feats.lastPlayType = (int)m_lastPlay->pattern.type;
            feats.lastPlayRank = m_lastPlay->pattern.mainRank;

            setHasBombRocket(feats, m_computerHand);

            memPrefs = m_aiMemory->queryPlays(feats, 5);
            useMemory = !memPrefs.empty();
        }

        for (auto& opt : options) {
            bool isBomb = (opt.pattern.type == HandType::Bomb
                        || opt.pattern.type == HandType::Rocket);

            float score;
            if (isBomb) {
                if (handSize > 4) continue;
                score = (float)(handSize * 10);
            } else {
                int cost = evaluatePlayCost(opt.handIndices, m_computerHand);
                score = (float)(cost * 100 + opt.pattern.mainRank);
            }

            // 用记忆偏好调整分数: 玩家常出的牌型加分
            if (useMemory) {
                for (auto& pref : memPrefs) {
                    if ((int)opt.pattern.type == pref.handType && !pref.passed) {
                        float rankMatch = 1.f / (1.f + std::abs(opt.pattern.mainRank - pref.mainRank));
                        score -= pref.weight * (3.f + rankMatch) * 20.f;
                    }
                }
                // 玩家爱pass → 不出选项更受青睐 (在后续处理)
            }

            if (score < bestScore) {
                bestScore = score;
                best = &opt;
            }
        }

        if (!best) best = &options[0];

        played = extractCards(m_computerHand, best->handIndices);
        removeIndices(m_computerHand, best->handIndices);
        m_computerDisplayed = played;
        m_playerDisplayed.clear();
        m_lastPlay = PlayedCards{best->pattern, played};
        m_lastPlayer = 1;
    } else {
        auto opt = findLowestPlay(m_computerHand);
        played = extractCards(m_computerHand, opt.handIndices);
        removeIndices(m_computerHand, opt.handIndices);
        m_computerDisplayed = played;
        m_playerDisplayed.clear();
        m_lastPlay = PlayedCards{opt.pattern, played};
        m_lastPlayer = 1;
    }

    // 清除敌人回合buff
    m_enemyBuffs.clear();

    if (m_computerHand.empty()) {
        m_phase = Phase::ComputerWins;
    } else {
        m_phase = Phase::PlayerTurn;
    }
    return played;
}

