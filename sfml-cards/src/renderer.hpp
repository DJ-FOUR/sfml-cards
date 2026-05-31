#pragma once

#include <SFML/Graphics.hpp>
#include "card.hpp"
#include "game_state.hpp"
#include "skill.hpp"
#include "character.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>

class Renderer
{
public:
    static constexpr int   DEFAULT_W = 1200;
    static constexpr int   DEFAULT_H = 750;
    static constexpr float CARD_W = 105.f;
    static constexpr float CARD_H = 150.f;

    explicit Renderer(sf::RenderWindow& window);

    bool initialize(const std::string& imageDir, const std::string& fontPath);

    // ---- 游戏 (改造: 技能UI) ----
    void renderGame(const GameState& state,
                    const std::vector<int>& selectedIndices,
                    sf::Vector2u winSize,
                    bool canPlaySelected,
                    const std::array<int, MAX_SKILL_SLOTS>& playerSkillIds,
                    const sf::Vector2f& mousePos,
                    float dt);
    int hitTestCard(const sf::Vector2f& worldPos,
                    int cardCount, sf::Vector2u winSize,
                    const std::vector<int>& selectedIndices) const;
    int hitTestGameButton(const sf::Vector2f& worldPos,
                          bool canPass, sf::Vector2u winSize) const;
    int hitTestSkillSlot(const sf::Vector2f& worldPos, sf::Vector2u winSize) const;
    int hitTestDebugButton(const sf::Vector2f& worldPos, sf::Vector2u winSize) const;

    // ---- 主菜单 ----
    void drawMainMenu(sf::Vector2u winSize, const sf::Vector2f& mousePos);
    int  hitMainMenu(const sf::Vector2f& pos, sf::Vector2u winSize);

    // ---- 角色选择 ----
    void drawCharacterSelect(sf::Vector2u winSize, const sf::Vector2f& mousePos);
    int  hitCharacterSelect(const sf::Vector2f& pos, sf::Vector2u winSize);

    // ---- 癞子点数选择 (谋略家被动) ----
    void drawWildcardSelect(sf::Vector2u winSize, const sf::Vector2f& mousePos);
    int  hitWildcardSelect(const sf::Vector2f& pos, sf::Vector2u winSize);

    // ---- 关卡过渡 (每关前装备技能) ----
    void drawTransition(sf::Vector2u winSize, const sf::Vector2f& mousePos,
                        int level, const std::vector<int>& acquiredSkills,
                        const std::array<int, MAX_SKILL_SLOTS>& equipped,
                        int hoveredAcquiredIdx, int hoveredSlotIdx);
    int  hitTransitionSkill(const sf::Vector2f& pos, sf::Vector2u winSize,
                            int acquiredCount, int equippedCount);
    int  hitTransitionSlot(const sf::Vector2f& pos, sf::Vector2u winSize);
    int  hitTransitionFight(const sf::Vector2f& pos, sf::Vector2u winSize);

    // ---- 奖励界面 (过关后选技能) ----
    void drawReward(sf::Vector2u winSize, const sf::Vector2f& mousePos,
                    const std::vector<int>& skillIds,
                    const std::vector<int>& acquiredSkills);
    int  hitReward(const sf::Vector2f& pos, sf::Vector2u winSize);

    // ---- 失败界面 ----
    void drawGameOver(sf::Vector2u winSize, const sf::Vector2f& mousePos,
                      int levelReached, int skillCount);
    int  hitGameOver(const sf::Vector2f& pos, sf::Vector2u winSize);

    // 角色卡片悬停动画更新
    void updateAnimations(float dt);

    // 发牌动画
    void startDealAnimation(int cardCount);
    bool isDealAnimating() const { return m_dealActive; }

private:
    sf::RenderWindow& m_window;
    sf::Font m_font;

    std::unordered_map<int, sf::Texture> m_faceTextures;
    sf::Texture m_backTexture;
    sf::Texture m_bgTexture;

    // 游戏UI
    std::unique_ptr<sf::Text> m_playerLabel;
    std::unique_ptr<sf::Text> m_computerLabel;
    std::unique_ptr<sf::Text> m_statusText;
    std::unique_ptr<sf::Text> m_playBtnText;
    std::unique_ptr<sf::Text> m_passBtnText;
    std::unique_ptr<sf::Text> m_returnBtnText;
    std::unique_ptr<sf::Text> m_skillBtnTexts[MAX_SKILL_SLOTS];

    sf::RectangleShape m_playBtn;
    sf::RectangleShape m_passBtn;
    sf::RectangleShape m_returnBtn;
    sf::RectangleShape m_skillBtns[MAX_SKILL_SLOTS];

    // ---- 动态布局 ----
    float handScale(float h) const;
    float playedScale(float h) const;
    float handCardY(float h) const;
    float computerHandY(float h) const;
    float computerPlayedY(float h) const;
    float playerPlayedY(float h) const;
    sf::Vector2f handCardPos(int index, int total,
                             float yBase, sf::Vector2u winSize) const;

    // ---- 基础绘制 ----
    void drawCard(const Card& card, float x, float y, float scale, bool faceUp);
    void drawCardBack(float x, float y, float scale);
    void drawPlayedCards(const std::vector<Card>& cards, float yCenter,
                         float scale, sf::Vector2u winSize);
    void drawGameUI(const GameState& state, bool canPass, bool canPlaySelected,
                    sf::Vector2u winSize,
                    const std::array<int, MAX_SKILL_SLOTS>& playerSkillIds,
                    const sf::Vector2f& mousePos);

    // ---- 菜单通用 ----
    sf::FloatRect menuButtonRect(int idx, int total, sf::Vector2u winSize) const;
    void drawMenuButton(const sf::FloatRect& rect, const sf::String& text,
                        bool enabled, bool hover, sf::Vector2u winSize);
    void drawTitle(const sf::String& text, float yRatio, sf::Vector2u winSize);

    void drawBackground(sf::Vector2u winSize);
    void drawBackButton(sf::Vector2u winSize, const sf::Vector2f& mousePos);

    // 通用: 技能卡片
    void drawSkillCard(float x, float y, float w, float h,
                       int skillId, bool owned, bool hover, sf::Vector2u winSize);
    sf::FloatRect skillCardRect(int idx, int total, sf::Vector2u winSize) const;

    // ---------- 角色卡片悬停交互 ----------
    static constexpr unsigned CHAR_RT_W = 280;
    static constexpr unsigned CHAR_RT_H = 480;
    sf::RenderTexture m_charRT[CHAR_COUNT];

    struct HoverAnimState {
        float currentYOffset = 0.0f;
        float currentScale   = 1.0f;
        float targetYOffset  = 0.0f;
        float targetScale    = 1.0f;
    };
    HoverAnimState m_charHover[CHAR_COUNT];
    HoverAnimState m_skillHover[3];
    std::vector<float> m_handCardYOffsets;
    std::vector<float> m_handCardTargets;
    float m_playBtnHoverScale   = 1.0f;
    float m_passBtnHoverScale   = 1.0f;
    float m_playBtnTargetScale  = 1.0f;
    float m_passBtnTargetScale  = 1.0f;

    void renderCharCardToRT(int charIdx, bool hover);

    // ---------- 发牌动画 ----------
    struct CardDealAnim {
        float delay = 0.f;
        float progress = 0.f;
        bool started = false;
    };
    std::vector<CardDealAnim> m_dealAnim;
    bool m_dealActive = false;
    float m_dealTimer = 0.f;
    float m_shakeTimer = 0.f;

    static constexpr float DEAL_STAGGER   = 0.08f;
    static constexpr float DEAL_DURATION  = 0.60f;
    static constexpr float DEAL_INIT_SCL  = 0.50f;
    static constexpr float DEAL_INIT_YOFF = 200.f;
};
