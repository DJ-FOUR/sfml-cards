#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
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
                    float dt,
                    int charId,
                    int roundScore,
                    int totalScore);
    int hitTestCard(const sf::Vector2f& worldPos,
                    int cardCount, sf::Vector2u winSize,
                    const std::vector<int>& selectedIndices) const;
    int hitTestGameButton(const sf::Vector2f& worldPos,
                          bool canPass, sf::Vector2u winSize) const;
    int hitTestMomentumButton(const sf::Vector2f& worldPos, sf::Vector2u winSize) const;
    int hitTestScheduleButton(const sf::Vector2f& worldPos, sf::Vector2u winSize) const;
    int hitTestSkillSlot(const sf::Vector2f& worldPos, sf::Vector2u winSize) const;
    int hitTestDebugButton(const sf::Vector2f& worldPos, sf::Vector2u winSize) const;
    int hitTestCharPortrait(const sf::Vector2f& worldPos, sf::Vector2u winSize,
                            int charId, const GameState& state) const;

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
                        const std::array<int, MAX_SKILL_SLOTS>& enemySkills,
                        int hoveredAcquiredIdx, int hoveredSlotIdx,
                        int dragSourceType, int dragSourceIndex,
                        int dragSkillId, bool isDragging);
    int  hitTransitionSlot(const sf::Vector2f& pos, sf::Vector2u winSize);
    int  hitTransitionFight(const sf::Vector2f& pos, sf::Vector2u winSize);
    // 卡池命中检测
    int  hitTransitionPoolCard(const sf::Vector2f& pos, sf::Vector2u winSize,
                               int acquiredCount);
    bool hitTransitionPool(const sf::Vector2f& pos, sf::Vector2u winSize);
    sf::FloatRect transitionPoolCardRect(int cardIndex, sf::Vector2u winSize) const;
    sf::Vector2f transitionSlotCenter(int slotIndex, sf::Vector2u winSize) const;

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

    // 技能槽点击反馈动画 (toggle)
    void setSkillSlotLifted(int slotIndex, bool lifted);
    void resetSkillSlotAnims();

    // 发牌动画
    void startDealAnimation(int cardCount);
    bool isDealAnimating() const { return m_dealActive; }

    // 炸弹生成动画 (新卡牌飞入)
    void startBombDealAnimation(const std::vector<int>& cardIndices);

    // 音量控制
    void setMusicVolume(float v);
    void setSoundVolume(float v);
    float musicVolume() const { return m_musicVolume; }
    float soundVolume() const { return m_soundVolume; }

    // 设置弹窗
    void drawSettingsPopup(sf::Vector2u winSize, const sf::Vector2f& mousePos,
                           bool draggingMusic, bool draggingSound);
    struct SettingsHitResult { int action = 0; float sliderVal = 0.f; };
    SettingsHitResult hitTestSettings(const sf::Vector2f& pos, sf::Vector2u winSize);
    int hitTestSettingsButton(const sf::Vector2f& pos, sf::Vector2u winSize);

    // 积分显示
    void drawHighScore(sf::Vector2u winSize, int highScore);
    void drawTotalScore(sf::Vector2u winSize, int totalScore);
    void drawRoundScore(sf::Vector2u winSize, int roundScore, int totalScore);
    void startScoreAnim(const std::vector<Card>& enemyCards, int score);
    void advanceScoreAnim();  // 点击推进到飞行阶段
    bool isScoreAnimating() const;

    // 过渡界面 — 飞牌动画 (双击装备/卸下)
    void startTransitionFly(sf::Vector2f src, sf::Vector2f dst, int skillId,
                            bool toSlot, int poolIdx, int slotIdx);
    bool isTransitionFlyDone() const { return m_flyProgress < 0.f; }
    int  transitionFlySkillId() const { return m_flySkillId; }
    bool transitionFlyToSlot() const { return m_flyToSlot; }

private:
    sf::RenderWindow& m_window;
    sf::Font m_font;

    std::unordered_map<int, sf::Texture> m_faceTextures;
    sf::Texture m_backTexture;
    sf::Texture m_bgTexture;
    sf::Texture m_gameBgTexture;
    sf::Texture m_battleBgTexture;     // 对战界面专用背景
    sf::Texture m_charTextures[CHAR_COUNT];        // 角色选择立绘 (char0/1/2.png)
    sf::Texture m_battleCharTextures[CHAR_COUNT]; // 对战立绘 (char_0/1/_2.png)
    sf::Texture m_skillTextures[SKILL_COUNT];     // 技能卡牌图片 (skill00/01/02.png)
    sf::Music   m_bgMusic;
    sf::SoundBuffer m_hoverSndBuf;
    std::unique_ptr<sf::Sound> m_hoverSnd;
    float m_musicVolume = 100.f;
    float m_soundVolume = 100.f;

    // 卡牌悬停音效 — 记录上一帧悬停目标，仅在 hover 进入时播放
    int m_prevCharHoveredIdx  = -1;
    int m_prevRewardHoveredIdx = -1;
    int m_prevHandHoveredIdx  = -1;
    int m_prevPoolHoveredIdx  = -1;
    int m_prevSlotHoveredIdx  = -1;

    void playHoverTick();
    std::unique_ptr<sf::Text> m_playerLabel;
    std::unique_ptr<sf::Text> m_computerLabel;
    std::unique_ptr<sf::Text> m_statusText;
    std::unique_ptr<sf::Text> m_playBtnText;
    std::unique_ptr<sf::Text> m_passBtnText;
    std::unique_ptr<sf::Text> m_returnBtnText;
    std::unique_ptr<sf::Text> m_skillBtnTexts[MAX_SKILL_SLOTS];

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
                    const sf::Vector2f& mousePos,
                    const std::vector<int>& selectedIndices);

    // ---- 菜单通用 ----
    sf::FloatRect menuButtonRect(int idx, int total, sf::Vector2u winSize) const;
    void drawMenuButton(const sf::FloatRect& rect, const sf::String& text,
                        bool enabled, bool hover, sf::Vector2u winSize);
    void drawTitle(const sf::String& text, float yRatio, sf::Vector2u winSize);

    void drawBackground(sf::Vector2u winSize, bool useGameBg = false);
    void drawBackButton(sf::Vector2u winSize, const sf::Vector2f& mousePos);

    // 通用: 技能卡片
    void drawSkillCard(float x, float y, float w, float h,
                       int skillId, bool owned, bool hover, sf::Vector2u winSize,
                       sf::RenderTarget* target = nullptr);
    sf::FloatRect skillCardRect(int idx, int total, sf::Vector2u winSize) const;

    // 角色立绘悬停提示
    void drawCharTooltip(float w, float h, sf::FloatRect charRect,
                         const std::wstring& passiveName, const std::wstring& passiveDesc);

    // ---------- 角色卡片悬停交互 ----------
    static constexpr unsigned CHAR_RT_W = 500;
    static constexpr unsigned CHAR_RT_H = 857;  // 与 char0/1/2.png 分辨率一致，避免模糊
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
    float m_skillSlotY[MAX_SKILL_SLOTS]{};         // 技能槽Y偏移当前值
    float m_skillSlotTarget[MAX_SKILL_SLOTS]{};    // 技能槽Y偏移目标值
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

    // 炸弹生成动画 (新卡牌飞入)
    std::vector<CardDealAnim> m_bombDealAnim;
    bool m_bombDealActive = false;
    float m_bombDealTimer = 0.f;
    std::vector<int> m_bombCardIndices;   // 炸弹牌在手牌中的实际索引 (动画目标)
    std::vector<int> m_prevHandImgIndices; // 上一帧手牌的imageIndex列表

    // 过渡界面 — 卡池悬停动效 (当前使用 drawSkillCard 内建 hover)

    // 过渡界面 — 双击飞牌动画
    float m_flyProgress = -1.f;    // <0 = 不活跃, 0→1 = 飞行中
    sf::Vector2f m_flySrc, m_flyDst;
    int  m_flySkillId = -1;
    int  m_flyPoolIdx = -1;        // 飞牌来源: 卡池索引
    int  m_flySlotIdx = -1;        // 飞牌来源: 装备槽索引
    bool m_flyToSlot = true;

    float m_shakeTimer = 0.f;
    float m_momentumAnimTimer = 0.f;   // 连击之势触发动画计时
    float m_scheduleAnimTimer = 0.f;   // 调度触发动画计时
    float m_scheduleFlyProgress = -1.f; // 调度立绘飞行动画: <0=不活跃, 0→1=飞行中
    bool  m_wasSchedulePlay = false;    // 上一帧是否在调度阶段

    // 胜利积分动画
    int   m_scoreAnimPhase = -1;        // -1=不活跃, 0=翻牌, 1=等待点击, 2=飞行
    float m_scoreAnimTimer = 0.f;
    float m_scoreFlipEndTime = 0.f;
    std::vector<Card> m_scoreAnimCards;  // 敌人剩余手牌
    int   m_scoreAnimValue = 0;         // 本局得分
    bool  m_pendingScheduleFly = false; // 卡牌动画结束后再启动立绘飞行
    GameState::Phase m_prevPhase = GameState::Phase::PlayerTurn;

    // 角色立绘悬停/点击
    float m_charPortraitLift = 0.f;   // 悬停上移量 (lerp)
    bool  m_charTooltipOpen = false;  // 点击切换被动描述面板
public:
    void toggleCharTooltip() { m_charTooltipOpen = !m_charTooltipOpen; }
    void closeCharTooltip() { m_charTooltipOpen = false; }
private:

    static constexpr float DEAL_STAGGER   = 0.08f;
    static constexpr float DEAL_DURATION  = 0.60f;
    static constexpr float DEAL_INIT_SCL  = 0.50f;
    static constexpr float DEAL_INIT_YOFF = 200.f;
};
