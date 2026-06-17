#include "renderer.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}
float easeOutCubic(float t) {
    t = std::clamp(t, 0.f, 1.f);
    return 1.0f - std::pow(1.0f - t, 3.0f);
}

sf::Color btnColor(10, 10, 10);
sf::Color btnDisabledColor(8, 8, 8);
sf::Color btnHoverColor(26, 42, 10);
sf::Color skillCardColor(13, 13, 13);
sf::Color skillCardHover(20, 30, 10);
sf::Color skillCardOwned(8, 8, 8);
sf::Color slotEmptyColor(17, 17, 17);
sf::Color slotFilledColor(20, 30, 10);
// 新风格主色
constexpr sf::Color NEON_GREEN(204, 255, 0);
constexpr sf::Color DARK_BG(250, 250, 250);
constexpr sf::Color PANEL_BG(13, 13, 13);
constexpr sf::Color BORDER_NORMAL(51, 51, 51);
constexpr sf::Color TEXT_DIM(170, 170, 170);
constexpr sf::Color TEXT_DISABLED(85, 85, 85);
constexpr sf::Color ENEMY_RED(255, 51, 51);
constexpr sf::Color DARK_RED_BG(26, 10, 10);

// ====== 街头潮流风格配色 ======
constexpr sf::Color STREET_PINK   (255, 20, 147);   // 荧光粉
constexpr sf::Color STREET_CYAN   (0, 230, 255);    // 青蓝
constexpr sf::Color STREET_YELLOW (255, 200, 0);    // 橙黄
constexpr sf::Color STREET_BLACK  (10, 10, 10);     // 极黑底
constexpr sf::Color OUTLINE_BLACK (0, 0, 0);        // 纯黑描边
constexpr sf::Color STREET_WHITE  (255, 255, 255);  // 纯白
constexpr sf::Color BATTLE_BLUE   (50, 120, 255);   // 对战界面蓝色文字

// 根据技能类型返回街头潮流色
sf::Color skillTypeStreetColor(SkillType t) {
    switch (t) {
        case SkillType::BUFF:    return STREET_PINK;
        case SkillType::TRIGGER: return STREET_CYAN;
        case SkillType::PASSIVE: return STREET_YELLOW;
    }
    return STREET_WHITE;
}

// 技能类型 → 显示标签
std::wstring skillTypeLabel(SkillType t) {
    switch (t) {
        case SkillType::BUFF:    return L"BUFF";
        case SkillType::TRIGGER: return L"TRIG";
        case SkillType::PASSIVE: return L"PASV";
    }
    return L"";
}

// 绘制技能八角形图标框
void drawOctagonIcon(sf::RenderTarget& window,
    float iconX, float iconY, float iconSize,
    sf::Color fillColor, sf::Color outlineColor, float outlineThickness = 1.f)
{
    float ic = iconSize * 0.15f;
    sf::ConvexShape oct(8);
    oct.setPoint(0, {iconX + ic, iconY});
    oct.setPoint(1, {iconX + iconSize - ic, iconY});
    oct.setPoint(2, {iconX + iconSize, iconY + ic});
    oct.setPoint(3, {iconX + iconSize, iconY + iconSize - ic});
    oct.setPoint(4, {iconX + iconSize - ic, iconY + iconSize});
    oct.setPoint(5, {iconX + ic, iconY + iconSize});
    oct.setPoint(6, {iconX, iconY + iconSize - ic});
    oct.setPoint(7, {iconX, iconY + ic});
    oct.setFillColor(fillColor);
    oct.setOutlineColor(outlineColor);
    oct.setOutlineThickness(outlineThickness);
    window.draw(oct);
}

// 绘制切角矩形（八边形）
void drawBeveledRect(sf::RenderTarget& window, float x, float y, float w, float h, float cut,
                     sf::Color fill, sf::Color outline, float outlineThick)
{
    sf::ConvexShape shape(8);
    shape.setPoint(0, {x + cut, y});
    shape.setPoint(1, {x + w - cut, y});
    shape.setPoint(2, {x + w, y + cut});
    shape.setPoint(3, {x + w, y + h - cut});
    shape.setPoint(4, {x + w - cut, y + h});
    shape.setPoint(5, {x + cut, y + h});
    shape.setPoint(6, {x, y + h - cut});
    shape.setPoint(7, {x, y + cut});
    shape.setFillColor(fill);
    if (outlineThick > 0.f) {
        shape.setOutlineColor(outline);
        shape.setOutlineThickness(outlineThick);
    }
    window.draw(shape);
}

// 绘制警示斜条纹
void drawHazardStripes(sf::RenderWindow& window, float x, float y, float w, float h, float stripeW)
{
    sf::RenderTexture rt;
    if (!rt.resize({(unsigned)w, (unsigned)h})) return;
    rt.clear(sf::Color::Transparent);

    sf::RectangleShape bg({w, h});
    bg.setFillColor(sf::Color(204, 255, 0));
    rt.draw(bg);

    sf::RectangleShape blk({stripeW * 1.414f, h * 2.f});
    blk.setFillColor(sf::Color::Black);
    for (float ox = -h; ox < w + h; ox += stripeW * 2.f) {
        blk.setPosition({ox, -h / 2.f});
        blk.setRotation(sf::degrees(45.f));
        rt.draw(blk);
        blk.setRotation(sf::degrees(0.f));
    }
    rt.display();
    sf::Sprite sp(rt.getTexture());
    sp.setPosition({x, y});
    window.draw(sp);
}

// 绘制战术网格背景
void drawTacticalGrid(sf::RenderWindow& window, sf::Vector2u winSize)
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;
    window.clear(DARK_BG);

    // 扫描线
    for (float y = 0; y < h; y += 4.f) {
        sf::RectangleShape line({w, 1.f});
        line.setPosition({0.f, y});
        line.setFillColor(sf::Color(230, 230, 230, 30));
        window.draw(line);
    }

    // 网格
    sf::Color gridCol(200, 200, 200, 100);
    for (float x = 0; x < w; x += 80.f) {
        sf::RectangleShape vl({1.f, h});
        vl.setPosition({x, 0.f});
        vl.setFillColor(gridCol);
        window.draw(vl);
    }
    for (float y = 0; y < h; y += 80.f) {
        sf::RectangleShape hl({w, 1.f});
        hl.setPosition({0.f, y});
        hl.setFillColor(gridCol);
        window.draw(hl);
    }
}

// 绘制四角 L 形标记
void drawCornerMarkers(sf::RenderWindow& window, float x, float y, float w, float h,
                       float len, sf::Color col, float thick)
{
    sf::RectangleShape lines[8];
    // 左上
    lines[0].setSize({len, thick}); lines[0].setPosition({x, y});
    lines[1].setSize({thick, len}); lines[1].setPosition({x, y});
    // 右上
    lines[2].setSize({len, thick}); lines[2].setPosition({x + w - len, y});
    lines[3].setSize({thick, len}); lines[3].setPosition({x + w - thick, y});
    // 左下
    lines[4].setSize({len, thick}); lines[4].setPosition({x, y + h - thick});
    lines[5].setSize({thick, len}); lines[5].setPosition({x, y + h - len});
    // 右下
    lines[6].setSize({len, thick}); lines[6].setPosition({x + w - len, y + h - thick});
    lines[7].setSize({thick, len}); lines[7].setPosition({x + w - thick, y + h - len});
    for (auto& ln : lines) {
        ln.setFillColor(col);
        window.draw(ln);
    }
}

// ====== 街头潮流装饰绘制函数 ======

// 绘制五角星（带粗黑描边）
void drawStar(sf::RenderWindow& window, float cx, float cy, float r,
              sf::Color fillColor, float outlineThick = 3.f)
{
    sf::ConvexShape star(10);
    for (int i = 0; i < 10; ++i) {
        float angle = (i * 36.f - 90.f) * 3.14159265f / 180.f;
        float rad = (i % 2 == 0) ? r : r * 0.4f;
        star.setPoint(i, {cx + std::cos(angle) * rad, cy + std::sin(angle) * rad});
    }
    star.setFillColor(fillColor);
    if (outlineThick > 0.f) {
        star.setOutlineColor(OUTLINE_BLACK);
        star.setOutlineThickness(outlineThick);
    }
    window.draw(star);
}

// 绘制涂鸦喷漆滴落效果
void drawPaintDrip(sf::RenderWindow& window, float x, float y, float w, float h,
                   sf::Color color)
{
    // 主体椭圆
    sf::CircleShape drip(w / 2.f);
    drip.setPosition({x, y});
    drip.setFillColor(color);
    drip.setOutlineColor(OUTLINE_BLACK);
    drip.setOutlineThickness(2.f);
    window.draw(drip);
    // 滴落的小圆
    sf::CircleShape drop(w * 0.15f);
    drop.setPosition({x + w * 0.35f, y + h * 0.7f});
    drop.setFillColor(color);
    drop.setOutlineColor(OUTLINE_BLACK);
    drop.setOutlineThickness(1.5f);
    window.draw(drop);
}

// 绘制波点纹理（局部装饰）
void drawHalftoneDots(sf::RenderWindow& window, float x, float y, float w, float h,
                      float dotSize, sf::Color color)
{
    float spacing = dotSize * 2.5f;
    for (float dy = y + spacing; dy < y + h - spacing; dy += spacing) {
        for (float dx = x + spacing; dx < x + w - spacing; dx += spacing) {
            sf::CircleShape dot(dotSize);
            dot.setPosition({dx, dy});
            dot.setFillColor(color);
            window.draw(dot);
        }
    }
}

// 绘制锯齿/闪电形边框装饰线
void drawZigzagBorder(sf::RenderWindow& window, float x, float y, float w,
                      sf::Color color, float thick = 3.f, float amp = 6.f)
{
    sf::VertexArray zig(sf::PrimitiveType::TriangleStrip);
    int segments = (int)(w / 12.f);
    float step = w / segments;
    for (int i = 0; i <= segments; ++i) {
        float px = x + i * step;
        float py = y + ((i % 2 == 0) ? -amp : amp);
        sf::Vertex v1;
        v1.position = {px, py - thick / 2.f};
        v1.color = color;
        zig.append(v1);
        sf::Vertex v2;
        v2.position = {px, py + thick / 2.f};
        v2.color = color;
        zig.append(v2);
    }
    window.draw(zig);
}

// 绘制粗描边矩形（通用）
void drawThickOutlineRect(sf::RenderWindow& window, float x, float y, float w, float h,
                          sf::Color fill, sf::Color outline, float outlineThick)
{
    sf::RectangleShape rect({w, h});
    rect.setPosition({x, y});
    rect.setFillColor(fill);
    rect.setOutlineColor(outline);
    rect.setOutlineThickness(outlineThick);
    window.draw(rect);
}

// 根据角色ID返回街头潮流色
sf::Color charStreetColor(int charIdx) {
    switch (charIdx) {
        case 0: return STREET_PINK;
        case 1: return STREET_CYAN;
        case 2: return STREET_YELLOW;
    }
    return STREET_WHITE;
}

} // namespace

Renderer::Renderer(sf::RenderWindow& window)
    : m_window(window)
{
}

// ====== 动态尺度 ======

float Renderer::handScale(float h)        const { return h * 0.135f / CARD_H; }
float Renderer::playedScale(float h)      const { return h * 0.165f / CARD_H; }
float Renderer::handCardY(float h)        const { return h * 0.66f; }
float Renderer::computerHandY(float h)    const { return h * 0.05f; }
float Renderer::computerPlayedY(float h)  const { return h * 0.27f; }
float Renderer::playerPlayedY(float h)    const { return h * 0.45f; }

// ====== 初始化 ======

bool Renderer::initialize(const std::string& imageDir, const std::string& fontPath)
{
    if (!m_font.openFromFile(fontPath)) {
        if (!m_font.openFromFile("C:/Windows/Fonts/msyh.ttc")) {
            std::fprintf(stderr, "Failed to load font\n");
            return false;
        }
    }

    char buf[512];
    for (int i = 0; i < 54; ++i) {
        std::snprintf(buf, sizeof(buf), "%s/card%d.png", imageDir.c_str(), i);
        sf::Texture tex;
        if (!tex.loadFromFile(buf)) {
            std::fprintf(stderr, "Failed to load: %s\n", buf);
            return false;
        }
        tex.setSmooth(true);
        m_faceTextures[i] = std::move(tex);
    }

    if (!m_backTexture.loadFromFile("images/character-card/card00.png")) {
        std::fprintf(stderr, "Failed to load card back\n");
        return false;
    }
    m_backTexture.setSmooth(true);

    if (!m_bgTexture.loadFromFile("images/background/start.png")) {
        std::fprintf(stderr, "Failed to load background\n");
        return false;
    }
    m_bgTexture.setSmooth(true);

    if (!m_gameBgTexture.loadFromFile("images/background/back1.png")) {
        std::fprintf(stderr, "Failed to load game background\n");
        return false;
    }
    m_gameBgTexture.setSmooth(true);

    if (!m_battleBgTexture.loadFromFile("images/character-card/背景.jpg")) {
        std::fprintf(stderr, "Failed to load battle background\n");
        return false;
    }
    m_battleBgTexture.setSmooth(true);

    // 背景音乐
    if (!m_bgMusic.openFromFile("resources/music/first.mp3")) {
        std::fprintf(stderr, "Failed to load music\n");
        return false;
    }
    m_bgMusic.setLooping(true);
    m_bgMusic.play();

    // 角色选择立绘 (char0/1/2.png)
    for (int i = 0; i < CHAR_COUNT; ++i) {
        std::snprintf(buf, sizeof(buf), "images/character-card/char%d.png", i);
        if (!m_charTextures[i].loadFromFile(buf)) {
            std::fprintf(stderr, "Failed to load character texture: %s\n", buf);
            return false;
        }
        m_charTextures[i].setSmooth(true);
    }

    // 技能卡牌图片 (skill00/01/03.png)
    {
        const char* skillFiles[SKILL_COUNT] = {
            "images/character-card/skill00.png",
            "images/character-card/skill01.png",
            "images/character-card/skill03.png",
        };
        for (int i = 0; i < SKILL_COUNT; ++i) {
            if (!m_skillTextures[i].loadFromFile(skillFiles[i])) {
                std::fprintf(stderr, "Failed to load skill texture: %s\n", skillFiles[i]);
                return false;
            }
            m_skillTextures[i].setSmooth(true);
        }
    }

    // 对战立绘 (char_0/1/_2.png)
    for (int i = 0; i < CHAR_COUNT; ++i) {
        std::snprintf(buf, sizeof(buf), "images/character-card/char_%d.png", i);
        if (!m_battleCharTextures[i].loadFromFile(buf)) {
            std::fprintf(stderr, "Failed to load battle character texture: %s\n", buf);
            return false;
        }
        m_battleCharTextures[i].setSmooth(true);
    }

    // 卡牌悬停音效
    if (!m_hoverSndBuf.loadFromFile("resources/sound/touch.mp3")) {
        std::fprintf(stderr, "Failed to load hover sound\n");
        return false;
    }
    m_hoverSnd = std::make_unique<sf::Sound>(m_hoverSndBuf);

    m_playerLabel   = std::make_unique<sf::Text>(m_font, L"玩家", 18);
    m_computerLabel = std::make_unique<sf::Text>(m_font, L"镜像AI", 18);
    m_statusText    = std::make_unique<sf::Text>(m_font, L"", 22);
    m_playBtnText   = std::make_unique<sf::Text>(m_font, L"出牌", 18);
    m_passBtnText   = std::make_unique<sf::Text>(m_font, L"不出", 18);
    m_returnBtnText = std::make_unique<sf::Text>(m_font, L"返回", 18);

    for (int i = 0; i < MAX_SKILL_SLOTS; ++i)
        m_skillBtnTexts[i] = std::make_unique<sf::Text>(m_font, L"", 16);

    for (auto* t : {m_playerLabel.get(), m_computerLabel.get(), m_statusText.get(),
                    m_playBtnText.get(), m_passBtnText.get(), m_returnBtnText.get()})
        t->setFillColor(sf::Color::White);
    for (int i = 0; i < MAX_SKILL_SLOTS; ++i)
        m_skillBtnTexts[i]->setFillColor(sf::Color::White);

    // 预创建角色卡片的渲染纹理
    for (int i = 0; i < CHAR_COUNT; ++i) {
        if (!m_charRT[i].resize({CHAR_RT_W, CHAR_RT_H})) {
            std::fprintf(stderr, "Failed to create char render texture %d\n", i);
            return false;
        }
    }

    return true;
}

// ---------- 动画更新 ----------

void Renderer::updateAnimations(float dt)
{
    if (dt > 0.05f) dt = 0.05f;
    const float SPEED = 14.0f;
    for (int i = 0; i < CHAR_COUNT; ++i) {
        auto& h = m_charHover[i];
        h.currentYOffset += (h.targetYOffset - h.currentYOffset) * SPEED * dt;
        h.currentScale   += (h.targetScale   - h.currentScale)   * SPEED * dt;
    }
    for (int i = 0; i < 3; ++i) {
        auto& h = m_skillHover[i];
        h.currentYOffset += (h.targetYOffset - h.currentYOffset) * SPEED * dt;
        h.currentScale   += (h.targetScale   - h.currentScale)   * SPEED * dt;
    }
    // 按钮悬停缩放动画
    constexpr float BTN_SPEED = 18.0f;
    m_playBtnHoverScale += (m_playBtnTargetScale - m_playBtnHoverScale) * BTN_SPEED * dt;
    m_passBtnHoverScale += (m_passBtnTargetScale - m_passBtnHoverScale) * BTN_SPEED * dt;
    // 手牌悬停浮动动画
    constexpr float CARD_SPEED = 16.0f;
    for (size_t i = 0; i < m_handCardTargets.size(); ++i) {
        if (i >= m_handCardYOffsets.size()) break;
        m_handCardYOffsets[i] += (m_handCardTargets[i] - m_handCardYOffsets[i]) * CARD_SPEED * dt;
    }

    // 技能槽点击反馈动画: 平滑到目标位置
    constexpr float SKSLOT_SPEED = 14.0f;
    for (int i = 0; i < MAX_SKILL_SLOTS; ++i)
        m_skillSlotY[i] += (m_skillSlotTarget[i] - m_skillSlotY[i]) * SKSLOT_SPEED * dt;

    m_shakeTimer += dt;

    // 发牌动画更新
    if (m_dealActive) {
        m_dealTimer += dt;
        bool allDone = true;
        for (auto& a : m_dealAnim) {
            if (!a.started && m_dealTimer >= a.delay) {
                a.started = true;
            }
            if (a.started && a.progress < 1.f) {
                a.progress += dt / DEAL_DURATION;
                if (a.progress > 1.f) a.progress = 1.f;
            }
            if (a.progress < 1.f) allDone = false;
        }
        if (allDone) {
            m_dealActive = false;
            m_dealAnim.clear();
        }
    }

    // 炸弹生成动画更新
    if (m_bombDealActive) {
        m_bombDealTimer += dt;
        bool allDone = true;
        for (auto& a : m_bombDealAnim) {
            if (!a.started && m_bombDealTimer >= a.delay) {
                a.started = true;
            }
            if (a.started && a.progress < 1.f) {
                a.progress += dt / DEAL_DURATION;
                if (a.progress > 1.f) a.progress = 1.f;
            }
            if (a.progress < 1.f) allDone = false;
        }
        if (allDone) {
            m_bombDealActive = false;
            m_bombDealAnim.clear();
            // 卡牌飞入结束 → 触发待执行的调度立绘飞行
            if (m_pendingScheduleFly) {
                m_scheduleFlyProgress = 0.f;
                m_pendingScheduleFly = false;
            }
        }
    }

    // 过渡界面飞牌动画更新
    if (m_flyProgress >= 0.f) {
        m_flyProgress += dt / 0.35f;  // 0.35s 飞行时间
        if (m_flyProgress >= 1.f)
            m_flyProgress = -1.f;      // 动画结束
    }

}

void Renderer::playHoverTick()
{
    if (!m_hoverSnd) return;
    m_hoverSnd->stop();
    m_hoverSnd->play();
}

void Renderer::setMusicVolume(float v)
{
    m_musicVolume = std::clamp(v, 0.f, 100.f);
    m_bgMusic.setVolume(m_musicVolume * 0.5f);
}

void Renderer::setSoundVolume(float v)
{
    m_soundVolume = std::clamp(v, 0.f, 100.f);
    if (m_hoverSnd) m_hoverSnd->setVolume(m_soundVolume);
}

void Renderer::startTransitionFly(sf::Vector2f src, sf::Vector2f dst, int skillId,
                                   bool toSlot, int poolIdx, int slotIdx)
{
    m_flySrc = src;
    m_flyDst = dst;
    m_flySkillId = skillId;
    m_flyToSlot = toSlot;
    m_flyPoolIdx = poolIdx;
    m_flySlotIdx = slotIdx;
    m_flyProgress = 0.f;
}

void Renderer::setSkillSlotLifted(int slotIndex, bool lifted)
{
    if (slotIndex >= 0 && slotIndex < MAX_SKILL_SLOTS)
        m_skillSlotTarget[slotIndex] = lifted ? -18.f : 0.f;
}

void Renderer::resetSkillSlotAnims()
{
    for (int i = 0; i < MAX_SKILL_SLOTS; ++i) {
        m_skillSlotTarget[i] = 0.f;
        m_skillSlotY[i] = 0.f;
    }
}

// ---------- 发牌动画 ----------

void Renderer::startDealAnimation(int cardCount)
{
    m_dealAnim.clear();
    m_dealAnim.resize(cardCount);
    for (int i = 0; i < cardCount; ++i) {
        m_dealAnim[i].delay    = i * DEAL_STAGGER;
        m_dealAnim[i].progress = 0.f;
        m_dealAnim[i].started  = false;
    }
    m_dealTimer  = 0.f;
    m_dealActive = true;
}

void Renderer::startBombDealAnimation(const std::vector<int>& cardIndices)
{
    int count = (int)cardIndices.size();
    m_bombDealAnim.clear();
    m_bombDealAnim.resize(count);
    for (int i = 0; i < count; ++i) {
        m_bombDealAnim[i].delay    = i * DEAL_STAGGER;
        m_bombDealAnim[i].progress = 0.f;
        m_bombDealAnim[i].started  = false;
    }
    m_bombCardIndices = cardIndices;
    m_bombDealTimer   = 0.f;
    m_bombDealActive  = true;
}

// 将单张角色卡片的内容绘制到预分配的 RenderTexture
// 设计：全幅角色图片 + 底部白色区域写角色名
void Renderer::renderCharCardToRT(int charIdx, bool hover)
{
    auto& rt = m_charRT[charIdx];
    auto& c  = getAllCharacters()[charIdx];

    rt.clear(sf::Color::Transparent);

    float cw = (float)CHAR_RT_W;   // 210
    float ch = (float)CHAR_RT_H;   // 360

    // ====== 1. 全幅角色图片 ======
    {
        sf::Sprite charSprite(m_charTextures[charIdx]);
        auto texSz = m_charTextures[charIdx].getSize();
        float s = cw / (float)texSz.x;
        charSprite.setScale({s, s});
        rt.draw(charSprite);
    }

    // ====== 2. 角色名称（图片底部白色区域） ======
    float nameY = ch * 0.87f;
    float nameF = ch * 0.07f;
    sf::Text nameText(m_font, c.name, (unsigned)nameF);
    nameText.setFillColor(sf::Color::Black);
    nameText.setStyle(sf::Text::Bold);
    auto nsz = nameText.getGlobalBounds().size;
    nameText.setPosition({(cw - nsz.x) / 2.f, nameY});
    rt.draw(nameText);

    rt.display();
}

// ====== 通用: 背景 ======

void Renderer::drawBackground(sf::Vector2u winSize, bool useGameBg)
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;

    auto& bgTex = useGameBg ? m_gameBgTexture : m_bgTexture;

    // 背景图片铺满窗口
    if (bgTex.getSize().x > 0) {
        sf::Sprite bgSprite(bgTex);
        float bgScale = std::max(w / (float)bgTex.getSize().x,
                                 h / (float)bgTex.getSize().y);
        bgSprite.setScale({bgScale, bgScale});
        bgSprite.setPosition({(w - bgTex.getSize().x * bgScale) / 2.f,
                              (h - bgTex.getSize().y * bgScale) / 2.f});
        m_window.draw(bgSprite);
    } else {
        m_window.clear(sf::Color(10, 10, 10));
    }

    // 版本号
    sf::Text ver(m_font, L"v1.0.0_OS", 14);
    ver.setFillColor(sf::Color(160, 160, 160));
    auto vsz = ver.getGlobalBounds().size;
    ver.setPosition({w - vsz.x - 16.f, h - vsz.y - 12.f});
    m_window.draw(ver);
}

// ====== 通用: 返回按钮 ======

void Renderer::drawBackButton(sf::Vector2u winSize, const sf::Vector2f& mousePos)
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;
    float gbSz = h * 0.04f;
    float gbX = w * 0.03f;
    float gbY = h * 0.03f;
    bool gHover = sf::FloatRect({gbX, gbY}, {gbSz, gbSz}).contains(mousePos);

    sf::Color btnColor(10, 10, 10);
    drawBeveledRect(m_window, gbX, gbY, gbSz, gbSz, 3.f,
                    btnColor, gHover ? STREET_CYAN : OUTLINE_BLACK, 2.f);

    // 齿轮图标: 中心圆 + 外围齿
    float gcX = gbX + gbSz / 2.f;
    float gcY = gbY + gbSz / 2.f;
    float gR = gbSz * 0.28f;
    sf::CircleShape gearCenter(gR);
    gearCenter.setOrigin({gR, gR});
    gearCenter.setPosition({gcX, gcY});
    gearCenter.setFillColor(sf::Color::Transparent);
    gearCenter.setOutlineColor(gHover ? STREET_CYAN : TEXT_DIM);
    gearCenter.setOutlineThickness(1.5f);
    m_window.draw(gearCenter);
    for (int j = 0; j < 8; ++j) {
        float ang = j * 3.14159265f * 2.f / 8.f;
        float ir = gR + 2.f;
        float or2 = gR + 5.f;
        sf::RectangleShape tooth({or2 - ir, 2.f});
        tooth.setOrigin({tooth.getSize().x / 2.f, 1.f});
        tooth.setPosition({gcX + std::cos(ang) * (ir + or2) / 2.f,
                           gcY + std::sin(ang) * (ir + or2) / 2.f});
        tooth.setRotation(sf::degrees(ang * 180.f / 3.14159265f + 90.f));
        tooth.setFillColor(gHover ? STREET_CYAN : TEXT_DIM);
        m_window.draw(tooth);
    }
}

// ====== 通用: 菜单按钮 ======

sf::FloatRect Renderer::menuButtonRect(int idx, int total, sf::Vector2u winSize) const
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;
    float bw = w * 0.28f;
    float bh = h * 0.07f;
    float gap = h * 0.04f;
    float totalH = total * bh + (total - 1) * gap;
    float startY = (h - totalH) / 2.f;
    float x = (w - bw) / 2.f;
    float y = startY + idx * (bh + gap);
    return {{x, y}, {bw, bh}};
}

void Renderer::drawMenuButton(const sf::FloatRect& rect, const sf::String& text,
                               bool enabled, bool hover, sf::Vector2u winSize)
{
    (void)winSize;
    float offsetX = hover ? 4.f : 0.f;
    float x = rect.position.x + offsetX;
    float y = rect.position.y;
    float w = rect.size.x;
    float h = rect.size.y;
    float cut = 6.f;

    sf::Color fill = enabled ? (hover ? STREET_PINK : STREET_BLACK) : btnDisabledColor;
    sf::Color outline = enabled ? (hover ? OUTLINE_BLACK : OUTLINE_BLACK) : sf::Color(34, 34, 34);
    float outlineThick = hover ? 4.f : 3.f;

    drawBeveledRect(m_window, x, y, w, h, cut, fill, outline, outlineThick);

    // 左侧五角星指示器（悬停时）
    if (hover && enabled) {
        drawStar(m_window, x + 18.f, y + h / 2.f, 7.f, STREET_YELLOW, 2.f);
    }

    // 文字粗黑描底（偏移 3px）
    sf::Text shadow(m_font, text, (unsigned)(h * 0.45f));
    shadow.setFillColor(OUTLINE_BLACK);
    auto sz = shadow.getGlobalBounds().size;
    shadow.setPosition({x + (w - sz.x) / 2.f + 3.f,
                        y + (h - sz.y) / 2.f - sz.y * 0.15f + 3.f});
    m_window.draw(shadow);

    sf::Text label(m_font, text, (unsigned)(h * 0.45f));
    label.setFillColor(enabled ? STREET_WHITE : TEXT_DISABLED);
    label.setStyle(sf::Text::Bold);
    label.setPosition({x + (w - sz.x) / 2.f,
                       y + (h - sz.y) / 2.f - sz.y * 0.15f});
    m_window.draw(label);
}

void Renderer::drawTitle(const sf::String& text, float yRatio, sf::Vector2u winSize)
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;
    float fontSize = h * 0.08f;

    // 解析双语：中文 // 英文
    std::wstring full = text.toWideString();
    std::wstring cn = full;
    std::wstring en;
    size_t pos = full.find(L" // ");
    if (pos != std::wstring::npos) {
        cn = full.substr(0, pos);
        en = full.substr(pos + 4);
    }

    // 标题粗黑描底（偏移 4px）
    sf::Text titleShadow(m_font, cn, (unsigned)fontSize);
    titleShadow.setFillColor(OUTLINE_BLACK);
    titleShadow.setStyle(sf::Text::Bold);
    auto sz = titleShadow.getGlobalBounds().size;
    float tx = (w - sz.x) / 2.f;
    float ty = h * yRatio;
    titleShadow.setPosition({tx + 4.f, ty + 4.f});
    m_window.draw(titleShadow);

    sf::Text title(m_font, cn, (unsigned)fontSize);
    title.setFillColor(STREET_WHITE);
    title.setStyle(sf::Text::Bold);
    title.setPosition({tx, ty});
    m_window.draw(title);

    // 英文副标题
    if (!en.empty()) {
        sf::Text sub(m_font, en, (unsigned)(fontSize * 0.35f));
        sub.setFillColor(TEXT_DIM);
        sub.setStyle(sf::Text::Italic);
        auto ssz = sub.getGlobalBounds().size;
        sub.setPosition({tx + sz.x + 12.f, ty + sz.y - ssz.y - 2.f});
        m_window.draw(sub);
    }

    // 荧光粉粗底线（6px）
    sf::RectangleShape underline({sz.x * 1.2f, 6.f});
    underline.setFillColor(STREET_PINK);
    underline.setPosition({tx - sz.x * 0.1f, ty + sz.y + 10.f});
    m_window.draw(underline);
}

// ====== 通用: 技能卡片绘制 ======

sf::FloatRect Renderer::skillCardRect(int idx, int total, sf::Vector2u winSize) const
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;
    float ch = h * 0.42f;                       // 高度占窗口 42%
    float cw = ch * CARD_W / CARD_H;            // 宽基于高 7:12 卡牌比例
    float gap = w * 0.05f;
    float totalW = total * cw + (total - 1) * gap;
    float startX = (w - totalW) / 2.f;
    float y = h * 0.22f;                        // 下移到窗口 22% 处
    float x = startX + idx * (cw + gap);
    return {{x, y}, {cw, ch}};
}

void Renderer::drawSkillCard(float x, float y, float w, float h,
                              int skillId, bool owned, bool hover,
                              sf::Vector2u winSize, sf::RenderTarget* target)
{
    (void)winSize;
    auto& rt = target ? *target : m_window;

    if (skillId < 0 || skillId >= SKILL_COUNT) {
        drawBeveledRect(rt, x, y, w, h, 6.f, slotEmptyColor, BORDER_NORMAL, 1.f);
        return;
    }

    auto& sk = getAllSkills()[skillId];

    // 全幅技能图片作为卡牌（始终不透明）
    {
        auto& tex = m_skillTextures[skillId];
        sf::Sprite skSprite(tex);
        skSprite.setScale({w / (float)tex.getSize().x, h / (float)tex.getSize().y});
        skSprite.setPosition({x, y});
        if (owned) skSprite.setColor(sf::Color(180, 180, 180));
        rt.draw(skSprite);
    }

    // 黑色描边（常驻）
    {
        sf::RectangleShape border({w, h});
        border.setPosition({x, y});
        border.setFillColor(sf::Color::Transparent);
        border.setOutlineColor(OUTLINE_BLACK);
        border.setOutlineThickness(3.f);
        rt.draw(border);
    }
    // 悬停时叠加青蓝描边
    if (hover) {
        sf::RectangleShape border({w, h});
        border.setPosition({x, y});
        border.setFillColor(sf::Color::Transparent);
        border.setOutlineColor(STREET_CYAN);
        border.setOutlineThickness(2.f);
        rt.draw(border);
    }

    // 技能名称（底部居中）
    float nameF = h * 0.09f;
    sf::Text nameShadow(m_font, sk.name, (unsigned)nameF);
    nameShadow.setFillColor(OUTLINE_BLACK);
    nameShadow.setStyle(sf::Text::Bold);
    auto nsz = nameShadow.getGlobalBounds().size;
    nameShadow.setPosition({x + (w - nsz.x) / 2.f + 2.f, y + h * 0.88f + 2.f});
    rt.draw(nameShadow);
    sf::Text name(m_font, sk.name, (unsigned)nameF);
    name.setFillColor(STREET_WHITE);
    name.setStyle(sf::Text::Bold);
    name.setPosition({x + (w - nsz.x) / 2.f, y + h * 0.88f});
    rt.draw(name);
}

// ====== 主菜单 ======

void Renderer::drawMainMenu(sf::Vector2u winSize, const sf::Vector2f& mousePos)
{
    drawBackground(winSize);
    // [IMG-LOGO] 标题区
    drawTitle(L"斗牌ROGUE", 0.15f, winSize);

    auto r1 = menuButtonRect(0, 2, winSize);
    auto r2 = menuButtonRect(1, 2, winSize);
    drawMenuButton(r1, L"开始游戏", true, r1.contains(mousePos), winSize);
    drawMenuButton(r2, L"退出",     true, r2.contains(mousePos), winSize);
}

int Renderer::hitMainMenu(const sf::Vector2f& pos, sf::Vector2u winSize)
{
    if (menuButtonRect(0, 2, winSize).contains(pos)) return 1; // 开始游戏
    if (menuButtonRect(1, 2, winSize).contains(pos)) return 2; // 退出
    return 0;
}

// ====== 角色选择 ======

void Renderer::drawCharacterSelect(sf::Vector2u winSize, const sf::Vector2f& mousePos)
{
    drawBackground(winSize);
    drawBackButton(winSize, mousePos);

    float w = (float)winSize.x;
    float h = (float)winSize.y;

    // 实际显示尺寸（比例匹配 char0/1/2.png，三卡铺满屏幕）
    float actualCW = w * 0.22f;
    float actualCH = actualCW * (float)CHAR_RT_H / (float)CHAR_RT_W;
    float gap = w * 0.04f;
    float totalW = CHAR_COUNT * actualCW + (CHAR_COUNT - 1) * gap;
    float startX = (w - totalW) / 2.0f;
    float startY = (h - actualCH) / 2.0f - h * 0.04f;

    // ---- 更新每张卡的目标悬停状态 ----
    int charHovered = -1;
    for (int i = 0; i < CHAR_COUNT; ++i) {
        float cx = startX + i * (actualCW + gap);
        float curS = m_charHover[i].currentScale;
        float curW = actualCW * curS;
        float curH = actualCH * curS;
        float curX = cx + (actualCW - curW) / 2.0f;
        float curY = startY + (actualCH - curH) / 2.0f + m_charHover[i].currentYOffset;
        sf::FloatRect rect({curX, curY}, {curW, curH});
        bool hover = rect.contains(mousePos);
        if (hover) charHovered = i;

        m_charHover[i].targetYOffset = hover ? -h * 0.065f : 0.0f;
        m_charHover[i].targetScale   = hover ? 1.10f : 1.0f;
    }
    if (charHovered != m_prevCharHoveredIdx && charHovered >= 0)
        playHoverTick();
    m_prevCharHoveredIdx = charHovered;

    // ---- 绘制 3 张角色卡片 (与技能牌一致的上浮+缩放动效) ----
    for (int i = 0; i < CHAR_COUNT; ++i) {
        float cx = startX + i * (actualCW + gap);
        float s = m_charHover[i].currentScale;
        float curW = actualCW * s;
        float curH = actualCH * s;
        float curX = cx + (actualCW - curW) / 2.0f;
        float curY = startY + (actualCH - curH) / 2.0f + m_charHover[i].currentYOffset;

        bool hover = m_charHover[i].targetYOffset < -0.1f;
        renderCharCardToRT(i, hover);

        sf::Sprite cardSprite(m_charRT[i].getTexture());
        cardSprite.setPosition({curX, curY});
        cardSprite.setScale({curW / CHAR_RT_W, curH / CHAR_RT_H});
        m_window.draw(cardSprite);
    }

    // ---- 悬停角色说明面板 (白底粗黑边，仅显示被动描述) ----
    if (charHovered >= 0) {
        auto& c = getAllCharacters()[charHovered];

        float panelW = w * 0.65f;
        float panelH = h * 0.14f;
        float panelX = (w - panelW) / 2.f;
        float panelY = h * 0.78f;
        float cut = 8.f;

        drawBeveledRect(m_window, panelX, panelY, panelW, panelH, cut,
                        sf::Color::White, OUTLINE_BLACK, 8.f);

        // 顶部色带（角色专属色）
        sf::Color charColor = charStreetColor(charHovered);
        sf::RectangleShape topBar({panelW - cut * 2, 5.f});
        topBar.setPosition({panelX + cut, panelY + 2.f});
        topBar.setFillColor(charColor);
        m_window.draw(topBar);

        // 被动技描述（左侧）
        sf::Text descText(m_font, c.passiveDesc, (unsigned)(panelH * 0.24f));
        descText.setFillColor(sf::Color(60, 60, 60));
        auto dsz = descText.getGlobalBounds().size;
        descText.setPosition({panelX + panelW * 0.05f,
                              panelY + (panelH - dsz.y) / 2.f});
        m_window.draw(descText);
    }
}

int Renderer::hitCharacterSelect(const sf::Vector2f& pos, sf::Vector2u winSize)
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;

    float cw = w * 0.22f;
    float ch = cw * (float)CHAR_RT_H / (float)CHAR_RT_W;
    float gap = w * 0.04f;
    float totalW = CHAR_COUNT * cw + (CHAR_COUNT - 1) * gap;
    float startX = (w - totalW) / 2.f;
    float startY = (h - ch) / 2.f - h * 0.04f;

    for (int i = 0; i < CHAR_COUNT; ++i) {
        float cx = startX + i * (cw + gap);
        float curS = m_charHover[i].currentScale;
        float curW = cw * curS;
        float curH = ch * curS;
        float curX = cx + (cw - curW) / 2.0f;
        float curY = startY + (ch - curH) / 2.0f + m_charHover[i].currentYOffset;
        if (sf::FloatRect({curX, curY}, {curW, curH}).contains(pos))
            return i + 1;
    }

    float gbSz = h * 0.04f;
    float gbX = w * 0.03f, gbY = h * 0.03f;
    if (sf::FloatRect({gbX, gbY}, {gbSz, gbSz}).contains(pos)) return 9;
    return 0;
}

// ====== 癞子点数选择 (谋略家被动) ======

namespace {
const wchar_t* WILDCARD_RANK_NAMES[13] = {
    L"3", L"4", L"5", L"6", L"7", L"8", L"9", L"10", L"J", L"Q", L"K", L"A", L"2"
};
}

void Renderer::drawWildcardSelect(sf::Vector2u winSize, const sf::Vector2f& mousePos)
{
    drawBackground(winSize);
    drawBackButton(winSize, mousePos);
    drawTitle(L"选择癞子点数", 0.06f, winSize);

    float w = (float)winSize.x;
    float h = (float)winSize.y;

    constexpr int RANK_COUNT = 13;
    float cardW = w * 0.055f;
    float cardH = cardW * CARD_H / CARD_W;
    float gap = w * 0.015f;
    float totalW = RANK_COUNT * cardW + (RANK_COUNT - 1) * gap;
    float startX = (w - totalW) / 2.0f;
    float baseY = h * 0.38f;

    for (int i = 0; i < RANK_COUNT; ++i) {
        float cx = startX + i * (cardW + gap);
        sf::FloatRect cardRect({cx, baseY}, {cardW, cardH});
        bool hover = cardRect.contains(mousePos);

        float curS = hover ? 1.12f : 1.0f;
        float curW = cardW * curS;
        float curH = cardH * curS;
        float curX = cx + (cardW - curW) / 2.0f;
        float curY = baseY + (cardH - curH) / 2.0f + (hover ? -h * 0.025f : 0.0f);

        sf::Color fill = hover ? sf::Color(STREET_CYAN.r/5, STREET_CYAN.g/5, STREET_CYAN.b/5) : STREET_BLACK;
        sf::Color outline = hover ? STREET_CYAN : OUTLINE_BLACK;
        drawBeveledRect(m_window, curX, curY, curW, curH, 6.f, fill, outline, hover ? 4.f : 3.f);

        sf::Text rankText(m_font, WILDCARD_RANK_NAMES[i], (unsigned)(curH * 0.42f));
        rankText.setFillColor(hover ? STREET_CYAN : sf::Color(220, 220, 220));
        rankText.setStyle(sf::Text::Bold);
        auto tsz = rankText.getGlobalBounds().size;
        rankText.setPosition({curX + (curW - tsz.x) / 2.0f, curY + (curH - tsz.y) / 2.0f});
        m_window.draw(rankText);
    }

    // 底部提示
    sf::Text hintSh(m_font, L"选择一张点数作为癞子牌（万能牌）", (unsigned)(h * 0.03f));
    hintSh.setFillColor(OUTLINE_BLACK);
    auto hsz = hintSh.getGlobalBounds().size;
    float hx = (w - hsz.x) / 2.0f, hy = baseY + cardH + h * 0.06f;
    hintSh.setPosition({hx + 2.f, hy + 2.f});
    m_window.draw(hintSh);

    sf::Text hint(m_font, L"选择一张点数作为癞子牌（万能牌）", (unsigned)(h * 0.03f));
    hint.setFillColor(STREET_WHITE);
    hint.setPosition({hx, hy});
    m_window.draw(hint);
}

int Renderer::hitWildcardSelect(const sf::Vector2f& pos, sf::Vector2u winSize)
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;

    constexpr int RANK_COUNT = 13;
    float cardW = w * 0.055f;
    float cardH = cardW * CARD_H / CARD_W;
    float gap = w * 0.015f;
    float totalW = RANK_COUNT * cardW + (RANK_COUNT - 1) * gap;
    float startX = (w - totalW) / 2.0f;
    float baseY = h * 0.38f;

    for (int i = 0; i < RANK_COUNT; ++i) {
        float cx = startX + i * (cardW + gap);
        if (sf::FloatRect({cx, baseY}, {cardW, cardH}).contains(pos))
            return i;  // 返回 doudizhuOrder 值 0-12
    }
    return -1;
}

// ====== 关卡过渡 (装备技能) ======

sf::FloatRect Renderer::transitionPoolCardRect(int cardIndex, sf::Vector2u winSize) const
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;
    float cardH = h * 0.16f;
    float cardW = cardH * CARD_W / CARD_H;
    float poolX = w * 0.06f;
    float poolY = h * 0.14f;
    float colGap = w * 0.03f;
    float rowGap = h * 0.015f;
    static constexpr int COLS = 2;
    int col = cardIndex % COLS;
    int row = cardIndex / COLS;
    float x = poolX + col * (cardW + colGap);
    float y = poolY + row * (cardH + rowGap);
    return {{x, y}, {cardW, cardH}};
}

int Renderer::hitTransitionPoolCard(const sf::Vector2f& pos, sf::Vector2u winSize,
                                     int acquiredCount)
{
    for (int i = 0; i < acquiredCount; ++i)
        if (transitionPoolCardRect(i, winSize).contains(pos))
            return i;
    return -1;
}

bool Renderer::hitTransitionPool(const sf::Vector2f& pos, sf::Vector2u winSize)
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;
    float cardH = h * 0.16f;
    float cardW = cardH * CARD_W / CARD_H;
    float poolX = w * 0.06f;
    float poolY = h * 0.14f;
    float colGap = w * 0.03f;
    float rowGap = h * 0.015f;
    static constexpr int COLS = 2;
    int maxRows = (SKILL_COUNT + COLS - 1) / COLS;
    float areaW = COLS * cardW + (COLS - 1) * colGap + w * 0.04f;
    float areaH = maxRows * cardH + (maxRows - 1) * rowGap + h * 0.03f;
    return sf::FloatRect({poolX - w * 0.02f, poolY - h * 0.015f},
                         {areaW, areaH}).contains(pos);
}

sf::Vector2f Renderer::transitionSlotCenter(int slotIndex, sf::Vector2u winSize) const
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;
    float slotH = h * 0.20f;
    float slotW = slotH * CARD_W / CARD_H;
    float slotGap = w * 0.03f;
    float rightX = w * 0.53f;
    float slotStartY = h * 0.14f;
    float x = rightX + slotIndex * (slotW + slotGap) + slotW / 2.f;
    float y = slotStartY + slotH / 2.f;
    return {x, y};
}

void Renderer::drawTransition(sf::Vector2u winSize, const sf::Vector2f& mousePos,
                               int level, const std::vector<int>& acquiredSkills,
                               const std::array<int, MAX_SKILL_SLOTS>& equipped,
                               const std::array<int, MAX_SKILL_SLOTS>& enemySkills,
                               int hoveredAcquiredIdx, int hoveredSlotIdx,
                               int dragSourceType, int dragSourceIndex,
                               int dragSkillId, bool isDragging)
{
    drawBackground(winSize, true);
    drawBackButton(winSize, mousePos);

    float w = (float)winSize.x;
    float h = (float)winSize.y;

    auto& allSkills = getAllSkills();

    // ---- 左侧: 已获得技能卡池 (2列网格, 与装备槽同尺寸) ----
    float poolCardH = h * 0.20f;
    float poolCardW = poolCardH * CARD_W / CARD_H;
    float poolX2 = w * 0.06f;
    float poolY = h * 0.14f;
    float colGap = w * 0.03f;
    float rowGap = h * 0.015f;
    static constexpr int POOL_COLS = 2;

    sf::Text heading(m_font, L"已获得协议", (unsigned)(h * 0.028f));
    heading.setFillColor(TEXT_DIM);
    heading.setPosition({poolX2, poolY - h * 0.04f});
    m_window.draw(heading);

    for (size_t i = 0; i < acquiredSkills.size(); ++i) {
        int sid = acquiredSkills[i];
        if (sid < 0 || sid >= SKILL_COUNT) continue;
        int col = (int)i % POOL_COLS;
        int row = (int)i / POOL_COLS;
        float cx = poolX2 + col * (poolCardW + colGap);
        float cy = poolY + row * (poolCardH + rowGap);

        bool isEquipped = false;
        for (int e = 0; e < MAX_SKILL_SLOTS; ++e)
            if (equipped[e] == sid) { isEquipped = true; break; }
        bool hover = ((int)i == hoveredAcquiredIdx);
        bool isBeingDragged = isDragging && dragSourceType == 1
                              && dragSourceIndex == (int)i;
        bool isFlyingAway = (m_flyProgress >= 0.f && (int)i == m_flyPoolIdx && !m_flyToSlot);

        if (isBeingDragged || isEquipped || isFlyingAway) {
            if (isEquipped && !isBeingDragged && !isFlyingAway) {
                sf::Text equippedHint(m_font, L"已装备", (unsigned)(poolCardH * 0.09f));
                equippedHint.setFillColor(TEXT_DISABLED);
                auto ehsz = equippedHint.getGlobalBounds().size;
                equippedHint.setPosition({cx + (poolCardW - ehsz.x) / 2.f, cy + poolCardH * 0.80f});
                m_window.draw(equippedHint);
            }
        } else {
            drawSkillCard(cx, cy, poolCardW, poolCardH, sid, false, hover, winSize);
        }
    }

    if (acquiredSkills.empty()) {
        sf::Text empty(m_font, L"暂无技能 (击败敌人后获得)", (unsigned)(h * 0.026f));
        empty.setFillColor(sf::Color(128, 128, 128));
        empty.setPosition({poolX2, poolY});
        m_window.draw(empty);
    }

    // ---- 飞牌动画 (卡牌直接移动) ----
    if (m_flyProgress >= 0.f) {
        float t = m_flyProgress;
        float eased = easeOutCubic(t);
        float gx = m_flySrc.x + (m_flyDst.x - m_flySrc.x) * eased;
        float gy = m_flySrc.y + (m_flyDst.y - m_flySrc.y) * eased;
        float gW = poolCardW * (m_flyToSlot ? (1.f - 0.15f * eased) : 1.f);
        float gH = poolCardH * (m_flyToSlot ? (1.f - 0.15f * eased) : 1.f);
        drawSkillCard(gx - gW / 2.f, gy - gH / 2.f, gW, gH, m_flySkillId, false, true, winSize);
    }

    // ---- 右侧: 装备槽 ----
    float rightX = w * 0.53f;
    float slotH = h * 0.20f;
    float slotW = slotH * CARD_W / CARD_H;
    float slotGap = w * 0.03f;
    float slotStartY = h * 0.14f;

    sf::Text slotHeading(m_font, L"装备槽", (unsigned)(h * 0.028f));
    slotHeading.setFillColor(TEXT_DIM);
    slotHeading.setPosition({rightX, slotStartY - h * 0.04f});
    m_window.draw(slotHeading);

    for (int i = 0; i < MAX_SKILL_SLOTS; ++i) {
        int sid = equipped[i];
        float sx = rightX + i * (slotW + slotGap);
        bool slotHover = (i == hoveredSlotIdx);
        bool isDropTarget = isDragging && slotHover;
        bool isBeingDraggedFrom = isDragging && dragSourceType == 2
                                  && dragSourceIndex == i;
        bool isFlyingFromSlot = (m_flyProgress >= 0.f && i == m_flySlotIdx && !m_flyToSlot);
        bool highlight = slotHover || isDropTarget;

        float slotCut = 4.f;
        float baseY = slotStartY + (highlight ? -2.f : 0.f);

        // 被拖出或飞回卡池时显示空槽
        int drawSid = (isBeingDraggedFrom || isFlyingFromSlot) ? -1 : sid;

        if (drawSid >= 0) {
            auto& sk = allSkills[drawSid];
            // 全幅技能图片
            {
                auto& tex = m_skillTextures[drawSid];
                sf::Sprite skSprite(tex);
                skSprite.setScale({slotW / (float)tex.getSize().x, slotH / (float)tex.getSize().y});
                skSprite.setPosition({sx, baseY});
                m_window.draw(skSprite);
            }
            // 黑色描边
            sf::RectangleShape border({slotW, slotH});
            border.setPosition({sx, baseY});
            border.setFillColor(sf::Color::Transparent);
            border.setOutlineColor(OUTLINE_BLACK);
            border.setOutlineThickness(3.f);
            m_window.draw(border);
            // 技能名
            sf::Text slotText(m_font, sk.name, (unsigned)(slotH * 0.15f));
            slotText.setFillColor(STREET_WHITE);
            slotText.setStyle(sf::Text::Bold);
            auto tsz = slotText.getGlobalBounds().size;
            slotText.setPosition({sx + (slotW - tsz.x) / 2.f, baseY + slotH * 0.80f});
            m_window.draw(slotText);
        } else {
            float cx = sx + slotW / 2.f;
            float cy = baseY + slotH / 2.f;
            sf::RectangleShape crossH({slotW * 0.25f, 1.f});
            crossH.setPosition({cx - slotW * 0.125f, cy});
            crossH.setFillColor(BORDER_NORMAL);
            m_window.draw(crossH);
            sf::RectangleShape crossV({1.f, slotH * 0.25f});
            crossV.setPosition({cx, cy - slotH * 0.125f});
            crossV.setFillColor(BORDER_NORMAL);
            m_window.draw(crossV);

            sf::Text empty(m_font, L"-- EMPTY --", (unsigned)(slotH * 0.12f));
            empty.setFillColor(TEXT_DISABLED);
            auto esz = empty.getGlobalBounds().size;
            empty.setPosition({sx + (slotW - esz.x) / 2.f, baseY + slotH * 0.65f});
            m_window.draw(empty);
        }
    }

    // ---- 技能说明声明 (敌人卡牌可追加触发) ----
    bool showSkillDesc = (hoveredAcquiredIdx >= 0
                       && hoveredAcquiredIdx < (int)acquiredSkills.size())
                      || (hoveredSlotIdx >= 0 && equipped[hoveredSlotIdx] >= 0);
    int hoveredSid = -1;
    if (hoveredAcquiredIdx >= 0 && hoveredAcquiredIdx < (int)acquiredSkills.size())
        hoveredSid = acquiredSkills[hoveredAcquiredIdx];
    else if (hoveredSlotIdx >= 0 && equipped[hoveredSlotIdx] >= 0)
        hoveredSid = equipped[hoveredSlotIdx];

    // ---- 敌人预览 (mirrored skills) ----
    float enemyY = slotStartY + slotH + h * 0.03f;
    sf::Text enemyHeading(m_font, L"敌方继承协议", (unsigned)(h * 0.028f));
    enemyHeading.setFillColor(STREET_PINK);
    enemyHeading.setStyle(sf::Text::Bold);
    enemyHeading.setPosition({rightX, enemyY});
    m_window.draw(enemyHeading);

    if (level == 1) {
        sf::Text noSkill(m_font, L"（第1关 - 敌人无技能）", (unsigned)(h * 0.026f));
        noSkill.setFillColor(sf::Color(150, 150, 150));
        noSkill.setPosition({rightX, enemyY + h * 0.035f});
        m_window.draw(noSkill);
    } else {
        float enemyCardH = slotH;
        float enemyCardW = slotW;
        float enemyCardGap = slotGap;
        int enemyHovered = -1;
        for (int i = 0; i < MAX_SKILL_SLOTS; ++i) {
            int sid = enemySkills[i];
            float ecx = rightX + i * (enemyCardW + enemyCardGap);
            float ecy = enemyY + h * 0.035f;
            bool ehover = sf::FloatRect({ecx, ecy}, {enemyCardW, enemyCardH}).contains(mousePos);
            if (ehover && sid >= 0) enemyHovered = i;
            drawSkillCard(ecx, ecy, enemyCardW, enemyCardH, sid, false, ehover, winSize);
        }
        if (enemyHovered >= 0 && enemySkills[enemyHovered] >= 0) {
            showSkillDesc = true;
            hoveredSid = enemySkills[enemyHovered];
        }
    }

    // ---- 技能说明 (敌人卡牌下方, 右侧) ----
    if (showSkillDesc && hoveredSid >= 0 && hoveredSid < SKILL_COUNT) {
        auto& sk = allSkills[hoveredSid];
        float descW = w * 0.36f;
        float descH = h * 0.08f;
        float descX = rightX;
        float descY = enemyY + h * 0.035f + slotH + h * 0.015f;
        sf::Color tc = skillTypeStreetColor(sk.type);
        drawBeveledRect(m_window, descX, descY, descW, descH, 8.f,
                        sf::Color(15, 15, 15, 220), tc, 3.f);

        float nameFont = (unsigned)(h * 0.026f);
        sf::Text nameText(m_font, sk.name, (unsigned)nameFont);
        nameText.setFillColor(tc);
        nameText.setStyle(sf::Text::Bold);
        nameText.setPosition({descX + w * 0.015f, descY + descH * 0.05f});
        m_window.draw(nameText);

        auto st = sk.type;
        std::wstring typeStr = skillTypeLabel(st);
        sf::Text typeTag(m_font, typeStr, (unsigned)(h * 0.018f));
        typeTag.setFillColor(tc);
        auto ttsz = nameText.getGlobalBounds().size;
        typeTag.setPosition({descX + w * 0.015f + ttsz.x + w * 0.015f,
                             descY + descH * 0.08f});
        m_window.draw(typeTag);

        sf::Text descText(m_font, sk.desc, (unsigned)(h * 0.021f));
        descText.setFillColor(sf::Color(220, 220, 220));
        descText.setPosition({descX + w * 0.015f, descY + descH * 0.50f});
        m_window.draw(descText);
    }

    // ---- 开始战斗按钮 ----
    float btnW = w * 0.18f;
    float btnH = h * 0.09f;
    float btnX = (w - btnW) / 2.f;
    float btnY = h * 0.83f;
    float btnCut = 8.f;
    bool fightHover = sf::FloatRect({btnX, btnY}, {btnW, btnH}).contains(mousePos);

    sf::Color btnFill = fightHover ? STREET_PINK : STREET_YELLOW;
    sf::Color btnOutline = OUTLINE_BLACK;
    drawBeveledRect(m_window, btnX, btnY, btnW, btnH, btnCut,
                    btnFill, btnOutline, 4.f);

    if (fightHover) {
        drawHazardStripes(m_window, btnX, btnY + 2.f, btnW, 4.f, 8.f);
        drawHazardStripes(m_window, btnX, btnY + btnH - 6.f, btnW, 4.f, 8.f);
    }

    float fontSize = btnH * 0.35f;
    sf::Text fightText(m_font, L"开始战斗", (unsigned)fontSize);
    fightText.setFillColor(fightHover ? STREET_WHITE : OUTLINE_BLACK);
    fightText.setStyle(sf::Text::Bold);
    auto fsz = fightText.getGlobalBounds().size;
    float baseX = btnX + (btnW - fsz.x) / 2.f;
    float baseY = btnY + (btnH - fsz.y) / 2.f - fsz.y * 0.15f;

    float shakeX = 0.f, shakeY = 0.f;
    if (fightHover) {
        float t = m_shakeTimer;
        shakeX = std::sin(t * 83.f) * 4.5f + std::cos(t * 137.f) * 3.2f + std::sin(t * 211.f) * 2.1f;
        shakeY = std::cos(t * 79.f) * 4.2f + std::sin(t * 149.f) * 3.5f + std::cos(t * 193.f) * 2.8f;
    }
    fightText.setPosition({baseX + shakeX, baseY + shakeY});
    m_window.draw(fightText);

    // ---- 幽灵卡 (拖拽跟随鼠标) ----
    if (isDragging && dragSkillId >= 0 && dragSkillId < SKILL_COUNT) {
        float ghostW = poolCardW * 1.05f;
        float ghostH = poolCardH * 1.05f;
        float gx = mousePos.x - ghostW / 2.f;
        float gy = mousePos.y - ghostH / 2.f;

        // 全幅技能图片 (半透明)
        {
            auto& tex = m_skillTextures[dragSkillId];
            sf::Sprite gSprite(tex);
            gSprite.setScale({ghostW / (float)tex.getSize().x, ghostH / (float)tex.getSize().y});
            gSprite.setColor(sf::Color(255, 255, 255, 170));
            gSprite.setPosition({gx, gy});
            m_window.draw(gSprite);
        }
        // 黑色描边
        sf::RectangleShape gBorder({ghostW, ghostH});
        gBorder.setPosition({gx, gy});
        gBorder.setFillColor(sf::Color::Transparent);
        gBorder.setOutlineColor(OUTLINE_BLACK);
        gBorder.setOutlineThickness(3.f);
        m_window.draw(gBorder);
    }

    // 卡牌悬停音效 — 卡池
    if (hoveredAcquiredIdx != m_prevPoolHoveredIdx && hoveredAcquiredIdx >= 0)
        playHoverTick();
    m_prevPoolHoveredIdx = hoveredAcquiredIdx;
    // 卡牌悬停音效 — 装备槽
    if (hoveredSlotIdx != m_prevSlotHoveredIdx && hoveredSlotIdx >= 0)
        playHoverTick();
    m_prevSlotHoveredIdx = hoveredSlotIdx;
}

int Renderer::hitTransitionSlot(const sf::Vector2f& pos, sf::Vector2u winSize)
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;
    float rightX = w * 0.53f;
    float slotH = h * 0.20f;
    float slotW = slotH * CARD_W / CARD_H;
    float slotGap = w * 0.03f;
    float slotStartY = h * 0.14f;

    for (int i = 0; i < MAX_SKILL_SLOTS; ++i) {
        float sx = rightX + i * (slotW + slotGap);
        if (sf::FloatRect({sx, slotStartY}, {slotW, slotH}).contains(pos))
            return i;
    }
    return -1;
}

int Renderer::hitTransitionFight(const sf::Vector2f& pos, sf::Vector2u winSize)
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;
    float btnW = w * 0.18f;
    float btnH = h * 0.09f;
    float btnX = (w - btnW) / 2.f;
    float btnY = h * 0.83f;
    if (sf::FloatRect({btnX, btnY}, {btnW, btnH}).contains(pos)) return 1;

    float gbSz = h * 0.04f;
    float gbX = w * 0.03f, gbY = h * 0.03f;
    if (sf::FloatRect({gbX, gbY}, {gbSz, gbSz}).contains(pos)) return 9;
    return 0;
}

// ====== 奖励界面 ======

void Renderer::drawReward(sf::Vector2u winSize, const sf::Vector2f& mousePos,
                           const std::vector<int>& skillIds,
                           const std::vector<int>& acquiredSkills)
{
    drawBackground(winSize, true);
    drawTitle(acquiredSkills.empty() ? L"获取协议" : L"奖励结算", 0.07f, winSize);

    float w = (float)winSize.x;
    float h = (float)winSize.y;
    int hoveredSid = -1;
    int rewardHoveredIdx = -1;

    for (int i = 0; i < 3; ++i) {
        auto baseRect = skillCardRect(i, 3, winSize);
        int sid = (i < (int)skillIds.size()) ? skillIds[i] : -1;
        bool owned = false;
        if (sid >= 0)
            owned = std::find(acquiredSkills.begin(), acquiredSkills.end(), sid)
                    != acquiredSkills.end();

        float curS = m_skillHover[i].currentScale;
        float curW = baseRect.size.x * curS;
        float curH = baseRect.size.y * curS;
        float curX = baseRect.position.x + (baseRect.size.x - curW) / 2.0f;
        float curY = baseRect.position.y + (baseRect.size.y - curH) / 2.0f
                   + m_skillHover[i].currentYOffset;

        sf::FloatRect displayRect({curX, curY}, {curW, curH});
        bool hover = displayRect.contains(mousePos);
        if (hover && sid >= 0) { hoveredSid = sid; rewardHoveredIdx = i; }

        m_skillHover[i].targetYOffset = hover ? -h * 0.065f : 0.0f;
        m_skillHover[i].targetScale   = hover ? 1.10f : 1.0f;

        drawSkillCard(curX, curY, curW, curH, sid, owned, hover, winSize);
    }

    if (rewardHoveredIdx != m_prevRewardHoveredIdx && rewardHoveredIdx >= 0)
        playHoverTick();
    m_prevRewardHoveredIdx = rewardHoveredIdx;

    // 悬停技能描述面板
    if (hoveredSid >= 0) {
        auto& skills = getAllSkills();
        auto& sk = skills[hoveredSid];

        float panelW = w * 0.56f;
        float panelH = h * 0.13f;
        float panelX = (w - panelW) / 2.f;
        float panelY = h * 0.70f;
        float cut = 8.f;

        sf::Color tc = skillTypeStreetColor(sk.type);
        drawBeveledRect(m_window, panelX, panelY, panelW, panelH, cut,
                        sf::Color(10, 13, 8), tc, 3.f);

        // 顶部色带（6px 粗）
        sf::RectangleShape topBar({panelW - cut * 2, 6.f});
        topBar.setPosition({panelX + cut, panelY + 2.f});
        topBar.setFillColor(tc);
        m_window.draw(topBar);

        // 编号 + 技能名
        std::wstring header = L"S0" + std::to_wstring(hoveredSid + 1) + L"  " + sk.name;
        sf::Text headerText(m_font, header, (unsigned)(panelH * 0.18f));
        headerText.setFillColor(tc);
        headerText.setStyle(sf::Text::Bold);
        headerText.setPosition({panelX + panelW * 0.04f, panelY + panelH * 0.12f});
        m_window.draw(headerText);

        // 描述
        sf::Text descText(m_font, sk.desc, (unsigned)(panelH * 0.16f));
        descText.setFillColor(sf::Color(220, 220, 220));
        descText.setPosition({panelX + panelW * 0.04f, panelY + panelH * 0.42f});
        m_window.draw(descText);

        // 技能类型
        std::wstring typeStr = skillTypeLabel(sk.type);
        sf::Text costText(m_font, typeStr, (unsigned)(panelH * 0.14f));
        costText.setFillColor(tc);
        auto csz = costText.getGlobalBounds().size;
        costText.setPosition({panelX + panelW - csz.x - panelW * 0.05f,
                              panelY + panelH - csz.y - panelH * 0.12f});
        m_window.draw(costText);
    }
}

int Renderer::hitReward(const sf::Vector2f& pos, sf::Vector2u winSize)
{
    for (int i = 0; i < 3; ++i) {
        auto baseRect = skillCardRect(i, 3, winSize);
        float curS = m_skillHover[i].currentScale;
        float curW = baseRect.size.x * curS;
        float curH = baseRect.size.y * curS;
        float curX = baseRect.position.x + (baseRect.size.x - curW) / 2.0f;
        float curY = baseRect.position.y + (baseRect.size.y - curH) / 2.0f
                   + m_skillHover[i].currentYOffset;
        if (sf::FloatRect({curX, curY}, {curW, curH}).contains(pos))
            return i;
    }
    return -1;
}

// ====== 失败界面 ======

void Renderer::drawGameOver(sf::Vector2u winSize, const sf::Vector2f& mousePos,
                              int levelReached, int skillCount)
{
    drawBackground(winSize);
    drawBackButton(winSize, mousePos);

    float w = (float)winSize.x;
    float h = (float)winSize.y;

    // [IMG-LOSE-BG] 半透明遮罩
    sf::RectangleShape overlay({w, h});
    overlay.setFillColor(sf::Color(0, 0, 0, 200));
    m_window.draw(overlay);

    // ERROR 闪烁
    static float errTimer = 0.f;
    errTimer += 0.016f; // approx 60fps
    if (errTimer > 1.f) errTimer = 0.f;
    float errAlpha = errTimer < 0.5f ? 255.f : 100.f;
    sf::Text errText(m_font, L"ERROR", (unsigned)(h * 0.06f));
    errText.setFillColor(sf::Color(255, 51, 51, (std::uint8_t)errAlpha));
    errText.setStyle(sf::Text::Bold);
    errText.setOutlineColor(OUTLINE_BLACK);
    errText.setOutlineThickness(3.f);
    auto esz = errText.getGlobalBounds().size;
    errText.setPosition({(w - esz.x) / 2.f, h * 0.14f});
    m_window.draw(errText);

    // "任务失败 // MISSION FAILED"
    float loseFont = h * 0.10f;
    sf::Text loseText(m_font, L"任务失败", (unsigned)loseFont);
    loseText.setFillColor(STREET_WHITE);
    loseText.setStyle(sf::Text::Bold);
    // 纯黑粗描底（4px 偏移）
    sf::Text loseShadow(m_font, L"任务失败", (unsigned)loseFont);
    loseShadow.setFillColor(OUTLINE_BLACK);
    loseShadow.setStyle(sf::Text::Bold);
    auto lsz = loseText.getGlobalBounds().size;
    loseShadow.setPosition({(w - lsz.x) / 2.f + 4.f, h * 0.22f + 4.f});
    m_window.draw(loseShadow);
    loseText.setPosition({(w - lsz.x) / 2.f, h * 0.22f});
    m_window.draw(loseText);

    // 底部青蓝色锯齿装饰线
    drawZigzagBorder(m_window, (w - lsz.x * 1.3f) / 2.f, h * 0.22f + lsz.y + 12.f,
                     lsz.x * 1.3f, STREET_CYAN, 4.f, 8.f);

    // 终端日志风格统计
    float logY = h * 0.38f;
    float logGap = h * 0.045f;
    auto drawLog = [&](const std::wstring& line, int idx) {
        // 黄色星星前缀替代 >
        float starX = w * 0.30f;
        float starY = logY + idx * logGap + h * 0.012f;
        drawStar(m_window, starX + 6.f, starY, 7.f, STREET_YELLOW, 2.f);

        sf::Text lt(m_font, line, (unsigned)(h * 0.032f));
        lt.setFillColor(STREET_CYAN);
        lt.setPosition({w * 0.30f + 22.f, logY + idx * logGap});
        m_window.draw(lt);
    };
    drawLog(L"到达关卡 : LEVEL " + std::to_wstring(levelReached), 0);
    drawLog(L"获得协议 : " + std::to_wstring(skillCount) + L" PROTOCOLS", 1);
    drawLog(L"存活时间 : --:--", 2);

    // 回到主菜单按钮
    float btnW = w * 0.30f;
    float btnH = h * 0.07f;
    sf::FloatRect menuBtn({(w - btnW) / 2.f, h * 0.58f}, {btnW, btnH});
    drawMenuButton(menuBtn, L"回到主菜单", true, menuBtn.contains(mousePos), winSize);
}

int Renderer::hitGameOver(const sf::Vector2f& pos, sf::Vector2u winSize)
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;
    float btnW = w * 0.25f;
    float btnH = h * 0.07f;
    if (sf::FloatRect({(w - btnW) / 2.f, h * 0.55f}, {btnW, btnH}).contains(pos))
        return 1;
    return 0;
}

// ====== 坐标计算 ======

sf::Vector2f Renderer::handCardPos(int index, int total,
                                    float yBase, sf::Vector2u winSize) const
{
    if (total <= 0) return {0, yBase};
    float w = (float)winSize.x;
    float s = handScale((float)winSize.y);
    float dispW = CARD_W * s;
    float avail = w * 0.92f;
    float overlap = dispW * 0.52f;
    if (total > 1) {
        float maxOverlap = (avail - dispW) / (total - 1);
        if (overlap > maxOverlap) overlap = maxOverlap;
    }
    float totalW = dispW + (total - 1) * overlap;
    float startX = (w - totalW) / 2.f;
    return {startX + index * overlap, yBase};
}

// ====== 游戏: 基础绘制 ======

void Renderer::drawCard(const Card& card, float x, float y, float scale, bool faceUp)
{
    const sf::Texture* tex = nullptr;
    if (faceUp) {
        auto it = m_faceTextures.find(card.imageIndex);
        tex = (it != m_faceTextures.end()) ? &it->second : &m_backTexture;
    } else {
        tex = &m_backTexture;
    }
    sf::Sprite sprite(*tex);
    sprite.setPosition({x, y});
    sprite.setScale({scale, scale});
    m_window.draw(sprite);
}

void Renderer::drawCardBack(float x, float y, float scale)
{
    sf::Sprite sprite(m_backTexture);
    sprite.setPosition({x, y});
    sprite.setScale({scale, scale});
    m_window.draw(sprite);

    // 细黑色描边
    float dispW = CARD_W * scale;
    float dispH = CARD_H * scale;
    float b = 1.5f;
    sf::RectangleShape top({dispW, b});       top.setPosition({x, y});              top.setFillColor(OUTLINE_BLACK); m_window.draw(top);
    sf::RectangleShape bot({dispW, b});       bot.setPosition({x, y + dispH - b});  bot.setFillColor(OUTLINE_BLACK); m_window.draw(bot);
    sf::RectangleShape lft({b, dispH});       lft.setPosition({x, y});              lft.setFillColor(OUTLINE_BLACK); m_window.draw(lft);
    sf::RectangleShape rgt({b, dispH});       rgt.setPosition({x + dispW - b, y});  rgt.setFillColor(OUTLINE_BLACK); m_window.draw(rgt);
}

void Renderer::drawPlayedCards(const std::vector<Card>& cards, float yCenter,
                                float scale, sf::Vector2u winSize)
{
    if (cards.empty()) return;
    float w = (float)winSize.x;
    int n = (int)cards.size();
    float dispW = CARD_W * scale;
    float avail = w * 0.55f;
    float overlap = std::min(dispW * 0.5f,
                             (avail - dispW) / std::max(1, n - 1));
    float totalW = dispW + (n - 1) * overlap;
    float startX = (w - totalW) / 2.f;
    float y = yCenter - CARD_H * scale / 2.f;

    for (int i = 0; i < n; ++i)
        drawCard(cards[i], startX + i * overlap, y, scale, true);
}

// ====== 游戏: UI (含技能/能量) ======

// 斗地主点数 → 显示名
static const wchar_t* dzRankName(int dzRank) {
    static const wchar_t* names[] = {
        L"3", L"4", L"5", L"6", L"7", L"8", L"9", L"10",
        L"J", L"Q", L"K", L"A", L"2", L"小王", L"大王"
    };
    if (dzRank >= 0 && dzRank < 15) return names[dzRank];
    return L"?";
}
// 牌型 → 中文名
static const wchar_t* handTypeName(HandType t) {
    switch (t) {
    case HandType::Single:           return L"单张";
    case HandType::Pair:             return L"对子";
    case HandType::Triple:           return L"三条";
    case HandType::TriplePlusOne:    return L"三带一";
    case HandType::TriplePlusTwo:    return L"三带二";
    case HandType::Straight:         return L"顺子";
    case HandType::ConsecutivePairs: return L"连对";
    case HandType::Airplane:         return L"飞机";
    case HandType::Bomb:             return L"炸弹";
    case HandType::Rocket:           return L"火箭";
    default:                         return L"";
    }
}

void Renderer::drawGameUI(const GameState& state, bool canPass, bool canPlaySelected,
                           sf::Vector2u winSize,
                           const std::array<int, MAX_SKILL_SLOTS>& playerSkillIds,
                           const sf::Vector2f& mousePos,
                           const std::vector<int>& selectedIndices)
{
    float w = (float)winSize.x;
    uint8_t glowMask = state.skillGlowMask(selectedIndices);
    float h = (float)winSize.y;

    // ---- 选中牌型预览 ----
    if ((state.phase() == GameState::Phase::PlayerTurn || state.phase() == GameState::Phase::SchedulePlay)
        && !selectedIndices.empty()) {
        // 调度阶段显示弃牌张数
        if (state.phase() == GameState::Phase::SchedulePlay) {
            sf::Text prevText(m_font,
                L"弃牌 " + std::to_wstring((int)selectedIndices.size()) + L" / 3 张",
                (unsigned)(h * 0.024f));
            prevText.setFillColor(BATTLE_BLUE);
            auto psz = prevText.getGlobalBounds().size;
            prevText.setPosition({(w - psz.x) / 2.f, h * 0.81f});
            m_window.draw(prevText);
        } else {
        // 提取选中牌并分类
        std::vector<Card> selCards;
        auto& ph = state.playerHand();
        for (int idx : selectedIndices)
            if (idx >= 0 && idx < (int)ph.size())
                selCards.push_back(ph[idx]);
        auto pattern = GameState::classifyHand(selCards, &state.playerBuffs());

        std::wstring preview;
        sf::Color prevCol = BATTLE_BLUE;
        if (pattern) {
            int wildRank = state.playerBuffs().wildcardRank;
            int mainR = pattern->mainRank;
            int wc = 0;
            std::array<int, 15> rf{};
            for (auto& c : selCards) {
                int r = doudizhuOrder(c.rank);
                if (wildRank >= 0 && r == wildRank) wc++;
                else rf[r]++;
            }

            auto dn = [](int r) { return std::wstring(dzRankName(r)); };

            switch (pattern->type) {
            case HandType::Single:
                preview = L"单张 " + dn(mainR); break;
            case HandType::Pair:
                preview = L"对 " + dn(mainR); break;
            case HandType::Triple:
                preview = L"三条 " + dn(mainR); break;
            case HandType::TriplePlusOne: {
                // 从剩余牌中找kicker
                int useF = std::min(rf[mainR], 3);
                int useW = std::max(0, 3 - useF);
                rf[mainR] -= useF;
                int rw = wc - useW;
                int kicker = -1;
                for (int r = 0; r < 15 && kicker < 0; ++r)
                    if (rf[r] > 0) kicker = r;
                if (kicker < 0 && rw > 0) kicker = wildRank;
                preview = L"三条" + dn(mainR) + L" + 单" + dn(kicker);
                break;
            }
            case HandType::TriplePlusTwo: {
                int useF = std::min(rf[mainR], 3);
                int useW = std::max(0, 3 - useF);
                rf[mainR] -= useF;
                int rw = wc - useW;
                int pairR = -1;
                for (int r = 0; r < 15 && pairR < 0; ++r)
                    if (r != mainR && rf[r] + rw >= 2) pairR = r;
                if (pairR < 0) pairR = wildRank;
                preview = L"三条" + dn(mainR) + L" + 对" + dn(pairR);
                break;
            }
            case HandType::Bomb:
                preview = L"炸弹 " + dn(mainR); break;
            case HandType::Rocket:
                preview = L"火箭"; break;
            case HandType::Straight: {
                int start = mainR - pattern->length + 1;
                preview = L"顺子 " + dn(start) + L"-" + dn(mainR);
                break;
            }
            case HandType::ConsecutivePairs: {
                int start = mainR - pattern->length + 1;
                preview = L"连对 " + dn(start) + L"-" + dn(mainR);
                break;
            }
            case HandType::Airplane: {
                int start = mainR - pattern->length + 1;
                preview = L"飞机 " + dn(start) + L"-" + dn(mainR);
                if (pattern->kickerCount > 0)
                    preview += L" + " + std::to_wstring(pattern->kickerCount) + L"张";
                break;
            }
            default: preview = handTypeName(pattern->type); break;
            }
        } else {
            preview = L"— 无效牌型 —";
            prevCol = ENEMY_RED;
        }

        sf::Text prevText(m_font, preview, (unsigned)(h * 0.024f));
        prevText.setFillColor(prevCol);
        auto psz = prevText.getGlobalBounds().size;
        float pvX = (w - psz.x) / 2.f;
        float pvY = h * 0.81f;
        prevText.setPosition({pvX, pvY});
        m_window.draw(prevText);
        } // end else (non-Schedule)
    }

    std::wstring statusStr;
    sf::Color statusCol = TEXT_DIM;
    switch (state.phase()) {
    case GameState::Phase::PlayerTurn:
        statusStr = state.isNewRound() ? L"[STATUS] 新一轮"
                                       : L"[STATUS] 你的回合";
        statusCol = STREET_CYAN;
        break;
    case GameState::Phase::SchedulePlay:
        statusStr = L"[擢升] 选0~3张牌弃掉换牌";
        statusCol = STREET_CYAN;
        break;
    case GameState::Phase::MomentumPlay:
        statusStr = state.isMomentumEnemy() ? L"[STATUS] 敌方连击之势" : L"[STATUS] 连击之势";
        statusCol = state.isMomentumEnemy() ? STREET_PINK : STREET_CYAN;
        break;
    case GameState::Phase::ComputerTurn:
        statusStr = L"[STATUS] 敌方运算中";
        statusCol = STREET_PINK;
        break;
    case GameState::Phase::PlayerWins:
        statusStr = L"[STATUS] 任务完成";
        statusCol = STREET_YELLOW;
        break;
    case GameState::Phase::ComputerWins:
        statusStr = L"[STATUS] 任务失败";
        statusCol = STREET_PINK;
        break;
    }
    m_statusText->setString(statusStr);
    m_statusText->setFillColor(statusCol);
    auto sz = m_statusText->getGlobalBounds().size;
    float stX = (w - sz.x) / 2.f;
    float stY = h * 0.96f;

    // 黑色衬底条
    sf::RectangleShape stBg({sz.x + 40.f, h * 0.04f});
    stBg.setPosition({stX - 20.f, stY - 2.f});
    stBg.setFillColor(sf::Color(0, 0, 0, 200));
    m_window.draw(stBg);

    m_statusText->setPosition({stX, stY});
    m_window.draw(*m_statusText);

    // 炸弹收藏家印记显示（仅该角色可见）
    if (state.isBombCollector()) {
    float bmH = h * 0.019f;
    float bmX = w * 0.78f;
    float bmY = h * 0.965f;
    int marks = state.playerBombMarks();
    int flash = state.bombGenFlash();

    // 背景框
    float bgPad = 6.f;
    float bgW = 3 * (bmH + 8.f) + 90.f;  // 三星 + 文字宽度
    float bgH = bmH + bgPad * 2;
    float bgX = bmX - bgPad;
    float bgY = bmY - bgPad;
    sf::RectangleShape bgBox({bgW, bgH});
    bgBox.setPosition({bgX, bgY});
    bgBox.setFillColor(sf::Color(10, 10, 10, 200));
    bgBox.setOutlineColor(OUTLINE_BLACK);
    bgBox.setOutlineThickness(1.f);
    m_window.draw(bgBox);

    // 红色斜条纹（炸弹生成闪光）
    if (flash > 0) {
        float alpha = std::min(1.f, flash / 30.f) * 200.f;
        for (float sx = bgX - bgH; sx < bgX + bgW; sx += 8.f) {
            sf::RectangleShape stripe({2.f, bgH * 1.5f});
            stripe.setPosition({sx, bgY - bgH * 0.25f});
            stripe.setRotation(sf::degrees(45.f));
            stripe.setFillColor(sf::Color(220, 30, 30, (uint8_t)alpha));
            m_window.draw(stripe);
        }
    }

    // 星星
    for (int m = 0; m < 3; ++m) {
        float bx = bmX + m * (bmH + 8.f);
        float starR = bmH * 0.6f;
        if (m < marks) {
            drawStar(m_window, bx + starR, bmY + starR, starR, STREET_PINK, 2.f);
        } else {
            drawStar(m_window, bx + starR, bmY + starR, starR, sf::Color(60, 60, 60), 1.f);
        }
    }

    // 印记文字
    sf::Text bmLabel(m_font, L"印记 " + std::to_wstring(marks) + L"/3",
                     (unsigned)(bmH * 0.9f));
    bmLabel.setFillColor(marks > 0 ? STREET_PINK : TEXT_DIM);
    bmLabel.setPosition({bmX + 3 * (bmH + 8.f) + 6.f, bmY - 2.f});
    m_window.draw(bmLabel);
    }

    // 开发者调试按钮 (DEV-ONLY) — 暗红低调风格
    if (state.phase() == GameState::Phase::PlayerTurn
        || state.phase() == GameState::Phase::ComputerTurn)
    {
        float dbgW = w * 0.04f;
        float dbgH = h * 0.028f;
        float dbgY = h * 0.035f;
        float dbgX1 = w * 0.14f;
        float dbgX2 = w * 0.19f;
        float dbgFontSize = h * 0.015f;

        auto drawDbgBtn = [&](float bx, const sf::String& label) {
            drawBeveledRect(m_window, bx, dbgY, dbgW, dbgH, 2.f,
                            sf::Color(51, 0, 0), sf::Color(102, 34, 34), 1.f);
            sf::Text txt(m_font, label, (unsigned)dbgFontSize);
            txt.setFillColor(sf::Color(136, 136, 136));
            auto tsz = txt.getGlobalBounds().size;
            txt.setPosition({bx + (dbgW - tsz.x) / 2.f,
                            dbgY + (dbgH - tsz.y) / 2.f - tsz.y * 0.15f});
            m_window.draw(txt);
        };
        drawDbgBtn(dbgX1, L"我赢");
        drawDbgBtn(dbgX2, L"我输");
    }

    // 设置按钮
    drawBackButton(winSize, mousePos);

    // 技能槽 (卡牌比例 CARD_W:CARD_H, 左下角) — 整体面板
    auto& allSkills = getAllSkills();
    float skH = h * 0.11f;
    float skW = skH * CARD_W / CARD_H;
    float skGap = w * 0.015f;
    float skStartX = w * 0.03f;
    float skY = h * 0.83f;
    float panelW = skW * 3 + skGap * 2 + 16.f;
    float panelH = skH + 16.f;

    // 整体切角面板（3px 青蓝描边）
    drawBeveledRect(m_window, skStartX - 8.f, skY - 8.f, panelW, panelH, 6.f,
                    sf::Color(10, 10, 10), STREET_CYAN, 3.f);

    for (int i = 0; i < MAX_SKILL_SLOTS; ++i) {
        float sx = skStartX + i * (skW + skGap);
        float sy = skY + m_skillSlotY[i]; // 点击反馈动画
        int sid = playerSkillIds[i];
        float skCut = 3.f;

        if (sid >= 0) {
            auto& sk = allSkills[sid];
            // 全幅技能图片
            {
                auto& tex = m_skillTextures[sid];
                sf::Sprite skSprite(tex);
                skSprite.setScale({skW / (float)tex.getSize().x, skH / (float)tex.getSize().y});
                skSprite.setPosition({sx, sy});
                m_window.draw(skSprite);
            }
            // 黑色描边
            sf::RectangleShape border({skW, skH});
            border.setPosition({sx, sy});
            border.setFillColor(sf::Color::Transparent);
            border.setOutlineColor(STREET_CYAN);
            border.setOutlineThickness(3.f);
            m_window.draw(border);
            // 技能名
            float nameF = skW * 0.22f;
            m_skillBtnTexts[i]->setString(sk.name);
            m_skillBtnTexts[i]->setCharacterSize((unsigned)nameF);
            m_skillBtnTexts[i]->setFillColor(BATTLE_BLUE);
            auto tsz = m_skillBtnTexts[i]->getGlobalBounds().size;
            m_skillBtnTexts[i]->setPosition({sx + (skW - tsz.x) / 2.f,
                                              sy + skH * 0.80f});
            m_window.draw(*m_skillBtnTexts[i]);
        } else {
            // 空槽准星
            float cx = sx + skW / 2.f;
            float cy = sy + skH / 2.f - 4.f;
            sf::RectangleShape crossH({skW * 0.20f, 1.f});
            crossH.setPosition({cx - skW * 0.10f, cy});
            crossH.setFillColor(BORDER_NORMAL);
            m_window.draw(crossH);
            sf::RectangleShape crossV({1.f, skH * 0.18f});
            crossV.setPosition({cx, cy - skH * 0.09f});
            crossV.setFillColor(BORDER_NORMAL);
            m_window.draw(crossV);

            m_skillBtnTexts[i]->setString(L"");
        }
    }

    // 敌人技能槽 (右上角, 与玩家技能槽相同大小/布局) — 使用图片素材
    auto& enemySlots = state.enemySkillSlots();
    float eTotalW = skW * 3 + skGap * 2 + 16.f;
    float eStartX = w - eTotalW + 8.f - w * 0.03f;
    float eY = h * 0.02f;
    float ePanelW = skW * 3 + skGap * 2 + 16.f;
    float ePanelH = skH + 16.f;

    drawBeveledRect(m_window, eStartX - 8.f, eY, ePanelW, ePanelH, 6.f,
                    sf::Color(10, 10, 10), STREET_PINK, 3.f);

    for (int i = 0; i < MAX_SKILL_SLOTS; ++i) {
        int esid = enemySlots[i];
        float ex = eStartX + i * (skW + skGap);
        float ey = eY + 8.f;

        if (esid >= 0) {
            auto& tex = m_skillTextures[esid];
            sf::Sprite skSprite(tex);
            skSprite.setScale({skW / (float)tex.getSize().x, skH / (float)tex.getSize().y});
            skSprite.setPosition({ex, ey});
            m_window.draw(skSprite);

            // 描边
            sf::RectangleShape border({skW, skH});
            border.setPosition({ex, ey});
            border.setFillColor(sf::Color::Transparent);
            border.setOutlineColor(STREET_PINK);
            border.setOutlineThickness(3.f);
            m_window.draw(border);

            // 技能名
            float nameF = skW * 0.22f;
            sf::Text et(m_font, allSkills[esid].name, (unsigned)nameF);
            et.setFillColor(sf::Color(255, 180, 180));
            auto etsz = et.getGlobalBounds().size;
            et.setPosition({ex + (skW - etsz.x) / 2.f, ey + skH * 0.80f});
            m_window.draw(et);
        } else {
            // 空槽准星
            float cx = ex + skW / 2.f;
            float cy = ey + skH / 2.f - 4.f;
            sf::RectangleShape crossH({skW * 0.20f, 1.f});
            crossH.setPosition({cx - skW * 0.10f, cy});
            crossH.setFillColor(BORDER_NORMAL);
            m_window.draw(crossH);
            sf::RectangleShape crossV({1.f, skH * 0.18f});
            crossV.setPosition({cx, cy - skH * 0.09f});
            crossV.setFillColor(BORDER_NORMAL);
            m_window.draw(crossV);
        }
    }

    // 「调度」过牌按钮 (手牌上方居中)
    if (state.phase() == GameState::Phase::SchedulePlay) {
        float sBtnW = w * 0.12f;
        float sBtnH = h * 0.055f;
        float sBtnGap = w * 0.03f;
        float sTotalW = sBtnW * 2 + sBtnGap;
        float sStartX = (w - sTotalW) / 2.f;
        float sBtnY = h * 0.56f;
        float sFontSize = h * 0.024f;

        bool skipHover = sf::FloatRect({sStartX, sBtnY}, {sBtnW, sBtnH}).contains(mousePos);
        bool schedHover = sf::FloatRect({sStartX + sBtnW + sBtnGap, sBtnY}, {sBtnW, sBtnH}).contains(mousePos);

        // 跳过按钮
        {
            sf::Color fill = skipHover ? STREET_PINK : STREET_BLACK;
            drawBeveledRect(m_window, sStartX, sBtnY, sBtnW, sBtnH, 5.f, fill, OUTLINE_BLACK, 3.f);
            sf::Text txt(m_font, L"跳过", (unsigned)sFontSize);
            txt.setFillColor(STREET_WHITE);
            auto tsz = txt.getGlobalBounds().size;
            txt.setPosition({sStartX + (sBtnW - tsz.x) / 2.f, sBtnY + sBtnH * 0.22f});
            m_window.draw(txt);
        }
        // 过牌按钮
        {
            sf::Color fill = schedHover ? STREET_CYAN : STREET_BLACK;
            drawBeveledRect(m_window, sStartX + sBtnW + sBtnGap, sBtnY, sBtnW, sBtnH, 5.f, fill, OUTLINE_BLACK, 3.f);
            sf::Text txt(m_font, L"过牌", (unsigned)sFontSize);
            txt.setFillColor(STREET_WHITE);
            auto tsz = txt.getGlobalBounds().size;
            txt.setPosition({sStartX + sBtnW + sBtnGap + (sBtnW - tsz.x) / 2.f, sBtnY + sBtnH * 0.22f});
            m_window.draw(txt);
        }
    }

    // 出牌/不出按钮 (手牌上方居中, 含悬停缩放动效) — 新风格
    bool isMomentum = (state.phase() == GameState::Phase::MomentumPlay);
    bool isEnemyMomentum = isMomentum && state.isMomentumEnemy();
    if ((state.phase() == GameState::Phase::PlayerTurn || isMomentum) && !isEnemyMomentum) {
        float btnW = w * 0.12f;
        float btnH = h * 0.055f;
        float btnGap = w * 0.03f;
        float totalW = btnW * 2 + btnGap;
        float startX = (w - totalW) / 2.f;
        float btnY = h * 0.56f;
        float fontSize = h * 0.024f;

        bool passHover = sf::FloatRect({startX, btnY}, {btnW, btnH}).contains(mousePos);
        bool playHover = sf::FloatRect({startX + btnW + btnGap, btnY}, {btnW, btnH}).contains(mousePos);
        m_passBtnTargetScale = passHover ? 1.08f : 1.0f;
        m_playBtnTargetScale = playHover ? 1.08f : 1.0f;

        // 不出按钮 (左侧) — 连击之势时不显示
        if (!isMomentum) {
            float px = startX;
            float pw = btnW * m_passBtnHoverScale;
            float ph = btnH * m_passBtnHoverScale;
            float px2 = px + (btnW - pw) / 2.f;
            float py2 = btnY + (btnH - ph) / 2.f;
            sf::Color pfill = canPass ? (passHover ? STREET_PINK : STREET_BLACK) : btnDisabledColor;
            sf::Color pout = canPass ? OUTLINE_BLACK : sf::Color(34, 34, 34);
            drawBeveledRect(m_window, px2, py2, pw, ph, 5.f, pfill, pout, 3.f);

            m_passBtnText->setString(L"跳过");
            m_passBtnText->setCharacterSize((unsigned)(fontSize * m_passBtnHoverScale));
            m_passBtnText->setFillColor(canPass ? STREET_WHITE : TEXT_DISABLED);
            auto psz = m_passBtnText->getGlobalBounds().size;
            m_passBtnText->setPosition({px + (btnW - psz.x) / 2.f, btnY + btnH * 0.22f});
            m_window.draw(*m_passBtnText);
        }

        // 出牌按钮 — 连击之势时居中
        {
            float px = isMomentum ? (w - btnW) / 2.f : startX + btnW + btnGap;
            float pw = btnW * m_playBtnHoverScale;
            float ph = btnH * m_playBtnHoverScale;
            float px2 = px + (btnW - pw) / 2.f;
            float py2 = btnY + (btnH - ph) / 2.f;
            sf::Color pfill = canPlaySelected ? (playHover ? STREET_YELLOW : STREET_CYAN) : btnDisabledColor;
            sf::Color pout = canPlaySelected ? OUTLINE_BLACK : sf::Color(68, 68, 0);
            drawBeveledRect(m_window, px2, py2, pw, ph, 5.f, pfill, pout, 3.f);

            m_playBtnText->setString(L"出牌");
            m_playBtnText->setCharacterSize((unsigned)(fontSize * m_playBtnHoverScale));
            m_playBtnText->setFillColor(canPlaySelected ? sf::Color::Black : sf::Color(85, 85, 0));
            auto plsz = m_playBtnText->getGlobalBounds().size;
            m_playBtnText->setPosition({px + (btnW - plsz.x) / 2.f, btnY + btnH * 0.22f});
            m_window.draw(*m_playBtnText);
        }
    }
}

// ====== 游戏: 主渲染 ======

void Renderer::renderGame(const GameState& state,
                          const std::vector<int>& selectedIndices,
                          sf::Vector2u winSize,
                          bool canPlaySelected,
                          const std::array<int, MAX_SKILL_SLOTS>& playerSkillIds,
                          const sf::Vector2f& mousePos,
                          float dt,
                          int charId)
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;
    float hs = handScale(h);
    float ps = playedScale(h);
    float hoverLift = h * 0.025f;

    // 连击之势动画计时
    if (state.phase() == GameState::Phase::MomentumPlay) {
        m_momentumAnimTimer += dt;
    } else {
        m_momentumAnimTimer = 0.f;
    }
    // 调度动画计时 + 飞行过渡检测
    if (state.phase() == GameState::Phase::SchedulePlay) {
        m_scheduleAnimTimer += dt;
        m_wasSchedulePlay = true;
    } else {
        m_scheduleAnimTimer = 0.f;
        // 刚退出调度 → 标记待飞 (卡牌动画结束后再启动立绘飞行)
        if (m_wasSchedulePlay && charId == 2) {
            m_pendingScheduleFly = true;
        }
        m_wasSchedulePlay = false;
    }
    // 飞行动画推进
    if (m_scheduleFlyProgress >= 0.f) {
        m_scheduleFlyProgress += dt * 2.5f;  // ~0.4s 完成
        if (m_scheduleFlyProgress >= 1.f) {
            m_scheduleFlyProgress = -1.f;     // 动画结束
        }
    }

    // 对战专用背景
    {
        auto& bgTex = m_battleBgTexture;
        if (bgTex.getSize().x > 0) {
            sf::Sprite bgSprite(bgTex);
            float bgScale = std::max(w / (float)bgTex.getSize().x,
                                     h / (float)bgTex.getSize().y);
            bgSprite.setScale({bgScale, bgScale});
            bgSprite.setPosition({(w - bgTex.getSize().x * bgScale) / 2.f,
                                  (h - bgTex.getSize().y * bgScale) / 2.f});
            m_window.draw(bgSprite);
        } else {
            m_window.clear(sf::Color(10, 10, 10));
        }
    }

    // --- 电脑手牌 (牌背) ---
    auto& ch = state.computerHand();
    int cn = (int)ch.size();
    float chY = computerHandY(h);
    for (int i = 0; i < cn; ++i) {
        auto pos = handCardPos(i, cn, chY, winSize);
        drawCardBack(pos.x, pos.y, hs);
    }
    m_computerLabel->setString(L"敌方单位");
    m_computerLabel->setPosition({w * 0.012f, chY - h * 0.007f});
    m_computerLabel->setFillColor(BATTLE_BLUE);
    m_window.draw(*m_computerLabel);
    // 手牌数角标
    sf::Text cCount(m_font, std::to_wstring(cn), (unsigned)(h * 0.022f));
    cCount.setFillColor(BATTLE_BLUE);
    cCount.setStyle(sf::Text::Bold);
    auto ccsz = cCount.getGlobalBounds().size;
    sf::RectangleShape cTag({ccsz.x + 12.f, ccsz.y + 6.f});
    cTag.setPosition({w * 0.012f + m_computerLabel->getGlobalBounds().size.x + 8.f, chY - h * 0.007f});
    cTag.setFillColor(STREET_PINK);
    m_window.draw(cTag);
    cCount.setPosition({w * 0.012f + m_computerLabel->getGlobalBounds().size.x + 8.f + 6.f, chY - h * 0.007f + 2.f});
    m_window.draw(cCount);

    // --- 电脑出的牌 ---
    {
        // 战场淡框
        sf::RectangleShape eBox({w * 0.56f, h * 0.14f});
        eBox.setPosition({(w - w * 0.56f) / 2.f, computerPlayedY(h) - h * 0.07f});
        eBox.setFillColor(sf::Color(10, 10, 10));
        eBox.setOutlineColor(STREET_PINK);
        eBox.setOutlineThickness(3.f);
        m_window.draw(eBox);
    }
    drawPlayedCards(state.lastComputerPlay(), computerPlayedY(h), ps, winSize);

    drawPlayedCards(state.lastPlayerPlay(), playerPlayedY(h), ps, winSize);

    // --- 连击之势/调度: 全局变暗 (手牌之前绘制，手牌保持亮度) ---
    if (state.phase() == GameState::Phase::MomentumPlay) {
        float fadeT = std::clamp(m_momentumAnimTimer / 0.3f, 0.f, 1.f);
        sf::RectangleShape dimOverlay({w, h});
        dimOverlay.setFillColor(sf::Color(0, 0, 0, (uint8_t)(160 * fadeT)));
        m_window.draw(dimOverlay);
    }
    if (state.phase() == GameState::Phase::SchedulePlay) {
        sf::RectangleShape dimOverlay({w, h});
        dimOverlay.setFillColor(sf::Color(0, 0, 0, 120));
        m_window.draw(dimOverlay);
    }

    // --- 玩家手牌 (正面, 含悬停浮动动效) ---
    auto& ph = state.playerHand();
    int pn = (int)ph.size();
    float phY = handCardY(h);
    float dispW = CARD_W * hs;
    float dispH = CARD_H * hs;

    m_handCardTargets.resize(pn, 0.0f);
    m_handCardYOffsets.resize(pn, 0.0f);

    int hoveredIdx = -1;

    // 检测新卡牌加入 (炸弹生成 / 调度换牌) → 启动飞入动画
    std::vector<int> curImgIndices; curImgIndices.reserve(pn);
    for (const auto& c : ph) curImgIndices.push_back(c.imageIndex);
    bool justExitedSchedule = (m_prevPhase == GameState::Phase::SchedulePlay
                               && state.phase() != GameState::Phase::SchedulePlay);
    if ((state.bombGenFlash() > 0 || justExitedSchedule)
        && !m_bombDealActive && !m_dealActive
        && !m_prevHandImgIndices.empty()) {
        // 找出新增的imageIndex (简单差分: 统计每个idx的新增数量)
        std::unordered_map<int, int> prevCnt, curCnt;
        for (int idx : m_prevHandImgIndices) prevCnt[idx]++;
        for (int idx : curImgIndices) curCnt[idx]++;
        std::vector<int> newImgIndices;
        for (auto& [imgIdx, cnt] : curCnt) {
            int diff = cnt - prevCnt[imgIdx];
            for (int k = 0; k < diff; ++k) newImgIndices.push_back(imgIdx);
        }
        // 在手牌中找到这些imageIndex对应的位置
        std::vector<int> bombIndices;
        for (int i = 0; i < pn && bombIndices.size() < newImgIndices.size(); ++i) {
            for (int j = 0; j < (int)newImgIndices.size(); ++j) {
                if (newImgIndices[j] >= 0 && ph[i].imageIndex == newImgIndices[j]) {
                    bombIndices.push_back(i);
                    newImgIndices[j] = -1; // 标记已找到
                    break;
                }
            }
        }
        if (!bombIndices.empty())
            startBombDealAnimation(bombIndices);
    }
    m_prevHandImgIndices = std::move(curImgIndices);

    // 调度结束后: 若没有卡牌动画则立即启动立绘飞行, 否则等卡牌动画结束
    if (m_pendingScheduleFly && !m_bombDealActive) {
        m_scheduleFlyProgress = 0.f;
        m_pendingScheduleFly = false;
    }
    m_prevPhase = state.phase();

    if (m_dealActive) {
        // --- 发牌动画模式: 底部中心旋转 + 扇形展开 + 飞入 ---
        float middle = (pn - 1) / 2.0f;
        float maxAngleDeg = 0.0f;
        float arcCurve = 0.0f;
        for (int i = 0; i < pn; ++i) {
            if (i >= (int)m_dealAnim.size() || !m_dealAnim[i].started)
                continue;

            auto pos = handCardPos(i, pn, phY, winSize);
            float t = m_dealAnim[i].progress;
            float eased = easeOutCubic(t);

            float norm = (pn > 1) ? (float)(i - middle) / middle : 0.0f;
            float fanRot = maxAngleDeg * std::sin(norm * 3.14159265f / 2.0f) * eased;
            float dist = std::abs(i - middle);
            float arcSink = dist * dist * arcCurve * eased;

            float curScale = hs * (DEAL_INIT_SCL + (1.0f - DEAL_INIT_SCL) * eased);
            std::uint8_t curAlpha = (std::uint8_t)(255 * eased);

            float bcx = pos.x + dispW / 2.0f;
            float animYOff = DEAL_INIT_YOFF + (arcSink - DEAL_INIT_YOFF) * eased;
            float bcy = pos.y + dispH + animYOff;

            auto it = m_faceTextures.find(ph[i].imageIndex);
            const sf::Texture* tex = (it != m_faceTextures.end()) ? &it->second : &m_backTexture;

            sf::Sprite sprite(*tex);
            sprite.setOrigin({CARD_W / 2.0f, CARD_H});
            sprite.setScale({curScale, curScale});
            sprite.setPosition({bcx, bcy});
            sprite.setRotation(sf::degrees(fanRot));
            sprite.setColor(sf::Color(255, 255, 255, curAlpha));
            m_window.draw(sprite);
        }
    } else {
        // --- 扇形排布 ---
        hoveredIdx = -1;
        float middle = (pn - 1) / 2.0f;
        float maxAngleDeg = 0.0f;
        float arcCurve = 0.0f;

        // ---- 第一步：根据遮挡顺序（从上层到下层）找出唯一悬停的卡牌 ----
        for (int i = pn - 1; i >= 0; --i) {
            auto pos = handCardPos(i, pn, phY, winSize);
            bool sel = std::find(selectedIndices.begin(), selectedIndices.end(), i)
                       != selectedIndices.end();
            float norm = (pn > 1) ? (float)(i - middle) / middle : 0.0f;
            float fanRot = maxAngleDeg * std::sin(norm * 3.14159265f / 2.0f);
            float dist = std::abs(i - middle);
            float arcSink = dist * dist * arcCurve;

            float curLift = m_handCardYOffsets[i];
            if (curLift < 0.5f) curLift = (sel ? hoverLift : 0.0f);
            float liftX = 0.f, liftY = curLift;
            if (std::abs(fanRot) > 0.5f) {
                float rad = fanRot * 3.14159265f / 180.0f;
                liftX = curLift * std::sin(rad);
                liftY = curLift * std::cos(rad);
            }

            float bcx = pos.x + dispW / 2.0f + liftX;
            float bcy = pos.y + dispH + arcSink - liftY;

            sf::FloatRect visualRect({bcx - dispW / 2.0f, bcy - dispH}, {dispW, dispH});
            if (visualRect.contains(mousePos)) {
                hoveredIdx = i;
                break; // 最上层（右侧）优先命中，下层被遮挡
            }
        }

        // ---- 第二步：绘制手牌 (底部中心旋转 + 扇形下沉) ----
        for (int i = 0; i < pn; ++i) {
            auto pos = handCardPos(i, pn, phY, winSize);
            bool sel = std::find(selectedIndices.begin(), selectedIndices.end(), i)
                       != selectedIndices.end();

            float norm = (pn > 1) ? (float)(i - middle) / middle : 0.0f;
            float fanRot = maxAngleDeg * std::sin(norm * 3.14159265f / 2.0f);
            float dist = std::abs(i - middle);
            float arcSink = dist * dist * arcCurve;

            // 1. 先计算当前视觉偏移（与绘制一致）
            float curLift = m_handCardYOffsets[i];
            if (curLift < 0.5f) curLift = (sel ? hoverLift : 0.0f); // 首帧立即到位

            // 2. 只有最上层被悬停的卡牌才算 hover，避免穿透到下层
            bool hover = (i == hoveredIdx);
            m_handCardTargets[i] = (sel || hover) ? hoverLift : 0.0f;

            float liftX = 0.f, liftY = curLift;
            if (std::abs(fanRot) > 0.5f) {
                float rad = fanRot * 3.14159265f / 180.0f;
                liftX = curLift * std::sin(rad);
                liftY = curLift * std::cos(rad);
            }

            float bcx = pos.x + dispW / 2.0f + liftX;
            float bcy = pos.y + dispH + arcSink - liftY;

            // 炸弹生成动画期间, 新牌由动画层绘制 (避免双绘)
            if (m_bombDealActive) {
                bool isBombCard = false;
                for (int bi : m_bombCardIndices) {
                    if (i == bi) { isBombCard = true; break; }
                }
                if (isBombCard) continue;
            }

            auto it = m_faceTextures.find(ph[i].imageIndex);
            const sf::Texture* tex = (it != m_faceTextures.end()) ? &it->second : &m_backTexture;

            sf::Sprite sprite(*tex);
            sprite.setOrigin({CARD_W / 2.0f, CARD_H});
            sprite.setScale({hs, hs});
            sprite.setPosition({bcx, bcy});
            sprite.setRotation(sf::degrees(fanRot));
            m_window.draw(sprite);

            // 癞子牌荧光粉标记
            if (state.playerBuffs().wildcardRank >= 0
                && doudizhuOrder(ph[i].rank) == state.playerBuffs().wildcardRank) {
                sf::Sprite wglow(it->second);
                wglow.setOrigin({CARD_W / 2.0f, CARD_H});
                wglow.setScale({hs * 1.03f, hs * 1.03f});
                wglow.setPosition({bcx, bcy});
                wglow.setRotation(sf::degrees(fanRot));
                wglow.setColor(sf::Color(STREET_PINK.r, STREET_PINK.g, STREET_PINK.b, 80));
                m_window.draw(wglow);
            }

            if (sel) {
                // 橙黄 overlay
                sf::Sprite hl(it->second);
                hl.setOrigin({CARD_W / 2.0f, CARD_H});
                hl.setScale({hs, hs});
                hl.setPosition({bcx, bcy});
                hl.setRotation(sf::degrees(fanRot));
                hl.setColor(sf::Color(STREET_YELLOW.r, STREET_YELLOW.g, STREET_YELLOW.b, 60));
                m_window.draw(hl);
                // 外发光边框（青蓝，放大 1.02 倍绘制）
                sf::Sprite glow(it->second);
                glow.setOrigin({CARD_W / 2.0f, CARD_H});
                glow.setScale({hs * 1.02f, hs * 1.02f});
                glow.setPosition({bcx, bcy});
                glow.setRotation(sf::degrees(fanRot));
                glow.setColor(sf::Color(STREET_CYAN.r, STREET_CYAN.g, STREET_CYAN.b, 40));
                m_window.draw(glow);
            }
        }
    }
    if (hoveredIdx != m_prevHandHoveredIdx && hoveredIdx >= 0)
        playHoverTick();
    m_prevHandHoveredIdx = hoveredIdx;

    // --- 炸弹生成动画: 新牌飞入 (覆盖在正常手牌之上) ---
    if (m_bombDealActive && !m_dealActive) {
        float middle = (pn - 1) / 2.0f;
        float maxAngleDeg = 0.0f;
        float arcCurve = 0.0f;
        for (int j = 0; j < (int)m_bombDealAnim.size(); ++j) {
            if (!m_bombDealAnim[j].started) continue;
            int i = m_bombCardIndices[j];
            if (i < 0 || i >= pn) continue;

            auto pos = handCardPos(i, pn, phY, winSize);
            float t = m_bombDealAnim[j].progress;
            float eased = easeOutCubic(t);

            float norm = (pn > 1) ? (float)(i - middle) / middle : 0.0f;
            float fanRot = maxAngleDeg * std::sin(norm * 3.14159265f / 2.0f) * eased;
            float dist = std::abs(i - middle);
            float arcSink = dist * dist * arcCurve * eased;

            float curScale = hs * (DEAL_INIT_SCL + (1.0f - DEAL_INIT_SCL) * eased);
            std::uint8_t curAlpha = (std::uint8_t)(255 * eased);

            // 从屏幕右侧飞入 (起始位置在目标右侧 +800px)
            float startXOff = 800.f * (1.f - eased);
            float bcx = pos.x + dispW / 2.0f + startXOff;
            float animYOff = arcSink - DEAL_INIT_YOFF * (1.f - eased);
            float bcy = pos.y + dispH + arcSink - animYOff;

            auto it = m_faceTextures.find(ph[i].imageIndex);
            const sf::Texture* tex = (it != m_faceTextures.end()) ? &it->second : &m_backTexture;

            sf::Sprite sprite(*tex);
            sprite.setOrigin({CARD_W / 2.0f, CARD_H});
            sprite.setScale({curScale, curScale});
            sprite.setPosition({bcx, bcy});
            sprite.setRotation(sf::degrees(fanRot));
            sprite.setColor(sf::Color(255, 255, 255, curAlpha));
            m_window.draw(sprite);

            // 粉色发光外框
            sf::Sprite glow(*tex);
            glow.setOrigin({CARD_W / 2.0f, CARD_H});
            glow.setScale({curScale * 1.04f, curScale * 1.04f});
            glow.setPosition({bcx, bcy});
            glow.setRotation(sf::degrees(fanRot));
            glow.setColor(sf::Color(STREET_PINK.r, STREET_PINK.g, STREET_PINK.b,
                          (std::uint8_t)(100 * eased)));
            m_window.draw(glow);
        }
    }

    m_playerLabel->setString(L"我方单位");
    m_playerLabel->setPosition({w * 0.012f, phY - h * 0.033f});
    m_playerLabel->setFillColor(BATTLE_BLUE);
    m_window.draw(*m_playerLabel);
    // 手牌数角标
    sf::Text pCount(m_font, std::to_wstring(pn), (unsigned)(h * 0.022f));
    pCount.setFillColor(BATTLE_BLUE);
    pCount.setStyle(sf::Text::Bold);
    auto pcsz = pCount.getGlobalBounds().size;
    sf::RectangleShape pTag({pcsz.x + 12.f, pcsz.y + 6.f});
    pTag.setPosition({w * 0.012f + m_playerLabel->getGlobalBounds().size.x + 8.f, phY - h * 0.033f});
    pTag.setFillColor(STREET_CYAN);
    m_window.draw(pTag);
    pCount.setPosition({w * 0.012f + m_playerLabel->getGlobalBounds().size.x + 8.f + 6.f, phY - h * 0.033f + 2.f});
    m_window.draw(pCount);

    // --- UI (含技能/能量) ---
    drawGameUI(state, !state.isNewRound(), canPlaySelected, winSize, playerSkillIds, mousePos, selectedIndices);

    // --- 连击之势: 中央技能卡牌 (手牌之上) ---
    if (state.phase() == GameState::Phase::MomentumPlay) {
        // 入场缩放动画: 0→0.5s 从 1.5x 缩小到 0.3x
        float scaleT = std::clamp(m_momentumAnimTimer / 0.5f, 0.f, 1.f);
        float ease = 1.f - (1.f - scaleT) * (1.f - scaleT);
        float curScale = 1.5f - 1.2f * ease;

        float cardW = w * 0.22f * curScale;
        float cardH = cardW * CARD_H / CARD_W;
        float cardX = (w - cardW) / 2.f;
        float cardY = h * 0.18f;

        drawSkillCard(cardX, cardY, cardW, cardH, 0, false, true, winSize);

        // 提示文字 (延迟0.3s后渐显)
        float hintAlpha = std::clamp((m_momentumAnimTimer - 0.3f) / 0.3f, 0.f, 1.f);
        const sf::String hintStr = state.isMomentumEnemy()
            ? L"敌方触发连击之势"
            : L"选择1张手牌打出";
        sf::Text hint(m_font, hintStr, (unsigned)(h * 0.032f));
        sf::Color hintCol = state.isMomentumEnemy()
            ? sf::Color(STREET_PINK.r, STREET_PINK.g, STREET_PINK.b, (uint8_t)(255 * hintAlpha))
            : sf::Color(STREET_YELLOW.r, STREET_YELLOW.g, STREET_YELLOW.b, (uint8_t)(255 * hintAlpha));
        hint.setFillColor(hintCol);
        hint.setStyle(sf::Text::Bold);
        auto hsz = hint.getGlobalBounds().size;
        hint.setPosition({(w - hsz.x) / 2.f, cardY + cardH + h * 0.03f});
        m_window.draw(hint);
    }

    // --- 掌控者「擢升」+ 飞行动画 (charId==2) ---
    if (charId == 2) {
        auto& tex = m_battleCharTextures[2];
        auto texSz = tex.getSize();

        // 目标: 右下角
        float dstImgH = h * 0.25f;
        float dstImgW = dstImgH * (float)texSz.x / (float)texSz.y;
        float dstBoxW = h * 0.15f;
        float dstBoxH = h * 0.24f;
        float dstBoxX = w - dstBoxW - w * 0.02f;
        float dstBoxY = h - dstBoxH - h * 0.02f;

        // 起始: 上方中央
        float srcImgH = h * 0.22f;
        float srcImgW = srcImgH * (float)texSz.x / (float)texSz.y;
        float srcImgX = (w - srcImgW) / 2.f;
        float srcImgY = h * 0.06f;

        float imgX, imgY, imgH, imgW, boxW, boxH, boxX, boxY;
        bool drawBox = true;

        if (state.phase() == GameState::Phase::SchedulePlay) {
            // 阶段1: 上方中央 + 入场缩放
            float scaleT = std::clamp(m_scheduleAnimTimer / 0.4f, 0.f, 1.f);
            float ease = 1.f - (1.f - scaleT) * (1.f - scaleT);
            float curS = 1.0f - 0.7f * (1.f - ease);
            imgH = srcImgH * curS;
            imgW = srcImgW * curS;
            imgX = (w - imgW) / 2.f;
            imgY = srcImgY;
            boxW = imgW + 6.f;
            boxH = imgH + 6.f;
            boxX = imgX - 3.f;
            boxY = imgY - 3.f;
        } else if (m_scheduleFlyProgress >= 0.f) {
            // 阶段2: 飞行动画 (上方中央 → 右下角)
            float t = m_scheduleFlyProgress;
            float ease = t * t * (3.f - 2.f * t);  // smoothstep
            imgH = srcImgH + (dstImgH - srcImgH) * ease;
            imgW = srcImgW + (dstImgW - srcImgW) * ease;
            imgX = srcImgX + (dstBoxX + (dstBoxW - dstImgW) / 2.f - srcImgX) * ease;
            imgY = srcImgY + (dstBoxY + (dstBoxH - dstImgH) / 2.f - srcImgY) * ease;
            boxW = imgW + 6.f + (dstBoxW - (imgW + 6.f)) * ease;
            boxH = imgH + 6.f + (dstBoxH - (imgH + 6.f)) * ease;
            boxX = imgX - 3.f + (dstBoxX - (imgX - 3.f)) * ease;
            boxY = imgY - 3.f + (dstBoxY - (imgY - 3.f)) * ease;
        } else if (m_pendingScheduleFly) {
            // 等待飞牌动画结束 → 保持在中央，不跳右下角
            imgH = srcImgH;
            imgW = srcImgW;
            imgX = (w - imgW) / 2.f;
            imgY = srcImgY;
            boxW = imgW + 6.f;
            boxH = imgH + 6.f;
            boxX = imgX - 3.f;
            boxY = imgY - 3.f;
        } else {
            // 阶段3: 右下角固定 (非调度期间常态)
            imgH = dstImgH;
            imgW = dstImgW;
            imgX = dstBoxX + (dstBoxW - dstImgW) / 2.f;
            imgY = dstBoxY + (dstBoxH - dstImgH) / 2.f;
            boxW = dstBoxW;
            boxH = dstBoxH;
            boxX = dstBoxX;
            boxY = dstBoxY;
        }

        // 立绘 (悬停上移 + 点击弹窗)
        float chLiftTarget = 0.f;
        {
            sf::FloatRect chkRect({imgX, imgY}, {imgW, imgH});
            if (chkRect.contains(mousePos))
                chLiftTarget = h * 0.03f;
        }
        m_charPortraitLift = lerp(m_charPortraitLift, chLiftTarget, dt * 14.f);
        sf::Sprite charSprite(tex);
        charSprite.setScale({imgW / (float)texSz.x, imgH / (float)texSz.y});
        charSprite.setPosition({imgX, imgY - m_charPortraitLift});
        m_window.draw(charSprite);

        // 调度提示文字
        if (state.phase() == GameState::Phase::SchedulePlay) {
            float hintAlpha = std::clamp((m_scheduleAnimTimer - 0.15f) / 0.25f, 0.f, 1.f);
            sf::Text hint(m_font, L"掌控者 · 擢升 — 弃牌结构不变，点数随机提升",
                          (unsigned)(h * 0.022f));
            hint.setFillColor(sf::Color(STREET_CYAN.r, STREET_CYAN.g, STREET_CYAN.b,
                                        (uint8_t)(255 * hintAlpha)));
            hint.setStyle(sf::Text::Bold);
            auto hsz = hint.getGlobalBounds().size;
            hint.setPosition({(w - hsz.x) / 2.f, imgY + imgH + h * 0.015f});
            m_window.draw(hint);
        }

        // 被动技能点击弹窗 (右下角固定态)
        if (m_charTooltipOpen
            && state.phase() != GameState::Phase::SchedulePlay
            && m_scheduleFlyProgress < 0.f) {
            sf::FloatRect chkRect({imgX, imgY}, {imgW, imgH});
            auto& ch = getAllCharacters()[2];
            drawCharTooltip(w, h, chkRect, ch.passiveName, ch.passiveDesc);
        }
    } else if (charId >= 0 && charId < CHAR_COUNT) {
        // 其他角色: 右下角立绘 (悬停上移 + 点击弹窗)
        auto& tex = m_battleCharTextures[charId];
        auto texSz = tex.getSize();
        float imgH = h * 0.25f;
        float imgW = imgH * (float)texSz.x / (float)texSz.y;
        float boxW = h * 0.15f;
        float boxH = h * 0.24f;
        float boxX = w - boxW - w * 0.02f;
        float boxY = h - boxH - h * 0.02f;
        float spriteX = boxX + (boxW - imgW) / 2.f;
        float spriteY = boxY + (boxH - imgH) / 2.f;

        float chLiftTarget = 0.f;
        {
            sf::FloatRect chkRect({spriteX, spriteY}, {imgW, imgH});
            if (chkRect.contains(mousePos))
                chLiftTarget = h * 0.03f;
        }
        m_charPortraitLift = lerp(m_charPortraitLift, chLiftTarget, dt * 14.f);
        sf::Sprite charSprite(tex);
        charSprite.setScale({imgW / (float)texSz.x, imgH / (float)texSz.y});
        charSprite.setPosition({spriteX, spriteY - m_charPortraitLift});
        m_window.draw(charSprite);

        // 被动技能点击弹窗
        if (m_charTooltipOpen) {
            sf::FloatRect chkRect({spriteX, spriteY}, {imgW, imgH});
            auto& ch = getAllCharacters()[charId];
            drawCharTooltip(w, h, chkRect, ch.passiveName, ch.passiveDesc);
        }
    }
}

// ====== 角色立绘悬停提示 ======

void Renderer::drawCharTooltip(float w, float h, sf::FloatRect charRect,
                               const std::wstring& passiveName, const std::wstring& passiveDesc)
{
    float tipW = w * 0.18f;
    float tipH = h * 0.15f;  // 加高适配多行描述
    float tipX = charRect.position.x - tipW - 16.f;
    float tipY = charRect.position.y + (charRect.size.y - tipH) / 2.f;

    // 确保不超出左边界
    if (tipX < 8.f) tipX = 8.f;

    // 背景面板
    drawBeveledRect(m_window, tipX, tipY, tipW, tipH, 6.f,
                    sf::Color(10, 10, 10, 230), STREET_CYAN, 2.f);

    // 被动技能名称
    float nameSize = h * 0.022f;
    sf::Text nameText(m_font, passiveName, (unsigned)nameSize);
    nameText.setFillColor(STREET_YELLOW);
    nameText.setStyle(sf::Text::Bold);
    auto nsz = nameText.getGlobalBounds().size;
    nameText.setPosition({tipX + (tipW - nsz.x) / 2.f, tipY + h * 0.012f});
    m_window.draw(nameText);

    // 描述文字 (自动换行)
    float descSize = h * 0.017f;
    float maxLineW = tipW - 16.f;

    // 先试单行
    sf::Text measure(m_font, passiveDesc, (unsigned)descSize);
    if (measure.getGlobalBounds().size.x <= maxLineW) {
        measure.setFillColor(TEXT_DIM);
        auto dsz = measure.getGlobalBounds().size;
        measure.setPosition({tipX + (tipW - dsz.x) / 2.f, tipY + tipH * 0.38f});
        m_window.draw(measure);
    } else {
        // 需要换行: 按宽度拆分
        std::wstring remain = passiveDesc;
        std::vector<std::wstring> lines;
        while (!remain.empty()) {
            // 二分找最大可放下片段的长度
            int lo = 1, hi = (int)remain.size(), best = 1;
            while (lo <= hi) {
                int mid = (lo + hi) / 2;
                sf::Text probe(m_font, remain.substr(0, mid), (unsigned)descSize);
                if (probe.getGlobalBounds().size.x <= maxLineW) {
                    best = mid; lo = mid + 1;
                } else { hi = mid - 1; }
            }
            lines.push_back(remain.substr(0, best));
            remain = remain.substr(best);
            if (lines.size() >= 3) break; // 最多3行
        }
        float lineH = descSize * 1.35f;
        float startY = tipY + tipH * 0.25f;
        for (size_t i = 0; i < lines.size(); ++i) {
            sf::Text line(m_font, lines[i], (unsigned)descSize);
            line.setFillColor(TEXT_DIM);
            auto lsz = line.getGlobalBounds().size;
            line.setPosition({tipX + (tipW - lsz.x) / 2.f, startY + i * lineH});
            m_window.draw(line);
        }
    }
}

// ====== 设置弹窗 ======

void Renderer::drawSettingsPopup(sf::Vector2u winSize, const sf::Vector2f& mousePos,
                                  bool draggingMusic, bool draggingSound, bool canRestart)
{
    // 更新重开拒绝计时器
    if (m_restartDeniedTimer > 0.f) m_restartDeniedTimer -= 0.016f;
    float w = (float)winSize.x;
    float h = (float)winSize.y;

    // 半透明暗色遮罩
    sf::RectangleShape overlay({w, h});
    overlay.setFillColor(sf::Color(0, 0, 0, 180));
    m_window.draw(overlay);

    // 面板尺寸
    float panelW = w * 0.55f;
    float panelH = h * 0.52f;
    float panelX = (w - panelW) / 2.f;
    float panelY = (h - panelH) / 2.f;

    drawBeveledRect(m_window, panelX, panelY, panelW, panelH, 12.f,
                    STREET_BLACK, STREET_PINK, 3.f);

    // 标题
    sf::Text title(m_font, L"设置", (unsigned)(h * 0.045f));
    title.setFillColor(STREET_WHITE);
    title.setStyle(sf::Text::Bold);
    auto tsz = title.getGlobalBounds().size;
    title.setPosition({panelX + (panelW - tsz.x) / 2.f, panelY + h * 0.025f});
    // 标题阴影
    sf::Text titleSh(m_font, L"设置", (unsigned)(h * 0.045f));
    titleSh.setFillColor(OUTLINE_BLACK);
    titleSh.setStyle(sf::Text::Bold);
    titleSh.setPosition({title.getPosition().x + 3.f, title.getPosition().y + 3.f});
    m_window.draw(titleSh);
    m_window.draw(title);

    // 关闭按钮 X (面板右上角)
    float closeSz = h * 0.035f;
    float closeX = panelX + panelW - closeSz - 16.f;
    float closeY = panelY + 10.f;
    bool closeHover = sf::FloatRect({closeX, closeY}, {closeSz, closeSz}).contains(mousePos);
    sf::Color closeCol = closeHover ? STREET_PINK : TEXT_DIM;
    // X 形线条
    sf::RectangleShape xLine1({closeSz * 0.7f, 3.f});
    xLine1.setOrigin({xLine1.getSize().x / 2.f, 1.5f});
    xLine1.setPosition({closeX + closeSz / 2.f, closeY + closeSz / 2.f});
    xLine1.setRotation(sf::degrees(45.f));
    xLine1.setFillColor(closeCol);
    m_window.draw(xLine1);
    sf::RectangleShape xLine2({closeSz * 0.7f, 3.f});
    xLine2.setOrigin({xLine2.getSize().x / 2.f, 1.5f});
    xLine2.setPosition({closeX + closeSz / 2.f, closeY + closeSz / 2.f});
    xLine2.setRotation(sf::degrees(-45.f));
    xLine2.setFillColor(closeCol);
    m_window.draw(xLine2);

    // --- 滑块参数 ---
    float sliderY1 = panelY + h * 0.15f;
    float sliderY2 = panelY + h * 0.26f;
    float sliderW = panelW * 0.65f;
    float sliderH = h * 0.012f;
    float sliderX = panelX + panelW * 0.18f;
    float knobR = h * 0.016f;

    // 辅助: 绘制滑块
    auto drawSlider = [&](float sy, float value, sf::Color fillCol, bool isDragging) {
        // 轨道背景
        drawBeveledRect(m_window, sliderX, sy, sliderW, sliderH, 2.f,
                        sf::Color(40, 40, 40), OUTLINE_BLACK, 1.5f);
        // 已填充部分
        float fillW = sliderW * (value / 100.f);
        if (fillW > 0.f) {
            sf::RectangleShape fillBar({fillW, sliderH});
            fillBar.setPosition({sliderX, sy});
            fillBar.setFillColor(fillCol);
            m_window.draw(fillBar);
        }
        // 滑块圆钮
        float knobX = sliderX + fillW;
        float knobY = sy + sliderH / 2.f;
        sf::CircleShape knob(knobR);
        knob.setOrigin({knobR, knobR});
        knob.setPosition({knobX, knobY});
        knob.setFillColor(isDragging ? STREET_YELLOW : STREET_WHITE);
        knob.setOutlineColor(OUTLINE_BLACK);
        knob.setOutlineThickness(2.f);
        m_window.draw(knob);
        // 百分比文字
        sf::Text pct(m_font, std::to_wstring((int)value) + L"%",
                     (unsigned)(h * 0.022f));
        pct.setFillColor(TEXT_DIM);
        auto psz = pct.getGlobalBounds().size;
        pct.setPosition({sliderX + sliderW + 16.f, sy - psz.y / 2.f + sliderH / 2.f});
        m_window.draw(pct);
    };

    // 标签
    sf::Text musicLabel(m_font, L"音乐", (unsigned)(h * 0.026f));
    musicLabel.setFillColor(TEXT_DIM);
    musicLabel.setStyle(sf::Text::Bold);
    auto mlsz = musicLabel.getGlobalBounds().size;
    musicLabel.setPosition({sliderX - mlsz.x - 16.f,
                            sliderY1 + sliderH / 2.f - mlsz.y / 2.f});
    m_window.draw(musicLabel);

    sf::Text soundLabel(m_font, L"音效", (unsigned)(h * 0.026f));
    soundLabel.setFillColor(TEXT_DIM);
    soundLabel.setStyle(sf::Text::Bold);
    auto slsz = soundLabel.getGlobalBounds().size;
    soundLabel.setPosition({sliderX - slsz.x - 16.f,
                            sliderY2 + sliderH / 2.f - slsz.y / 2.f});
    m_window.draw(soundLabel);

    drawSlider(sliderY1, m_musicVolume, STREET_CYAN, draggingMusic);
    drawSlider(sliderY2, m_soundVolume, STREET_PINK, draggingSound);

    // --- 底部按钮 ---
    float btnW = panelW * 0.26f;
    float btnH = h * 0.06f;
    float btnY = panelY + panelH - btnH - h * 0.06f;
    float btnGap = panelW * 0.03f;
    float totalBtnW = btnW * 3 + btnGap * 2;
    float btnStartX = panelX + (panelW - totalBtnW) / 2.f;

    // 放弃本轮
    sf::FloatRect menuBtn({btnStartX, btnY}, {btnW, btnH});
    drawMenuButton(menuBtn, L"放弃本轮", true, menuBtn.contains(mousePos), winSize);

    // 重开 (R) — 非对局时变暗
    float restartX = btnStartX + btnW + btnGap;
    sf::FloatRect restartBtn({restartX, btnY}, {btnW, btnH});
    drawMenuButton(restartBtn, L"重开 (R)", canRestart, canRestart && restartBtn.contains(mousePos), winSize);

    // 关闭
    sf::FloatRect closeBtn({restartX + btnW + btnGap, btnY}, {btnW, btnH});
    drawMenuButton(closeBtn, L"关闭", true, closeBtn.contains(mousePos), winSize);

    // 无法重开提示
    if (m_restartDeniedTimer > 0.f) {
        sf::Text denied(m_font, L"现在无法重开！", (unsigned)(h * 0.028f));
        denied.setFillColor(STREET_PINK);
        denied.setStyle(sf::Text::Bold);
        auto dsz = denied.getGlobalBounds().size;
        denied.setPosition({panelX + (panelW - dsz.x) / 2.f, btnY - h * 0.06f});
        m_window.draw(denied);
    }
}

Renderer::SettingsHitResult Renderer::hitTestSettings(const sf::Vector2f& pos,
                                                       sf::Vector2u winSize)
{
    SettingsHitResult result;
    float w = (float)winSize.x;
    float h = (float)winSize.y;

    float panelW = w * 0.55f;
    float panelH = h * 0.52f;
    float panelX = (w - panelW) / 2.f;
    float panelY = (h - panelH) / 2.f;

    // 关闭按钮 X
    float closeSz = h * 0.035f;
    float closeX = panelX + panelW - closeSz - 16.f;
    float closeY = panelY + 10.f;
    if (sf::FloatRect({closeX, closeY}, {closeSz, closeSz}).contains(pos)) {
        result.action = 1; // 关闭
        return result;
    }

    // 音乐滑块
    float sliderW = panelW * 0.65f;
    float sliderH = h * 0.012f;
    float sliderX = panelX + panelW * 0.18f;
    float sliderY1 = panelY + h * 0.15f;
    float knobR = h * 0.016f;
    // 扩大滑块点击区域
    float hitPad = knobR * 2.f;
    if (sf::FloatRect({sliderX - hitPad, sliderY1 - hitPad},
                       {sliderW + hitPad * 2, sliderH + hitPad * 2}).contains(pos)) {
        result.action = 3; // 音乐滑块
        result.sliderVal = std::clamp((pos.x - sliderX) / sliderW, 0.f, 1.f) * 100.f;
        return result;
    }

    // 音效滑块
    float sliderY2 = panelY + h * 0.26f;
    if (sf::FloatRect({sliderX - hitPad, sliderY2 - hitPad},
                       {sliderW + hitPad * 2, sliderH + hitPad * 2}).contains(pos)) {
        result.action = 4; // 音效滑块
        result.sliderVal = std::clamp((pos.x - sliderX) / sliderW, 0.f, 1.f) * 100.f;
        return result;
    }

    // 返回主界面按钮
    float btnW = panelW * 0.26f;
    float btnH = h * 0.06f;
    float btnY2 = panelY + panelH - btnH - h * 0.06f;
    float btnGap = panelW * 0.03f;
    float totalBtnW = btnW * 3 + btnGap * 2;
    float btnStartX = panelX + (panelW - totalBtnW) / 2.f;
    if (sf::FloatRect({btnStartX, btnY2}, {btnW, btnH}).contains(pos)) {
        result.action = 2; // 返回主界面
        return result;
    }

    // 重开按钮
    if (sf::FloatRect({btnStartX + btnW + btnGap, btnY2}, {btnW, btnH}).contains(pos)) {
        result.action = 5; // 重开
        return result;
    }

    // 关闭按钮
    if (sf::FloatRect({btnStartX + (btnW + btnGap) * 2, btnY2}, {btnW, btnH}).contains(pos)) {
        result.action = 1; // 关闭
        return result;
    }

    // 点击面板外 = 关闭
    if (!sf::FloatRect({panelX, panelY}, {panelW, panelH}).contains(pos))
        result.action = 1;

    return result;
}

// ====== 游戏: 点击检测 ======

int Renderer::hitTestCard(const sf::Vector2f& worldPos,
                           int cardCount, sf::Vector2u winSize,
                           const std::vector<int>& selectedIndices) const
{
    float s = handScale((float)winSize.y);
    float dispW = CARD_W * s;
    float dispH = CARD_H * s;
    float yBase = handCardY((float)winSize.y);
    float lift = winSize.y * 0.025f;
    float middle = (cardCount - 1) / 2.0f;
    float maxAngleDeg = 0.0f;
    float arcCurve = 0.0f;

    for (int i = cardCount - 1; i >= 0; --i) {
        auto pos = handCardPos(i, cardCount, yBase, winSize);
        bool sel = std::find(selectedIndices.begin(), selectedIndices.end(), i)
                   != selectedIndices.end();

        float norm = (cardCount > 1) ? (float)(i - middle) / middle : 0.0f;
        float fanRot = maxAngleDeg * std::sin(norm * 3.14159265f / 2.0f);
        float dist = std::abs(i - middle);
        float arcSink = dist * dist * arcCurve;

        // 1. 计算当前视觉偏移（与 renderGame 绘制逻辑一致）
        float curLift = 0.0f;
        if (sel) {
            curLift = lift;
        } else if (i < (int)m_handCardYOffsets.size()) {
            curLift = m_handCardYOffsets[i];
            if (curLift < 0.5f) curLift = 0.0f;
        }

        float liftX = 0.f, liftY = curLift;
        if (std::abs(fanRot) > 0.5f) {
            float rad = fanRot * 3.14159265f / 180.0f;
            liftX = curLift * std::sin(rad);
            liftY = curLift * std::cos(rad);
        }

        float bcx = pos.x + dispW / 2.0f + liftX;
        float bcy = pos.y + dispH + arcSink - liftY;

        // 2. 实时检测鼠标是否在当前视觉区域内（AABB 近似）
        sf::FloatRect visualBounds({bcx - dispW / 2.0f, bcy - dispH}, {dispW, dispH});
        bool hover = visualBounds.contains(worldPos);

        // 3. 选中或正被悬停时，检测区域直接上浮到完整高度（不受动画延迟影响）
        float hitLift = (sel || hover) ? lift : curLift;
        float hitLiftX = 0.f, hitLiftY = hitLift;
        if (std::abs(fanRot) > 0.5f) {
            float rad = fanRot * 3.14159265f / 180.0f;
            hitLiftX = hitLift * std::sin(rad);
            hitLiftY = hitLift * std::cos(rad);
        }
        float hitCx = pos.x + dispW / 2.0f + hitLiftX;
        float hitCy = pos.y + dispH + arcSink - hitLiftY;

        sf::FloatRect bounds({hitCx - dispW / 2.0f, hitCy - dispH}, {dispW, dispH});
        if (bounds.contains(worldPos)) return i;
    }
    return -1;
}

int Renderer::hitTestGameButton(const sf::Vector2f& worldPos,
                                 bool canPass, sf::Vector2u winSize) const
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;

    // 出牌/不出按钮 (手牌上方居中)
    float btnW = w * 0.10f;
    float btnH = h * 0.05f;
    float btnGap = w * 0.03f;
    float totalW = btnW * 2 + btnGap;
    float startX = (w - totalW) / 2.f;
    float btnY = h * 0.56f;

    sf::FloatRect playBounds({startX + btnW + btnGap, btnY}, {btnW, btnH});
    if (playBounds.contains(worldPos)) return 1;
    if (canPass) {
        sf::FloatRect passBounds({startX, btnY}, {btnW, btnH});
        if (passBounds.contains(worldPos)) return 2;
    }
    return 0;
}

int Renderer::hitTestSettingsButton(const sf::Vector2f& worldPos,
                                     sf::Vector2u winSize)
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;
    float gbSz = h * 0.04f;
    float gbX = w * 0.03f;
    float gbY = h * 0.03f;
    if (sf::FloatRect({gbX, gbY}, {gbSz, gbSz}).contains(worldPos)) return 1;
    return 0;
}

int Renderer::hitTestMomentumButton(const sf::Vector2f& worldPos,
                                     sf::Vector2u winSize) const
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;
    float btnW = w * 0.12f;
    float btnH = h * 0.055f;
    float btnX = (w - btnW) / 2.f;
    float btnY = h * 0.56f;
    if (sf::FloatRect({btnX, btnY}, {btnW, btnH}).contains(worldPos)) return 1;
    return 0;
}

int Renderer::hitTestScheduleButton(const sf::Vector2f& worldPos,
                                     sf::Vector2u winSize) const
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;
    float btnW = w * 0.12f;
    float btnH = h * 0.055f;
    float btnGap = w * 0.03f;
    float totalW = btnW * 2 + btnGap;
    float startX = (w - totalW) / 2.f;
    float btnY = h * 0.56f;
    // 跳过
    if (sf::FloatRect({startX, btnY}, {btnW, btnH}).contains(worldPos)) return 2;
    // 过牌
    if (sf::FloatRect({startX + btnW + btnGap, btnY}, {btnW, btnH}).contains(worldPos)) return 1;
    return 0;
}

int Renderer::hitTestSkillSlot(const sf::Vector2f& worldPos, sf::Vector2u winSize) const
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;
    float skH = h * 0.11f;
    float skW = skH * CARD_W / CARD_H;
    float skGap = w * 0.015f;
    float skStartX = w * 0.03f;
    float skY = h * 0.83f;

    for (int i = 0; i < MAX_SKILL_SLOTS; ++i) {
        float sx = skStartX + i * (skW + skGap);
        float sy = skY + m_skillSlotY[i];
        if (sf::FloatRect({sx, sy}, {skW, skH}).contains(worldPos))
            return i;
    }
    return -1;
}

int Renderer::hitTestDebugButton(const sf::Vector2f& worldPos, sf::Vector2u winSize) const
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;
    float dbgW = w * 0.05f;
    float dbgH = h * 0.035f;
    float dbgY = h * 0.035f;

    if (sf::FloatRect({w * 0.14f, dbgY}, {dbgW, dbgH}).contains(worldPos)) return 1; // 我赢
    if (sf::FloatRect({w * 0.20f, dbgY}, {dbgW, dbgH}).contains(worldPos)) return 2; // 我输
    return 0;
}

int Renderer::hitTestCharPortrait(const sf::Vector2f& worldPos, sf::Vector2u winSize,
                                  int charId, const GameState& state) const
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;

    // 掌控者在擢升/飞行期间不响应点击
    if (charId == 2 && (state.phase() == GameState::Phase::SchedulePlay
                        || m_scheduleFlyProgress >= 0.f))
        return 0;

    float imgH = h * 0.25f;
    float boxW = h * 0.15f;
    float boxH = h * 0.24f;
    float boxX = w - boxW - w * 0.02f;
    float boxY = h - boxH - h * 0.02f;
    if (sf::FloatRect({boxX, boxY}, {boxW, boxH}).contains(worldPos)) return 1;
    return 0;
}
