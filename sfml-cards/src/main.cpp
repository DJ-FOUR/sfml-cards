#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <optional>
#include <algorithm>
#include <cstdio>
#include <string>

#include "game_state.hpp"
#include "renderer.hpp"
#include "skill.hpp"
#include "run_state.hpp"
#include "ai_memory.hpp"

enum class Screen { MainMenu, CharacterSelect, WildcardSelect, Transition, Game, Reward, GameOver, Settings };

int main()
{
    auto mode = sf::VideoMode({Renderer::DEFAULT_W, Renderer::DEFAULT_H});
    sf::RenderWindow window(mode, L"斗牌Rogue",
                            sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize,
                            sf::State::Windowed);
    window.setFramerateLimit(60);

    Renderer renderer(window);
    if (!renderer.initialize("images/card", "resources/simsun.ttc")) {
        std::fprintf(stderr, "Renderer init failed\n");
        return 1;
    }

    // ---- 游戏状态 ----
    GameState game;
    RunState  run;
    run.loadHighScore();
    AIMemory  aiMemory;
    std::vector<int> selectedIndices;
    sf::Clock aiClock;
    bool aiTriggered = false;
    bool canPlaySelected = false;

    // ---- 窗口状态 ----
    Screen screen = Screen::MainMenu;
    bool fullscreen = false;
    sf::Vector2u windowedSize = {Renderer::DEFAULT_W, Renderer::DEFAULT_H};

    // ---- 过渡界面状态 ----
    int hoveredAcquiredIdx = -1;
    int hoveredSlotIdx = -1;

    // ---- 设置弹窗状态 ----
    Screen previousScreen = Screen::MainMenu;
    bool   draggingMusicSlider = false;
    bool   draggingSoundSlider = false;

    // ---- 拖拽状态 ----
    int  dragSourceType  = 0;    // 0=无, 1=卡池, 2=装备槽
    int  dragSourceIndex = -1;
    int  dragSkillId     = -1;
    bool dragActive      = false;
    float dragStartX     = 0.f;
    float dragStartY     = 0.f;
    constexpr float DRAG_THRESHOLD = 5.0f;

    // ---- 过渡界面双击 ----
    sf::Clock dblClickClock;
    int  lastClickedPoolIdx = -1;
    int  lastClickedSlotIdx = -1;
    constexpr float DBL_CLICK_INTERVAL = 0.35f;
    bool pendingEquip = false;   // 飞牌完成后执行装备
    bool pendingUnequip = false;
    int  pendingEquipSid = -1;
    int  pendingEquipSlot = -1;
    int  pendingUnequipSlot = -1;

    // ---- 游戏阶段状态 ----
    bool phaseHandled = false;
    bool scoreAnimTriggered = false;
    std::vector<int> rewardSkills; // 当前奖励界面的3个技能
    bool skillToggled[MAX_SKILL_SLOTS] = {}; // 技能槽toggle状态

    // ---- 动画帧时间 ----
    sf::Clock animClock;

    auto resetAndDealGame = [&]() {
        game.setPlayerWildcard(run.wildcardRank());
        game.setPlayerIsBombCollector(run.currentCharId() == 0);
        game.setPlayerIsScheduler(run.currentCharId() == 2);
        game.dealCards(run.extraCards());
        game.setPlayerSkillSlots(run.equippedSkills());
        if (run.currentLevel() == 1)
            game.setEnemySkills({-1, -1, -1});
        else
            game.setEnemySkills(run.mirroredSkills());
        game.setAIMemory(&aiMemory);
        selectedIndices.clear();
        canPlaySelected = false;
        aiTriggered = false;
        aiClock.restart();
        renderer.resetSkillSlotAnims();
        renderer.closeCharTooltip();
        for (auto& t : skillToggled) t = false;
        phaseHandled = false;
        scoreAnimTriggered = false;
        renderer.startDealAnimation((int)game.playerHand().size());
    };

    while (window.isOpen())
    {
        sf::Vector2u winSize = window.getSize();
        sf::Vector2i mp = sf::Mouse::getPosition(window);
        sf::Vector2f mw = window.mapPixelToCoords(mp);

        // ========== 事件处理 ==========
        while (const std::optional event = window.pollEvent())
        {
            // --- 全局事件 ---
            if (event->is<sf::Event::Closed>()) {
                window.close();
                break;
            }

            if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                window.setView(sf::View(sf::FloatRect(
                    {0.f, 0.f}, {(float)resized->size.x, (float)resized->size.y})));
                if (!fullscreen)
                    windowedSize = resized->size;
            }

            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::F11) {
                    fullscreen = !fullscreen;
                    if (fullscreen) {
                        auto desktop = sf::VideoMode::getDesktopMode();
                        window.create(desktop, L"斗牌Rogue", sf::State::Fullscreen);
                    } else {
                        window.create(sf::VideoMode(windowedSize), L"斗牌Rogue",
                                      sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize,
                                      sf::State::Windowed);
                    }
                    window.setFramerateLimit(60);
                    if (!renderer.initialize("images/card", "resources/simsun.ttc")) {
                        std::fprintf(stderr, "Renderer reinit failed\n");
                        return 1;
                    }
                    continue;
                }
            }

            // --- 鼠标移动 (设置界面滑块拖拽) ---
            if (const auto* mm = event->getIf<sf::Event::MouseMoved>()) {
                sf::Vector2f mpos = window.mapPixelToCoords(mm->position);
                if (screen == Screen::Settings) {
                    if (draggingMusicSlider) {
                        auto hit = renderer.hitTestSettings(mpos, winSize);
                        if (hit.action == 3) renderer.setMusicVolume(hit.sliderVal);
                    }
                    if (draggingSoundSlider) {
                        auto hit = renderer.hitTestSettings(mpos, winSize);
                        if (hit.action == 4) renderer.setSoundVolume(hit.sliderVal);
                    }
                }
            }

            if (const auto* btn = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (btn->button != sf::Mouse::Button::Left) continue;

                // ========== 设置弹窗 ==========
                if (screen == Screen::Settings) {
                    sf::Vector2f pos = window.mapPixelToCoords(btn->position);
                    auto hit = renderer.hitTestSettings(pos, winSize);
                    if (hit.action == 1) {
                        screen = previousScreen;
                    } else if (hit.action == 2) {
                        run.updateHighScore();
                        game = GameState{};
                        aiMemory.clear();
                        screen = Screen::MainMenu;
                    } else if (hit.action == 3) {
                        draggingMusicSlider = true;
                        renderer.setMusicVolume(hit.sliderVal);
                    } else if (hit.action == 4) {
                        draggingSoundSlider = true;
                        renderer.setSoundVolume(hit.sliderVal);
                    }
                    continue;
                }

                // ========== 主菜单 ==========
                if (screen == Screen::MainMenu) {
                    sf::Vector2f pos = window.mapPixelToCoords(btn->position);
                    int setHit = renderer.hitTestSettingsButton(pos, winSize);
                    if (setHit == 1) {
                        previousScreen = Screen::MainMenu;
                        screen = Screen::Settings;
                        continue;
                    }
                    int hit = renderer.hitMainMenu(pos, winSize);
                    if (hit == 1) {
                        screen = Screen::CharacterSelect;
                    } else if (hit == 2) {
                        window.close();
                        break;
                    }
                }

                // ========== 角色选择 ==========
                else if (screen == Screen::CharacterSelect) {
                    sf::Vector2f pos = window.mapPixelToCoords(btn->position);
                    int hit = renderer.hitCharacterSelect(pos, winSize);
                    if (hit >= 1 && hit <= 3) {
                        aiMemory.clear();
                        run.startNewRun(hit - 1);
                        if (hit == 2) {
                            screen = Screen::WildcardSelect;
                        } else {
                            rewardSkills = run.rollRewardSkills();
                            screen = rewardSkills.empty() ? Screen::Transition : Screen::Reward;
                        }
                    } else if (hit == 9) {
                        previousScreen = Screen::CharacterSelect;
                        screen = Screen::Settings;
                    }
                }

                // ========== 癞子选择 ==========
                else if (screen == Screen::WildcardSelect) {
                    sf::Vector2f pos = window.mapPixelToCoords(btn->position);
                    int setHit = renderer.hitTestSettingsButton(pos, winSize);
                    if (setHit == 1) {
                        previousScreen = Screen::WildcardSelect;
                        screen = Screen::Settings;
                        continue;
                    }
                    int hit = renderer.hitWildcardSelect(pos, winSize);
                    if (hit >= 0 && hit <= 12) {
                        run.setWildcardRank(hit);
                        rewardSkills = run.rollRewardSkills();
                        screen = rewardSkills.empty() ? Screen::Transition : Screen::Reward;
                    }
                }

                // ========== 关卡过渡 ==========
                else if (screen == Screen::Transition) {
                    sf::Vector2f pos = window.mapPixelToCoords(btn->position);

                    int poolHit = renderer.hitTransitionPoolCard(pos, winSize,
                        (int)run.acquiredSkills().size());
                    int slHit = renderer.hitTransitionSlot(pos, winSize);
                    int fightHit = renderer.hitTransitionFight(pos, winSize);

                    if (poolHit >= 0) {
                        // 按下卡池卡片 → 已装备的技能不响应
                        int sid = run.acquiredSkills()[poolHit];
                        if (run.equippedSlotOf(sid) < 0) {
                            float elapsed = dblClickClock.getElapsedTime().asSeconds();
                            if (poolHit == lastClickedPoolIdx && elapsed < DBL_CLICK_INTERVAL) {
                                // 双击 → 飞入装备槽
                                int targetSlot = -1;
                                for (int s = 0; s < MAX_SKILL_SLOTS; ++s)
                                    if (run.equippedSkills()[s] < 0) { targetSlot = s; break; }
                                if (targetSlot >= 0) {
                                    sf::Vector2f src = {mw.x, mw.y}; // 飞牌起始位置用点击位置
                                    sf::Vector2f dst = renderer.transitionSlotCenter(targetSlot, winSize);
                                    renderer.startTransitionFly(src, dst, sid, true, poolHit, -1);
                                    pendingEquip = true;
                                    pendingEquipSid = sid;
                                    pendingEquipSlot = targetSlot;
                                }
                                lastClickedPoolIdx = -1;
                                dblClickClock.restart();
                            } else {
                                // 单击 → 准备拖拽
                                dblClickClock.restart();
                                lastClickedPoolIdx = poolHit;
                                dragSourceType = 1;
                                dragSourceIndex = poolHit;
                                dragSkillId = sid;
                                dragActive = false;
                                dragStartX = mw.x;
                                dragStartY = mw.y;
                            }
                        }
                    } else if (slHit >= 0 && run.equippedSkills()[slHit] >= 0) {
                        float elapsed = dblClickClock.getElapsedTime().asSeconds();
                        if (slHit == lastClickedSlotIdx && elapsed < DBL_CLICK_INTERVAL) {
                            // 双击 → 飞回卡池
                            int sid = run.equippedSkills()[slHit];
                            sf::Vector2f dst = {mw.x, mw.y}; // 目标用点击位置
                            sf::Vector2f src = renderer.transitionSlotCenter(slHit, winSize);
                            renderer.startTransitionFly(src, dst, sid, false, -1, slHit);
                            pendingUnequip = true;
                            pendingUnequipSlot = slHit;
                            lastClickedSlotIdx = -1;
                            dblClickClock.restart();
                        } else {
                            // 单击 → 准备拖拽
                            dblClickClock.restart();
                            lastClickedSlotIdx = slHit;
                            dragSourceType = 2;
                            dragSourceIndex = slHit;
                            dragSkillId = run.equippedSkills()[slHit];
                            dragActive = false;
                            dragStartX = mw.x;
                            dragStartY = mw.y;
                        }
                    } else if (fightHit == 1) {
                        // 开始战斗
                        resetAndDealGame();
                        screen = Screen::Game;
                    } else if (fightHit == 9) {
                        previousScreen = Screen::Transition;
                        screen = Screen::Settings;
                    }
                }

                // ========== 奖励界面 ==========
                else if (screen == Screen::Reward) {
                    sf::Vector2f pos = window.mapPixelToCoords(btn->position);
                    int hit = renderer.hitReward(pos, winSize);
                    if (hit >= 0 && hit < (int)rewardSkills.size()) {
                        int chosenSkill = rewardSkills[hit];
                        run.addSkill(chosenSkill);
                        bool initialPick = (run.currentLevel() == 1
                                         && run.acquiredSkills().size() == 1);
                        if (initialPick) {
                            run.equipSkill(0, chosenSkill);
                        } else {
                            run.advanceToNextLevel();
                        }
                        screen = Screen::Transition;
                    } else if (hit == 9) {
                        previousScreen = Screen::Reward;
                        screen = Screen::Settings;
                    }
                }

                // ========== 失败界面 ==========
                else if (screen == Screen::GameOver) {
                    sf::Vector2f pos = window.mapPixelToCoords(btn->position);
                    int setHit = renderer.hitTestSettingsButton(pos, winSize);
                    if (setHit == 1) {
                        previousScreen = Screen::GameOver;
                        screen = Screen::Settings;
                        continue;
                    }
                    int hit = renderer.hitGameOver(pos, winSize);
                    if (hit == 1) {
                        run.updateHighScore();
                        game = GameState{};  // 重置全部状态（含炸弹印记）
                        screen = Screen::MainMenu;
                    }
                }
            }

            // ========== 鼠标释放: 拖拽结束 ==========
            if (const auto* rel = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (rel->button == sf::Mouse::Button::Left
                    && screen == Screen::Transition
                    && dragSourceType != 0)
                {
                    sf::Vector2f pos = window.mapPixelToCoords(rel->position);

                    int poolHit = renderer.hitTransitionPoolCard(pos, winSize,
                        (int)run.acquiredSkills().size());
                    int slHit = renderer.hitTransitionSlot(pos, winSize);
                    bool poolArea = renderer.hitTransitionPool(pos, winSize);

                    if (dragActive) {
                        // ---- 拖拽完成 ----
                        if (dragSourceType == 1) {
                            // 从卡池拖出 → 放入槽位
                            if (slHit >= 0) {
                                int oldSkill = run.equippedSkills()[slHit];
                                if (oldSkill >= 0 && oldSkill != dragSkillId)
                                    run.unequipSlot(slHit);
                                run.equipSkill(slHit, dragSkillId);
                            }
                        } else if (dragSourceType == 2) {
                            // 从槽位拖出
                            if (slHit >= 0 && slHit != dragSourceIndex) {
                                // 放入不同槽 → 交换
                                run.swapSlots(dragSourceIndex, slHit);
                            } else if (poolArea || poolHit >= 0) {
                                // 放入卡池区域 → 卸载
                                run.unequipSlot(dragSourceIndex);
                            }
                        }
                    }
                    // 点击（未拖拽）不处理: 装备/卸载仅通过拖拽完成

                    // 清除拖拽状态
                    dragSourceType = 0;
                    dragSourceIndex = -1;
                    dragSkillId = -1;
                    dragActive = false;
                }
            }

            // ========== 游戏界面事件 ==========
            if (screen == Screen::Game) {
                if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                    if (key->code == sf::Keyboard::Key::R) {
                        resetAndDealGame();
                    }
                }

                // 胜利积分动画点击继续
                if (scoreAnimTriggered) {
                    if (const auto* scBtn = event->getIf<sf::Event::MouseButtonPressed>()) {
                        if (scBtn->button == sf::Mouse::Button::Left)
                            renderer.advanceScoreAnim();
                    }
                }
                // 返回按钮 / 调试按钮
                if (const auto* btn2 = event->getIf<sf::Event::MouseButtonPressed>()) {
                    if (btn2->button == sf::Mouse::Button::Left) {
                        sf::Vector2f pos2 = window.mapPixelToCoords(btn2->position);
                        int retHit = renderer.hitTestGameButton(pos2, !game.isNewRound(), winSize);
                        if (retHit == 3) {
                            aiMemory.clear();
                            game = GameState{};
                            screen = Screen::MainMenu;
                            continue;
                        }
                        int setHit = renderer.hitTestSettingsButton(pos2, winSize);
                        if (setHit == 1) {
                            previousScreen = Screen::Game;
                            screen = Screen::Settings;
                            continue;
                        }
                        int dbgHit = renderer.hitTestDebugButton(pos2, winSize);
                        if (dbgHit == 1) {
                            game.forceWin();
                            continue;
                        }
                        if (dbgHit == 2) {
                            game.forceLose();
                            continue;
                        }
                    }
                }

                if (game.phase() == GameState::Phase::PlayerTurn) {
                    if (const auto* btn3 = event->getIf<sf::Event::MouseButtonPressed>()) {
                        if (btn3->button != sf::Mouse::Button::Left) continue;

                        sf::Vector2f pos3 = window.mapPixelToCoords(btn3->position);
                        bool canPass = !game.isNewRound();

                        int btnHit = renderer.hitTestGameButton(pos3, canPass, winSize);
                        int skHit = renderer.hitTestSkillSlot(pos3, winSize);
                        int chHit = renderer.hitTestCharPortrait(pos3, winSize, run.currentCharId(), game);

                        if (chHit == 1) {
                            renderer.toggleCharTooltip();
                        } else if (skHit >= 0) {
                            renderer.closeCharTooltip();
                            // 所有技能均为被动，装备即生效，无需点击
                        } else if (btnHit == 1 && canPlaySelected) {
                            renderer.closeCharTooltip();
                            auto sorted = selectedIndices;
                            std::sort(sorted.begin(), sorted.end());
                            if (game.playerPlay(sorted)) {
                                selectedIndices.clear();
                                canPlaySelected = false;
                                renderer.resetSkillSlotAnims();
                                for (auto& t : skillToggled) t = false;
                                if (game.phase() == GameState::Phase::ComputerTurn) {
                                    aiClock.restart();
                                    aiTriggered = false;
                                }
                            }
                        } else if (btnHit == 2) {
                            renderer.closeCharTooltip();
                            // 玩家不出
                            game.playerPass();
                            selectedIndices.clear();
                            canPlaySelected = false;
                            aiClock.restart();
                            aiTriggered = false;
                        } else if (btnHit == 0) {
                            // 点牌
                            int idx = renderer.hitTestCard(pos3,
                                (int)game.playerHand().size(), winSize, selectedIndices);
                            if (idx >= 0) {
                                renderer.closeCharTooltip();
                                auto it = std::find(selectedIndices.begin(),
                                                    selectedIndices.end(), idx);
                                if (it != selectedIndices.end())
                                    selectedIndices.erase(it);
                                else
                                    selectedIndices.push_back(idx);
                            }
                        }
                    }
                }

                // ---- 掌控者「调度」: 弃牌换牌 ----
                if (game.phase() == GameState::Phase::SchedulePlay) {
                    if (const auto* btnS = event->getIf<sf::Event::MouseButtonPressed>()) {
                        if (btnS->button != sf::Mouse::Button::Left) continue;
                        sf::Vector2f posS = window.mapPixelToCoords(btnS->position);
                        int sHit = renderer.hitTestScheduleButton(posS, winSize);
                        if (sHit == 1) {
                            // 过牌 — 弃选中牌并补牌
                            auto sorted = selectedIndices;
                            game.scheduleDiscard(sorted);
                            selectedIndices.clear();
                            canPlaySelected = false;
                            aiClock.restart();
                            aiTriggered = false;
                        } else if (sHit == 2) {
                            // 跳过
                            game.scheduleSkip();
                            selectedIndices.clear();
                            canPlaySelected = false;
                        } else {
                            // 点牌选弃牌 (最多3张)
                            int idx = renderer.hitTestCard(posS,
                                (int)game.playerHand().size(), winSize, selectedIndices);
                            if (idx >= 0) {
                                auto it = std::find(selectedIndices.begin(),
                                                    selectedIndices.end(), idx);
                                if (it != selectedIndices.end())
                                    selectedIndices.erase(it);
                                else if ((int)selectedIndices.size() < 3)
                                    selectedIndices.push_back(idx);
                            }
                        }
                    }
                }

                // ---- 连击之势: 选1张牌打出 ----
                if (game.phase() == GameState::Phase::MomentumPlay) {
                    if (const auto* btn4 = event->getIf<sf::Event::MouseButtonPressed>()) {
                        if (btn4->button != sf::Mouse::Button::Left) continue;

                        sf::Vector2f pos4 = window.mapPixelToCoords(btn4->position);

                        int btnHit = renderer.hitTestMomentumButton(pos4, winSize);
                        if (btnHit == 1 && selectedIndices.size() == 1) {
                            // 打出选中的1张牌
                            if (game.momentumPlay(selectedIndices[0])) {
                                selectedIndices.clear();
                                canPlaySelected = false;
                            }
                        } else if (btnHit == 0) {
                            // 点牌 — 只能选1张 (替换式)
                            int idx = renderer.hitTestCard(pos4,
                                (int)game.playerHand().size(), winSize, selectedIndices);
                            if (idx >= 0) {
                                selectedIndices.clear();
                                selectedIndices.push_back(idx);
                            }
                        }
                    }
                }
            }

            // --- 鼠标释放 (停止滑块拖拽) ---
            if (const auto* rel = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (rel->button == sf::Mouse::Button::Left) {
                    draggingMusicSlider = false;
                    draggingSoundSlider = false;
                }
            }
        } // end pollEvent

        // ========== 更新 ==========

        // 拖拽阈值检测
        if (dragSourceType != 0 && !dragActive) {
            float dx = mw.x - dragStartX;
            float dy = mw.y - dragStartY;
            if (dx * dx + dy * dy > DRAG_THRESHOLD * DRAG_THRESHOLD)
                dragActive = true;
        }

        // 过渡界面飞牌完成 → 执行装备/卸下
        if (renderer.isTransitionFlyDone()) {
            int flySid = renderer.transitionFlySkillId();
            bool toSlot = renderer.transitionFlyToSlot();
            if (toSlot && pendingEquip) {
                run.equipSkill(pendingEquipSlot, pendingEquipSid);
                pendingEquip = false;
            } else if (!toSlot && pendingUnequip) {
                run.unequipSlot(pendingUnequipSlot);
                pendingUnequip = false;
            }
        }

        if (screen == Screen::Game) {
            // 掌控者调度: 检查是否应进入调度阶段 (发牌动画未结束时不触发)
            if (game.phase() == GameState::Phase::PlayerTurn && !renderer.isDealAnimating())
                game.activateScheduleIfReady();

            auto sorted = selectedIndices;
            std::sort(sorted.begin(), sorted.end());
            canPlaySelected = game.canPlay(sorted);

            // 连击之势: 选1张牌即可出牌
            if (game.phase() == GameState::Phase::MomentumPlay
                && selectedIndices.size() == 1)
                canPlaySelected = true;

            if (game.phase() == GameState::Phase::ComputerTurn && !aiTriggered) {
                if (aiClock.getElapsedTime().asSeconds() > 0.8f) {
                    game.computerTakeTurn();
                    aiTriggered = true;

                    if (game.phase() == GameState::Phase::ComputerWins) {
                        // 失败 → 延迟后弹出失败界面
                        // 直接进入失败界面
                    }
                }
            }

            // 敌人连击之势: 延迟后自动打出1张牌
            if (game.phase() == GameState::Phase::MomentumPlay
                && game.isMomentumEnemy() && !aiTriggered) {
                if (aiClock.getElapsedTime().asSeconds() > 0.8f) {
                    game.enemyMomentumPlay();
                    // 重置时钟和触发标记，让后续ComputerTurn自然触发
                    aiClock.restart();
                    aiTriggered = false;
                }
            }

            // 胜利/失败转换
            if (game.phase() == GameState::Phase::PlayerWins && !phaseHandled) {
                phaseHandled = true;
                int score = game.calculateScore();
                run.addScore(score);
                auto enemyCards = game.computerHand();  // 捕获敌人剩余手牌
                renderer.startScoreAnim(enemyCards, score);
                scoreAnimTriggered = true;
            }
            if (scoreAnimTriggered && !renderer.isScoreAnimating()) {
                scoreAnimTriggered = false;
                if ((int)run.acquiredSkills().size() >= SKILL_COUNT) {
                    run.advanceToNextLevel();
                    screen = Screen::Transition;
                } else {
                    rewardSkills = run.rollRewardSkills();
                    screen = rewardSkills.empty() ? Screen::Transition : Screen::Reward;
                }
            }
            if (game.phase() == GameState::Phase::ComputerWins && !phaseHandled) {
                phaseHandled = true;
                aiMemory.clear();
                screen = Screen::GameOver;
            }
            if (game.phase() != GameState::Phase::PlayerWins
                && game.phase() != GameState::Phase::ComputerWins)
                phaseHandled = false;
        }

        // ---------- 更新角色卡片悬停动画 ----------
        float dt = animClock.restart().asSeconds();
        renderer.updateAnimations(dt);
        game.tickBombFlash();

        // ---------- 过渡界面悬停检测 ----------
        if (screen == Screen::Transition) {
            hoveredAcquiredIdx = renderer.hitTransitionPoolCard(mw, winSize,
                (int)run.acquiredSkills().size());
            if (hoveredAcquiredIdx >= 0) {
                int sid = run.acquiredSkills()[hoveredAcquiredIdx];
                if (run.equippedSlotOf(sid) >= 0)
                    hoveredAcquiredIdx = -1;  // 已装备的技能在卡池中悬停不生效
            }
            hoveredSlotIdx = renderer.hitTransitionSlot(mw, winSize);
            if (dragActive) hoveredAcquiredIdx = -1;
        }

        // ========== 渲染 ==========
        winSize = window.getSize();

        switch (screen) {
        case Screen::MainMenu:
            renderer.drawMainMenu(winSize, mw);
            renderer.drawHighScore(winSize, run.highScore());
            break;
        case Screen::CharacterSelect:
            renderer.drawCharacterSelect(winSize, mw);
            renderer.drawHighScore(winSize, run.highScore());
            break;
        case Screen::WildcardSelect:
            renderer.drawWildcardSelect(winSize, mw);
            renderer.drawTotalScore(winSize, run.totalScore());
            break;
        case Screen::Transition:
            renderer.drawTransition(winSize, mw,
                run.currentLevel(), run.acquiredSkills(),
                run.equippedSkills(), run.mirroredSkills(),
                hoveredAcquiredIdx, hoveredSlotIdx,
                dragSourceType, dragSourceIndex, dragSkillId, dragActive);
            renderer.drawTotalScore(winSize, run.totalScore());
            break;
        case Screen::Game:
            renderer.renderGame(game, selectedIndices, winSize,
                              canPlaySelected, run.equippedSkills(), mw, dt,
                              run.currentCharId(), run.roundScore(), run.totalScore());
            break;
        case Screen::Reward:
            renderer.drawReward(winSize, mw, rewardSkills, run.acquiredSkills());
            renderer.drawTotalScore(winSize, run.totalScore());
            break;
        case Screen::GameOver:
            renderer.drawGameOver(winSize, mw,
                run.currentLevel(), (int)run.acquiredSkills().size());
            renderer.drawTotalScore(winSize, run.totalScore());
            break;
        case Screen::Settings:
            // 绘制底层游戏画面 (冻结)
            if (previousScreen == Screen::Game)
                renderer.renderGame(game, selectedIndices, winSize,
                                    canPlaySelected, run.equippedSkills(), mw, 0.f,
                                    run.currentCharId(), run.roundScore(), run.totalScore());
            renderer.drawSettingsPopup(winSize, mw, draggingMusicSlider, draggingSoundSlider);
            break;
        }
        window.display();
    }

    return 0;
}
