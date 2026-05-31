<!-- AGENTS.md for 斗牌Rogue -->
# 斗牌Rogue — SFML 卡牌游戏

## 项目概述

本项目是一个基于 **C++17 + SFML 3.0.2** 开发的单机斗地主肉鸽卡牌对战游戏，游戏名为 **斗牌Rogue**。

核心玩法为无尽闯关（Endless Run）：玩家选择角色后进入第1关，与镜像AI对战。每关双方各发15张牌（部分角色有额外手牌），轮流出牌，先清空手牌者获胜。选择角色后先从3个随机技能中选1个初始技能，过关后再次从3个随机技能中选1个获得。每关开始前可装备最多3个技能。从第2关起，敌人会自动装备玩家上一关实际使用的技能（镜像机制）。AI还会通过学习玩家在历史对局中的出牌习惯和技能使用频率来模仿玩家行为。

所有UI文本、代码注释、模块文档均使用**中文**。

---

## 技术栈

| 层级 | 技术 |
|------|------|
| 语言 | C++17 |
| 图形/窗口/音频 | SFML 3.0.2（本地捆绑，静态链接） |
| 构建系统 | CMake ≥ 3.16 |
| 编译器 | MinGW-w64 GCC（基于 `.vscode/c_cpp_properties.json` 配置） |
| IDE | VS Code（已配置 tasks / launch / c_cpp_properties） |
| 平台 | primarily Windows |

无 `package.json`、`pyproject.toml`、`Cargo.toml` 等其他语言生态的配置文件。本项目为纯 C++ 项目。

---

## 关键配置文件

| 文件 | 作用 |
|------|------|
| `CMakeLists.txt` | 构建配置：定义 C++17 标准、SFML 静态链接、MinGW 静态运行时、资源自动复制 |
| `.vscode/tasks.json` | VS Code 构建任务：`cmake configure` + `cmake build`（默认 `Ctrl+Shift+B`） |
| `.vscode/launch.json` | VS Code 调试配置：gdb 调试 `build/main.exe`，预执行构建任务 |
| `.vscode/c_cpp_properties.json` | IntelliSense 配置：包含路径、编译器路径、C++17 标准 |

---

## 项目结构

```
src/
├── card.hpp / card.cpp              # 卡牌数据模型：Suit、Rank、Card 结构体、洗牌
├── game_state.hpp / game_state.cpp  # 核心规则：牌型识别、大小比较、回合管理、AI逻辑、技能Buff
├── renderer.hpp / renderer.cpp      # 全部渲染与鼠标点击检测（所有界面 + 游戏）
├── skill.hpp / skill.cpp            # 技能数据定义（8个技能）与回合Buff结构体
├── character.hpp / character.cpp    # 3个角色定义（仅影响初始手牌数）
├── run_state.hpp / run_state.cpp    # Run全局状态：关卡进度、技能获取/装备、镜像继承
├── ai_memory.hpp / ai_memory.cpp    # AI学习系统：k-NN出牌 mimicry + 技能使用概率跟踪
└── main.cpp                         # 入口、主循环、Screen状态机、事件路由

images/
├── background/                      # 背景图（back0.png）
├── card/                            # 牌面 card0.png ~ card53.png + 牌背 card.png
└── cardchoose/                      # 备用卡图（当前未使用）

resources/
├── simsun.ttc                       # 中文字体（宋体，~18MB）
└── tuffy.ttf                        # 备用字体（当前未使用）

SFML-3.0.2/                          # 捆绑的 SFML 静态库（include/ + lib/ + bin/）
build/                               # CMake 构建输出（含 main.exe、资源副本）
dist/                                # 独立发布包
```

---

## 构建与运行

### 使用 VS Code（推荐）

1. 按 `Ctrl+Shift+B` 运行默认构建任务 **"cmake build"**（会自动先执行 configure）。
2. 按 `F5` 启动调试（gdb → `build/main.exe`）。
3. 游戏中按 `F11` 切换全屏/窗口模式。

### 命令行手动构建

```bash
cmake -B build -S . -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

构建产物为 `build/main.exe`。CMake 会自动将 `images/` 和 `resources/` 复制到构建输出目录，确保 exe 双击即可运行。

**注意**：CMake 中设置了 `-static` 和 `SFML_STATIC`，链接的是带 `-s` 后缀的静态库。但构建输出目录中仍可能出现 `libgcc_s_seh-1.dll`、`libstdc++-6.dll`、`libwinpthread-1.dll` 等 MinGW 运行时 DLL。若需完全单文件分发，需确认静态链接 flags 实际生效。

### 发布目录

`dist/` 目录下已有独立可执行文件及其依赖资源，可直接分发。**注意**：`dist/resources/` 目前缺少 `simsun.ttc`，若发现中文显示异常，需手动复制该字体文件。

---

## 代码组织与模块职责

### 模块依赖关系

```
main.cpp
  ├── renderer.hpp/cpp    — 所有 SFML 绘制 + 碰撞检测（全部界面）
  ├── game_state.hpp/cpp  — 核心规则引擎：牌型识别、大小比较、AI、技能激活
  │     ├── card.hpp/cpp  — 卡牌数据模型、创建牌组、图片索引映射
  │     └── skill.hpp/cpp — 技能定义（8个）+ SkillBuffs 结构体
  ├── run_state.hpp/cpp   — Run 级状态：关卡数、已获得技能、装备槽、镜像快照
  │     ├── character.hpp/cpp — 3 个角色定义
  │     └── skill.hpp/cpp
  ├── ai_memory.hpp/cpp   — AI 学习记忆（k-NN + 技能使用统计）
```

### `card` 模块

- 定义 `Suit`（花色：`Spade`, `Heart`, `Club`, `Diamond`, `None`）、`Rank`（点数 `Three`~`BigJoker`）枚举，含大小王。
- `Card` 结构体含 `suit`、`rank`、`imageIndex`，重载 `==` 以支持卡牌比较。排序由 `sortByDZ()` 自定义 lambda 按斗地主牌力顺序（3 < 4 < ... < A < 2 < 小王 < 大王）处理。
- `createDeck()` 创建 54 张牌并洗牌。
- `cardFromImageIndex()` 将图片索引 0~53 映射为具体卡牌。
- 硬编码 `CARD_MAP[54]` 数组定义图片索引到 `(suit, rank)` 的映射关系：
  - 0~3: 黑桃/红桃/梅花/方片 3
  - 4~7: 4
  - ...依此类推...
  - 44~47: A
  - 48~51: 2
  - 52~53: 小王、大王

### `skill` 模块

- `SkillDef`：技能定义结构体（id、`std::wstring` 名称/描述）。**当前版本无能量消耗、无冷却回合**。
- `SKILL_COUNT = 8`，`MAX_SKILL_SLOTS = 3`。
- `SkillBuffs`：当前回合生效的技能效果结构体（`bombBoosted`, `bombRankBonus=3`, `straightExtended`, `pairsExtended`, `tripleExtraKicker`, `airplaneExtended`）。
- `getAllSkills()` 返回静态技能数组。

当前已实现的8个技能（S01~S08）：

| ID | 名称 | 效果 |
|----|------|------|
| 0 | 炸弹强化 | 本回合炸弹视为更大炸弹（+3级），可压过同级炸弹 |
| 1 | 火箭冲刺 | 打出火箭时额外抽3张牌 |
| 2 | 顺子延长 | 顺子最低长度-1（4张即可出） |
| 3 | 连对增幅 | 连对最低组数-1（2组即可出） |
| 4 | 三带一强化 | 三带一/三带二可额外多带一张单牌 |
| 5 | 飞机连射 | 飞机最低组数-1（1组即可出） |
| 6 | 炸弹馈赠 | 打出炸弹后额外抽1张牌 |
| 7 | 连环炸弹 | 出牌后若手中有炸弹自动打出（限1次） |

**注意**：技能在战斗内是“本回合生效”的BUFF类型（除S01/S07/S08为触发标记）。点击技能槽即激活，无能量限制，无冷却限制。敌人AI则根据学习到的玩家使用概率决定是否激活技能（阈值 0.4）。

### `character` 模块

- `CharacterDef`：角色定义（id、`std::wstring` 名称/被动技名称/描述、`extraCards` 额外手牌数）。
- `CHAR_COUNT = 3`。

三个角色：

| ID | 名称 | 被动技 | 效果 |
|----|------|--------|------|
| 0 | 争锋者 | 强袭 | 手牌上限+1（初始16张） |
| 1 | 谋略家 | 谋定 | 手牌上限不变 |
| 2 | 掌控者 | 储备 | 手牌上限+2（初始17张） |

**注意**：当前版本角色效果仅影响初始发牌数量，无其他战斗内被动效果。

### `run_state` 模块

- `RunState`：管理一次Run的全局状态。
- 包含当前关卡数、角色id、已获得技能列表、已装备技能槽（3个，`-1`表示空）。
- `rollRewardSkills()`：从8个技能中随机选出3个作为过关奖励（允许重复出现已拥有技能，UI会标记"已拥有"）。
- `mirroredSkills()`：返回当前已装备技能，供下一关敌人继承。
- `startNewRun()` / `advanceToNextLevel()`：Run生命周期管理。进入下一关时快照当前装备技能到 `m_mirroredSkills`。

### `game_state` 模块

- `HandType` / `HandPattern`：11种牌型定义（`None`, `Single`, `Pair`, `Triple`, `TriplePlusOne`, `TriplePlusTwo`, `Straight`, `ConsecutivePairs`, `Airplane`, `Bomb`, `Rocket`）。
- `GameState`：核心状态机，管理玩家/电脑手牌、抽牌堆、`m_lastPlay`（上一手牌）、回合流转、胜负判定。
- `classifyHand()` 静态方法：识别一组牌是什么牌型，支持 `SkillBuffs` 修正规则（如顺子长度-1）。
- `beats()` 静态方法：比较两手牌大小，支持炸弹强化加成（`bombRankBonus = 3`）。
- `playerPlay()` / `playerPass()`：玩家出牌/不出。
- `computerTakeTurn()`：AI决策。跟牌时优先出最小非炸弹；自由出牌时出最小单张；手牌≤4时才考虑炸弹/火箭。AI还会根据 `AIMemory` 中的 k-NN 查询结果模仿玩家的出牌偏好。
- `activatePlayerSkill()`：玩家激活装备槽中的技能（**无能量消耗，无冷却**）。
- `enemyActivateSkills()`：敌人回合开始时自动激活其继承的技能，激活概率由 `AIMemory::querySkillProb()` 决定（≥0.4 才激活）。
- `applyPostPlayEffects()`：处理打出后的触发效果（S01火箭抽牌、S06炸弹抽牌、S08连环炸弹）。
- 标准发牌数 `STANDARD_DEAL = 15`（game_state.cpp 匿名命名空间），角色额外手牌通过 `dealCards(int extraCards)` 传入。
- `setAIMemory()` 注入 `AIMemory` 指针，用于记录玩家行为和学习。

### `ai_memory` 模块

- `AIMemory`：AI 学习记忆系统，被 `GameState` 和 `main.cpp` 共同使用。
- **出牌学习**：记录玩家每次出牌/不出时的 `PlayFeatures`（手牌数、是否新一轮、上一手牌型/牌力、是否有炸弹/火箭、关卡数）和 `PlayAction`（出的牌型/牌力/张数、是否不出）。采用 k-NN（k=5）按距离反比加权查询最相似历史记录。
- **技能学习**：记录玩家使用各技能的次数。`querySkillProb(skillId)` 返回该技能使用次数 / 最大使用次数（0.0~1.0）。若玩家从未用过某技能，AI 也不用。
- 最大记录数 `MAX_PLAY_RECORDS = 2000`，超出时丢弃最旧记录。
- `clear()`：玩家死亡/返回主菜单时清空记忆。

### `renderer` 模块

- 一次性加载54张牌面纹理、牌背、背景图、字体（`simsun.ttc`，失败时回退到 `C:/Windows/Fonts/msyh.ttc`）。
- **动态比例布局**：所有坐标按窗口宽高比例计算，支持任意缩放。默认窗口 `1200×750`。
- **扇形手牌**：相邻牌重叠约65%，自动居中。选中的牌会抬起 `h*0.05`。
- 提供各界面的 `draw*` 与 `hit*`（点击检测）函数对。
- 渲染的界面包括：
  - `drawMainMenu` / `hitMainMenu`：主菜单（开始游戏、退出）
  - `drawCharacterSelect` / `hitCharacterSelect`：角色选择（3D透视倾斜悬停动画）
  - `drawTransition` / `hitTransitionSkill` / `hitTransitionSlot` / `hitTransitionFight`：关卡过渡（装备技能、查看敌人预览、开始战斗）
  - `renderGame` / `hitTestCard` / `hitTestGameButton` / `hitTestSkillSlot` / `hitTestDebugButton`：战斗界面（手牌、出牌区、技能槽、出牌/不出按钮、开发调试用"我赢"/"我输"按钮）
  - `drawReward` / `hitReward`：过关奖励（3选1技能，带悬停缩放发光动画）
  - `drawGameOver` / `hitGameOver`：失败界面
- `updateAnimations(float dt)`：统一更新所有界面的悬停/缩放/浮动动效。使用 `HoverAnimState` 结构体做 lerp 平滑（`ANIM_SPEED_CHAR_HOVER = 14.0f`）。
- `renderCharCardToRT(int charIdx, bool hover)`：预渲染角色卡片到 `sf::RenderTexture`，支持悬停时的3D透视倾斜效果。
- `startDealAnimation()` / 发牌动画系统：卡牌从下方飞入，带延迟和缩放效果。

### `main` 模块

- `Screen` 枚举：`MainMenu → CharacterSelect → Reward → Transition → Game → Reward → Transition → Game → ... → GameOver`。
- 主循环三阶段：事件处理 → 更新（AI计时、胜负判定、阶段切换）→ 渲染。
- 全局快捷键：`R` 重开发牌，`F11` 全屏切换。
- 游戏中点击技能槽激活技能，点击手牌选择/取消选择，点击"出牌"/"不出"进行操作。
- 开发者调试用 "我赢"/"我输" 按钮（仅渲染，点击调用 `game.forceWin()` / `game.forceLose()`）。

---

## 代码风格规范

- **头文件保护**：使用 `#pragma once`。
- **枚举**：全部使用 `enum class`（强类型）。
- **常量**：编译期常量用 `constexpr`。
- **成员变量前缀**：私有成员以 `m_` 开头，例如 `m_playerHand`、`m_phase`。
- **工具方法**：不依赖实例状态的函数声明为 `static`（如 `GameState::classifyHand`）。
- **内部辅助**：`.cpp` 文件中的模块私有函数放入匿名命名空间。
- **可选值**：使用 `std::optional` 表示可能不存在的返回值。
- **智能指针**：使用 `std::unique_ptr` 管理 `sf::Text` 等对象。
- **排序与算法**：优先使用 lambda 配合 STL（`std::sort`、`std::find` 等）。
- **跨平台**：需要平台差异的代码用 `#ifdef _WIN32` 分支处理。
- **宽字符串**：所有中文UI文本使用 `std::wstring` / `L"..."`，与 SFML 3 的 `sf::Text` 兼容。
- **注释**：代码注释和文档使用**中文**。

---

## 测试策略

- **当前无自动化测试框架**，也没有单元测试目录。
- 验证方式以**手工运行**为主：构建 → 启动 → 遍历各界面 → 出牌/过牌/技能释放/胜负判定。
- 关键验证点：
  - 牌型识别（`classifyHand`）对各种组合的正确性
  - 牌型大小比较（`beats`）
  - 技能效果是否在本回合正确生效并清除
  - 镜像机制：第N关敌人技能 = 玩家第N-1关装备的3个技能
  - 敌人AI学习行为是否合理（k-NN 查询、技能概率阈值）
  - 各界面悬停动画流畅性
  - 窗口缩放后布局正确性

---

## 安全与部署注意事项

- **SFML 静态链接**：CMake 定义了 `SFML_STATIC` 并传入 `-static`，链接的是带 `-s` 后缀的静态库。但构建目录中仍可能出现 MinGW 运行时 DLL（`libgcc_s_seh-1.dll`、`libstdc++-6.dll`、`libwinpthread-1.dll`），若需完全单文件分发，需确认链接 flags 实际生效。
- **字体版权**：`resources/simsun.ttc`（宋体）体积约 18MB，分发时需注意字体许可。
- **图片映射**：`card{idx}.png` 的索引与 `CARD_MAP` 硬编码表一一对应（0~3 为黑桃/红桃/梅花/方片 3，依此类推，48~51 为 2，52~53 为大小王）。更换素材时只需修改 `CARD_MAP`，无需改动渲染逻辑。
- **素材替换标记**：代码中留有 `[IMG-CHAR-AVATAR]`、`[IMG-SKILL-S01~S08]`、`[IMG-SLOT-01~03]`、`[IMG-ENERGY-BAR]` 等注释标记，指示未来可替换为具体美术素材的位置。
- **输入安全**：游戏无网络功能，无用户可注入的脚本接口，无外部配置文件解析。所有用户输入仅限于鼠标点击和键盘快捷键，由 SFML 事件系统统一处理。

---

## 扩展与待办

`valiant-spinning-floyd.md` 是原始的完整策划文档，规划了30个技能（当前仅实现8个）、敌人AI难度曲线、更多干扰/资源/辅助类技能等。`UI_REDESIGN.md` 包含一套赛博朋克军事终端风格的完整视觉重设计方案（尚未实施）。

如需扩展技能系统：

1. 在 `skill.hpp/cpp` 中增加 `SkillDef` 定义。
2. 在 `SkillBuffs` 中新增对应的 buff 字段。
3. 在 `GameState::classifyHand()` 和 `beats()` 中处理新 buff 对牌型规则的影响。
4. 在 `GameState::activatePlayerSkill()` 和 `enemyActivateSkills()` 中设置 buff。
5. 如需新触发效果（如打出特定牌型后的回调），在 `applyPostPlayEffects()` 或相关位置添加逻辑。
6. 在 `renderer.cpp` 中技能卡片和按钮的渲染是通用的，通常只需确保技能名称/描述正确即可自动显示。

---

## 相关文档索引

| 文件 | 内容 |
|------|------|
| `AGENTS.md` | 本文件：面向 AI 编码代理的项目总览 |
| `CLAUDE.md` | Claude Code 专用速查：构建命令、架构速览、关键常量 |
| `GAMEPLAY.md` | 面向玩家的完整玩法说明：规则、角色、技能、操作 |
| `valiant-spinning-floyd.md` | 原始策划文档：30技能规划、敌人AI设计、局外成长方案 |
| `UI_REDESIGN.md` | 视觉重设计方案：赛博朋克军事终端风格（未实施） |
