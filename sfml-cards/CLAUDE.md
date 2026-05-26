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

`Screen` enum drives the game loop: `MainMenu → CharacterSelect → Reward(初始技能) → Transition → Game → Reward → GameOver`. After character select, the player picks an initial skill (3 random choices), then goes to Transition to equip before the first battle. Each screen has corresponding `draw*`/`hit*` method pairs in the Renderer. State transitions happen in the event handling block of `main.cpp:95-288`.

### Module dependency graph

```
main.cpp
  ├── renderer.hpp/cpp    — All SFML drawing + hit-testing (every screen)
  ├── game_state.hpp/cpp  — Core rules engine: hand classification, comparison, AI, skill activation
  │     ├── card.hpp/cpp  — Card data model, deck creation, image index mapping
  │     └── skill.hpp/cpp — Skill definitions (8 skills) + SkillBuffs struct
  ├── run_state.hpp/cpp   — Run-level state: level #, acquired skills, equipped slots, mirroring
  │     ├── character.hpp/cpp — 3 character definitions with passive abilities
  │     └── skill.hpp/cpp
  └── save.hpp/cpp        — 3-slot text-based save (legacy, not central to gameplay)
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

The core roguelike hook: enemies inherit the player's 3 equipped skills from the previous level. `RunState::mirroredSkills()` returns `m_mirroredSkills` — a snapshot taken in `advanceToNextLevel()` right before the level increments. Level 1 enemies always get `{-1,-1,-1}` (no skills); level 2+ enemies get the snapshot. Enemy skills run on cooldown only — they don't consume energy.

### Energy system

- Start: `START_ENERGY=3`, max: `MAX_ENERGY=10`
- +1 energy at start of each player turn (争锋者: +2). `GameState::startPlayerTurn()` handles this.
- 争锋者 passive: `extraEnergy=1` adds to `energyPerTurn` via `setEnergyPerTurn(1 + run.extraEnergy())`
- Enemy skills don't consume energy (cooldown-only)
- Cooldowns tick down by 1 in `startPlayerTurn()`; enemy cooldowns tick in `enemyActivateSkills()`
- Buffs clear at end of player turn (`endPlayerTurnCleanup()`)

### Game UI layout

- Enemy hand cards: top of screen (`computerHandY = h*0.05`)
- Enemy skills: top, card-proportioned rectangles
- Played cards: center area (enemy above, player below)
- "出牌"/"不出" buttons: centered above player hand cards (`h*0.56`), only visible during PlayerTurn
- Player hand cards: bottom (`handCardY = h*0.66`)
- Player skills: bottom-left corner (`w*0.03`, `h*0.83`), card-proportioned (CARD_W/CARD_H ratio)
- Energy bar: bottom-center
- Dev debug buttons ("我赢"/"我输"): top-left area, only during active gameplay

### Hover effects

- Hand cards: smooth float-up animation (`h*0.05` lift) on hover, golden glow border on selected
- Play/Pass buttons: scale to 110% on hover (smooth lerp animation)
- Character select cards: float + scale + 3D perspective tilt
- Reward skill cards: float + scale + glow border
- All animations use `HoverAnimState` struct with lerp-based smoothing (`SPEED = 14.0f`)

### AI decision flow (`GameState::computerTakeTurn()`)

1. If following (not a new round): call `findBeatingPlays()` to find the smallest legal play that beats `m_lastPlay`
2. If no beating play found: pass
3. If new round (free play): call `findLowestPlay()` which plays the smallest single card
4. Bomb/Rocket only considered when hand ≤ 4 cards

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
| 0 | 争锋者 | 强袭 | Energy per turn +1 (2/turn instead of 1) |
| 1 | 谋略家 | 谋定 | First skill switch per level has no cooldown |
| 2 | 掌控者 | 储备 | +2 hand size (17 cards instead of 15) |

## Skills (SKILL_COUNT=8)

| ID | Name | Effect | Cost | CD |
|----|------|--------|------|----|
| 0 | 炸弹强化 | Bombs rank +3, can beat same-rank bombs | 2 | 2 |
| 1 | 火箭冲刺 | Draw 3 cards when playing Rocket | 1 | 3 |
| 2 | 顺子延长 | Min straight length 4 (down from 5) | 3 | 3 |
| 3 | 连对增幅 | Min consecutive pairs 2 (down from 3) | 2 | 3 |
| 4 | 三带一强化 | Triple+one/two can carry 1 extra kicker | 2 | 1 |
| 5 | 飞机连射 | Min airplane groups 1 (down from 2) | 3 | 3 |
| 6 | 炸弹馈赠 | +2 energy when playing a bomb | 1 | 2 |
| 7 | 连环炸弹 | Auto-play a bomb after playing (1/turn) | 4 | 4 |

Buff-type skills (0/2/3/4/5) set `SkillBuffs` fields cleared at turn end. Trigger-type (1/6/7) set flags consumed on activation; enemy implementation of trigger skills is limited.

## Key constants

| Constant | Value | Location |
|----------|-------|----------|
| `DECK_SIZE` | 54 | card.hpp |
| `STANDARD_DEAL` | 15 | game_state.cpp |
| `SKILL_COUNT` | 8 | skill.hpp |
| `MAX_SKILL_SLOTS` | 3 | skill.hpp |
| `MAX_ENERGY` | 10 | skill.hpp |
| `START_ENERGY` | 3 | skill.hpp |
| `CHAR_COUNT` | 3 | character.hpp |
| `DEFAULT_W × DEFAULT_H` | 1200×750 | renderer.hpp |
| `CARD_W × CARD_H` | 105×150 | renderer.hpp |

## Testing

No automated test framework. Validation is manual: build → run → exercise each screen, hand type, skill activation, and the mirror mechanic across multiple levels.
