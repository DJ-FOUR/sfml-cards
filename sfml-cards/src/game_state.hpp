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
    enum class Phase { PlayerTurn, ComputerTurn, SchedulePlay, MomentumPlay, PlayerWins, ComputerWins };

    GameState();

    // extraCards: 角色额外手牌数 (掌控者=2)
    void dealCards(int extraCards);
    // enemySkills: 镜像给敌人的技能id列表
    void setEnemySkills(const std::array<int, MAX_SKILL_SLOTS>& skills);
    // 设置玩家癞子点数 (角色被动技)
    void setPlayerWildcard(int rank) { m_playerBuffs.wildcardRank = rank; }
    // 设置角色被动: 炸弹收藏家
    void setPlayerIsBombCollector(bool v) { m_isBombCollector = v; }
    // 设置角色被动: 掌控者
    void setPlayerIsScheduler(bool v) { m_isScheduler = v; }
    // 设置玩家装备的技能槽 — 自动应用所有被动效果
    void setPlayerSkillSlots(const std::array<int, MAX_SKILL_SLOTS>& slots);

    const std::vector<Card>& playerHand() const   { return m_playerHand; }
    const std::vector<Card>& computerHand() const { return m_computerHand; }
    Phase phase() const { return m_phase; }

    // 玩家出牌
    bool playerPlay(const std::vector<int>& handIndices);
    void playerPass();

    // 连击之势: 选1张牌打出
    bool momentumPlay(int handIndex);
    // 连击之势 (敌人触发): AI自动选1张牌打出
    bool enemyMomentumPlay();
    bool isMomentumEnemy() const { return m_momentumIsEnemy; }

    bool canPlay(const std::vector<int>& handIndices) const;

    // 掌控者「调度」: 过牌
    bool isScheduleAvailable() const { return m_scheduleAvailable; }
    int  scheduleCooldown() const { return m_scheduleCooldown; }
    bool scheduleDiscard(const std::vector<int>& handIndices);
    void scheduleSkip();
    void activateScheduleIfReady();   // 回合开始时调用

    // 电脑自动出牌 + 技能
    std::vector<Card> computerTakeTurn();

    // 渲染用
    const std::vector<Card>& lastPlayerPlay() const   { return m_playerDisplayed; }
    const std::vector<Card>& lastComputerPlay() const { return m_computerDisplayed; }
    bool isNewRound() const { return !m_lastPlay.has_value(); }

    static std::optional<HandPattern> classifyHand(const std::vector<Card>& cards,
                                                     const SkillBuffs* buffs = nullptr);
    static std::optional<HandPattern> classifyHandNoWild(const std::vector<Card>& cards,
                                                           const SkillBuffs* buffs = nullptr);
    // 非静态: 需要访问成员变量 (王牌意志检测防守方buffs)
    bool beats(const PlayedCards& play, const PlayedCards& lastPlay,
               const SkillBuffs* buffs = nullptr) const;

    // ------- 技能 -------
    const SkillBuffs& playerBuffs() const { return m_playerBuffs; }

    // 玩家激活/取消技能, 返回是否成功
    bool activatePlayerSkill(int skillId);
    bool deactivatePlayerSkill(int skillId);

    // 回合结束清理 (清除buff)
    void endPlayerTurnCleanup();

    // 抽牌
    int  drawPileSize() const { return (int)m_drawPile.size(); }
    void drawCards(int count);

    // 敌人技能信息 (渲染用)
    const std::array<int, MAX_SKILL_SLOTS>& enemySkillSlots() const { return m_enemySkills; }
    const SkillBuffs& enemyBuffs() const { return m_enemyBuffs; }

    // 技能状态查询 (渲染用)
    bool hasPassiveSkill(int skillId) const;
    int  playerBombMarks() const { return m_bombMarks; }
    uint8_t skillGlowMask(const std::vector<int>& selectedIndices) const;

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
    std::array<int, MAX_SKILL_SLOTS> m_playerSkillSlots = {-1, -1, -1};

    // Skill 0: 连击之势 (TRIGGER)
    bool m_momentumActive = false;
    int  m_enemyPassStreak = 0;   // 敌人连续不出牌计数
    bool m_enemyMomentumActive = false; // 敌人装备了连击之势
    int  m_playerPassStreak = 0;  // 玩家连续不出牌计数
    bool m_momentumIsEnemy = false; // 当前MomentumPlay是否为敌人触发

    // 角色被动: 炸弹收藏家
    bool m_isBombCollector = false;
    int  m_bombMarks = 0;

    // 角色被动: 掌控者「调度」
    bool m_isScheduler       = false;
    bool m_scheduleAvailable = false;
    int  m_scheduleCooldown  = 0;      // 冷却回合数
    bool m_firstScheduleFree = true;   // 首轮免费

    // 敌人技能
    std::array<int, MAX_SKILL_SLOTS> m_enemySkills = {-1, -1, -1};
    SkillBuffs m_enemyBuffs;

    void startNewRound();
    void applyPostPlayEffects();
    void recordBombPlayed();

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

    // 敌人回合开始激活技能
    void enemyActivateSkills();

    AIMemory* m_aiMemory = nullptr;
};
