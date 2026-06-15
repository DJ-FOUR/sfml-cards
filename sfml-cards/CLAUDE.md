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
所有 VS Code 配置文件（`.vscode/`）都通过 `${env:MINGW_HOME}` 引用 MinGW 路径，换电脑只需设置 `MINGW_HOME` 环境变量即可。

Build output goes to `build/main.exe`. CMake auto-copies `images/` and `resources/` post-build.

## Controls

- `F11` — toggle fullscreen/windowed
- `R` — redeal current level (development only)
- Dev buttons ("我赢"/"我输") — force win/lose, top-left during gameplay

## Project

**斗牌Rogue** — C++17 + SFML 3.0.2 单机斗地主肉鸽卡牌游戏。SFML 本地捆绑在 `SFML-3.0.2/`，静态链接。

Full architecture docs: [AGENTS.md](AGENTS.md) | Gameplay rules: [GAMEPLAY.md](GAMEPLAY.md) | Art style: [画面风格.md](画面风格.md)

## Architecture

### Screen state machine (`main.cpp`)

`Screen` enum: `MainMenu → CharacterSelect → WildcardSelect(仅谋略家) → Reward → Transition → Game → Reward → GameOver`. `Settings` 可从 Game / Transition / Reward 界面打开（右上角齿轮按钮），关闭后回到原界面。

`Game = GameState{}` 在所有返回主菜单路径上调用，完全重置游戏状态。`AIMemory::clear()` 同步清空。

### Module dependency graph

```
main.cpp
  ├── renderer.hpp/cpp    — 全部 SFML 绘制 + 点击检测（所有界面）
  ├── game_state.hpp/cpp  — 核心规则引擎：牌型识别、大小比较、AI、技能效果
  │     ├── card.hpp/cpp  — 卡牌数据模型、创建牌组、imageIndex 映射
  │     ├── skill.hpp/cpp — 技能定义 (3 skills) + SkillBuffs 结构体
  │     └── ai_memory.hpp/cpp — k-NN 学习：记录玩家决策，引导 AI 模仿
  ├── run_state.hpp/cpp   — Run 级状态：关卡数、已获得技能、装备槽、镜像快照
  │     ├── character.hpp/cpp — 3 个角色定义
  │     └── skill.hpp/cpp
```

### Phase enum

`Phase { PlayerTurn, ComputerTurn, SchedulePlay, MomentumPlay, PlayerWins, ComputerWins }`

- **SchedulePlay**: 掌控者「调度」— 选择至多 3 张手牌弃掉换牌（首回合可用，之后冷却 2 回合）
- **MomentumPlay**: 连击之势触发（玩家或敌人连续 2 回合不出牌）— 选择 1 张牌免费打出

### 手牌分类管道

`GameState::classifyHand()` 是核心规则函数。接收 Card 向量 + 可选 `SkillBuffs*`，返回 `std::optional<HandPattern>`。SkillBuffs 携带规则修正（`straightExtended` 降低顺子最小长度；`jokerWill` 使小王免疫炸弹）。`GameState::beats()`（非 static，需访问 `m_enemyBuffs`）比较两手牌。

支持癞子：`SkillBuffs::wildcardRank` 标记的点数可替代任何缺失牌。

`GameState::classifyHandNoWild()` 是忽略癞子的变体，用于 `findBeatingPlays()` 中枚举合法出牌选项（避免癞子替换所有缺失牌）。

**新增技能需要修改规则时**：
1. 在 `SkillBuffs` 中加字段 (skill.hpp)
2. 在 `classifyHand()` / `beats()` / `findBeatingPlays()` 中检查该字段 (game_state.cpp)
3. 在 `setPlayerSkillSlots()` 和 `setEnemySkills()` 中设置该字段 (game_state.cpp)

### 保底炸弹（炸弹收藏家）

`dealCards()` 为炸弹收藏家确保手牌中至少有 1 个炸弹（4 张同点数）。发牌后若手牌无炸弹，自动从手牌中移除点数最少的牌，从抽牌堆换入 4 张同点数牌（不足时合成）。

### 镜像机制

敌人继承玩家上一关的 3 个已装备技能。`RunState::mirroredSkills()` 返回 `m_mirroredSkills`（关卡推进前的快照）。第 1 关敌人无技能 `{-1,-1,-1}`；第 2 关起继承。

### 技能系统

- **全部技能为 PASSIVE** — 装备即生效，无需手动激活。`SkillType` 枚举含 `BUFF`、`TRIGGER`、`PASSIVE` 三种，当前仅使用 `PASSIVE`
- `SkillBuffs` 字段在 `setPlayerSkillSlots()` 中一次设置，跨回合持久。`endPlayerTurnCleanup()` 清除后重新应用
- `enemyActivateSkills()` 是空操作；敌人 buff 在 `setEnemySkills()` 中一次设置

### AI 学习系统 (`ai_memory.hpp/cpp`)

单次 Run 内的双组件在线学习：

**出牌学习 (k-NN)**：记录玩家每次决策为 `(PlayFeatures, PlayAction)` 对（最多 2000 条）。K=5 最近邻，加权特征距离，相似度加权动作分布。在 `computerTakeTurn()` 中用于重排合法选项。

**技能使用学习**：追踪 `skillId → use count`。AI 在概率 ≥ 0.4 时激活 buff。

**生命周期**：`AIMemory` 由 `main.cpp` 持有，选角时 / 游戏结束 / 返回菜单时清空。Run 内跨关卡累积。

### AI 决策流程 (`GameState::computerTakeTurn()`)

1. 跟牌时：`findBeatingPlays()` 生成所有合法压牌选项
2. 选择：基于成本的启发式（`evaluatePlayCost`：拆炸弹 +50，三条 +15，对子 +3）结合 k-NN 偏好重排
3. 炸弹/火箭：手牌 >4 张时跳过；≤4 张时优先
4. 无法跟牌：检查连击之势连败次数（玩家 `m_playerPassStreak` / 敌人 `m_enemyPassStreak`）→ ≥2 则进入 MomentumPlay（`playerMomentumPlay()` 或 `enemyMomentumPlay()`）；否则过牌切回 PlayerTurn
5. 新一轮：`findLowestPlay()` 优先多牌组合（顺子 → 三带二 → 三带一 → 三条 → 对子 → 单牌）

### 游戏 UI 布局

- **敌人手牌**：顶部 (`computerHandY = h*0.05`)
- **敌人技能槽**：右上角，暗红警示风格，3 个小卡牌比例矩形
- **设置按钮**：右上角齿轮图标，hover 变青，点击打开设置弹窗
- **出牌区**：中央（敌人在上，玩家在下）
- **出牌/不出按钮**：手牌上方居中 (`h*0.56`)，PlayerTurn 和 MomentumPlay 时可见
- **玩家手牌**：底部 (`handCardY = h*0.66`)，竖直排列，扇形排布
- **玩家技能槽**：左下角 (`w*0.03`, `h*0.83`)，3 个卡牌比例面板，标签"装备槽"
- **炸弹印记**：右下角（星标 + "印记 N/3"），仅炸弹收藏家可见
- **返回按钮**：左上角切角风格
- **状态栏**：底部居中，按 Phase 变色（玩家回合=青，敌方=粉，胜利=黄）
- **设置弹窗**：半透明遮罩 + 居中切角面板，含音乐/音效滑块 + 返回主界面/关闭按钮

### 动画效果

| 动画 | 说明 |
|------|------|
| **发牌动画** | 开局卡牌从下方飞入，延迟交错 0.08s，持续 0.6s，带缩放+淡入 |
| **卡牌飞入** (飞牌动画) | 炸弹生成 / 调度换牌时，新牌从屏幕右侧飞入，粉色发光外框 |
| **悬停动效** | `HoverAnimState` + lerp 平滑：手牌上浮、按钮 108% 缩放、角色/技能牌上浮+缩放 |
| **连击之势** | 暗色遮罩 + 中央技能卡牌缩放动画 (1.5x→0.3x) + 提示文字淡入 |
| **调度立绘飞行** | 掌控者角色卡牌飞向技能槽（在卡牌飞入完成后执行） |
| **炸弹闪光** | 红色斜条纹闪光 2 秒 |

### 音频

- **背景音乐**：`resources/music/first.mp3`，`sf::Music`，循环播放，音量上限 50%。备选文件 `Start.mp3`、`second.mp3` 未使用
- **悬停音效**：`resources/sound/touch.mp3`，`sf::Sound`，卡牌 hover 进入时播放
- 音量通过设置弹窗滑块调节，调用 `Renderer::setMusicVolume()` / `setSoundVolume()`，音乐音量上限 50%

## Code conventions

- `#pragma once` 头文件，`enum class` 枚举，`constexpr` 编译期常量
- 私有成员前缀 `m_`（如 `m_playerHand`，`m_phase`）
- 无状态工具函数声明为 `static`（如 `GameState::classifyHand`）。`beats()` 非 static——需要访问 `m_enemyBuffs`
- `.cpp` 文件内局部辅助函数放入匿名命名空间
- `std::optional` 可空返回，`std::unique_ptr` 管理 SFML 对象
- 平台相关代码用 `#ifdef _WIN32`
- 所有注释和 UI 文本使用中文

## Characters (CHAR_COUNT=3)

| ID | Name | Passive | Effect |
|----|------|---------|--------|
| 0 | 炸弹收藏家 | 收藏 | +1 手牌 (16张)；炸弹印记：每炸一次 +1 印记，3 印记生成 1 个炸弹 |
| 1 | 谋略家 | 谋定 | 选择癞子点数 (3~2) |
| 2 | 掌控者 | 调度 | +1 手牌 (16张)；每 2 回合可弃至多 3 张手牌换等量牌 |

## Skills (SKILL_COUNT=3)

| ID | Name | Effect |
|----|------|--------|
| 0 | 连击之势 | 敌人连续 2 回合不出牌时触发；选 1 张手牌免费打出 |
| 1 | 顺子大师 | 顺子最低长度 4 张（原 5 张） |
| 2 | 王牌意志 | 你的小王只能被大王压制（炸弹无效） |

全部 `SkillType::PASSIVE`。SkillDef: `{id, name, desc, type}`。SkillBuffs 携带 `straightExtended` 和 `jokerWill`。

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

## 卡牌尺寸速查

| 位置 | 宽高比 | 默认尺寸 (1200×750) | 计算方式 |
|------|--------|---------------------|----------|
| 手牌 / 通用卡牌 | 105:150 (7:10) | 105×150 (基准) | `CARD_W × CARD_H` 常量 |
| 技能选择卡牌 | 7:10 | 221×315 | `h*0.42 × CARD_W/CARD_H` |
| 角色选择卡牌 | 7:12 | 240×411 | `w*0.20 × 12/7` |

## Testing

无自动化测试框架。手工验证：构建 → 运行 → 遍历各界面 → 出牌/过牌/技能/胜负。

## Key files quick reference

| File | Purpose |
|------|---------|
| `src/main.cpp` | 入口、主循环、Screen 状态机、事件路由、所有状态变量 |
| `src/renderer.cpp` | 全部绘制 + 点击检测（~3000 行，最大文件） |
| `src/game_state.cpp` | 核心规则、AI、技能效果、回合流转 |
| `CMakeLists.txt` | 构建配置，SFML 静态链接，MinGW 静态运行时 |
