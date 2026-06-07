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
void drawOctagonIcon(sf::RenderWindow& window,
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
void drawBeveledRect(sf::RenderWindow& window, float x, float y, float w, float h, float cut,
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

    std::snprintf(buf, sizeof(buf), "%s/card.png", imageDir.c_str());
    if (!m_backTexture.loadFromFile(buf)) {
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

    // 背景音乐
    if (!m_bgMusic.openFromFile("resources/music/first.mp3")) {
        std::fprintf(stderr, "Failed to load music\n");
        return false;
    }
    m_bgMusic.setLooping(true);
    m_bgMusic.play();

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
    m_bgMusic.setVolume(m_musicVolume);
}

void Renderer::setSoundVolume(float v)
{
    m_soundVolume = std::clamp(v, 0.f, 100.f);
    if (m_hoverSnd) m_hoverSnd->setVolume(m_soundVolume);
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

// 将单张角色卡片的内容绘制到预分配的 RenderTexture
void Renderer::renderCharCardToRT(int charIdx, bool hover)
{
    auto& rt = m_charRT[charIdx];
    auto& c  = getAllCharacters()[charIdx];

    rt.clear(sf::Color::Transparent);

    float cw = (float)CHAR_RT_W;
    float ch = (float)CHAR_RT_H;

    sf::Color charColor = charStreetColor(charIdx);
    float cut = 12.f;
    // ---- 背景底框（切角矩形） ----
    sf::ConvexShape bg(8);
    bg.setPoint(0, {cut, 0}); bg.setPoint(1, {cw - cut, 0});
    bg.setPoint(2, {cw, cut}); bg.setPoint(3, {cw, ch - cut});
    bg.setPoint(4, {cw - cut, ch}); bg.setPoint(5, {cut, ch});
    bg.setPoint(6, {0, ch - cut}); bg.setPoint(7, {0, cut});
    bg.setFillColor(hover ? sf::Color(charColor.r/4, charColor.g/4, charColor.b/4) : sf::Color(13, 13, 13));
    bg.setOutlineColor(hover ? charColor : OUTLINE_BLACK);
    bg.setOutlineThickness(hover ? 4.f : 3.f);
    rt.draw(bg);

    // ---- 顶部角标 OP-0N（角色专属色） ----
    sf::RectangleShape tag({60.f, 22.f});
    tag.setFillColor(charColor);
    rt.draw(tag);
    sf::Text tagText(m_font, L"OP-0" + std::to_wstring(charIdx + 1), 14);
    tagText.setFillColor(OUTLINE_BLACK);
    tagText.setStyle(sf::Text::Bold);
    auto tsz = tagText.getGlobalBounds().size;
    tagText.setPosition({(60.f - tsz.x) / 2.f, (22.f - tsz.y) / 2.f - 2.f});
    rt.draw(tagText);

    // ---- 头像占位 [IMG-CHAR-AVATAR] ----
    float iconSize = cw * 0.45f;
    float iconX = (cw - iconSize) / 2.0f;
    float iconY = ch * 0.10f;
    sf::RectangleShape iconBg({iconSize, iconSize});
    iconBg.setPosition({iconX, iconY});
    iconBg.setFillColor(sf::Color(26, 26, 26));
    iconBg.setOutlineColor(OUTLINE_BLACK);
    iconBg.setOutlineThickness(3.f);
    rt.draw(iconBg);
    // 内部波点纹理装饰
    float dotSize = 3.f;
    float spacing = dotSize * 2.5f;
    for (float dy = iconY + spacing; dy < iconY + iconSize - spacing; dy += spacing) {
        for (float dx = iconX + spacing; dx < iconX + iconSize - spacing; dx += spacing) {
            sf::CircleShape dot(dotSize);
            dot.setPosition({dx, dy});
            dot.setFillColor(sf::Color(charColor.r/3, charColor.g/3, charColor.b/3));
            rt.draw(dot);
        }
    }

    sf::Text avText(m_font, std::to_string(charIdx + 1),
                    (unsigned)(iconSize * 0.45f));
    avText.setFillColor(charColor);
    auto asz = avText.getGlobalBounds().size;
    avText.setPosition({iconX + (iconSize - asz.x) / 2.0f,
                        iconY + (iconSize - asz.y) / 2.0f});
    rt.draw(avText);

    // ---- 角色名称（粗黑描底） ----
    float nameF = ch * 0.065f;
    sf::Text nameShadow(m_font, c.name, (unsigned)nameF);
    nameShadow.setFillColor(OUTLINE_BLACK);
    nameShadow.setStyle(sf::Text::Bold);
    auto nsz = nameShadow.getGlobalBounds().size;
    nameShadow.setPosition({(cw - nsz.x) / 2.0f + 3.f, ch * 0.50f + 3.f});
    rt.draw(nameShadow);

    sf::Text name(m_font, c.name, (unsigned)nameF);
    name.setFillColor(STREET_WHITE);
    name.setStyle(sf::Text::Bold);
    name.setPosition({(cw - nsz.x) / 2.0f, ch * 0.50f});
    rt.draw(name);

    // ---- 被动技名 ----
    float passF = ch * 0.05f;
    sf::Text passName(m_font, c.passiveName, (unsigned)passF);
    passName.setFillColor(STREET_YELLOW);
    auto pnsz = passName.getGlobalBounds().size;
    passName.setPosition({(cw - pnsz.x) / 2.0f, ch * 0.60f});
    rt.draw(passName);

    // ---- 卡片四角小星星装饰 ----
    if (hover) {
        float starR = 8.f;
        auto drawStarLocal = [&](float sx, float sy) {
            sf::ConvexShape star(10);
            for (int i = 0; i < 10; ++i) {
                float angle = (i * 36.f - 90.f) * 3.14159265f / 180.f;
                float rad = (i % 2 == 0) ? starR : starR * 0.4f;
                star.setPoint(i, {sx + std::cos(angle) * rad, sy + std::sin(angle) * rad});
            }
            star.setFillColor(charColor);
            star.setOutlineColor(OUTLINE_BLACK);
            star.setOutlineThickness(2.f);
            rt.draw(star);
        };
        drawStarLocal(cut + starR, cut + starR);
        drawStarLocal(cw - cut - starR, cut + starR);
        drawStarLocal(cut + starR, ch - cut - starR);
        drawStarLocal(cw - cut - starR, ch - cut - starR);
    }

    // ---- 被动技描述 (超宽文本自动双行居中) ----
    float descF = ch * 0.035f;
    float margin = 28.f;
    std::wstring descText = c.passiveDesc;

    sf::Text measure(m_font, descText, (unsigned)descF);
    if (measure.getGlobalBounds().size.x > cw - margin * 2.f) {
        // 优先在标点处断开
        size_t split = std::wstring::npos;
        for (auto brk : {L'，', L',', L' '})
            if ((split = descText.find(brk)) != std::wstring::npos && split > 0)
                break;
        if (split == std::wstring::npos)
            split = descText.size() / 2;

        std::wstring l1 = descText.substr(0, split + 1);
        std::wstring l2 = descText.substr(split + 1);

        sf::Text line1(m_font, l1, (unsigned)descF);
        line1.setFillColor(sf::Color(200, 200, 200));
        auto sz1 = line1.getGlobalBounds().size;
        line1.setPosition({(cw - sz1.x) / 2.f, ch * 0.70f});
        rt.draw(line1);

        sf::Text line2(m_font, l2, (unsigned)descF);
        line2.setFillColor(sf::Color(200, 200, 200));
        auto sz2 = line2.getGlobalBounds().size;
        line2.setPosition({(cw - sz2.x) / 2.f, ch * 0.76f});
        rt.draw(line2);
    } else {
        sf::Text passDesc(m_font, descText, (unsigned)descF);
        passDesc.setFillColor(sf::Color(200, 200, 200));
        auto pdsz = passDesc.getGlobalBounds().size;
        passDesc.setPosition({(cw - pdsz.x) / 2.0f, ch * 0.73f});
        rt.draw(passDesc);
    }

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

    // 非游戏背景时叠加涂鸦装饰层
    if (!useGameBg) {
        // 角落小星星
        drawStar(m_window, w * 0.05f, h * 0.92f, 10.f, STREET_PINK, 2.f);
        drawStar(m_window, w * 0.95f, h * 0.08f, 12.f, STREET_CYAN, 2.f);
        drawStar(m_window, w * 0.15f, h * 0.15f, 8.f, STREET_YELLOW, 2.f);
        drawStar(m_window, w * 0.85f, h * 0.85f, 9.f, STREET_PINK, 2.f);
        // 底部锯齿装饰
        drawZigzagBorder(m_window, w * 0.2f, h * 0.96f, w * 0.6f, STREET_CYAN, 3.f, 6.f);
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
    float bx = w * 0.03f;
    float by = h * 0.03f;
    float bbw = w * 0.08f;
    float bbh = h * 0.045f;
    sf::FloatRect backRect({bx, by}, {bbw, bbh});
    drawMenuButton(backRect, L"返回", true, backRect.contains(mousePos), winSize);
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
                              sf::Vector2u winSize)
{
    (void)winSize;
    auto& skills = getAllSkills();
    float cut = 10.f;

    if (skillId < 0 || skillId >= SKILL_COUNT) {
        // 空槽位（切角矩形）
        drawBeveledRect(m_window, x, y, w, h, 6.f,
                        slotEmptyColor, BORDER_NORMAL, 1.f);
        return;
    }

    auto& sk = skills[skillId];
    sf::Color typeColor = skillTypeStreetColor(sk.type);

    // 底色（切角矩形）— 根据技能类型使用街头潮流色
    sf::Color fill = owned ? skillCardOwned
                   : (hover ? sf::Color(typeColor.r/5, typeColor.g/5, typeColor.b/5) : STREET_BLACK);
    sf::Color outline = hover ? typeColor : OUTLINE_BLACK;
    drawBeveledRect(m_window, x, y, w, h, cut, fill, outline, hover ? 4.f : 3.f);

    // 顶部色带 — 跟随技能类型色
    sf::RectangleShape topBar({w - cut * 2, h * 0.04f});
    topBar.setPosition({x + cut, y + 2.f});
    topBar.setFillColor(hover ? typeColor : sf::Color(26, 26, 26));
    m_window.draw(topBar);

    // 已拥有角标 — 荧光粉底黑字粗描边
    if (owned) {
        sf::RectangleShape ownedTag({w * 0.28f, h * 0.045f});
        ownedTag.setPosition({x + 2.f, y + 4.f});
        ownedTag.setFillColor(STREET_PINK);
        ownedTag.setOutlineColor(OUTLINE_BLACK);
        ownedTag.setOutlineThickness(2.f);
        m_window.draw(ownedTag);
        sf::Text ownedText(m_font, L"OWNED", (unsigned)(h * 0.035f));
        ownedText.setFillColor(OUTLINE_BLACK);
        ownedText.setStyle(sf::Text::Bold);
        auto osz = ownedText.getGlobalBounds().size;
        ownedText.setPosition({x + (w * 0.28f - osz.x) / 2.f, y + 4.f + (h * 0.045f - osz.y) / 2.f - 2.f});
        m_window.draw(ownedText);
    }

    // [IMG-SKILL-S01~S08] 技能图标占位（线框八角形 + 内部波点）
    float iconSize = w * 0.30f;
    float iconX = x + (w - iconSize) / 2.f;
    float iconY = y + h * 0.08f;
    drawOctagonIcon(m_window, iconX, iconY, iconSize, sf::Color(10, 10, 10), OUTLINE_BLACK, 3.f);
    // 内部波点纹理
    float dotSize = iconSize * 0.06f;
    float spacing = dotSize * 2.5f;
    for (float dy = iconY + spacing; dy < iconY + iconSize - spacing; dy += spacing) {
        for (float dx = iconX + spacing; dx < iconX + iconSize - spacing; dx += spacing) {
            sf::CircleShape dot(dotSize);
            dot.setPosition({dx, dy});
            dot.setFillColor(sf::Color(typeColor.r/4, typeColor.g/4, typeColor.b/4));
            m_window.draw(dot);
        }
    }

    sf::Text iconText(m_font, "S" + std::to_string(skillId + 1),
                      (unsigned)(iconSize * 0.38f));
    iconText.setFillColor(typeColor);
    auto isz = iconText.getGlobalBounds().size;
    iconText.setPosition({x + (w - isz.x) / 2.f, iconY + (iconSize - isz.y) / 2.f});
    m_window.draw(iconText);

    // 技能名 — 居中于图标和底部信息栏之间（粗黑描底）
    float iconBot = iconY + iconSize;
    float infoTop = y + h - h * 0.065f;
    float nameF = h * 0.10f;
    sf::Text nameShadow(m_font, sk.name, (unsigned)nameF);
    nameShadow.setFillColor(OUTLINE_BLACK);
    nameShadow.setStyle(sf::Text::Bold);
    auto nsz = nameShadow.getGlobalBounds().size;
    nameShadow.setPosition({x + (w - nsz.x) / 2.f + 2.f, (iconBot + infoTop - nsz.y) / 2.f + 2.f});
    m_window.draw(nameShadow);

    sf::Text name(m_font, sk.name, (unsigned)nameF);
    name.setFillColor(STREET_WHITE);
    name.setStyle(sf::Text::Bold);
    name.setPosition({x + (w - nsz.x) / 2.f, (iconBot + infoTop - nsz.y) / 2.f});
    m_window.draw(name);

    // 底部黑条区域
    sf::RectangleShape infoBar({w - cut * 2, h * 0.055f});
    infoBar.setPosition({x + cut, y + h - h * 0.065f});
    infoBar.setFillColor(sf::Color(5, 5, 5));
    m_window.draw(infoBar);

    std::wstring typeStr = skillTypeLabel(sk.type);
    sf::Text costText(m_font, typeStr, (unsigned)(h * 0.032f));
    costText.setFillColor(typeColor);
    auto csz = costText.getGlobalBounds().size;
    costText.setPosition({x + (w - csz.x) / 2.f, y + h - h * 0.058f});
    m_window.draw(costText);
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
    drawTitle(L"选择单位", 0.06f, winSize);

    float w = (float)winSize.x;
    float h = (float)winSize.y;

    // 实际显示尺寸（基于窗口比例）
    float actualCW = w * 0.20f;
    float actualCH = actualCW * 12.0f / 7.0f;
    float gap = w * 0.05f;
    float totalW = CHAR_COUNT * actualCW + (CHAR_COUNT - 1) * gap;
    float startX = (w - totalW) / 2.0f;
    float startY = (h - actualCH) / 2.0f + h * 0.03f;

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
}

int Renderer::hitCharacterSelect(const sf::Vector2f& pos, sf::Vector2u winSize)
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;

    float cw = w * 0.20f;
    float ch = cw * 12.0f / 7.0f;
    float gap = w * 0.05f;
    float totalW = CHAR_COUNT * cw + (CHAR_COUNT - 1) * gap;
    float startX = (w - totalW) / 2.f;
    float startY = (h - ch) / 2.f + h * 0.03f;

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

    float bx = w * 0.03f, by = h * 0.03f;
    float bbw = w * 0.08f, bbh = h * 0.045f;
    if (sf::FloatRect({bx, by}, {bbw, bbh}).contains(pos)) return 9;
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
    sf::Text hint(m_font, L"选择一张点数作为癞子牌（万能牌）", (unsigned)(h * 0.03f));
    hint.setFillColor(sf::Color(160, 160, 160));
    auto hsz = hint.getGlobalBounds().size;
    hint.setPosition({(w - hsz.x) / 2.0f, baseY + cardH + h * 0.06f});
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

// ---- Transition 卡池布局 ----
struct TransitionPoolLayout {
    float cardW, cardH, poolX, poolY, colGap, rowGap;
    static constexpr int COLS = 2;
};

static TransitionPoolLayout calcTransitionPoolLayout(sf::Vector2u winSize)
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;
    float cardH = h * 0.16f;                 // 竖版卡牌 7:12
    float cardW = cardH * Renderer::CARD_W / Renderer::CARD_H;
    return {
        cardW, cardH,
        w * 0.06f,       // poolX
        h * 0.14f,       // poolY
        w * 0.03f,       // colGap
        h * 0.015f       // rowGap
    };
}

sf::FloatRect Renderer::transitionPoolCardRect(int cardIndex, sf::Vector2u winSize) const
{
    auto L = calcTransitionPoolLayout(winSize);
    int col = cardIndex % L.COLS;
    int row = cardIndex / L.COLS;
    float x = L.poolX + col * (L.cardW + L.colGap);
    float y = L.poolY + row * (L.cardH + L.rowGap);
    return {{x, y}, {L.cardW, L.cardH}};
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
    auto L = calcTransitionPoolLayout(winSize);
    int maxRows = (SKILL_COUNT + L.COLS - 1) / L.COLS;
    float areaW = L.COLS * L.cardW + (L.COLS - 1) * L.colGap + (float)winSize.x * 0.04f;
    float areaH = maxRows * L.cardH + (maxRows - 1) * L.rowGap + (float)winSize.y * 0.03f;
    return sf::FloatRect({L.poolX - (float)winSize.x * 0.02f,
                          L.poolY - (float)winSize.y * 0.015f}, {areaW, areaH}).contains(pos);
}

void Renderer::drawTransition(sf::Vector2u winSize, const sf::Vector2f& mousePos,
                               int level, const std::vector<int>& acquiredSkills,
                               const std::array<int, MAX_SKILL_SLOTS>& equipped,
                               int hoveredAcquiredIdx, int hoveredSlotIdx,
                               int dragSourceType, int dragSourceIndex,
                               int dragSkillId, bool isDragging)
{
    drawBackground(winSize, true);
    drawBackButton(winSize, mousePos);

    float w = (float)winSize.x;
    float h = (float)winSize.y;

    // 标题
    drawTitle(L"第 " + std::to_wstring(level) + L" 关", 0.05f, winSize);

    auto& allSkills = getAllSkills();

    // ---- 卡池布局计算 ----
    auto L = calcTransitionPoolLayout(winSize);

    // ---- 左侧: 已获得技能卡池 ----
    sf::Text heading(m_font, L"已获得协议", (unsigned)(h * 0.028f));
    heading.setFillColor(TEXT_DIM);
    heading.setPosition({L.poolX, L.poolY - h * 0.04f});
    m_window.draw(heading);

    // 卡池背景 — 从槽位拖出时高亮
    bool poolHighlight = isDragging && dragSourceType == 2;
    int totalRows = ((int)acquiredSkills.size() + L.COLS - 1) / L.COLS;
    if (totalRows < 1) totalRows = 1;
    float poolBgW = L.COLS * L.cardW + (L.COLS - 1) * L.colGap + w * 0.04f;
    float poolBgH = totalRows * L.cardH + (totalRows - 1) * L.rowGap + h * 0.03f;
    sf::RectangleShape poolBg({poolBgW, poolBgH});
    poolBg.setPosition({L.poolX - w * 0.02f, L.poolY - h * 0.015f});
    poolBg.setFillColor(poolHighlight ? sf::Color(STREET_CYAN.r/6, STREET_CYAN.g/6, STREET_CYAN.b/6, 180) : sf::Color(10, 10, 10, 120));
    poolBg.setOutlineColor(poolHighlight ? STREET_CYAN : OUTLINE_BLACK);
    poolBg.setOutlineThickness(3.f);
    m_window.draw(poolBg);

    for (size_t i = 0; i < acquiredSkills.size(); ++i) {
        int sid = acquiredSkills[i];
        if (sid < 0 || sid >= SKILL_COUNT) continue;
        int col = (int)i % L.COLS;
        int row = (int)i / L.COLS;
        float cx = L.poolX + col * (L.cardW + L.colGap);
        float cy = L.poolY + row * (L.cardH + L.rowGap);

        bool isEquipped = false;
        for (int e = 0; e < MAX_SKILL_SLOTS; ++e)
            if (equipped[e] == sid) { isEquipped = true; break; }
        bool hover = ((int)i == hoveredAcquiredIdx);
        bool isBeingDragged = isDragging && dragSourceType == 1
                              && dragSourceIndex == (int)i;

        if (isBeingDragged || isEquipped) {
            // 占位孔: 拖拽中或已装备的技能显示为空槽
            drawBeveledRect(m_window, cx, cy, L.cardW, L.cardH, 10.f,
                            sf::Color(20, 20, 20, 60), BORDER_NORMAL, 1.f);
            if (isEquipped && !isBeingDragged) {
                // 已装备提示
                sf::Text equippedHint(m_font, L"已装备", (unsigned)(L.cardH * 0.09f));
                equippedHint.setFillColor(TEXT_DISABLED);
                auto ehsz = equippedHint.getGlobalBounds().size;
                equippedHint.setPosition({cx + (L.cardW - ehsz.x) / 2.f, cy + L.cardH * 0.80f});
                m_window.draw(equippedHint);
            }
        } else {
            drawSkillCard(cx, cy, L.cardW, L.cardH, sid, false, hover, winSize);
        }
    }

    if (acquiredSkills.empty()) {
        sf::Text empty(m_font, L"暂无技能 (击败敌人后获得)", (unsigned)(h * 0.026f));
        empty.setFillColor(sf::Color(128, 128, 128));
        empty.setPosition({L.poolX, L.poolY});
        m_window.draw(empty);
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
        bool highlight = slotHover || isDropTarget;

        float slotCut = 4.f;
        float baseY = slotStartY + (highlight ? -2.f : 0.f);

        // 被拖出时显示空槽样式
        int drawSid = isBeingDraggedFrom ? -1 : sid;

        sf::Color sfill = slotEmptyColor;
        sf::Color soutline = OUTLINE_BLACK;
        if (drawSid >= 0) {
            auto& sk = allSkills[drawSid];
            sf::Color tc = skillTypeStreetColor(sk.type);
            sfill = sf::Color(tc.r/6, tc.g/6, tc.b/6);
            soutline = highlight ? tc : OUTLINE_BLACK;
        } else if (highlight) {
            sfill = sf::Color(26, 26, 10);
            soutline = STREET_YELLOW;
        }
        drawBeveledRect(m_window, sx, baseY, slotW, slotH, slotCut,
                        sfill, soutline, highlight ? 4.f : 3.f);

        if (drawSid >= 0) {
            auto& sk = allSkills[drawSid];
            sf::Color tc = skillTypeStreetColor(sk.type);
            sf::RectangleShape sbar({slotW - slotCut * 2, 6.f});
            sbar.setPosition({sx + slotCut, baseY + 2.f});
            sbar.setFillColor(tc);
            m_window.draw(sbar);

            sf::Text slotText(m_font, sk.name, (unsigned)(slotH * 0.15f));
            slotText.setFillColor(STREET_WHITE);
            slotText.setStyle(sf::Text::Bold);
            auto tsz = slotText.getGlobalBounds().size;
            slotText.setPosition({sx + (slotW - tsz.x) / 2.f, baseY + slotH * 0.22f});
            m_window.draw(slotText);

            std::wstring typeStr = skillTypeLabel(sk.type);
            sf::Text cost(m_font, typeStr, (unsigned)(slotH * 0.12f));
            cost.setFillColor(tc);
            auto csz = cost.getGlobalBounds().size;
            cost.setPosition({sx + (slotW - csz.x) / 2.f, baseY + slotH * 0.60f});
            m_window.draw(cost);
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

    // ---- 技能说明 (鼠标悬停卡池或装备槽时显示) ----
    bool showSkillDesc = (hoveredAcquiredIdx >= 0
                       && hoveredAcquiredIdx < (int)acquiredSkills.size())
                      || (hoveredSlotIdx >= 0 && equipped[hoveredSlotIdx] >= 0);
    float descY = slotStartY + slotH + h * 0.005f;

    if (showSkillDesc) {
        int hoveredSid = -1;
        if (hoveredAcquiredIdx >= 0 && hoveredAcquiredIdx < (int)acquiredSkills.size())
            hoveredSid = acquiredSkills[hoveredAcquiredIdx];
        else if (hoveredSlotIdx >= 0 && equipped[hoveredSlotIdx] >= 0)
            hoveredSid = equipped[hoveredSlotIdx];
        if (hoveredSid >= 0 && hoveredSid < SKILL_COUNT) {
            auto& sk = allSkills[hoveredSid];

            // 说明背景
            float descW = slotW * 3.f + slotGap * 2.f;
            float descH = h * 0.065f;
            sf::Color tc = skillTypeStreetColor(sk.type);
            sf::RectangleShape descBg({descW, descH});
            descBg.setPosition({rightX, descY});
            descBg.setFillColor(sf::Color(15, 15, 15, 200));
            descBg.setOutlineColor(tc);
            descBg.setOutlineThickness(3.f);
            m_window.draw(descBg);

            // 技能名称
            float nameFont = (unsigned)(h * 0.026f);
            sf::Text nameText(m_font, sk.name, (unsigned)nameFont);
            nameText.setFillColor(tc);
            nameText.setStyle(sf::Text::Bold);
            nameText.setPosition({rightX + w * 0.01f, descY + descH * 0.05f});
            m_window.draw(nameText);

            // 技能类型标签
            auto st = sk.type;
            std::wstring typeStr = skillTypeLabel(st);
            sf::Text typeTag(m_font, typeStr, (unsigned)(h * 0.018f));
            typeTag.setFillColor(tc);
            auto ttsz = nameText.getGlobalBounds().size;
            typeTag.setPosition({rightX + w * 0.01f + ttsz.x + w * 0.015f,
                                 descY + descH * 0.08f});
            m_window.draw(typeTag);

            // 技能描述
            sf::Text descText(m_font, sk.desc, (unsigned)(h * 0.021f));
            descText.setFillColor(sf::Color(220, 220, 220));
            descText.setPosition({rightX + w * 0.01f, descY + descH * 0.50f});
            m_window.draw(descText);
        }
    }

    // ---- 敌人预览 ----
    float enemyY = showSkillDesc ? (descY + h * 0.075f) : (slotStartY + slotH + h * 0.03f);
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
        for (int i = 0; i < MAX_SKILL_SLOTS; ++i) {
            int sid = equipped[i];
            std::wstring label;
            if (sid >= 0) {
                auto& sk = allSkills[sid];
                label = L"[" + sk.name + L"]";
            } else {
                label = L"[空]";
            }
            sf::Text es(m_font, label, (unsigned)(h * 0.026f));
            es.setFillColor(sid >= 0 ? sf::Color(255, 200, 100) : sf::Color(120, 120, 120));
            es.setPosition({rightX + w * 0.12f * i, enemyY + h * 0.035f});
            m_window.draw(es);
        }
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
        auto& sk = allSkills[dragSkillId];
        float ghostW = L.cardW * 1.05f;
        float ghostH = L.cardH * 1.05f;
        float gx = mousePos.x - ghostW / 2.f;
        float gy = mousePos.y - ghostH / 2.f;
        float gcut = 10.f;

        // 半透明卡体
        drawBeveledRect(m_window, gx, gy, ghostW, ghostH, gcut,
                        sf::Color(15, 20, 10, 210),
                        sf::Color(204, 255, 0, 180), 2.f);

        // 顶部色带
        sf::RectangleShape gbar({ghostW - gcut * 2, ghostH * 0.04f});
        gbar.setPosition({gx + gcut, gy + 2.f});
        gbar.setFillColor(sf::Color(204, 255, 0, 180));
        m_window.draw(gbar);

        // 图标
        float iconSize = ghostW * 0.30f;
        float iconX = gx + (ghostW - iconSize) / 2.f;
        float iconY = gy + ghostH * 0.08f;
        drawOctagonIcon(m_window, iconX, iconY, iconSize, sf::Color(10, 10, 10, 210), sf::Color(204, 255, 0, 180));

        sf::Text gIcon(m_font, L"S" + std::to_wstring(dragSkillId + 1),
                       (unsigned)(iconSize * 0.38f));
        gIcon.setFillColor(sf::Color(204, 255, 0, 200));
        auto isz = gIcon.getGlobalBounds().size;
        gIcon.setPosition({gx + (ghostW - isz.x) / 2.f, iconY + (iconSize - isz.y) / 2.f});
        m_window.draw(gIcon);

        // 技能名
        float nameF = ghostH * 0.10f;
        sf::Text gName(m_font, sk.name, (unsigned)nameF);
        gName.setFillColor(sf::Color(255, 255, 255, 200));
        gName.setStyle(sf::Text::Bold);
        auto nsz = gName.getGlobalBounds().size;
        float iconBot = iconY + iconSize;
        float gInfoTop = gy + ghostH - ghostH * 0.065f;
        gName.setPosition({gx + (ghostW - nsz.x) / 2.f,
                           (iconBot + gInfoTop - nsz.y) / 2.f});
        m_window.draw(gName);
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

    float bx = w * 0.03f, by = h * 0.03f;
    float bbw = w * 0.08f, bbh = h * 0.045f;
    if (sf::FloatRect({bx, by}, {bbw, bbh}).contains(pos)) return 9;
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
    if (!selectedIndices.empty() && state.phase() == GameState::Phase::PlayerTurn) {
        // 提取选中牌并分类
        std::vector<Card> selCards;
        auto& ph = state.playerHand();
        for (int idx : selectedIndices)
            if (idx >= 0 && idx < (int)ph.size())
                selCards.push_back(ph[idx]);
        auto pattern = GameState::classifyHand(selCards, &state.playerBuffs());

        std::wstring preview;
        sf::Color prevCol = sf::Color::Black;
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
    }

    std::wstring statusStr;
    sf::Color statusCol = TEXT_DIM;
    switch (state.phase()) {
    case GameState::Phase::PlayerTurn:
        statusStr = state.isNewRound() ? L"[STATUS] 新一轮"
                                       : L"[STATUS] 你的回合";
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

    // 炸弹收藏家印记显示（荧光粉星星图标）
    float bmW = w * 0.07f;
    float bmH = h * 0.019f;
    float bmX = w * 0.78f;
    float bmY = h * 0.965f;
    int marks = state.playerBombMarks();
    for (int m = 0; m < 3; ++m) {
        float bx = bmX + m * (bmH + 8.f);
        float starR = bmH * 0.6f;
        if (m < marks) {
            drawStar(m_window, bx + starR, bmY + starR, starR, STREET_PINK, 2.f);
        } else {
            drawStar(m_window, bx + starR, bmY + starR, starR, sf::Color(60, 60, 60), 1.f);
        }
    }
    sf::Text bmLabel(m_font, L"印记 " + std::to_wstring(marks) + L"/3",
                     (unsigned)(bmH * 0.9f));
    bmLabel.setFillColor(marks > 0 ? STREET_PINK : TEXT_DIM);
    bmLabel.setPosition({bmX + 3 * (bmH + 8.f) + 6.f, bmY - 2.f});
    m_window.draw(bmLabel);

    // 返回按钮（切角风格）
    {
        float rx = w * 0.03f;
        float ry = h * 0.03f;
        float rw = w * 0.09f;
        float rh = h * 0.045f;
        bool rHover = sf::FloatRect({rx, ry}, {rw, rh}).contains(mousePos);
        drawBeveledRect(m_window, rx, ry, rw, rh, 4.f,
                        btnColor, rHover ? STREET_CYAN : OUTLINE_BLACK, 3.f);

        m_returnBtnText->setString(L"返回");
        m_returnBtnText->setCharacterSize((unsigned)(rh * 0.38f));
        m_returnBtnText->setFillColor(sf::Color::White);
        auto rsz = m_returnBtnText->getGlobalBounds().size;
        m_returnBtnText->setPosition({rx + (rw - rsz.x) / 2.f, ry + rh * 0.12f});
        m_window.draw(*m_returnBtnText);
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

    // 设置按钮 (右上角) — 低调齿轮风格
    {
        float gbSz = h * 0.04f;               // 小正方形
        float gbX = w - gbSz - w * 0.025f;
        float gbY = h * 0.028f;
        bool gHover = sf::FloatRect({gbX, gbY}, {gbSz, gbSz}).contains(mousePos);
        drawBeveledRect(m_window, gbX, gbY, gbSz, gbSz, 3.f,
                        btnColor, gHover ? STREET_CYAN : OUTLINE_BLACK, 2.f);
        // 齿轮图标: 中心圆 + 外围短线
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
        // 外围齿
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

    // 技能槽 (卡牌比例 CARD_W:CARD_H, 左下角) — 整体面板
    auto& allSkills = getAllSkills();
    float skH = h * 0.11f;
    float skW = skH * CARD_W / CARD_H;
    float skGap = w * 0.015f;
    float skStartX = w * 0.03f;
    float skY = h * 0.83f;
    float panelW = skW * 3 + skGap * 2 + 16.f;
    float panelH = skH + 28.f;

    // 整体切角面板（3px 纯黑描边）
    drawBeveledRect(m_window, skStartX - 8.f, skY - 22.f, panelW, panelH, 6.f,
                    sf::Color(10, 10, 10), OUTLINE_BLACK, 3.f);
    sf::Text slotPanelLabel(m_font, L"装备槽", (unsigned)(h * 0.018f));
    slotPanelLabel.setFillColor(STREET_CYAN);
    slotPanelLabel.setPosition({skStartX - 4.f, skY - 20.f});
    m_window.draw(slotPanelLabel);

    for (int i = 0; i < MAX_SKILL_SLOTS; ++i) {
        float sx = skStartX + i * (skW + skGap);
        float sy = skY + m_skillSlotY[i]; // 点击反馈动画
        int sid = playerSkillIds[i];
        float skCut = 3.f;

        bool slotGlow = (glowMask >> i) & 1;
        sf::Color slotColor = slotEmptyColor;
        sf::Color topBand = BORDER_NORMAL;
        sf::Color outlineCol = OUTLINE_BLACK;
        if (sid >= 0) {
            auto& sk = allSkills[sid];
            sf::Color tc = skillTypeStreetColor(sk.type);
            slotColor = sf::Color(tc.r/6, tc.g/6, tc.b/6);
            topBand = tc;
            outlineCol = slotGlow ? STREET_YELLOW : OUTLINE_BLACK;
        }
        drawBeveledRect(m_window, sx, sy, skW, skH, skCut,
                        slotColor, outlineCol, slotGlow ? 4.f : 3.f);

        if (sid >= 0) {
            auto& sk = allSkills[sid];
            // 顶部色带
            sf::RectangleShape tbar({skW - skCut * 2, 6.f});
            tbar.setPosition({sx + skCut, sy + 2.f});
            tbar.setFillColor(topBand);
            m_window.draw(tbar);

            // 线框八角形图标
            float iconSize = skW * 0.28f;
            float iconX = sx + (skW - iconSize) / 2.f;
            float iconY = sy + skH * 0.10f;
            drawOctagonIcon(m_window, iconX, iconY, iconSize, sf::Color(10, 10, 10), OUTLINE_BLACK, 2.f);

            sf::Text iconText(m_font, "S" + std::to_string(sid + 1),
                              (unsigned)(iconSize * 0.38f));
            iconText.setFillColor(topBand);
            auto isz = iconText.getGlobalBounds().size;
            iconText.setPosition({sx + (skW - isz.x) / 2.f, iconY + (iconSize - isz.y) / 2.f});
            m_window.draw(iconText);

            // 技能名
            float nameF = skW * 0.22f;
            m_skillBtnTexts[i]->setString(sk.name);
            m_skillBtnTexts[i]->setCharacterSize((unsigned)nameF);
            m_skillBtnTexts[i]->setFillColor(sf::Color::White);
            auto tsz = m_skillBtnTexts[i]->getGlobalBounds().size;
            m_skillBtnTexts[i]->setPosition({sx + (skW - tsz.x) / 2.f,
                                              sy + skH * 0.48f});
            m_window.draw(*m_skillBtnTexts[i]);

            // 技能类型标签
            std::wstring info = skillTypeLabel(sk.type);
            sf::Text infoText(m_font, info, (unsigned)(skW * 0.20f));
            infoText.setFillColor(topBand);
            auto csz = infoText.getGlobalBounds().size;
            infoText.setPosition({sx + (skW - csz.x) / 2.f, sy + skH * 0.78f});
            m_window.draw(infoText);
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

    // 敌人技能图标显示 (顶部, 卡牌比例) — 暗红警示风格
    auto& enemySlots = state.enemySkillSlots();
    float eskH = h * 0.065f;
    float eskW = eskH * CARD_W / CARD_H;
    float eskGap = w * 0.012f;
    float totalEskW = MAX_SKILL_SLOTS * eskW + (MAX_SKILL_SLOTS - 1) * eskGap;
    float eskX = w - totalEskW - w * 0.03f;
    float eskY = h * 0.02f;

    for (int i = 0; i < MAX_SKILL_SLOTS; ++i) {
        int esid = enemySlots[i];
        float ex = eskX + i * (eskW + eskGap);
        float ec = 2.f;

        sf::Color efill = (esid >= 0) ? sf::Color(80, 20, 40)
                          : sf::Color(17, 17, 17);
        sf::Color eout = (esid >= 0) ? STREET_PINK : OUTLINE_BLACK;
        drawBeveledRect(m_window, ex, eskY, eskW, eskH, ec, efill, eout, 3.f);

        if (esid >= 0) {
            auto& esk = allSkills[esid];
            sf::Text et(m_font, esk.name, (unsigned)(eskW * 0.18f));
            et.setFillColor(sf::Color(255, 180, 180));
            auto etsz = et.getGlobalBounds().size;
            et.setPosition({ex + (eskW - etsz.x) / 2.f, eskY + eskH * 0.20f});
            m_window.draw(et);

            std::wstring typeStr = skillTypeLabel(esk.type);
            sf::Text costText(m_font, typeStr, (unsigned)(eskW * 0.20f));
            costText.setFillColor(sf::Color(200, 150, 150));
            auto csz = costText.getGlobalBounds().size;
            costText.setPosition({ex + (eskW - csz.x) / 2.f, eskY + eskH * 0.55f});
            m_window.draw(costText);
        } else {
            sf::Text empty(m_font, L"--", (unsigned)(eskW * 0.22f));
            empty.setFillColor(TEXT_DISABLED);
            auto esz = empty.getGlobalBounds().size;
            empty.setPosition({ex + (eskW - esz.x) / 2.f, eskY + eskH * 0.35f});
            m_window.draw(empty);
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

            if (playHover && canPlaySelected) {
                drawHazardStripes(m_window, px2 + 4.f, py2 + 2.f, pw - 8.f, 3.f, 6.f);
            }

            m_playBtnText->setString(L"出牌");
            m_playBtnText->setCharacterSize((unsigned)(fontSize * m_playBtnHoverScale));
            m_playBtnText->setFillColor(canPlaySelected ? (playHover ? OUTLINE_BLACK : STREET_WHITE) : sf::Color(85, 85, 0));
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
                          float dt)
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

    drawBackground(winSize, true);

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
    m_computerLabel->setFillColor(sf::Color::White);
    m_window.draw(*m_computerLabel);
    // 手牌数角标
    sf::Text cCount(m_font, std::to_wstring(cn), (unsigned)(h * 0.022f));
    cCount.setFillColor(sf::Color::Black);
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
        sf::Text epl(m_font, L"敌方出牌", (unsigned)(h * 0.018f));
        epl.setFillColor(TEXT_DIM);
        auto epsz = epl.getGlobalBounds().size;
        epl.setPosition({(w - epsz.x) / 2.f, computerPlayedY(h) - h * 0.08f});
        m_window.draw(epl);
        // 战场淡框
        sf::RectangleShape eBox({w * 0.56f, h * 0.14f});
        eBox.setPosition({(w - w * 0.56f) / 2.f, computerPlayedY(h) - h * 0.07f});
        eBox.setFillColor(sf::Color(10, 10, 10));
        eBox.setOutlineColor(STREET_PINK);
        eBox.setOutlineThickness(3.f);
        m_window.draw(eBox);
    }
    drawPlayedCards(state.lastComputerPlay(), computerPlayedY(h), ps, winSize);

    // --- 玩家出的牌 ---
    {
        sf::Text apl(m_font, L"我方出牌", (unsigned)(h * 0.018f));
        apl.setFillColor(TEXT_DIM);
        auto apsz = apl.getGlobalBounds().size;
        apl.setPosition({(w - apsz.x) / 2.f, playerPlayedY(h) + h * 0.09f});
        m_window.draw(apl);
    }
    drawPlayedCards(state.lastPlayerPlay(), playerPlayedY(h), ps, winSize);

    // --- 连击之势: 全局变暗 (手牌之前绘制，手牌保持亮度) ---
    if (state.phase() == GameState::Phase::MomentumPlay) {
        float fadeT = std::clamp(m_momentumAnimTimer / 0.3f, 0.f, 1.f);
        sf::RectangleShape dimOverlay({w, h});
        dimOverlay.setFillColor(sf::Color(0, 0, 0, (uint8_t)(160 * fadeT)));
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

    m_playerLabel->setString(L"我方单位");
    m_playerLabel->setPosition({w * 0.012f, phY - h * 0.033f});
    m_playerLabel->setFillColor(sf::Color::White);
    m_window.draw(*m_playerLabel);
    // 手牌数角标
    sf::Text pCount(m_font, std::to_wstring(pn), (unsigned)(h * 0.022f));
    pCount.setFillColor(sf::Color::Black);
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
}

// ====== 设置弹窗 ======

void Renderer::drawSettingsPopup(sf::Vector2u winSize, const sf::Vector2f& mousePos,
                                  bool draggingMusic, bool draggingSound)
{
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
    float btnW = panelW * 0.32f;
    float btnH = h * 0.06f;
    float btnY = panelY + panelH - btnH - h * 0.06f;
    float btnGap = panelW * 0.06f;
    float totalBtnW = btnW * 2 + btnGap;
    float btnStartX = panelX + (panelW - totalBtnW) / 2.f;

    // 返回主界面按钮
    sf::FloatRect menuBtn({btnStartX, btnY}, {btnW, btnH});
    drawMenuButton(menuBtn, L"返回主界面", true, menuBtn.contains(mousePos), winSize);

    // 关闭按钮
    sf::FloatRect closeBtn({btnStartX + btnW + btnGap, btnY}, {btnW, btnH});
    drawMenuButton(closeBtn, L"关闭", true, closeBtn.contains(mousePos), winSize);
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
    float btnW = panelW * 0.32f;
    float btnH = h * 0.06f;
    float btnY2 = panelY + panelH - btnH - h * 0.06f;
    float btnGap = panelW * 0.06f;
    float totalBtnW = btnW * 2 + btnGap;
    float btnStartX = panelX + (panelW - totalBtnW) / 2.f;
    if (sf::FloatRect({btnStartX, btnY2}, {btnW, btnH}).contains(pos)) {
        result.action = 2; // 返回主界面
        return result;
    }

    // 关闭按钮
    if (sf::FloatRect({btnStartX + btnW + btnGap, btnY2}, {btnW, btnH}).contains(pos)) {
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

    // 返回按钮
    float rx = w * 0.03f;
    float ry = h * 0.03f;
    float rw = w * 0.08f;
    float rh = h * 0.045f;
    if (sf::FloatRect({rx, ry}, {rw, rh}).contains(worldPos)) return 3;

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
    float gbX = w - gbSz - w * 0.025f;
    float gbY = h * 0.028f;
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
