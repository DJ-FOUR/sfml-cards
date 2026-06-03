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

`Screen` enum drives the game loop: `MainMenu → CharacterSelect → WildcardSelect(谋略家) → Reward(初始技能) → Transition → Game → Reward → GameOver`. Main menu has "开始游戏" (→ CharacterSelect) and "退出". After character select, the player picks an initial skill (3 random choices from 3 skills), then goes to Transition to equip before the first battle. If all 3 skills are already acquired, the Reward screen is skipped and the game goes directly to Transition. Each screen has corresponding `draw*`/`hit*` method pairs in the Renderer. State transitions happen in the event handling block of `main.cpp`.

`Game = GameState{}` is called on all return-to-main-menu paths to fully reset game state (bomb marks, buffs, etc.).

### Module dependency graph

```
main.cpp
  ├── renderer.hpp/cpp    — All SFML drawing + hit-testing (every screen)
  ├── game_state.hpp/cpp  — Core rules engine: hand classification, comparison, AI, skill effects
  │     ├── card.hpp/cpp  — Card data model, deck creation, image index mapping
  │     ├── skill.hpp/cpp — Skill definitions (3 skills) + SkillBuffs struct
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

### Phase enum

`Phase { PlayerTurn, ComputerTurn, MomentumPlay, PlayerWins, ComputerWins }`. `MomentumPlay` is entered when 连击之势 triggers — player selects 1 card to play freely, then `startNewRound()` + `PlayerTurn`.

### Hand classification pipeline

`GameState::classifyHand()` is the central rules function. It takes a vector of Cards + optional `SkillBuffs*` and returns `std::optional<HandPattern>`. The SkillBuffs struct carries rule modifications (`straightExtended` reduces min straight length from 5 to 4; `jokerWill` makes Small Joker immune to bombs in `beats()`). `GameState::beats()` (non-static) compares two `PlayedCards`.

`classifyHand` also supports wildcards (癞子) via `SkillBuffs::wildcardRank` — cards of that rank substitute for missing pieces in any hand type.

**Adding a new skill that modifies hand rules** requires:
1. Add a field to `SkillBuffs` (skill.hpp)
2. Check that field in `classifyHand()` / `beats()` / `findBeatingPlays()` (game_state.cpp)
3. Set the field in `setPlayerSkillSlots()` and `setEnemySkills()` (game_state.cpp) — all skills are passive

### Guaranteed bomb

`dealCards()` ensures the player always starts with a bomb (4-of-a-kind). After dealing, it checks frequency; if no rank has ≥4 cards, it picks the rank with fewest cards in hand, removes those, and replaces with 4 of that rank from the draw pile (synthesizing if needed).

### Mirror mechanic

The core roguelike hook: enemies inherit the player's 3 equipped skills from the previous level. `RunState::mirroredSkills()` returns `m_mirroredSkills` — a snapshot taken in `advanceToNextLevel()` right before the level increments. Level 1 enemies always get `{-1,-1,-1}` (no skills); level 2+ enemies get the snapshot.

### Skill system

- **All skills are PASSIVE** — equip-and-forget, no manual activation needed. Effects apply automatically when equipped via `setPlayerSkillSlots()`.
- `SkillBuffs` fields (`straightExtended`, `jokerWill`) are set once in `setPlayerSkillSlots()` and persist across turns. `endPlayerTurnCleanup()` re-applies them after clearing.
- `enemyActivateSkills()` is a no-op; enemy buffs are set once in `setEnemySkills()` and persist.
- Adding a new passive skill:
  1. Add `SkillDef` entry in `skill.cpp`
  2. Add any needed field to `SkillBuffs` (skill.hpp) if it modifies hand rules
  3. Set the field in `setPlayerSkillSlots()` and `setEnemySkills()` (game_state.cpp)
  4. Check the field in `classifyHand()` / `beats()` / `findBeatingPlays()` (game_state.cpp)
- Skill-specific state (e.g., `m_momentumActive`, `m_enemyPassStreak`) is managed in GameState.
- Renderer shows skill type labels via `skillTypeLabel()` helper.

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

1. Enemy activates skills (no-op for passive skills — buffs already set).
2. If following (not a new round): `findBeatingPlays()` generates all legal beating plays.
3. Selection: cost-based heuristic (`evaluatePlayCost`: breaking bomb +50, triple +15, pair +3) combined with k-NN memory preference re-ranking.
4. Bombs/rockets: skipped when hand > 4 cards; prioritized when ≤ 4.
5. If no beating play: check 连击之势 streak → if ≥2, enter MomentumPlay; otherwise pass, switch to PlayerTurn.
6. If new round: `findLowestPlay()` tries multi-card combos first (straight → triple+two → triple+one → triple → pair → single).

### Game UI layout

- Enemy hand cards: top of screen (`computerHandY = h*0.05`)
- Enemy skills: top, card-proportioned rectangles (dark red, shows PASV label)
- Played cards: center area (enemy above, player below)
- "出牌"/"不出" buttons: centered above player hand cards (`h*0.56`), visible during PlayerTurn and MomentumPlay
- Player hand cards: bottom (`handCardY = h*0.66`), upright (no fan rotation/arc), straight line layout
- Player skills: bottom-left corner (`w*0.03`, `h*0.83`), card-proportioned, golden border when skill is actively triggered
- Bomb marks display: bottom-right (3 dots + "印记 N/3"), only for 炸弹收藏家 character
- MomentumPlay overlay: dark dim (behind hand cards, hand stays bright), animated skill card center screen (1.5x→0.3x over 0.5s), hint text
- Dev debug buttons ("我赢"/"我输"): top-left area, only during active gameplay
- "开始战斗" button (Transition screen): rectangular (`w*0.18 × h*0.09`), green default / magenta hover, text shakes violently on hover

### Hover effects

- Hand cards: smooth float-up animation (`h*0.025f` lift) on hover, golden glow border on selected
- Play/Pass buttons: scale to 108% on hover (smooth lerp animation)
- Character select cards: float + scale (same as reward skill cards, no 3D tilt)
- Reward skill cards: float + scale + glow border
- Fight button (Transition): magenta outline + hazard stripes + text shake (6-frequency sine wave叠加)
- All animations use `HoverAnimState` struct with lerp-based smoothing (`ANIM_SPEED_CHAR_HOVER = 14.0f` for characters/skills, `ANIM_SPEED_BTN_HOVER = 18.0f` for buttons, `ANIM_SPEED_CARD_HOVER = 16.0f` for hand cards)

## Code conventions

- `#pragma once` for headers, `enum class` for enums, `constexpr` for compile-time constants
- Private members prefixed with `m_` (e.g., `m_playerHand`, `m_phase`)
- Stateless utility functions declared `static` (e.g., `GameState::classifyHand`, `GameState::classifyHandNoWild`). `beats()` is non-static — it needs access to `m_enemyBuffs` for 王牌意志.
- File-local helpers go in anonymous namespaces in `.cpp` files
- `std::optional` for nullable returns, `std::unique_ptr` for SFML objects
- Platform-specific code uses `#ifdef _WIN32`
- All comments and UI text in Chinese (中文)

## Characters

| ID | Name | Passive | Effect |
|----|------|---------|--------|
| 0 | 炸弹收藏家 | 收藏 | +1 hand size (16 cards); bomb marks: each bomb played → +1 mark, 3 marks → generate a bomb |
| 1 | 谋略家 | 谋定 | Pick a wildcard rank (癞子) from 3~2 |
| 2 | 掌控者 | 储备 | +2 hand size (17 cards instead of 15) |

CharacterDef struct: `{id, name, passiveName, passiveDesc, extraCards}`.
Character 0 passive (bomb collector) is wired via `GameState::setPlayerIsBombCollector(bool)` and tracked in `m_isBombCollector`/`m_bombMarks`.

## Skills (SKILL_COUNT=3)

| ID | Name | Effect |
|----|------|--------|
| 0 | 连击之势 | Enemy passes 2 consecutive turns → MomentumPlay: pick 1 card to play freely, then continue turn |
| 1 | 顺子大师 | Min straight length 4 (down from 5) |
| 2 | 王牌意志 | Your Small Joker can only be beaten by Big Joker (bombs can't beat it) |

All skills are `SkillType::PASSIVE` — automatically active when equipped.
SkillDef struct: `{id, name, desc, type}`. `SkillBuffs` carries `straightExtended` and `jokerWill`.

## Key constants

| Constant | Value | Location |
|----------|-------|----------|
| `DECK_SIZE` | 54 | card.hpp |
| `STANDARD_DEAL` | 15 | game_state.cpp |
| `SKILL_COUNT` | 3 | skill.hpp |
| `MAX_SKILL_SLOTS` | 3 | skill.hpp |
| `CHAR_COUNT` | 3 | character.hpp |
| `DEFAULT_W × DEFAULT_H` | 1200×750 | renderer.hpp |
| `CARD_W × CARD_H` | 105×150 | renderer.hpp |

## Testing

No automated test framework. Validation is manual: build → run → exercise each screen, hand type, skill activation, mirror mechanic, and AI learning across multiple levels.
