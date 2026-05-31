# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 环境要求

- **MinGW-w64** (GCC 14+) — 项目使用 MinGW Makefiles 生成器
- **CMake** 3.16+
- 设置环境变量 `MINGW_HOME` 指向 MinGW 安装目录，例如：
  - MSYS2: `C:\msys64\mingw64`
  - 独立安装: `D:\Software(English)\Mingw14.2.0\mingw64`
  - 确保 `${MINGW_HOME}/bin` 在 PATH 中

## Build & Run

```bash
# Configure (once)
cmake -B build -S . -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build --config Debug

# Run
./build/main.exe
```

In VS Code: `Ctrl+Shift+B` (cmake build) → `F5` (launch with gdb).
所有 VS Code 配置文件（`.vscode/`）都通过 `${env:MINGW_HOME}` 引用 MinGW 路径，换电脑只需设置 `MINGW_HOME` 环境变量即可，无需修改任何配置文件。

Build output goes to `build/main.exe`. CMake auto-copies `images/` and `resources/` post-build so the exe works from the build directory directly.

## Controls

- `F11` — toggle fullscreen/windowed
- `R` — redeal current level (development, gameplay only)
- Dev buttons ("我赢"/"我输") — force win/lose, rendered top-left during gameplay

## Project

**斗牌Rogue** — a C++17 + SFML 3.0.2 single-player card game combining Dou Di Zhu (斗地主) hand rules with roguelike endless-run progression. SFML is bundled locally in `SFML-3.0.2/` and statically linked.

Full architecture docs: [AGENTS.md](AGENTS.md) | Gameplay rules: [GAMEPLAY.md](GAMEPLAY.md) | Original design doc: [valiant-spinning-floyd.md](valiant-spinning-floyd.md)

## Architecture

### Screen state machine (`main.cpp`)

`Screen` enum drives the game loop: `MainMenu → CharacterSelect → WildcardSelect(谋略家) → Reward(初始技能) → Transition → Game → Reward → GameOver`. Main menu has "开始游戏" (→ CharacterSelect) and "退出". After character select, the player picks an initial skill (3 random choices), then goes to Transition to equip before the first battle. Each screen has corresponding `draw*`/`hit*` method pairs in the Renderer. State transitions happen in the event handling block of `main.cpp`.

### Module dependency graph

```
main.cpp
  ├── renderer.hpp/cpp    — All SFML drawing + hit-testing (every screen)
  ├── game_state.hpp/cpp  — Core rules engine: hand classification, comparison, AI, skill activation
  │     ├── card.hpp/cpp  — Card data model, deck creation, image index mapping
  │     ├── skill.hpp/cpp — Skill definitions (8 skills) + SkillBuffs struct
  │     └── ai_memory.hpp/cpp — k-NN learning: records player decisions, guides AI mimicry
  ├── run_state.hpp/cpp   — Run-level state: level #, acquired skills, equipped slots, mirroring
  │     ├── character.hpp/cpp — 3 character definitions with passive abilities
  │     └── skill.hpp/cpp
```

### Card image mapping (`card.cpp`)

`CARD_MAP[54]` maps `imageIndex` 0-53 → `(Suit, Rank)`:
- 0-3: four 3s (Spade/Heart/Club/Diamond), 4-7: four 4s, ..., 44-47: four Aces, 48-51: four 2s
- 52: Small Joker, 53: Big Joker

Image files: `images/card/card{idx}.png`. `cardFromImageIndex()` does the lookup; `createDeck()` builds and shuffles all 54.

### Hand classification pipeline

`GameState::classifyHand()` is the central rules function. It takes a vector of Cards + optional `SkillBuffs*` and returns `std::optional<HandPattern>`. The SkillBuffs struct carries temporary rule modifications (e.g., `straightExtended` reduces min straight length from 5 to 4). `GameState::beats()` compares two `PlayedCards` using the same buffs.

**Adding a new skill that modifies hand rules** requires touching three places:
1. Add a field to `SkillBuffs` (skill.hpp)
2. Check that field in `classifyHand()` / `beats()` (game_state.cpp)
3. Set the field in `activatePlayerSkill()` / `enemyActivateSkills()` (game_state.cpp)

### Mirror mechanic

The core roguelike hook: enemies inherit the player's 3 equipped skills from the previous level. `RunState::mirroredSkills()` returns `m_mirroredSkills` — a snapshot taken in `advanceToNextLevel()` right before the level increments. Level 1 enemies always get `{-1,-1,-1}` (no skills); level 2+ enemies get the snapshot.

### Skill system

- Skills have **no energy cost and no cooldown**. Players can activate any equipped skill freely during their turn.
- **Buff-type** skills (0/2/3/4/5): set `SkillBuffs` fields, cleared at end of player turn (`endPlayerTurnCleanup()`).
- **Trigger-type** skills (1/6/7): set flags (`m_s02_active`, `m_s07_active`, `m_s08_active`), consumed on activation during `applyPostPlayEffects()`. S08 limited to 1/turn via `m_chainBombUsed`.
- S07 (炸弹馈赠): changed from "+2 energy" to "draw 1 card when playing a bomb" (energy system removed).
- Enemy skills: no cost or cooldown. `enemyActivateSkills()` decides activation per-skill based on AI learning probabilities.

### AI learning system (`ai_memory.hpp/cpp`)

Two-component online learning within a single run:

**Card play learning (k-NN)**:
- Records every player decision as `(PlayFeatures, PlayAction)` pair, up to 2000 records.
- `PlayFeatures` (7-dim): hand size bucket, is new round, last play type/rank, has bomb, has rocket, level number.
- `PlayAction`: hand type, main rank, card count, passed flag.
- AI queries: finds K=5 nearest neighbors via weighted feature distance, returns similarity-weighted action distribution.
- Used in `computerTakeTurn()` to re-rank legal options — actions the player favored get score bonuses.

**Skill usage learning**:
- Tracks `skillId → use count` in `m_skillUses` map.
- Records on `activatePlayerSkill()` → `recordSkillUse(skillId)`.
- AI queries `querySkillProb(skillId)` = uses/maxUses ratio (0.0 if never used).
- `enemyActivateSkills()` activates buffs only when probability ≥ 0.4.

**Lifecycle**: `AIMemory` owned by `main.cpp`, cleared on run start (character select), game over, and return to menu. Persists across levels within a run — data accumulates.

**Recording hook points**:
- `playerPlay()`: records (pre-removal hand state, chosen cards) before modifying game state.
- `playerPass()`: records (pre-pass hand state, pass action).
- `activatePlayerSkill()`: records skill use.

### AI decision flow (`GameState::computerTakeTurn()`)

1. Enemy activates skills (probabilistic, based on player's usage history).
2. If following (not a new round): `findBeatingPlays()` generates all legal beating plays.
3. Selection: cost-based heuristic (`evaluatePlayCost`: breaking bomb +50, triple +15, pair +3) combined with k-NN memory preference re-ranking.
4. Bombs/rockets: skipped when hand > 4 cards; prioritized when ≤ 4.
5. If no beating play: pass, clear buffs, switch to PlayerTurn.
6. If new round: `findLowestPlay()` tries multi-card combos first (straight → triple+two → triple+one → triple → pair → single).

### Game UI layout

- Enemy hand cards: top of screen (`computerHandY = h*0.05`)
- Enemy skills: top, card-proportioned rectangles (dark red, shows BUFF/TRIG label)
- Played cards: center area (enemy above, player below)
- "出牌"/"不出" buttons: centered above player hand cards (`h*0.56`), only visible during PlayerTurn
- Player hand cards: bottom (`handCardY = h*0.66`), upright (no fan rotation/arc), straight line layout
- Player skills: bottom-left corner (`w*0.03`, `h*0.83`), card-proportioned (CARD_W/CARD_H ratio), shows BUFF/TRIG label
- Dev debug buttons ("我赢"/"我输"): top-left area, only during active gameplay
- "开始战斗" button (Transition screen): rectangular (`w*0.18 × h*0.09`), green default / magenta hover, text shakes violently on hover

### Hover effects

- Hand cards: smooth float-up animation (`h*0.025f` lift) on hover, golden glow border on selected
- Play/Pass buttons: scale to 108% on hover (smooth lerp animation)
- Character select cards: float + scale + 3D perspective tilt
- Reward skill cards: float + scale + glow border
- Fight button (Transition): magenta outline + hazard stripes + text shake (6-frequency sine wave叠加)
- All animations use `HoverAnimState` struct with lerp-based smoothing (`ANIM_SPEED_CHAR_HOVER = 14.0f` for characters/skills, `ANIM_SPEED_BTN_HOVER = 18.0f` for buttons, `ANIM_SPEED_CARD_HOVER = 16.0f` for hand cards)

## Code conventions

- `#pragma once` for headers, `enum class` for enums, `constexpr` for compile-time constants
- Private members prefixed with `m_` (e.g., `m_playerHand`, `m_phase`)
- Stateless utility functions declared `static` (e.g., `GameState::classifyHand`, `GameState::beats`)
- File-local helpers go in anonymous namespaces in `.cpp` files
- `std::optional` for nullable returns, `std::unique_ptr` for SFML objects
- Platform-specific code uses `#ifdef _WIN32`
- All comments and UI text in Chinese (中文)

## Characters

| ID | Name | Passive | Effect |
|----|------|---------|--------|
| 0 | 争锋者 | 强袭 | +1 hand size (16 cards instead of 15) |
| 1 | 谋略家 | 谋定 | Standard hand size (15 cards) |
| 2 | 掌控者 | 储备 | +2 hand size (17 cards instead of 15) |

CharacterDef struct: `{id, name, passiveName, passiveDesc, extraCards}`.

## Skills (SKILL_COUNT=8)

| ID | Name | Effect | Type |
|----|------|--------|------|
| 0 | 炸弹强化 | Bombs rank +3, can beat same-rank bombs | BUFF |
| 1 | 火箭冲刺 | Draw 3 cards when playing Rocket | TRIGGER |
| 2 | 顺子延长 | Min straight length 4 (down from 5) | BUFF |
| 3 | 连对增幅 | Min consecutive pairs 2 (down from 3) | BUFF |
| 4 | 三带一强化 | Triple+one/two can carry 1 extra kicker | BUFF |
| 5 | 飞机连射 | Min airplane groups 1 (down from 2) | BUFF |
| 6 | 炸弹馈赠 | Draw 1 card when playing a bomb | TRIGGER |
| 7 | 连环炸弹 | Auto-play a bomb after playing (1/turn) | TRIGGER |

SkillDef struct: `{id, name, desc}`. No energy cost or cooldown fields.
Buff-type skills (0/2/3/4/5) set `SkillBuffs` fields cleared at turn end. Trigger-type (1/6/7) set flags consumed on activation.

## Key constants

| Constant | Value | Location |
|----------|-------|----------|
| `DECK_SIZE` | 54 | card.hpp |
| `STANDARD_DEAL` | 15 | game_state.cpp |
| `SKILL_COUNT` | 8 | skill.hpp |
| `MAX_SKILL_SLOTS` | 3 | skill.hpp |
| `CHAR_COUNT` | 3 | character.hpp |
| `DEFAULT_W × DEFAULT_H` | 1200×750 | renderer.hpp |
| `CARD_W × CARD_H` | 105×150 | renderer.hpp |

## Testing

No automated test framework. Validation is manual: build → run → exercise each screen, hand type, skill activation, mirror mechanic, and AI learning across multiple levels.
