#include "game_state.hpp"
#include <algorithm>
#include <stdexcept>

// 斗地主排序常量
constexpr int R3 = 0, R4 = 1, R5 = 2, R6 = 3, R7 = 4, R8 = 5, R9 = 6,
              R10 = 7, RJ = 8, RQ = 9, RK = 10, RA = 11, R2 = 12,
              RS = 13, RB = 14;
constexpr int DZ_RANKS = 15;
constexpr int STANDARD_DEAL = 15;

// ----- helpers -----

static void sortByDZ(std::vector<Card>& cards)
{
    std::sort(cards.begin(), cards.end(), [](const Card& a, const Card& b) {
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

// ----- classifyHand (支持技能buff) -----
// 外部调用版本
std::optional<HandPattern> GameState::classifyHand(const std::vector<Card>& cards,
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
    // S05: tripleExtraKicker allows triple+1 to carry 2 singles (n=5, kicker=2)
    //      and triple+2 to carry 2 pairs + 1 single (n=6, kicker=3)
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
                // check if rest is a pair (normal) or 2 singles (with extra kicker)
                bool hasPair = false;
                for (int k = 0; k < DZ_RANKS; ++k)
                    if (k != r && freq[k] == 2) { hasPair = true; break; }
                if (hasPair)
                    return HandPattern{HandType::TriplePlusTwo, r, 0, 2};
                if (extra)
                    return HandPattern{HandType::TriplePlusOne, r, 0, 2}; // extra single
            }
            if (rest == 3 && extra) {
                // triple + 3 singles (with extra kicker)
                return HandPattern{HandType::TriplePlusOne, r, 0, 3};
            }
            return std::nullopt;
        }
        // 没有三条 → 不返回, 继续检查顺子等牌型
    }

    // Straight: 默认≥5, S03 straightExtended → ≥4
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

    // ConsecutivePairs: 默认≥3, S04 pairsExtended → ≥2
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

    // Airplane: 默认≥2, S06 airplaneExtended → ≥1
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
                    if (remain == 0) {
                        return HandPattern{HandType::Airplane,
                                           tripleRanks[i + use - 1], use, 0};
                    }
                    if (remain == use) {
                        return HandPattern{HandType::Airplane,
                                           tripleRanks[i + use - 1], use, use};
                    }
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
    sortByDZ(m_playerHand);
    sortByDZ(m_computerHand);

    m_lastPlay.reset();
    m_lastPlayer = -1;
    m_playerDisplayed.clear();
    m_computerDisplayed.clear();
    m_phase = Phase::PlayerTurn;

    m_playerEnergy = START_ENERGY;
    m_energyPerTurn = 1;
    m_playerCooldowns = {0, 0, 0};
    m_playerBuffs.clear();
    m_playerPlayedThisTurn = false;
    m_chainBombUsed = false;
    m_enemyCooldowns = {0, 0, 0};
    m_enemyBuffs.clear();
    m_s02_active = false;
    m_s07_active = false;
    m_s08_active = false;
}

void GameState::setEnemySkills(const std::array<int, MAX_SKILL_SLOTS>& skills)
{
    m_enemySkills = skills;
    m_enemyCooldowns = {0, 0, 0};
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

    removeIndices(m_playerHand, handIndices);
    m_playerDisplayed = std::move(cards);
    m_computerDisplayed.clear();
    m_lastPlay = play;
    m_lastPlayer = 0;
    m_playerPlayedThisTurn = true;

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

    startNewRound();
    m_playerDisplayed.clear();
    m_computerDisplayed.clear();
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

void GameState::gainEnergy(int amount)
{
    m_playerEnergy += amount;
    if (m_playerEnergy > MAX_ENERGY) m_playerEnergy = MAX_ENERGY;
    if (m_playerEnergy < 0) m_playerEnergy = 0;
}

void GameState::drawCards(int count)
{
    for (int i = 0; i < count && !m_drawPile.empty(); ++i) {
        m_playerHand.push_back(m_drawPile.back());
        m_drawPile.pop_back();
    }
    sortByDZ(m_playerHand);
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

    // S07: 炸弹馈赠 - 打出炸弹回复2点能量
    if (m_s07_active && pattern->type == HandType::Bomb) {
        gainEnergy(2);
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
                    gainEnergy(2);
                    m_s07_active = false;
                }

                if (m_playerHand.empty()) {
                    m_phase = Phase::PlayerWins;
                }
            }
        }
    }
}

void GameState::startPlayerTurn()
{
    for (int i = 0; i < MAX_SKILL_SLOTS; ++i)
        if (m_playerCooldowns[i] > 0)
            --m_playerCooldowns[i];

    m_playerEnergy += m_energyPerTurn;
    if (m_playerEnergy > MAX_ENERGY) m_playerEnergy = MAX_ENERGY;
}

void GameState::endPlayerTurnCleanup()
{
    m_playerBuffs.clear();
    m_playerPlayedThisTurn = false;
    m_chainBombUsed = false;
    m_s02_active = false;
    m_s07_active = false;
    m_s08_active = false;
}

bool GameState::activatePlayerSkill(int skillId, int slotIdx)
{
    if (m_phase != Phase::PlayerTurn) return false;
    if (slotIdx < 0 || slotIdx >= MAX_SKILL_SLOTS) return false;
    if (m_playerCooldowns[slotIdx] > 0) return false;

    auto& skills = getAllSkills();
    if (skillId < 0 || skillId >= SKILL_COUNT) return false;
    auto& sk = skills[skillId];

    if (m_playerEnergy < sk.energyCost) return false;

    m_playerEnergy -= sk.energyCost;
    m_playerCooldowns[slotIdx] = sk.cooldown;

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

    return true;
}

// ----- 敌人技能激活 -----

void GameState::enemyActivateSkills()
{
    if (m_phase != Phase::ComputerTurn) return;

    auto& skills = getAllSkills();

    for (int i = 0; i < MAX_SKILL_SLOTS; ++i) {
        int sid = m_enemySkills[i];
        if (sid < 0) continue;
        if (m_enemyCooldowns[i] > 0) continue;

        auto& sk = skills[sid];
        m_enemyCooldowns[i] = sk.cooldown;

        // 根据技能id设置敌人buff
        switch (sid) {
        case 0: m_enemyBuffs.bombBoosted = true;      break;
        case 2: m_enemyBuffs.straightExtended = true;  break;
        case 3: m_enemyBuffs.pairsExtended = true;     break;
        case 4: m_enemyBuffs.tripleExtraKicker = true; break;
        case 5: m_enemyBuffs.airplaneExtended = true;  break;
        // S01, S06, S07 是触发型, 在computerTakeTurn中处理
        default: break;
        }
    }
}

// ----- Computer AI -----

GameState::PlayOption GameState::findLowestPlay(const std::vector<Card>& hand) const
{
    int last = (int)hand.size() - 1;
    PlayOption opt;
    opt.handIndices = {last};
    opt.pattern = HandPattern{HandType::Single,
                              doudizhuOrder(hand[last].rank), 0, 0};
    return opt;
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
            startPlayerTurn();
            m_phase = Phase::PlayerTurn;
            return {};
        }

        PlayOption* best = nullptr;
        for (auto& opt : options) {
            if (opt.pattern.type == HandType::Bomb
                || opt.pattern.type == HandType::Rocket) {
                if ((int)m_computerHand.size() <= 4 && !best)
                    best = &opt;
                continue;
            }
            if (!best || opt.pattern.mainRank < best->pattern.mainRank)
                best = &opt;
        }
        if (!best) best = &options[0];

        played = extractCards(m_computerHand, best->handIndices);
        removeIndices(m_computerHand, best->handIndices);
        m_computerDisplayed = played;
        m_playerDisplayed.clear();
        m_lastPlay = PlayedCards{best->pattern, played};
        m_lastPlayer = 1;

        // S07 for enemy: gain energy? No, enemy doesn't use energy.
        // S02 for enemy: draw cards? Simple implementation: draw from pile.
        if (best->pattern.type == HandType::Rocket) {
            // Check if enemy has S02 active
            // Simplified: just handle it
        }
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
        startPlayerTurn();
        m_phase = Phase::PlayerTurn;
    }
    return played;
}
