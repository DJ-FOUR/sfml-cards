<!-- AGENTS.md for 斗牌Rogue -->
# 斗牌Rogue — SFML 卡牌游戏

## 项目概述

本项目是一个基于 **C++17 + SFML 3.0.2** 开发的单机斗地主肉鸽卡牌对战游戏，游戏名为 **斗牌Rogue**。

核心玩法为无尽闯关（Endless Run）：玩家选择角色后进入第1关，与镜像AI对战。每关双方各发15张牌（部分角色有额外手牌），轮流出牌，先清空手牌者获胜。选择角色后先从3个随机技能中选1个初始技能，过关后再次从3个随机技能中选1个获得。每关开始前可装备最多3个技能。从第2关起，敌人会自动装备玩家上一关实际使用的技能（镜像机制）。AI还会通过学习玩家在历史对局中的出牌习惯和技能使用频率来模仿玩家行为。

所有UI文本、代码注释、模块文档均使用**中文**。

美术风格：美式复古波普+街头涂鸦 — 粗重黑色硬轮廓，高饱和亮玫红/电光青蓝/明黄，平涂纯色。

---

## 技术栈

| 层级 | 技术 |
|------|------|
| 语言 | C++17 |
| 图形/窗口/音频 | SFML 3.0.2（本地捆绑，静态链接） |
| 构建系统 | CMake ≥ 3.16 |
| 编译器 | MinGW-w64 GCC |
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
├── game_state.hpp / game_state.cpp  # 核心规则：牌型识别、大小比较、回合管理、AI、技能、调度、印记
├── renderer.hpp / renderer.cpp      # 全部 SFML 绘制与点击检测（所有界面 + 动画 + 设置弹窗）
├── skill.hpp / skill.cpp            # 技能数据定义（3个技能）与 SkillBuffs 结构体
├── character.hpp / character.cpp    # 3个角色定义（角色被动技+额外手牌数）
├── run_state.hpp / run_state.cpp    # Run全局状态：关卡进度、技能获取/装备、镜像继承
├── ai_memory.hpp / ai_memory.cpp    # AI学习系统：k-NN出牌 mimicry + 技能使用概率跟踪
└── main.cpp                         # 入口、主循环、Screen状态机、事件路由

images/
├── background/                      # 背景图（start.png / back1.png）
├── card/                            # 牌面 card0.png ~ card53.png
├── character-card/                  # 角色素材
│   ├── card00.png                   # 对战牌背图片
│   ├── card01.png                   # 三合一角色卡牌原图（已切割）
│   ├── char0/1/2.png                # 角色选择界面立绘 (500×857)
│   └── char_0/1/_2.png              # 对战界面立绘 (512×1024)
└── cardchoose/                      # 备用卡图（当前未使用）

resources/
├── music/                           # 背景音乐 (first.mp3, Start.mp3, second.mp3)
├── sound/                           # 音效 (touch.mp3)
├── simsun.ttc                       # 中文字体（宋体，~18MB）
└── tuffy.ttf                        # 备用字体（当前未使用）

SFML-3.0.2/                          # 捆绑的 SFML 静态库（include/ + lib/ + bin/）
build/                               # CMake 构建输出（含 main.exe、资源副本）
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

构建产物为 `build/main.exe`。CMake 会自动将 `images/` 和 `resources/` 复制到构建输出目录。

**注意**：CMake 中设置了 `-static` 和 `SFML_STATIC`，链接的是带 `-s` 后缀的静态库。构建输出目录中仍可能出现 MinGW 运行时 DLL，若需完全单文件分发，需确认静态链接 flags 实际生效。

---

## 代码组织与模块职责

### 模块依赖关系

```
main.cpp
  ├── renderer.hpp/cpp    — 所有 SFML 绘制 + 碰撞检测（全部界面）
  ├── game_state.hpp/cpp  — 核心规则引擎：牌型识别、大小比较、AI、技能、调度、印记
  │     ├── card.hpp/cpp  — 卡牌数据模型、创建牌组、图片索引映射
  │     ├── skill.hpp/cpp — 技能定义 (3个) + SkillBuffs 结构体
  │     └── ai_memory.hpp/cpp — k-NN 学习：记录玩家决策，引导 AI 模仿
  ├── run_state.hpp/cpp   — Run 级状态：关卡数、已获得技能、装备槽、镜像快照
  │     ├── character.hpp/cpp — 3 个角色定义
  │     └── skill.hpp/cpp
```

### `card` 模块

- 定义 `Suit`（花色：`Spade`, `Heart`, `Club`, `Diamond`, `None`）、`Rank`（点数 `Three`~`BigJoker`）枚举，含大小王。
- `Card` 结构体含 `suit`、`rank`、`imageIndex`，重载 `==` 以支持卡牌比较。排序由 `sortByDZ()` 自定义 lambda 按斗地主牌力顺序（3 < 4 < ... < A < 2 < 小王 < 大王）处理。
- 硬编码 `CARD_MAP[54]` 数组定义图片索引到 `(suit, rank)` 的映射关系：
  - 0~3: 黑桃/红桃/梅花/方片 3
  - 4~7: 4 ...依此类推...
  - 48~51: 2
  - 52~53: 小王、大王

### `skill` 模块

- `SKILL_COUNT = 3`，`MAX_SKILL_SLOTS = 3`
- `SkillDef`：技能定义结构体（id、`std::wstring` 名称/描述）
- `SkillBuffs`：回合生效的技能效果结构体（`straightExtended` 降低顺子最小长度；`jokerWill` 使小王免疫炸弹；`wildcardRank` 癞子点数）
- **全部技能为 `SkillType::PASSIVE`** — 装备即生效，无需手动激活
- SkillBuffs 字段在 `setPlayerSkillSlots()` 中一次设置，跨回合持久。`endPlayerTurnCleanup()` 清除后重新应用
- `enemyActivateSkills()` 是空操作；敌人 buff 在 `setEnemySkills()` 中一次设置

### `character` 模块

- `CharacterDef`：角色定义（id、`std::wstring` 名称/被动技名称/描述、`extraCards` 额外手牌数）
- `CHAR_COUNT = 3`

三个角色：

| ID | 名称 | 被动技 | 效果 |
|----|------|--------|------|
| 0 | 炸弹收藏家 | 收藏 | +1手牌(16张)；场上每炸一次+1印记，3印记自动生成1个炸弹 |
| 1 | 谋略家 | 谋定 | 开局选择癞子点数(3~2)，该点数可替代任何缺失牌 |
| 2 | 掌控者 | 调度 | +1手牌(16张)；开局/每2回合可弃至多3张手牌换等量牌 |

### `run_state` 模块

- `RunState`：管理一次Run的全局状态
- 包含当前关卡数、角色id、已获得技能列表、已装备技能槽（3个，`-1`表示空）
- `rollRewardSkills()`：从3个技能中随机选出3个作为过关奖励
- `mirroredSkills()`：返回当前已装备技能，供下一关敌人继承
- `startNewRun()` / `advanceToNextLevel()`：Run生命周期管理

### `game_state` 模块

- `HandType` / `HandPattern`：11种牌型定义（`None`, `Single`, `Pair`, `Triple`, `TriplePlusOne`, `TriplePlusTwo`, `Straight`, `ConsecutivePairs`, `Airplane`, `Bomb`, `Rocket`）
- `Phase` 枚举：`PlayerTurn, ComputerTurn, SchedulePlay, MomentumPlay, PlayerWins, ComputerWins`
- **SchedulePlay**: 掌控者「调度」— 选择至多3张手牌弃掉换牌（开局可用，冷却2回合）
- **MomentumPlay**: 连击之势触发（连续2回合不出牌）— 选1张牌免费打出
- `classifyHand()` 静态方法：识别牌型，支持 `SkillBuffs` 规则修正（顺子缩短、癞子替换）
- `classifyHandNoWild()`：忽略癞子的变体，用于 `findBeatingPlays()` 枚举合法出牌
- `beats()` 比较两手牌大小
- `recordBombPlayed()`：炸弹印记累积，满3生成随机炸弹
- `scheduleDiscard()`/`scheduleSkip()`：掌控者调度操作
- AI决策：`computerTakeTurn()` — 跟牌时 `findBeatingPlays()` + 成本启发式 + k-NN；新一轮 `findLowestPlay()` 优先多牌组合

### `ai_memory` 模块

- k-NN (k=5) 出牌学习：`(PlayFeatures, PlayAction)` 对，最多2000条记录
- 技能使用学习：`querySkillProb(skillId)`，阈值 0.4
- 生命周期：`AIMemory` 由 `main.cpp` 持有，选角/结束/返回菜单时清空

### `renderer` 模块

- 一次性加载54张牌面、牌背（`card00.png`）、角色选择立绘（`char0/1/2.png`）、对战立绘（`char_0/1/_2.png`）、背景图、字体
- **动态比例布局**：所有坐标按窗口宽高比例计算，默认 `1200×750`
- 扇形手牌，选中上浮 `h*0.05`
- 所有界面 `draw*` / `hit*` 函数对
- 角色选择：三张 `char0/1/2.png` 全幅图片 + 悬停说明面板（白底黑边）
- 对战界面：右下角角色立绘（白底青蓝边框）+ 炸弹印记（黑底+星标+红色斜条纹闪光）
- 掌控者调度动画：上方中央入场缩放 → smoothstep飞向右下角
- 设置弹窗：半透明遮罩 + 切角面板，音乐/音效滑块 + "放弃本轮"/关闭
- 齿轮按钮：所有界面左上角统一齿轮图标（除主菜单），打开设置

### `main` 模块

- `Screen` 枚举：`MainMenu → CharacterSelect → WildcardSelect(仅谋略家) → Reward → Transition → Game → ... → GameOver`
- `Settings` 可从所有界面（除主菜单和设置自身）通过左上角齿轮按钮打开
- 主循环三阶段：事件处理 → 更新（AI计时、胜负判定、阶段切换）→ 渲染
- 全局快捷键：`R` 重开发牌，`F11` 全屏切换

---

## 代码风格规范

- **头文件保护**：`#pragma once`
- **枚举**：`enum class`（强类型）
- **常量**：`constexpr` 编译期常量
- **成员变量前缀**：私有成员以 `m_` 开头
- **工具方法**：不依赖实例状态的函数声明为 `static`
- **内部辅助**：`.cpp` 文件中的模块私有函数放入匿名命名空间
- **可选值**：`std::optional` 表示可能不存在的返回值
- **智能指针**：`std::unique_ptr` 管理 SFML 对象
- **宽字符串**：所有中文UI文本使用 `std::wstring` / `L"..."`
- **注释**：代码注释和文档使用**中文**

---

## 测试策略

- 无自动化测试框架
- 手工验证：构建 → 启动 → 遍历各界面 → 出牌/过牌/技能/胜负
- 关键验证点：牌型识别、大小比较、技能效果、镜像机制、AI学习、动画流畅性、窗口缩放

---

## 相关文档索引

| 文件 | 内容 |
|------|------|
| `CLAUDE.md` | Claude Code 专用速查：构建命令、架构速览、关键常量 |
| `AGENTS.md` | 本文件：面向 AI 编码代理的项目总览 |
| `GAMEPLAY.md` | 面向玩家的完整玩法说明：规则、角色、技能、操作 |
| `画面风格.md` | 美术风格定义：美式复古波普+街头涂鸦 |
| `valiant-spinning-floyd.md` | 原始策划文档：30技能规划、敌人AI设计、局外成长方案 |
