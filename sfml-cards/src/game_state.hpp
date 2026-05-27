#pragma once

#include "card.hpp"
#include "skill.hpp"
#include "ai_memory.hpp"
#include <vector>
#include <optional>
#include <array>

enum class HandType : uint8_t
{
    None,
    Single,
    Pair,
    Triple,
    TriplePlusOne,
    TriplePlusTwo,
    Straight,
    ConsecutivePairs,
    Airplane,
    Bomb,
    Rocket
};

struct HandPattern
{
    HandType type = HandType::None;
    int mainRank = -1;
    int length = 0;
    int kickerCount = 0;
};

struct PlayedCards
{
    HandPattern pattern;
    std::vector<Card> cards;
};

class GameState
{
public:
    enum class Phase { PlayerTurn, ComputerTurn, PlayerWins, ComputerWins };

    GameState();

    // extraCards: 角色额外手牌数 (掌控者=2)
    void dealCards(int extraCards);
    // enemySkills: 镜像给敌人的技能id列表
    void setEnemySkills(const std::array<int, MAX_SKILL_SLOTS>& skills);

    const std::vector<Card>& playerHand() const   { return m_playerHand; }
    const std::vector<Card>& computerHand() const { return m_computerHand; }
    Phase phase() const { return m_phase; }

    // 玩家出牌
    bool playerPlay(const std::vector<int>& handIndices);
    void playerPass();

    bool canPlay(const std::vector<int>& handIndices) const;

    // 电脑自动出牌 + 技能
    std::vector<Card> computerTakeTurn();

    // 渲染用
    const std::vector<Card>& lastPlayerPlay() const   { return m_playerDisplayed; }
    const std::vector<Card>& lastComputerPlay() const { return m_computerDisplayed; }
    bool isNewRound() const { return !m_lastPlay.has_value(); }

    static std::optional<HandPattern> classifyHand(const std::vector<Card>& cards,
                                                     const SkillBuffs* buffs = nullptr);
    static bool beats(const PlayedCards& play, const PlayedCards& lastPlay,
                      const SkillBuffs* buffs = nullptr);

    // ------- 技能 -------
    const SkillBuffs& playerBuffs() const { return m_playerBuffs; }

    // 玩家激活技能, 返回是否成功
    bool activatePlayerSkill(int skillId);

    // 回合结束清理 (清除buff)
    void endPlayerTurnCleanup();

    // 抽牌
    int  drawPileSize() const { return (int)m_drawPile.size(); }
    void drawCards(int count);

    // 敌人技能信息 (渲染用)
    const std::array<int, MAX_SKILL_SLOTS>& enemySkillSlots() const { return m_enemySkills; }
    const SkillBuffs& enemyBuffs() const { return m_enemyBuffs; }

    // 触发技能标志 (渲染用)
    bool s02Active() const { return m_s02_active; }
    bool s08Active() const { return m_s08_active; }

    // AI 学习记忆 (由 main.cpp 管理生命周期)
    void setAIMemory(AIMemory* mem) { m_aiMemory = mem; }

    // 开发者调试
    void forceWin()  { m_phase = Phase::PlayerWins; }
    void forceLose() { m_phase = Phase::ComputerWins; }

private:
    std::vector<Card> m_playerHand;
    std::vector<Card> m_computerHand;
    std::vector<Card> m_drawPile;

    std::optional<PlayedCards> m_lastPlay;
    int m_lastPlayer = -1;

    std::vector<Card> m_playerDisplayed;
    std::vector<Card> m_computerDisplayed;

    Phase m_phase = Phase::PlayerTurn;

    // 技能
    SkillBuffs m_playerBuffs;
    bool m_chainBombUsed = false;        // S08 每回合限1次

    // 触发技能标记
    bool m_s02_active = false;  // 火箭冲刺: 打出火箭时抽3张
    bool m_s07_active = false;  // 炸弹馈赠: 打出炸弹抽1张
    bool m_s08_active = false;  // 连环炸弹: 出牌后自动出炸弹

    // 敌人技能
    std::array<int, MAX_SKILL_SLOTS> m_enemySkills = {-1, -1, -1};
    SkillBuffs m_enemyBuffs;

    void startNewRound();
    void applyPostPlayEffects();

    struct PlayOption
    {
        HandPattern pattern;
        std::vector<int> handIndices;
    };
    std::vector<PlayOption> findBeatingPlays(const PlayedCards& target,
                                             const std::vector<Card>& hand,
                                             const SkillBuffs* buffs = nullptr) const;
    PlayOption findLowestPlay(const std::vector<Card>& hand) const;

    static std::vector<Card> extractCards(const std::vector<Card>& hand,
                                          const std::vector<int>& indices);
    static void removeIndices(std::vector<Card>& hand,
                              const std::vector<int>& indices);

    // 查找炸弹 (用于S08连环炸弹)
    std::vector<int> findBombInHand() const;
    // 敌人回合开始激活技能
    void enemyActivateSkills();

    AIMemory* m_aiMemory = nullptr;
};
