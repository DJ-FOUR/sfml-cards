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
sf::Color energyFillColor(204, 255, 0);
sf::Color energyBgColor(17, 17, 17);

// 新风格主色
constexpr sf::Color NEON_GREEN(204, 255, 0);
constexpr sf::Color DARK_BG(5, 5, 5);
constexpr sf::Color PANEL_BG(13, 13, 13);
constexpr sf::Color BORDER_NORMAL(51, 51, 51);
constexpr sf::Color TEXT_DIM(170, 170, 170);
constexpr sf::Color TEXT_DISABLED(85, 85, 85);
constexpr sf::Color ENEMY_RED(255, 51, 51);
constexpr sf::Color DARK_RED_BG(26, 10, 10);

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
        line.setFillColor(sf::Color(10, 10, 10, 30));
        window.draw(line);
    }

    // 网格
    sf::Color gridCol(26, 26, 26, 100);
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

} // namespace

Renderer::Renderer(sf::RenderWindow& window)
    : m_window(window)
{
    m_passBtn.setFillColor(btnColor);
    m_playBtn.setFillColor(btnColor);
    m_returnBtn.setFillColor(btnColor);
    m_energyBarBg.setFillColor(energyBgColor);
    m_energyBarFill.setFillColor(energyFillColor);
    for (int i = 0; i < MAX_SKILL_SLOTS; ++i)
        m_skillBtns[i].setFillColor(slotEmptyColor);
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

    if (!m_bgTexture.loadFromFile("images/background/back0.png")) {
        std::fprintf(stderr, "Failed to load background\n");
        return false;
    }
    m_bgTexture.setSmooth(true);

    m_playerLabel   = std::make_unique<sf::Text>(m_font, L"玩家", 18);
    m_computerLabel = std::make_unique<sf::Text>(m_font, L"镜像AI", 18);
    m_statusText    = std::make_unique<sf::Text>(m_font, L"", 22);
    m_playBtnText   = std::make_unique<sf::Text>(m_font, L"出牌", 18);
    m_passBtnText   = std::make_unique<sf::Text>(m_font, L"不出", 18);
    m_returnBtnText = std::make_unique<sf::Text>(m_font, L"返回", 18);
    m_energyText    = std::make_unique<sf::Text>(m_font, L"", 18);

    for (int i = 0; i < MAX_SKILL_SLOTS; ++i)
        m_skillBtnTexts[i] = std::make_unique<sf::Text>(m_font, L"", 16);

    for (auto* t : {m_playerLabel.get(), m_computerLabel.get(), m_statusText.get(),
                    m_playBtnText.get(), m_passBtnText.get(), m_returnBtnText.get(),
                    m_energyText.get()})
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

    float cut = 12.f;
    // ---- 背景底框（切角矩形） ----
    sf::ConvexShape bg(8);
    bg.setPoint(0, {cut, 0}); bg.setPoint(1, {cw - cut, 0});
    bg.setPoint(2, {cw, cut}); bg.setPoint(3, {cw, ch - cut});
    bg.setPoint(4, {cw - cut, ch}); bg.setPoint(5, {cut, ch});
    bg.setPoint(6, {0, ch - cut}); bg.setPoint(7, {0, cut});
    bg.setFillColor(hover ? sf::Color(20, 30, 10) : sf::Color(13, 13, 13));
    bg.setOutlineColor(hover ? NEON_GREEN : BORDER_NORMAL);
    bg.setOutlineThickness(hover ? 2.f : 1.f);
    rt.draw(bg);

    // ---- 顶部角标 OP-0N ----
    sf::RectangleShape tag({60.f, 22.f});
    tag.setFillColor(NEON_GREEN);
    rt.draw(tag);
    sf::Text tagText(m_font, L"OP-0" + std::to_wstring(charIdx + 1), 14);
    tagText.setFillColor(sf::Color::Black);
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
    iconBg.setOutlineColor(BORDER_NORMAL);
    iconBg.setOutlineThickness(1.f);
    rt.draw(iconBg);

    sf::Text avText(m_font, std::to_string(charIdx + 1),
                    (unsigned)(iconSize * 0.45f));
    avText.setFillColor(NEON_GREEN);
    auto asz = avText.getGlobalBounds().size;
    avText.setPosition({iconX + (iconSize - asz.x) / 2.0f,
                        iconY + (iconSize - asz.y) / 2.0f});
    rt.draw(avText);

    // ---- 角色名称 ----
    float nameF = ch * 0.065f;
    sf::Text name(m_font, c.name, (unsigned)nameF);
    name.setFillColor(sf::Color::White);
    name.setStyle(sf::Text::Bold);
    auto nsz = name.getGlobalBounds().size;
    name.setPosition({(cw - nsz.x) / 2.0f, ch * 0.50f});
    rt.draw(name);

    // ---- 被动技名 ----
    float passF = ch * 0.05f;
    sf::Text passName(m_font, c.passiveName, (unsigned)passF);
    passName.setFillColor(sf::Color(255, 200, 100));
    auto pnsz = passName.getGlobalBounds().size;
    passName.setPosition({(cw - pnsz.x) / 2.0f, ch * 0.60f});
    rt.draw(passName);

    // ---- 被动技描述 ----
    float descF = ch * 0.038f;
    sf::Text passDesc(m_font, c.passiveDesc, (unsigned)descF);
    passDesc.setFillColor(sf::Color(200, 200, 200));
    auto pdsz = passDesc.getGlobalBounds().size;
    passDesc.setPosition({(cw - pdsz.x) / 2.0f, ch * 0.70f});
    rt.draw(passDesc);

    rt.display();
}

// ====== 通用: 背景 ======

void Renderer::drawBackground(sf::Vector2u winSize)
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;
    drawTacticalGrid(m_window, winSize);

    // 若背景图可用，叠极低透明度作为纹理层
    if (m_bgTexture.getSize().x > 0) {
        sf::Sprite bgSprite(m_bgTexture);
        float bgScale = std::max(w / (float)m_bgTexture.getSize().x,
                                 h / (float)m_bgTexture.getSize().y);
        bgSprite.setScale({bgScale, bgScale});
        bgSprite.setPosition({(w - m_bgTexture.getSize().x * bgScale) / 2.f,
                              (h - m_bgTexture.getSize().y * bgScale) / 2.f});
        bgSprite.setColor(sf::Color(255, 255, 255, 25));
        m_window.draw(bgSprite);
    }

    // 四角安全区标记
    drawCornerMarkers(m_window, 12.f, 12.f, w - 24.f, h - 24.f, 40.f, NEON_GREEN, 2.f);

    // 版本号
    sf::Text ver(m_font, L"v1.0.0_OS", 14);
    ver.setFillColor(sf::Color(68, 68, 68));
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
    float bw = w * 0.40f;
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

    sf::Color fill = enabled ? (hover ? btnHoverColor : btnColor) : btnDisabledColor;
    sf::Color outline = enabled ? (hover ? NEON_GREEN : BORDER_NORMAL) : sf::Color(34, 34, 34);

    drawBeveledRect(m_window, x, y, w, h, cut, fill, outline, 2.f);

    // 左侧小型荧光绿方块指示器（悬停时）
    if (hover && enabled) {
        sf::RectangleShape ind({6.f, 6.f});
        ind.setPosition({x + 12.f, y + (h - 6.f) / 2.f});
        ind.setFillColor(NEON_GREEN);
        m_window.draw(ind);
    }

    // 文字描底（黑色偏移）
    sf::Text shadow(m_font, text, (unsigned)(h * 0.45f));
    shadow.setFillColor(sf::Color::Black);
    auto sz = shadow.getGlobalBounds().size;
    shadow.setPosition({x + (w - sz.x) / 2.f + 1.f,
                        y + (h - sz.y) / 2.f - sz.y * 0.15f + 1.f});
    m_window.draw(shadow);

    sf::Text label(m_font, text, (unsigned)(h * 0.45f));
    label.setFillColor(enabled ? sf::Color::White : TEXT_DISABLED);
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

    sf::Text title(m_font, cn, (unsigned)fontSize);
    title.setFillColor(sf::Color::White);
    title.setStyle(sf::Text::Bold);
    auto sz = title.getGlobalBounds().size;
    float tx = (w - sz.x) / 2.f;
    float ty = h * yRatio;
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

    // 荧光绿底线
    sf::RectangleShape underline({sz.x * 1.2f, 3.f});
    underline.setFillColor(NEON_GREEN);
    underline.setPosition({tx - sz.x * 0.1f, ty + sz.y + 8.f});
    m_window.draw(underline);
}

// ====== 通用: 技能卡片绘制 ======

sf::FloatRect Renderer::skillCardRect(int idx, int total, sf::Vector2u winSize) const
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;
    float ch = h * 0.42f;                       // 高度占窗口 42%
    float cw = ch * 7.0f / 12.0f;               // 宽基于高 7:12 卡牌比例
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

    // 底色（切角矩形）
    sf::Color fill = owned ? skillCardOwned : (hover ? skillCardHover : skillCardColor);
    sf::Color outline = hover ? NEON_GREEN : BORDER_NORMAL;
    drawBeveledRect(m_window, x, y, w, h, cut, fill, outline, hover ? 2.f : 1.f);

    // 顶部色带
    sf::RectangleShape topBar({w - cut * 2, h * 0.04f});
    topBar.setPosition({x + cut, y + 2.f});
    topBar.setFillColor(hover ? NEON_GREEN : sf::Color(26, 26, 26));
    m_window.draw(topBar);

    // 已拥有角标
    if (owned) {
        sf::RectangleShape ownedTag({w * 0.28f, h * 0.045f});
        ownedTag.setPosition({x + 2.f, y + 4.f});
        ownedTag.setFillColor(NEON_GREEN);
        m_window.draw(ownedTag);
        sf::Text ownedText(m_font, L"OWNED", (unsigned)(h * 0.035f));
        ownedText.setFillColor(sf::Color::Black);
        ownedText.setStyle(sf::Text::Bold);
        auto osz = ownedText.getGlobalBounds().size;
        ownedText.setPosition({x + (w * 0.28f - osz.x) / 2.f, y + 4.f + (h * 0.045f - osz.y) / 2.f - 2.f});
        m_window.draw(ownedText);
    }

    // [IMG-SKILL-S01~S08] 技能图标占位（线框八角形）
    float iconSize = w * 0.30f;
    float iconX = x + (w - iconSize) / 2.f;
    float iconY = y + h * 0.08f;
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
    oct.setFillColor(sf::Color(10, 10, 10));
    oct.setOutlineColor(BORDER_NORMAL);
    oct.setOutlineThickness(1.f);
    m_window.draw(oct);

    sf::Text iconText(m_font, "S" + std::to_string(skillId + 1),
                      (unsigned)(iconSize * 0.38f));
    iconText.setFillColor(NEON_GREEN);
    auto isz = iconText.getGlobalBounds().size;
    iconText.setPosition({x + (w - isz.x) / 2.f, iconY + (iconSize - isz.y) / 2.f});
    m_window.draw(iconText);

    // 技能名 — 居中于图标和底部信息栏之间
    float iconBot = iconY + iconSize;
    float infoTop = y + h - h * 0.065f;
    float nameF = h * 0.10f;
    sf::Text name(m_font, sk.name, (unsigned)nameF);
    name.setFillColor(sf::Color::White);
    name.setStyle(sf::Text::Bold);
    auto nsz = name.getGlobalBounds().size;
    name.setPosition({x + (w - nsz.x) / 2.f, (iconBot + infoTop - nsz.y) / 2.f});
    m_window.draw(name);

    // 底部黑条区域（能量/冷却）
    sf::RectangleShape infoBar({w - cut * 2, h * 0.055f});
    infoBar.setPosition({x + cut, y + h - h * 0.065f});
    infoBar.setFillColor(sf::Color(5, 5, 5));
    m_window.draw(infoBar);

    std::wstring costStr = L"ENERGY: " + std::to_wstring(sk.energyCost)
                         + L"    CD: " + std::to_wstring(sk.cooldown);
    sf::Text costText(m_font, costStr, (unsigned)(h * 0.032f));
    costText.setFillColor(sf::Color(136, 136, 136));
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

    auto r1 = menuButtonRect(0, 3, winSize);
    auto r2 = menuButtonRect(1, 3, winSize);
    auto r3 = menuButtonRect(2, 3, winSize);
    drawMenuButton(r1, L"开始游戏", true, r1.contains(mousePos), winSize);
    drawMenuButton(r2, L"选择角色", true, r2.contains(mousePos), winSize);
    drawMenuButton(r3, L"退出",     true, r3.contains(mousePos), winSize);
}

int Renderer::hitMainMenu(const sf::Vector2f& pos, sf::Vector2u winSize)
{
    if (menuButtonRect(0, 3, winSize).contains(pos)) return 1; // 开始游戏
    if (menuButtonRect(1, 3, winSize).contains(pos)) return 2; // 选择角色
    if (menuButtonRect(2, 3, winSize).contains(pos)) return 3; // 退出
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
    for (int i = 0; i < CHAR_COUNT; ++i) {
        float cx = startX + i * (actualCW + gap);
        float curS = m_charHover[i].currentScale;
        float curW = actualCW * curS;
        float curH = actualCH * curS;
        float curX = cx + (actualCW - curW) / 2.0f;
        float curY = startY + (actualCH - curH) / 2.0f + m_charHover[i].currentYOffset;
        sf::FloatRect rect({curX, curY}, {curW, curH});
        bool hover = rect.contains(mousePos);

        m_charHover[i].targetYOffset = hover ? -h * 0.065f : 0.0f;
        m_charHover[i].targetScale   = hover ? 1.10f : 1.0f;
    }

    // ---- 绘制 3 张角色卡片 ----
    for (int i = 0; i < CHAR_COUNT; ++i) {
        float cx = startX + i * (actualCW + gap);
        float cardCX = cx + actualCW / 2.0f;
        float cardCY = startY + actualCH / 2.0f + m_charHover[i].currentYOffset;

        // 只对鼠标实际悬停的卡片计算3D倾斜
        bool isHovered = m_charHover[i].targetYOffset < -0.1f;
        float normX = isHovered ? std::clamp((mousePos.x - cardCX) / (actualCW / 2.0f), -1.0f, 1.0f) : 0.0f;
        float normY = isHovered ? std::clamp((mousePos.y - cardCY) / (actualCH / 2.0f), -1.0f, 1.0f) : 0.0f;

        float s = m_charHover[i].currentScale;
        // 透视投影模拟: 卡片绕Y/X轴旋转 → 余弦压缩
        // 最大旋转角 ±36°, 强化四角翻转效果
        constexpr float MAX_TILT = 36.0f * 3.14159265f / 180.0f;
        float ry = normX * MAX_TILT;  // 绕Y轴
        float rx = normY * MAX_TILT;  // 绕X轴
        // 交叉耦合: Y轴旋转主导水平, X轴旋转主导垂直, 但对角位置互相增强
        float scaleX = s * std::cos(ry) * std::cos(rx * 0.45f);
        float scaleY = s * std::cos(rx) * std::cos(ry * 0.45f);
        // 对角位置产生最大位移 (模拟透视中心偏移)
        float shiftX = normX * actualCW * 0.045f * s;
        float shiftY = normY * actualCH * 0.025f * s;

        // ---- 绘制发光边框（在卡片底层）----
        if (m_charHover[i].currentYOffset < -1.0f) {
            float glowPad = 8.0f * s;
            sf::RectangleShape glow({actualCW + glowPad * 2, actualCH + glowPad * 2});
            glow.setOrigin({(actualCW + glowPad * 2) / 2.0f,
                            (actualCH + glowPad * 2) / 2.0f});
            glow.setPosition({cardCX + shiftX, cardCY + shiftY});
            glow.setFillColor(sf::Color::Transparent);
            float glowAlpha = 160.0f * std::abs(m_charHover[i].currentYOffset / (h * 0.065f));
            glow.setOutlineColor(sf::Color(204, 255, 0, (std::uint8_t)glowAlpha));
            glow.setOutlineThickness(2.5f * s);
            m_window.draw(glow);
        }

        // ---- 更新并绘制卡片内容 ----
        bool hover = m_charHover[i].targetYOffset < -0.1f;
        renderCharCardToRT(i, hover);

        sf::Sprite cardSprite(m_charRT[i].getTexture());
        cardSprite.setOrigin({CHAR_RT_W / 2.0f, CHAR_RT_H / 2.0f});
        cardSprite.setPosition({cardCX + shiftX, cardCY + shiftY});
        cardSprite.setScale({
            (actualCW / CHAR_RT_W) * scaleX,
            (actualCH / CHAR_RT_H) * scaleY
        });
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

// ====== 关卡过渡 (装备技能) ======

void Renderer::drawTransition(sf::Vector2u winSize, const sf::Vector2f& mousePos,
                               int level, const std::vector<int>& acquiredSkills,
                               const std::array<int, MAX_SKILL_SLOTS>& equipped,
                               int hoveredAcquiredIdx, int hoveredSlotIdx)
{
    drawBackground(winSize);
    drawBackButton(winSize, mousePos);

    float w = (float)winSize.x;
    float h = (float)winSize.y;

    // 标题
    drawTitle(L"第 " + std::to_wstring(level) + L" 关" + std::to_wstring(level), 0.05f, winSize);

    // ---- 左侧: 已获得技能列表 ----
    float leftX = w * 0.05f;
    float listY = h * 0.18f;
    float listW = w * 0.48f;
    float itemH = h * 0.06f;
    float itemGap = h * 0.01f;

    sf::Text heading(m_font, L"已获得协议", (unsigned)(h * 0.032f));
    heading.setFillColor(TEXT_DIM);
    heading.setPosition({leftX, listY - h * 0.04f});
    m_window.draw(heading);

    auto& allSkills = getAllSkills();

    for (size_t i = 0; i < acquiredSkills.size(); ++i) {
        int sid = acquiredSkills[i];
        if (sid < 0 || sid >= SKILL_COUNT) continue;
        auto& sk = allSkills[sid];
        float iy = listY + i * (itemH + itemGap);
        bool isEquipped = false;
        for (int e = 0; e < MAX_SKILL_SLOTS; ++e)
            if (equipped[e] == sid) { isEquipped = true; break; }
        bool hover = ((int)i == hoveredAcquiredIdx);

        // 分隔线
        sf::RectangleShape sep({listW, 1.f});
        sep.setPosition({leftX, iy + itemH});
        sep.setFillColor(BORDER_NORMAL);
        m_window.draw(sep);

        // 左侧竖线指示器
        float indW = isEquipped ? 4.f : (hover ? 3.f : 2.f);
        sf::RectangleShape ind({indW, itemH * 0.6f});
        ind.setPosition({leftX + 4.f, iy + itemH * 0.2f});
        ind.setFillColor(isEquipped ? NEON_GREEN : (hover ? NEON_GREEN : BORDER_NORMAL));
        m_window.draw(ind);

        // 悬停背景
        if (hover) {
            sf::RectangleShape hbg({listW, itemH});
            hbg.setPosition({leftX, iy});
            hbg.setFillColor(sf::Color(17, 26, 8));
            m_window.draw(hbg);
        }

        std::wstring label = L"[" + sk.name + L"]" + std::to_wstring(sid + 1) + L"  " + sk.desc + L" (费" + std::to_wstring(sk.energyCost) + L" CD" + std::to_wstring(sk.cooldown) + L")";
        sf::Text st(m_font, label, (unsigned)(itemH * 0.38f));
        st.setFillColor(isEquipped ? NEON_GREEN : sf::Color::White);
        st.setPosition({leftX + w * 0.012f, iy + itemH * 0.15f});
        m_window.draw(st);
    }

    if (acquiredSkills.empty()) {
        sf::Text empty(m_font, L"暂无技能 (击败敌人后获得)", (unsigned)(h * 0.03f));
        empty.setFillColor(sf::Color(128, 128, 128));
        empty.setPosition({leftX, listY});
        m_window.draw(empty);
    }

    // ---- 右侧: 装备槽 (卡牌 7:12 比例) ----
    float rightX = w * 0.58f;
    float slotW = h * 0.10f * 7.0f / 12.0f;
    float slotH = h * 0.10f;
    float slotGap = w * 0.03f;
    float slotStartY = h * 0.18f;

    sf::Text slotHeading(m_font, L"装备槽", (unsigned)(h * 0.032f));
    slotHeading.setFillColor(TEXT_DIM);
    slotHeading.setPosition({rightX, slotStartY - h * 0.04f});
    m_window.draw(slotHeading);

    for (int i = 0; i < MAX_SKILL_SLOTS; ++i) {
        int sid = equipped[i];
        float sx = rightX + i * (slotW + slotGap);
        bool hover = (i == hoveredSlotIdx);

        float slotCut = 4.f;
        // 技能槽底框（切角矩形）
        sf::Color sfill = sid >= 0 ? slotFilledColor
                          : (hover ? sf::Color(26, 26, 10) : slotEmptyColor);
        sf::Color soutline = hover ? NEON_GREEN : BORDER_NORMAL;
        drawBeveledRect(m_window, sx, slotStartY + (hover ? -2.f : 0.f), slotW, slotH, slotCut,
                        sfill, soutline, hover ? 2.f : 1.f);

        if (sid >= 0) {
            auto& sk = allSkills[sid];
            // 顶部色带
            sf::RectangleShape sbar({slotW - slotCut * 2, 6.f});
            sbar.setPosition({sx + slotCut, slotStartY + (hover ? -2.f : 0.f) + 2.f});
            sbar.setFillColor(NEON_GREEN);
            m_window.draw(sbar);

            sf::Text slotText(m_font, sk.name, (unsigned)(slotH * 0.15f));
            slotText.setFillColor(sf::Color::White);
            slotText.setStyle(sf::Text::Bold);
            auto tsz = slotText.getGlobalBounds().size;
            slotText.setPosition({sx + (slotW - tsz.x) / 2.f, slotStartY + (hover ? -2.f : 0.f) + slotH * 0.22f});
            m_window.draw(slotText);

            std::wstring costStr = L"费" + std::to_wstring(sk.energyCost)
                                 + L" CD" + std::to_wstring(sk.cooldown);
            sf::Text cost(m_font, costStr, (unsigned)(slotH * 0.12f));
            cost.setFillColor(TEXT_DIM);
            auto csz = cost.getGlobalBounds().size;
            cost.setPosition({sx + (slotW - csz.x) / 2.f, slotStartY + (hover ? -2.f : 0.f) + slotH * 0.60f});
            m_window.draw(cost);
        } else {
            // 空槽准星
            float cx = sx + slotW / 2.f;
            float cy = slotStartY + slotH / 2.f;
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
            empty.setPosition({sx + (slotW - esz.x) / 2.f, slotStartY + slotH * 0.65f});
            m_window.draw(empty);
        }
    }

    // ---- 敌人预览 ----
    float enemyY = slotStartY + slotH + h * 0.05f;
    sf::Text enemyHeading(m_font, L"敌方继承协议", (unsigned)(h * 0.028f));
    enemyHeading.setFillColor(ENEMY_RED);
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

    // ---- 开始战斗按钮（警示风格） ----
    float btnW = w * 0.55f;
    float btnH = h * 0.075f;
    float btnX = (w - btnW) / 2.f;
    float btnY = h * 0.86f;
    bool fightHover = sf::FloatRect({btnX, btnY}, {btnW, btnH}).contains(mousePos);

    // 顶部斜条纹
    drawHazardStripes(m_window, btnX, btnY, btnW, 4.f, 8.f);
    // 底部斜条纹
    drawHazardStripes(m_window, btnX, btnY + btnH - 4.f, btnW, 4.f, 8.f);
    // 按钮主体
    sf::Color fightFill = fightHover ? NEON_GREEN : sf::Color(26, 42, 10);
    sf::Color fightTextCol = fightHover ? sf::Color::Black : NEON_GREEN;
    drawBeveledRect(m_window, btnX, btnY + 4.f, btnW, btnH - 8.f, 8.f,
                    fightFill, fightHover ? sf::Color::White : NEON_GREEN, 2.f);

    sf::Text fightText(m_font, L"开始战斗", (unsigned)(btnH * 0.38f));
    fightText.setFillColor(fightTextCol);
    fightText.setStyle(sf::Text::Bold);
    auto fsz = fightText.getGlobalBounds().size;
    fightText.setPosition({btnX + (btnW - fsz.x) / 2.f, btnY + 4.f + (btnH - 8.f - fsz.y) / 2.f - 2.f});
    m_window.draw(fightText);
}

int Renderer::hitTransitionSkill(const sf::Vector2f& pos, sf::Vector2u winSize,
                                  int acquiredCount, int /*equippedCount*/)
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;
    float leftX = w * 0.05f;
    float listY = h * 0.18f;
    float listW = w * 0.48f;
    float itemH = h * 0.06f;
    float itemGap = h * 0.01f;

    for (int i = 0; i < acquiredCount; ++i) {
        float iy = listY + i * (itemH + itemGap);
        if (sf::FloatRect({leftX, iy}, {listW, itemH}).contains(pos))
            return i;
    }
    return -1;
}

int Renderer::hitTransitionSlot(const sf::Vector2f& pos, sf::Vector2u winSize)
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;
    float rightX = w * 0.58f;
    float slotW = h * 0.10f * 7.0f / 12.0f;
    float slotH = h * 0.10f;
    float slotGap = w * 0.03f;
    float slotStartY = h * 0.18f;

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
    float btnW = w * 0.20f;
    float btnH = h * 0.065f;
    float btnX = (w - btnW) / 2.f;
    float btnY = h * 0.88f;
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
    drawBackground(winSize);
    drawTitle(acquiredSkills.empty() ? L"获取协议" : L"奖励结算", 0.07f, winSize);

    float w = (float)winSize.x;
    float h = (float)winSize.y;
    int hoveredSid = -1;

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
        if (hover && sid >= 0) hoveredSid = sid;

        m_skillHover[i].targetYOffset = hover ? -h * 0.065f : 0.0f;
        m_skillHover[i].targetScale   = hover ? 1.10f : 1.0f;

        drawSkillCard(curX, curY, curW, curH, sid, owned, hover, winSize);
    }

    // 悬停技能描述面板
    if (hoveredSid >= 0) {
        auto& skills = getAllSkills();
        auto& sk = skills[hoveredSid];

        float panelW = w * 0.56f;
        float panelH = h * 0.13f;
        float panelX = (w - panelW) / 2.f;
        float panelY = h * 0.70f;
        float cut = 8.f;

        drawBeveledRect(m_window, panelX, panelY, panelW, panelH, cut,
                        sf::Color(10, 13, 8), NEON_GREEN, 1.5f);

        // 顶部色带
        sf::RectangleShape topBar({panelW - cut * 2, 4.f});
        topBar.setPosition({panelX + cut, panelY + 2.f});
        topBar.setFillColor(NEON_GREEN);
        m_window.draw(topBar);

        // 编号 + 技能名
        std::wstring header = L"S0" + std::to_wstring(hoveredSid + 1) + L"  " + sk.name;
        sf::Text headerText(m_font, header, (unsigned)(panelH * 0.18f));
        headerText.setFillColor(NEON_GREEN);
        headerText.setStyle(sf::Text::Bold);
        headerText.setPosition({panelX + panelW * 0.04f, panelY + panelH * 0.12f});
        m_window.draw(headerText);

        // 描述
        sf::Text descText(m_font, sk.desc, (unsigned)(panelH * 0.16f));
        descText.setFillColor(sf::Color(220, 220, 220));
        descText.setPosition({panelX + panelW * 0.04f, panelY + panelH * 0.42f});
        m_window.draw(descText);

        // 能量 / 冷却
        std::wstring costStr = L"ENERGY: " + std::to_wstring(sk.energyCost)
                             + L"    CD: " + std::to_wstring(sk.cooldown);
        sf::Text costText(m_font, costStr, (unsigned)(panelH * 0.14f));
        costText.setFillColor(sf::Color(150, 150, 150));
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
    auto esz = errText.getGlobalBounds().size;
    errText.setPosition({(w - esz.x) / 2.f, h * 0.14f});
    m_window.draw(errText);

    // "任务失败 // MISSION FAILED"
    float loseFont = h * 0.10f;
    sf::Text loseText(m_font, L"任务失败", (unsigned)loseFont);
    loseText.setFillColor(NEON_GREEN);
    loseText.setStyle(sf::Text::Bold);
    // 黑色粗描底
    sf::Text loseShadow(m_font, L"任务失败", (unsigned)loseFont);
    loseShadow.setFillColor(sf::Color::Black);
    loseShadow.setStyle(sf::Text::Bold);
    auto lsz = loseText.getGlobalBounds().size;
    loseShadow.setPosition({(w - lsz.x) / 2.f + 3.f, h * 0.22f + 3.f});
    m_window.draw(loseShadow);
    loseText.setPosition({(w - lsz.x) / 2.f, h * 0.22f});
    m_window.draw(loseText);

    // 终端日志风格统计
    float logY = h * 0.38f;
    float logGap = h * 0.045f;
    auto drawLog = [&](const std::wstring& line, int idx) {
        sf::Text gt(m_font, L"> ", (unsigned)(h * 0.032f));
        gt.setFillColor(NEON_GREEN);
        auto gsz = gt.getGlobalBounds().size;
        gt.setPosition({w * 0.30f, logY + idx * logGap});
        m_window.draw(gt);

        sf::Text lt(m_font, line, (unsigned)(h * 0.032f));
        lt.setFillColor(TEXT_DIM);
        lt.setPosition({w * 0.30f + gsz.x + 6.f, logY + idx * logGap});
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
    float overlap = dispW * 0.65f;
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
    // 统一向绿色调色偏
    sprite.setColor(sf::Color(220, 255, 180));
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

void Renderer::drawGameUI(const GameState& state, bool canPass, bool canPlaySelected,
                           sf::Vector2u winSize,
                           const std::array<int, MAX_SKILL_SLOTS>& playerSkillIds,
                           const sf::Vector2f& mousePos)
{
    float w = (float)winSize.x;
    float h = (float)winSize.y;

    std::wstring statusStr;
    sf::Color statusCol = TEXT_DIM;
    switch (state.phase()) {
    case GameState::Phase::PlayerTurn:
        statusStr = state.isNewRound() ? L"[STATUS] 新一轮"
                                       : L"[STATUS] 你的回合";
        statusCol = NEON_GREEN;
        break;
    case GameState::Phase::ComputerTurn:
        statusStr = L"[STATUS] 敌方运算中";
        statusCol = ENEMY_RED;
        break;
    case GameState::Phase::PlayerWins:
        statusStr = L"[STATUS] 任务完成";
        statusCol = NEON_GREEN;
        break;
    case GameState::Phase::ComputerWins:
        statusStr = L"[STATUS] 任务失败";
        statusCol = ENEMY_RED;
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

    // 返回按钮（切角风格）
    {
        float rx = w * 0.03f;
        float ry = h * 0.03f;
        float rw = w * 0.09f;
        float rh = h * 0.045f;
        bool rHover = sf::FloatRect({rx, ry}, {rw, rh}).contains(mousePos);
        drawBeveledRect(m_window, rx, ry, rw, rh, 4.f,
                        btnColor, rHover ? NEON_GREEN : BORDER_NORMAL, 1.f);

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

    // 能量条 [IMG-ENERGY-BAR] [IMG-ENERGY-FILL] — 分段式电池格
    {
        float eBarW = w * 0.14f;
        float eBarH = h * 0.035f;
        float eBarX = w * 0.42f;
        float eBarY = h * 0.92f;
        int segCount = MAX_ENERGY;
        float segTotalW = eBarW - 4.f;
        float segW = (segTotalW - (segCount - 1) * 2.f) / segCount;

        // 槽背景
        sf::RectangleShape eBg({eBarW, eBarH});
        eBg.setPosition({eBarX, eBarY});
        eBg.setFillColor(sf::Color(17, 17, 17));
        eBg.setOutlineColor(BORDER_NORMAL);
        eBg.setOutlineThickness(1.f);
        m_window.draw(eBg);

        // 分段填充
        for (int i = 0; i < segCount; ++i) {
            float sx = eBarX + 2.f + i * (segW + 2.f);
            sf::Color segFill = (i < state.playerEnergy()) ? NEON_GREEN : sf::Color(17, 17, 17);
            sf::RectangleShape seg({segW, eBarH - 4.f});
            seg.setPosition({sx, eBarY + 2.f});
            seg.setFillColor(segFill);
            m_window.draw(seg);
        }

        // 标签
        sf::Text eLabel(m_font, L"PWR", (unsigned)(eBarH * 0.65f));
        eLabel.setFillColor(sf::Color(136, 136, 136));
        auto elsz = eLabel.getGlobalBounds().size;
        eLabel.setPosition({eBarX - elsz.x - 8.f, eBarY + (eBarH - elsz.y) / 2.f - 2.f});
        m_window.draw(eLabel);

        std::wstring eStr = L"ENERGY: " + std::to_wstring(state.playerEnergy())
                          + L"/" + std::to_wstring(MAX_ENERGY);
        m_energyText->setString(eStr);
        m_energyText->setCharacterSize((unsigned)(eBarH * 0.75f));
        m_energyText->setFillColor(NEON_GREEN);
        auto esz = m_energyText->getGlobalBounds().size;
        m_energyText->setPosition({eBarX + eBarW + w * 0.012f,
                                    eBarY + (eBarH - esz.y) / 2.f - 2.f});
        m_window.draw(*m_energyText);
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

    // 整体切角面板
    drawBeveledRect(m_window, skStartX - 8.f, skY - 22.f, panelW, panelH, 6.f,
                    sf::Color(10, 10, 10), BORDER_NORMAL, 1.f);
    sf::Text slotPanelLabel(m_font, L"装备槽", (unsigned)(h * 0.018f));
    slotPanelLabel.setFillColor(sf::Color(136, 136, 136));
    slotPanelLabel.setPosition({skStartX - 4.f, skY - 20.f});
    m_window.draw(slotPanelLabel);

    for (int i = 0; i < MAX_SKILL_SLOTS; ++i) {
        float sx = skStartX + i * (skW + skGap);
        int sid = playerSkillIds[i];
        float skCut = 3.f;

        sf::Color slotColor = slotEmptyColor;
        sf::Color topBand = BORDER_NORMAL;
        if (sid >= 0) {
            if (state.playerCooldowns()[i] > 0) {
                slotColor = sf::Color(30, 30, 30);
                topBand = BORDER_NORMAL;
            } else if (state.playerEnergy() < allSkills[sid].energyCost) {
                slotColor = sf::Color(40, 40, 10);
                topBand = sf::Color(85, 85, 0);
            } else {
                slotColor = sf::Color(20, 30, 10);
                topBand = NEON_GREEN;
            }
        }
        drawBeveledRect(m_window, sx, skY, skW, skH, skCut,
                        slotColor, BORDER_NORMAL, 1.f);

        if (sid >= 0) {
            auto& sk = allSkills[sid];
            // 顶部色带
            sf::RectangleShape tbar({skW - skCut * 2, 6.f});
            tbar.setPosition({sx + skCut, skY + 2.f});
            tbar.setFillColor(topBand);
            m_window.draw(tbar);

            // 线框八角形图标
            float iconSize = skW * 0.28f;
            float iconX = sx + (skW - iconSize) / 2.f;
            float iconY = skY + skH * 0.10f;
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
            oct.setFillColor(sf::Color(10, 10, 10));
            oct.setOutlineColor(BORDER_NORMAL);
            oct.setOutlineThickness(1.f);
            m_window.draw(oct);

            sf::Text iconText(m_font, "S" + std::to_string(sid + 1),
                              (unsigned)(iconSize * 0.38f));
            iconText.setFillColor(NEON_GREEN);
            auto isz = iconText.getGlobalBounds().size;
            iconText.setPosition({sx + (skW - isz.x) / 2.f, iconY + (iconSize - isz.y) / 2.f});
            m_window.draw(iconText);

            // 技能名
            float nameF = skW * 0.22f;
            m_skillBtnTexts[i]->setString(sk.name);
            m_skillBtnTexts[i]->setCharacterSize((unsigned)nameF);
            if (state.playerCooldowns()[i] > 0)
                m_skillBtnTexts[i]->setFillColor(TEXT_DISABLED);
            else if (state.playerEnergy() < allSkills[sid].energyCost)
                m_skillBtnTexts[i]->setFillColor(TEXT_DISABLED);
            else
                m_skillBtnTexts[i]->setFillColor(sf::Color::White);
            auto tsz = m_skillBtnTexts[i]->getGlobalBounds().size;
            m_skillBtnTexts[i]->setPosition({sx + (skW - tsz.x) / 2.f,
                                              skY + skH * 0.48f});
            m_window.draw(*m_skillBtnTexts[i]);

            // 冷却/消耗
            std::wstring info;
            sf::Color infoCol;
            if (state.playerCooldowns()[i] > 0) {
                info = L"CD" + std::to_wstring(state.playerCooldowns()[i]);
                infoCol = ENEMY_RED;
            } else {
                info = L"费" + std::to_wstring(sk.energyCost);
                infoCol = NEON_GREEN;
            }
            sf::Text infoText(m_font, info, (unsigned)(skW * 0.20f));
            infoText.setFillColor(infoCol);
            auto csz = infoText.getGlobalBounds().size;
            infoText.setPosition({sx + (skW - csz.x) / 2.f, skY + skH * 0.78f});
            m_window.draw(infoText);
        } else {
            // 空槽准星
            float cx = sx + skW / 2.f;
            float cy = skY + skH / 2.f - 4.f;
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
    float eskX = w * 0.30f;
    float eskY = h * 0.02f;

    for (int i = 0; i < MAX_SKILL_SLOTS; ++i) {
        int esid = enemySlots[i];
        float ex = eskX + i * (eskW + eskGap);
        float ec = 2.f;

        sf::Color efill = (esid >= 0) ? (state.enemyCooldowns()[i] > 0 ? sf::Color(40, 20, 20) : sf::Color(60, 30, 30))
                          : sf::Color(17, 17, 17);
        sf::Color eout = (esid >= 0) ? ENEMY_RED : BORDER_NORMAL;
        drawBeveledRect(m_window, ex, eskY, eskW, eskH, ec, efill, eout, 1.f);

        if (esid >= 0) {
            if (state.enemyCooldowns()[i] > 0) {
                // 冷却中覆盖半透明黑
                sf::RectangleShape cdOverlay({eskW - ec * 2, eskH - ec * 2});
                cdOverlay.setPosition({ex + ec, eskY + ec});
                cdOverlay.setFillColor(sf::Color(0, 0, 0, 120));
                m_window.draw(cdOverlay);

                sf::Text cdText(m_font, L"CD" + std::to_wstring(state.enemyCooldowns()[i]),
                                (unsigned)(eskW * 0.28f));
                cdText.setFillColor(ENEMY_RED);
                auto cdsz = cdText.getGlobalBounds().size;
                cdText.setPosition({ex + (eskW - cdsz.x) / 2.f, eskY + eskH * 0.55f});
                m_window.draw(cdText);
            } else {
                auto& esk = allSkills[esid];
                sf::Text et(m_font, esk.name, (unsigned)(eskW * 0.18f));
                et.setFillColor(sf::Color(255, 180, 180));
                auto etsz = et.getGlobalBounds().size;
                et.setPosition({ex + (eskW - etsz.x) / 2.f, eskY + eskH * 0.20f});
                m_window.draw(et);

                sf::Text costText(m_font, L"费" + std::to_wstring(esk.energyCost),
                                  (unsigned)(eskW * 0.20f));
                costText.setFillColor(sf::Color(200, 150, 150));
                auto csz = costText.getGlobalBounds().size;
                costText.setPosition({ex + (eskW - csz.x) / 2.f, eskY + eskH * 0.55f});
                m_window.draw(costText);
            }
        } else {
            sf::Text empty(m_font, L"--", (unsigned)(eskW * 0.22f));
            empty.setFillColor(TEXT_DISABLED);
            auto esz = empty.getGlobalBounds().size;
            empty.setPosition({ex + (eskW - esz.x) / 2.f, eskY + eskH * 0.35f});
            m_window.draw(empty);
        }
    }

    // 出牌/不出按钮 (手牌上方居中, 含悬停缩放动效) — 新风格
    if (state.phase() == GameState::Phase::PlayerTurn) {
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

        // 不出按钮 (左侧) SKIP
        {
            float px = startX;
            float pw = btnW * m_passBtnHoverScale;
            float ph = btnH * m_passBtnHoverScale;
            float px2 = px + (btnW - pw) / 2.f;
            float py2 = btnY + (btnH - ph) / 2.f;
            sf::Color pfill = canPass ? (passHover ? sf::Color(20, 30, 10) : btnColor) : btnDisabledColor;
            sf::Color pout = canPass ? (passHover ? NEON_GREEN : BORDER_NORMAL) : sf::Color(34, 34, 34);
            drawBeveledRect(m_window, px2, py2, pw, ph, 5.f, pfill, pout, 2.f);

            m_passBtnText->setString(L"跳过");
            m_passBtnText->setCharacterSize((unsigned)(fontSize * m_passBtnHoverScale));
            m_passBtnText->setFillColor(canPass ? sf::Color::White : TEXT_DISABLED);
            auto psz = m_passBtnText->getGlobalBounds().size;
            m_passBtnText->setPosition({px + (btnW - psz.x) / 2.f, btnY + btnH * 0.22f});
            m_window.draw(*m_passBtnText);
        }

        // 出牌按钮 (右侧) EXECUTE
        {
            float px = startX + btnW + btnGap;
            float pw = btnW * m_playBtnHoverScale;
            float ph = btnH * m_playBtnHoverScale;
            float px2 = px + (btnW - pw) / 2.f;
            float py2 = btnY + (btnH - ph) / 2.f;
            sf::Color pfill = canPlaySelected ? (playHover ? NEON_GREEN : sf::Color(26, 42, 10)) : btnDisabledColor;
            sf::Color pout = canPlaySelected ? (playHover ? sf::Color::White : NEON_GREEN) : sf::Color(68, 68, 0);
            drawBeveledRect(m_window, px2, py2, pw, ph, 5.f, pfill, pout, 2.f);

            if (playHover && canPlaySelected) {
                // 顶部斜条纹装饰
                drawHazardStripes(m_window, px2 + 4.f, py2 + 2.f, pw - 8.f, 3.f, 6.f);
            }

            m_playBtnText->setString(L"执行");
            m_playBtnText->setCharacterSize((unsigned)(fontSize * m_playBtnHoverScale));
            m_playBtnText->setFillColor(canPlaySelected ? (playHover ? sf::Color::Black : NEON_GREEN) : sf::Color(85, 85, 0));
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
    (void)dt;
    float w = (float)winSize.x;
    float h = (float)winSize.y;
    float hs = handScale(h);
    float ps = playedScale(h);
    float hoverLift = h * 0.025f;

    drawBackground(winSize);

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
    cTag.setFillColor(NEON_GREEN);
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
        eBox.setOutlineColor(sf::Color(26, 26, 26));
        eBox.setOutlineThickness(1.f);
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

    // --- 玩家手牌 (正面, 含悬停浮动动效) ---
    auto& ph = state.playerHand();
    int pn = (int)ph.size();
    float phY = handCardY(h);
    float dispW = CARD_W * hs;
    float dispH = CARD_H * hs;

    m_handCardTargets.resize(pn, 0.0f);
    m_handCardYOffsets.resize(pn, 0.0f);

    if (m_dealActive) {
        // --- 发牌动画模式: 底部中心旋转 + 扇形展开 + 飞入 ---
        float middle = (pn - 1) / 2.0f;
        for (int i = 0; i < pn; ++i) {
            if (i >= (int)m_dealAnim.size() || !m_dealAnim[i].started)
                continue;

            auto pos = handCardPos(i, pn, phY, winSize);
            float t = m_dealAnim[i].progress;
            float eased = easeOutCubic(t);

            float offset = i - middle;
            float fanRot = offset * 2.0f;
            float arcSink = std::abs(offset) * 2.0f;

            float curScale = hs * (DEAL_INIT_SCL + (1.0f - DEAL_INIT_SCL) * eased);
            float curRot = fanRot * eased;
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
            sprite.setRotation(sf::degrees(curRot));
            sprite.setColor(sf::Color(255, 255, 255, curAlpha));
            m_window.draw(sprite);
        }
    } else {
        // --- 扇形排布 ---
        float middle = (pn - 1) / 2.0f;

        // ---- 第一步：根据遮挡顺序（从上层到下层）找出唯一悬停的卡牌 ----
        int hoveredIdx = -1;
        for (int i = pn - 1; i >= 0; --i) {
            auto pos = handCardPos(i, pn, phY, winSize);
            bool sel = std::find(selectedIndices.begin(), selectedIndices.end(), i)
                       != selectedIndices.end();
            float arcSink = std::abs(i - middle) * 2.0f;
            float offset = m_handCardYOffsets[i];
            if (offset < 0.5f) offset = (sel ? hoverLift : 0.0f);
            sf::FloatRect visualRect({pos.x, pos.y + arcSink - offset}, {dispW, dispH});
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

            // 1. 先计算当前视觉偏移（与绘制一致）
            float offset = m_handCardYOffsets[i];
            if (offset < 0.5f) offset = (sel ? hoverLift : 0.0f); // 首帧立即到位

            // 2. 只有最上层被悬停的卡牌才算 hover，避免穿透到下层
            bool hover = (i == hoveredIdx);
            m_handCardTargets[i] = (sel || hover) ? hoverLift : 0.0f;

            float fanRot = (i - middle) * 2.0f;
            float arcSink = std::abs(i - middle) * 2.0f;

            float bcx = pos.x + dispW / 2.0f;
            float bcy = pos.y + dispH + arcSink - offset;

            auto it = m_faceTextures.find(ph[i].imageIndex);
            const sf::Texture* tex = (it != m_faceTextures.end()) ? &it->second : &m_backTexture;

            sf::Sprite sprite(*tex);
            sprite.setOrigin({CARD_W / 2.0f, CARD_H});
            sprite.setScale({hs, hs});
            sprite.setPosition({bcx, bcy});
            sprite.setRotation(sf::degrees(fanRot));
            m_window.draw(sprite);

            if (sel) {
                // 荧光绿 overlay
                sf::Sprite hl(it->second);
                hl.setOrigin({CARD_W / 2.0f, CARD_H});
                hl.setScale({hs, hs});
                hl.setPosition({bcx, bcy});
                hl.setRotation(sf::degrees(fanRot));
                hl.setColor(sf::Color(204, 255, 0, 60));
                m_window.draw(hl);
                // 外发光边框（放大 1.02 倍绘制）
                sf::Sprite glow(it->second);
                glow.setOrigin({CARD_W / 2.0f, CARD_H});
                glow.setScale({hs * 1.02f, hs * 1.02f});
                glow.setPosition({bcx, bcy});
                glow.setRotation(sf::degrees(fanRot));
                glow.setColor(sf::Color(204, 255, 0, 40));
                m_window.draw(glow);
            }
        }
    }
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
    pTag.setFillColor(NEON_GREEN);
    m_window.draw(pTag);
    pCount.setPosition({w * 0.012f + m_playerLabel->getGlobalBounds().size.x + 8.f + 6.f, phY - h * 0.033f + 2.f});
    m_window.draw(pCount);

    // --- UI (含技能/能量) ---
    drawGameUI(state, !state.isNewRound(), canPlaySelected, winSize, playerSkillIds, mousePos);
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

    for (int i = cardCount - 1; i >= 0; --i) {
        auto pos = handCardPos(i, cardCount, yBase, winSize);
        bool sel = std::find(selectedIndices.begin(), selectedIndices.end(), i)
                   != selectedIndices.end();

        float arcSink = std::abs(i - middle) * 2.0f;

        // 1. 计算当前视觉偏移（与 renderGame 绘制逻辑一致）
        float offset = 0.0f;
        if (sel) {
            offset = lift;
        } else if (i < (int)m_handCardYOffsets.size()) {
            offset = m_handCardYOffsets[i];
            if (offset < 0.5f) offset = 0.0f;
        }

        // 2. 实时检测鼠标是否在当前视觉区域内
        sf::FloatRect visualBounds({pos.x, pos.y + arcSink - offset}, {dispW, dispH});
        bool hover = visualBounds.contains(worldPos);

        // 3. 选中或正被悬停时，检测区域直接上浮到完整高度（不受动画延迟影响）
        float hitOffset = (sel || hover) ? lift : offset;
        pos.y += arcSink - hitOffset;

        sf::FloatRect bounds(pos, {dispW, dispH});
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
        if (sf::FloatRect({sx, skY}, {skW, skH}).contains(worldPos))
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
