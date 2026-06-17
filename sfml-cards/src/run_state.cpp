#include "run_state.hpp"
#include "character.hpp"
#include <algorithm>
#include <fstream>
#include <random>

RunState::RunState()
{
    std::random_device rd;
    m_rng.seed(rd());
}

void RunState::startNewRun(int characterId)
{
    m_charId = characterId;
    m_level = 1;
    m_acquired.clear();
    m_equipped = {-1, -1, -1};
    m_mirroredSkills = {-1, -1, -1};
    m_wildcardRank = -1;
    m_totalScore = 0;
    m_roundScore = 0;
}

void RunState::advanceToNextLevel()
{
    m_mirroredSkills = m_equipped;
    ++m_level;
}

int RunState::extraCards() const
{
    return getAllCharacters()[m_charId].extraCards;
}

bool RunState::hasSkill(int id) const
{
    return std::find(m_acquired.begin(), m_acquired.end(), id) != m_acquired.end();
}

void RunState::addSkill(int id)
{
    if (!hasSkill(id))
        m_acquired.push_back(id);
}

void RunState::equipSkill(int slotIdx, int skillId)
{
    if (slotIdx < 0 || slotIdx >= MAX_SKILL_SLOTS) return;
    // 防止同一技能出现在多个槽位
    for (int i = 0; i < MAX_SKILL_SLOTS; ++i)
        if (i != slotIdx && m_equipped[i] == skillId)
            m_equipped[i] = -1;
    m_equipped[slotIdx] = skillId;
}

void RunState::unequipSlot(int slotIdx)
{
    if (slotIdx >= 0 && slotIdx < MAX_SKILL_SLOTS)
        m_equipped[slotIdx] = -1;
}

int RunState::equippedCount() const
{
    int n = 0;
    for (int id : m_equipped)
        if (id >= 0) ++n;
    return n;
}

int RunState::equippedSlotOf(int skillId) const
{
    for (int i = 0; i < MAX_SKILL_SLOTS; ++i)
        if (m_equipped[i] == skillId)
            return i;
    return -1;
}

void RunState::swapSlots(int a, int b)
{
    if (a >= 0 && a < MAX_SKILL_SLOTS && b >= 0 && b < MAX_SKILL_SLOTS)
        std::swap(m_equipped[a], m_equipped[b]);
}

std::vector<int> RunState::rollRewardSkills()
{
    // 技能系统重新设计后暂时为空
    if (SKILL_COUNT == 0) return {};
    std::vector<int> pool;
    for (int i = 0; i < SKILL_COUNT; ++i)
        pool.push_back(i);

    std::shuffle(pool.begin(), pool.end(), m_rng);
    pool.resize(3);
    return pool;
}

void RunState::addScore(int score)
{
    m_roundScore = score;
    m_totalScore += score;
}

void RunState::updateHighScore()
{
    if (m_totalScore > m_highScore) {
        m_highScore = m_totalScore;
        saveHighScore();
    }
}

void RunState::loadHighScore()
{
    std::ifstream f("highscore.txt");
    if (f.is_open()) {
        f >> m_highScore;
        f.close();
    }
}

void RunState::saveHighScore() const
{
    std::ofstream f("highscore.txt");
    if (f.is_open()) {
        f << m_highScore;
        f.close();
    }
}
