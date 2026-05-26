#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <optional>
#include <algorithm>
#include <cstdio>
#include <string>

#include "card.hpp"
#include "game_state.hpp"
#include "renderer.hpp"
#include "skill.hpp"
#include "character.hpp"
#include "run_state.hpp"

enum class Screen { MainMenu, CharacterSelect, Transition, Game, Reward, GameOver };

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
    int selectedAcquiredIdx = -1;

    // ---- 游戏阶段状态 ----
    bool phaseHandled = false;
    std::vector<int> rewardSkills; // 当前奖励界面的3个技能

    // ---- 动画帧时间 ----
    sf::Clock animClock;

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

            if (const auto* btn = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (btn->button != sf::Mouse::Button::Left) continue;

                // ========== 主菜单 ==========
                if (screen == Screen::MainMenu) {
                    sf::Vector2f pos = window.mapPixelToCoords(btn->position);
                    int hit = renderer.hitMainMenu(pos, winSize);
                    if (hit == 1) {
                        // 开始游戏: 如果已选过角色则直接开始, 否则先选角色
                        if (run.currentCharId() >= 0) {
                            run.startNewRun(run.currentCharId());
                            rewardSkills = run.rollRewardSkills();
                            screen = Screen::Reward;
                        } else {
                            screen = Screen::CharacterSelect;
                        }
                    } else if (hit == 2) {
                        screen = Screen::CharacterSelect;
                    } else if (hit == 3) {
                        window.close();
                        break;
                    }
                }

                // ========== 角色选择 ==========
                else if (screen == Screen::CharacterSelect) {
                    sf::Vector2f pos = window.mapPixelToCoords(btn->position);
                    int hit = renderer.hitCharacterSelect(pos, winSize);
                    if (hit >= 1 && hit <= 3) {
                        run.startNewRun(hit - 1);
                        rewardSkills = run.rollRewardSkills();
                        screen = Screen::Reward;
                    } else if (hit == 9) {
                        screen = Screen::MainMenu;
                    }
                }

                // ========== 关卡过渡 ==========
                else if (screen == Screen::Transition) {
                    sf::Vector2f pos = window.mapPixelToCoords(btn->position);

                    int skHit = renderer.hitTransitionSkill(pos, winSize,
                        (int)run.acquiredSkills().size(), run.equippedCount());
                    int slHit = renderer.hitTransitionSlot(pos, winSize);
                    int fightHit = renderer.hitTransitionFight(pos, winSize);

                    if (slHit >= 0) {
                        // 点击装备槽: 若已有选中的技能则装备
                        if (selectedAcquiredIdx >= 0) {
                            int skillId = run.acquiredSkills()[selectedAcquiredIdx];
                            run.equipSkill(slHit, skillId);
                            selectedAcquiredIdx = -1;
                        }
                        // 若装备槽已有技能, 卸下
                        else if (run.equippedSkills()[slHit] >= 0) {
                            run.unequipSlot(slHit);
                        }
                    } else if (skHit >= 0) {
                        // 点击已获得技能: 选中 (准备放入槽位)
                        selectedAcquiredIdx = skHit;
                        // 自动放入第一个空槽
                        auto& eq = run.equippedSkills();
                        int skillId = run.acquiredSkills()[skHit];
                        // 检查是否已装备
                        bool alreadyEquipped = false;
                        for (int i = 0; i < MAX_SKILL_SLOTS; ++i)
                            if (eq[i] == skillId) { alreadyEquipped = true; break; }
                        if (!alreadyEquipped) {
                            for (int i = 0; i < MAX_SKILL_SLOTS; ++i) {
                                if (eq[i] < 0) {
                                    run.equipSkill(i, skillId);
                                    break;
                                }
                            }
                        }
                        selectedAcquiredIdx = -1;
                    } else if (fightHit == 1) {
                        // 开始战斗
                        game.dealCards(run.extraCards());
                        if (run.currentLevel() == 1) {
                            game.setEnemySkills({-1, -1, -1});
                        } else {
                            game.setEnemySkills(run.mirroredSkills());
                        }
                        game.setEnergyPerTurn(1 + run.extraEnergy());
                        selectedIndices.clear();
                        canPlaySelected = false;
                        aiTriggered = false;
                        aiClock.restart();
                        renderer.startDealAnimation((int)game.playerHand().size());
                        screen = Screen::Game;
                    } else if (fightHit == 9) {
                        screen = Screen::MainMenu;
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
                    }
                }

                // ========== 失败界面 ==========
                else if (screen == Screen::GameOver) {
                    sf::Vector2f pos = window.mapPixelToCoords(btn->position);
                    int hit = renderer.hitGameOver(pos, winSize);
                    if (hit == 1) {
                        screen = Screen::MainMenu;
                    }
                }
            }

            // ========== 游戏界面事件 ==========
            if (screen == Screen::Game) {
                if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                    if (key->code == sf::Keyboard::Key::R) {
                        game.dealCards(run.extraCards());
                        if (run.currentLevel() == 1)
                            game.setEnemySkills({-1, -1, -1});
                        else
                            game.setEnemySkills(run.mirroredSkills());
                        game.setEnergyPerTurn(1 + run.extraEnergy());
                        selectedIndices.clear();
                        canPlaySelected = false;
                        aiClock.restart();
                        aiTriggered = false;
                        renderer.startDealAnimation((int)game.playerHand().size());
                    }
                }

                // 返回按钮 / 调试按钮
                if (const auto* btn2 = event->getIf<sf::Event::MouseButtonPressed>()) {
                    if (btn2->button == sf::Mouse::Button::Left) {
                        sf::Vector2f pos2 = window.mapPixelToCoords(btn2->position);
                        int retHit = renderer.hitTestGameButton(pos2, !game.isNewRound(), winSize);
                        if (retHit == 3) {
                            screen = Screen::MainMenu;
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

                        if (skHit >= 0) {
                            // 激活技能
                            int skillId = run.equippedSkills()[skHit];
                            if (skillId >= 0) {
                                game.activatePlayerSkill(skillId, skHit);
                            }
                        } else if (btnHit == 1 && canPlaySelected) {
                            auto sorted = selectedIndices;
                            std::sort(sorted.begin(), sorted.end());
                            if (game.playerPlay(sorted)) {
                                selectedIndices.clear();
                                canPlaySelected = false;
                                if (game.phase() == GameState::Phase::ComputerTurn) {
                                    aiClock.restart();
                                    aiTriggered = false;
                                }
                            }
                        } else if (btnHit == 2) {
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
            }
        } // end pollEvent

        // ========== 更新 ==========
        if (screen == Screen::Game) {
            auto sorted = selectedIndices;
            std::sort(sorted.begin(), sorted.end());
            canPlaySelected = game.canPlay(sorted);

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

            // 胜利/失败转换
            if (game.phase() == GameState::Phase::PlayerWins && !phaseHandled) {
                phaseHandled = true;
                if ((int)run.acquiredSkills().size() >= SKILL_COUNT) {
                    run.advanceToNextLevel();
                    screen = Screen::Transition;
                } else {
                    rewardSkills = run.rollRewardSkills();
                    screen = Screen::Reward;
                }
            }
            if (game.phase() == GameState::Phase::ComputerWins && !phaseHandled) {
                phaseHandled = true;
                screen = Screen::GameOver;
            }
            if (game.phase() != GameState::Phase::PlayerWins
                && game.phase() != GameState::Phase::ComputerWins)
                phaseHandled = false;
        }

        // ---------- 更新角色卡片悬停动画 ----------
        float dt = animClock.restart().asSeconds();
        renderer.updateAnimations(dt);

        // ========== 渲染 ==========
        winSize = window.getSize();

        switch (screen) {
        case Screen::MainMenu:
            renderer.drawMainMenu(winSize, mw);
            break;
        case Screen::CharacterSelect:
            renderer.drawCharacterSelect(winSize, mw);
            break;
        case Screen::Transition:
            renderer.drawTransition(winSize, mw,
                run.currentLevel(), run.acquiredSkills(),
                run.equippedSkills(), hoveredAcquiredIdx, hoveredSlotIdx);
            break;
        case Screen::Game:
            renderer.renderGame(game, selectedIndices, winSize,
                              canPlaySelected, run.equippedSkills(), mw, dt);
            break;
        case Screen::Reward:
            renderer.drawReward(winSize, mw, rewardSkills, run.acquiredSkills());
            break;
        case Screen::GameOver:
            renderer.drawGameOver(winSize, mw,
                run.currentLevel(), (int)run.acquiredSkills().size());
            break;
        }
        window.display();
    }

    return 0;
}
