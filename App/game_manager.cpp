#include "game_manager.hpp"
#include "theme.hpp"
#include "ui_draw.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

GameManager::GameManager()
    : window(sf::VideoMode({Theme::WINDOW_W, Theme::WINDOW_H}), "Tower Defence: Supply Lines",
             sf::Style::Titlebar | sf::Style::Close),
      worldView(sf::FloatRect({0.f, 0.f}, {Theme::GRID_W, Theme::GRID_H})),
      assetManager(),
      grid(GRID_WIDTH, GRID_HEIGHT),
      pathfinder(&grid),
      enemyManager(&grid, &pathfinder, &assetManager),
      towerManager(&grid, &pathfinder, &enemyManager, &assetManager) {
    window.setFramerateLimit(60);

    worldView.setViewport(sf::FloatRect({0.f, Theme::TOP_BAR_H / Theme::WINDOW_H},
                                        {Theme::GRID_W / Theme::WINDOW_W, Theme::GRID_H / Theme::WINDOW_H}));

    assetManager.loadAllAssets();
    Ui::setFonts(assetManager.getFont("ui_regular"), assetManager.getFont("ui_bold"));
    hud.emplace(assetManager);
    towerManager.setFeedback(&effects, &sound);

    std::string error;
    if (supplyNetwork.load("../data/worked_example.txt", error)) {
        std::cout << "Supply map loaded: tree cost " << supplyNetwork.getPlanner().getResult().totalCost
                  << ", supply value " << supplyNetwork.getReport().packing.totalValue << std::endl;
    } else {
        std::cout << "Supply map not loaded (" << error << "); playing without it" << std::endl;
    }

    if (assetManager.hasTexture("grid_tile")) {
        grid.setTexture(assetManager.getTexture("grid_tile"));
    }
    if (assetManager.hasTexture("start_tile")) {
        grid.setStartTexture(assetManager.getTexture("start_tile"));
    }
    if (assetManager.hasTexture("end_tile")) {
        grid.setEndTexture(assetManager.getTexture("end_tile"));
    }

    grid.setStartEnd({0, GRID_HEIGHT / 2}, {GRID_WIDTH - 1, GRID_HEIGHT / 2});

    enemyManager.recalculatePaths();

    currentState = GameState::MENU;
}

void GameManager::run() {
    while (window.isOpen()) {
        float realDt = std::min(0.1f, clock.restart().asSeconds());
        handleInput();
        updateUi(realDt);

        bool simulate = currentState == GameState::PLAYING && !views.isOpen() && !helpOpen;
        if (simulate) {
            float timeScale = gameSpeed * (supplyOverlay.isRaidPlaying() ? RAID_SLOW_MOTION : 1.0f);
            accumulator += realDt * timeScale;
            while (accumulator >= FIXED_TIMESTEP) {
                update(FIXED_TIMESTEP);
                accumulator -= FIXED_TIMESTEP;
            }
        } else {
            accumulator = 0.0f;
        }

        render();

        fpsFrames++;
        fpsTime += realDt;
        if (fpsTime >= 5.0f) {
            std::cout << "FPS: " << static_cast<int>(fpsFrames / fpsTime + 0.5f) << std::endl;
            fpsFrames = 0;
            fpsTime = 0.0f;
        }
    }
}

void GameManager::handleInput() {
    while (auto event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
            return;
        }

        if (event->is<sf::Event::FocusGained>()) focusClock.restart();
        const auto* press = event->getIf<sf::Event::MouseButtonPressed>();
        if (press) {
            bool focusClick = focusClock.getElapsedTime().asSeconds() < FOCUS_CLICK_GUARD;
            if (focusClick || !window.hasFocus()) continue;
        }

        sf::Vector2i pixel = press ? press->position : sf::Mouse::getPosition(window);
        sf::Vector2f mouse(static_cast<float>(pixel.x), static_cast<float>(pixel.y));
        bool leftClick = press && press->button == sf::Mouse::Button::Left;
        const auto* key = event->getIf<sf::Event::KeyPressed>();

        switch (currentState) {
            case GameState::MENU: {
                Menus::Action action = leftClick ? menus.clickMain(mouse) : Menus::Action::None;
                if (key && key->code == sf::Keyboard::Key::Enter) action = Menus::Action::Play;
                if (action != Menus::Action::None) sound.play(SoundManager::Effect::Click);
                if (action == Menus::Action::Play) restartGame();
                if (action == Menus::Action::HowToPlay) { howToPlayReturn = GameState::MENU; changeState(GameState::HOW_TO_PLAY); }
                if (action == Menus::Action::AlgorithmView) {
                    restartGame();
                    viewsFromMenu = true;
                    openView(AlgorithmViews::Tab::Kruskal);
                }
                if (action == Menus::Action::Quit) window.close();
                break;
            }
            case GameState::HOW_TO_PLAY: {
                bool back = (leftClick && menus.clickHowToPlay(mouse) == Menus::Action::Back) ||
                            (key && key->code == sf::Keyboard::Key::Escape);
                if (back) changeState(howToPlayReturn);
                break;
            }
            case GameState::PAUSED: {
                Menus::Action action = leftClick ? menus.clickPause(mouse) : Menus::Action::None;
                if (key && key->code == sf::Keyboard::Key::Escape) action = Menus::Action::Resume;
                if (action == Menus::Action::Resume) togglePause();
                if (action == Menus::Action::Restart) restartGame();
                if (action == Menus::Action::HowToPlay) { howToPlayReturn = GameState::PAUSED; changeState(GameState::HOW_TO_PLAY); }
                if (action == Menus::Action::MainMenu) changeState(GameState::MENU);
                if (action == Menus::Action::Quit) window.close();
                break;
            }
            case GameState::GAME_OVER: {
                Menus::Action action = leftClick ? menus.clickGameOver(mouse) : Menus::Action::None;
                if (key && key->code == sf::Keyboard::Key::R) action = Menus::Action::Restart;
                if (action == Menus::Action::Restart) restartGame();
                if (action == Menus::Action::MainMenu) changeState(GameState::MENU);
                break;
            }
            case GameState::PLAYING: {
                if (views.isOpen()) {
                    if (key && key->code == sf::Keyboard::Key::D) {
                        raidRoad();
                        openView(AlgorithmViews::Tab::Raid);
                    } else if (views.handleEvent(*event, mouse, supplyNetwork) == AlgorithmViews::Action::Close) {
                        views.close();
                        if (viewsFromMenu) {
                            viewsFromMenu = false;
                            changeState(GameState::MENU);
                        }
                    }
                } else if (helpOpen) {
                    bool closeHelp = (key && (key->code == sf::Keyboard::Key::Escape || key->code == sf::Keyboard::Key::H)) ||
                                     (leftClick && hud->helpCloseRect(1.f).contains(mouse));
                    if (closeHelp) helpOpen = false;
                } else {
                    handleGameEvent(*event, mouse);
                }
                break;
            }
        }
    }
}

void GameManager::handleGameEvent(const sf::Event& event, sf::Vector2f mouse) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (algorithmMenuOpen && keyPressed->code == sf::Keyboard::Key::Escape) {
            algorithmMenuOpen = false;
            return;
        }
        algorithmMenuOpen = false;
        switch (keyPressed->code) {
            case sf::Keyboard::Key::Num1:   selectedTower = TowerType::Barrier; break;
            case sf::Keyboard::Key::Num2:   selectedTower = TowerType::Gatling; break;
            case sf::Keyboard::Key::Num3:   selectedTower = TowerType::Frost; break;
            case sf::Keyboard::Key::Num4:   selectedTower = TowerType::Artillery; break;
            case sf::Keyboard::Key::Space:  startNextWave(); break;
            case sf::Keyboard::Key::Escape: togglePause(); break;
            case sf::Keyboard::Key::M:      supplyOverlay.toggle(); break;
            case sf::Keyboard::Key::D:      raidRoad(); break;
            case sf::Keyboard::Key::P:      openView(AlgorithmViews::Tab::Kruskal); break;
            case sf::Keyboard::Key::L:      openView(AlgorithmViews::Tab::Convoys); break;
            case sf::Keyboard::Key::V:      openView(AlgorithmViews::Tab::Raid); break;
            case sf::Keyboard::Key::H:      helpOpen = true; break;
            case sf::Keyboard::Key::F:      gameSpeed = gameSpeed == 1 ? 2 : 1; break;
            case sf::Keyboard::Key::N:      sound.toggleMute(); break;
            default: break;
        }
    }

    const auto* press = event.getIf<sf::Event::MouseButtonPressed>();
    if (!press || press->button != sf::Mouse::Button::Left) return;

    HudButton button = hud->buttonAt(mouse, hudState());
    bool menuWasOpen = algorithmMenuOpen;
    algorithmMenuOpen = false;
    switch (button) {
        case HudButton::Tower0: case HudButton::Tower1: case HudButton::Tower2: case HudButton::Tower3:
            selectedTower = static_cast<TowerType>(static_cast<int>(button) - static_cast<int>(HudButton::Tower0));
            break;
        case HudButton::Algorithms:  algorithmMenuOpen = !menuWasOpen; break;
        case HudButton::ViewKruskal: openView(AlgorithmViews::Tab::Kruskal); break;
        case HudButton::ViewRaid:    openView(AlgorithmViews::Tab::Raid); break;
        case HudButton::ViewConvoys: openView(AlgorithmViews::Tab::Convoys); break;
        case HudButton::Speed:       gameSpeed = gameSpeed == 1 ? 2 : 1; break;
        case HudButton::Sound:       sound.toggleMute(); break;
        case HudButton::StartWave:   startNextWave(); break;
        case HudButton::None:
            if (!menuWasOpen && isOverGrid(press->position))
                tryPlaceTower(selectedTower, worldToGrid(windowToWorld(press->position)));
            return;
    }
    sound.play(SoundManager::Effect::Click);
}

void GameManager::update(float dt) {
    const auto& enemies = enemyManager.getEnemies();
    towerManager.update(dt, const_cast<std::vector<std::unique_ptr<Enemy>>&>(enemies));

    enemyManager.update(dt);
    effects.update(dt);

    for (const EnemyDeath& death : enemyManager.takeDeaths()) {
        playerMoney += death.reward;
        kills++;
        effects.deathPuff(death.position);
        sound.play(SoundManager::Effect::Coin);
    }

    if (waveActive) {
        waveTime += dt;
        if (raidsThisWave < RAIDS_PER_WAVE && waveTime >= (raidsThisWave + 1) * RAID_INTERVAL &&
            !supplyOverlay.isRaidPlaying()) {
            raidsThisWave++;
            raidRoad();
        }
        if (enemyManager.allEnemiesDefeated()) {
            waveActive = false;
            onWaveEnded();
        }
    }

    checkLivesLost();
    checkWinLoss();

    if (currentWave > 0 && !waveActive && intermission > 0.0f) {
        intermission -= dt;
        if (intermission <= 0.0f) startNextWave();
    }
}

void GameManager::updateUi(float realDt) {
    uiTime += realDt;
    waveBannerAge += realDt;
    supplyOverlay.update(realDt);
    toastAge += realDt;
    views.update(realDt);

    auto approach = [realDt](float& value, bool target) {
        value = target ? std::min(1.f, value + realDt / 0.25f) : std::max(0.f, value - realDt / 0.25f);
    };
    approach(pauseAnim, currentState == GameState::PAUSED);
    approach(helpAnim, helpOpen);
    if (currentState == GameState::GAME_OVER) gameOverAge += realDt;
}

void GameManager::render() {
    window.clear(Theme::Background);
    sf::Vector2i pixel = sf::Mouse::getPosition(window);
    sf::Vector2f mouse(static_cast<float>(pixel.x), static_cast<float>(pixel.y));

    if (currentState == GameState::MENU) {
        menus.drawMain(window, mouse, uiTime);
    } else if (currentState == GameState::HOW_TO_PLAY) {
        menus.drawHowToPlay(window, mouse);
    } else if (views.isOpen()) {
        views.draw(window, supplyNetwork, mouse);
    } else {
        renderBattlefield(mouse);

        HudState state = hudState();
        hud->drawTopBar(window, state, mouse);
        hud->drawSidebar(window, state, supplyNetwork, mouse);
        hud->drawWaveButton(window, state, mouse);
        WavePlan plan = wavePlan(std::max(1, currentWave));
        hud->drawWaveBanner(window, "WAVE " + std::to_string(currentWave),
                            plan.name + "  -  " + std::to_string(plan.count) + " enemies", waveBannerAge);
        hud->drawToast(window, toastText, toastColor, toastAge, toastLength);
        if (currentState == GameState::PLAYING && !helpOpen && !algorithmMenuOpen)
            hud->drawTooltip(window, tooltipAt(mouse), mouse);
        if (algorithmMenuOpen) hud->drawAlgorithmMenu(window, mouse);
        hud->drawHelp(window, helpAnim, mouse);
    }

    if (currentState == GameState::PAUSED || pauseAnim > 0.f) {
        if (currentState == GameState::PAUSED) menus.drawPause(window, mouse, pauseAnim);
    }
    if (currentState == GameState::GAME_OVER)
        menus.drawGameOver(window, mouse, gameOverAge, currentWave, kills, totalRaids);

    window.display();
}

void GameManager::renderBattlefield(sf::Vector2f mouse) {
    window.setView(worldView);
    sf::Vector2i pixel(static_cast<int>(mouse.x), static_cast<int>(mouse.y));
    bool overGrid = isOverGrid(pixel) && currentState == GameState::PLAYING && !helpOpen;
    sf::Vector2f world = windowToWorld(pixel);

    int hoveredOutpost = overGrid ? supplyOverlay.outpostAt(supplyNetwork, world) : -1;
    int hoveredRoad = overGrid && hoveredOutpost < 0 ? supplyOverlay.roadAt(supplyNetwork, world) : -1;

    grid.draw(window);
    supplyOverlay.drawMap(window, supplyNetwork, hoveredRoad, hoveredOutpost);
    towerManager.draw(window);
    enemyManager.draw(window);
    effects.draw(window);
    supplyOverlay.drawTowerBadges(window, towerManager.getTowers());

    if (overGrid) {
        sf::Vector2i tile = worldToGrid(world);
        sf::Vector2f centre(tile.x * Theme::CELL + Theme::CELL / 2.f, tile.y * Theme::CELL + Theme::CELL / 2.f);
        const Tower* existing = towerManager.getTowerAt(tile);
        TowerType type = existing ? existing->getType() : selectedTower;
        float range = existing ? existing->getRange() : towerInfo(type).range;
        if (type == TowerType::Frost) {
            Ui::roundedRect(window, {{centre.x - 2.5f * Theme::CELL, centre.y - 2.5f * Theme::CELL}, {5 * Theme::CELL, 5 * Theme::CELL}},
                            6.f, sf::Color(150, 220, 255, 35), sf::Color(150, 220, 255, 160), 2.f);
        } else if (range > 0.f) {
            Ui::circle(window, centre, range, sf::Color(255, 255, 255, 28), sf::Color(255, 255, 255, 150), 2.f);
        }
        if (!existing) {
            bool canPlace = !towerManager.isOccupied(tile) && canAfford(selectedTower);
            sf::Color c = canPlace ? Theme::Good : Theme::Bad;
            Ui::roundedRect(window, {{tile.x * Theme::CELL + 2.f, tile.y * Theme::CELL + 2.f}, {Theme::CELL - 4.f, Theme::CELL - 4.f}},
                            6.f, Theme::withAlpha(c, 70), c, 2.f);
        }
    }
    window.setView(window.getDefaultView());
}

std::vector<std::string> GameManager::tooltipAt(sf::Vector2f mouse) const {
    sf::Vector2i pixel(static_cast<int>(mouse.x), static_cast<int>(mouse.y));
    if (!isOverGrid(pixel)) return sidebarTooltip(mouse);
    sf::Vector2f world = windowToWorld(pixel);

    for (const auto& enemy : enemyManager.getEnemies()) {
        sf::Vector2f p = enemy->getPosition();
        if (std::hypot(p.x - world.x, p.y - world.y) > 18.f) continue;
        std::vector<std::string> lines = {enemy->getName(),
                                          "Health " + std::to_string(enemy->getHealth()) + " / " + std::to_string(enemy->getMaxHealth())};
        if (const auto* shielded = dynamic_cast<const ShieldEnemy*>(enemy.get()))
            lines.push_back("Shield " + std::to_string(shielded->getShield()) + " / " + std::to_string(shielded->getMaxShield()) +
                            " (absorbs damage first)");
        lines.push_back("Speed " + std::to_string(static_cast<int>(enemy->getBaseSpeed())) + " px/s   Reward $" +
                        std::to_string(enemy->getReward()));
        return lines;
    }

    sf::Vector2i tile = worldToGrid(world);
    if (const Tower* tower = towerManager.getTowerAt(tile)) {
        const TowerInfo& info = towerInfo(tower->getType());
        std::vector<std::string> lines = {info.name + " tower", info.description};
        if (tower->canShoot()) {
            char rate[64];
            std::snprintf(rate, sizeof(rate), "Range %d px   %.2f shots/s", static_cast<int>(tower->getRange()), tower->getFireRate());
            lines.push_back(rate);
            int effect = tower->getSupplyEffect();
            lines.push_back(effect > 0 ? "Supply: +25% fire rate (green dot: supplied outpost nearby)"
                          : effect < 0 ? "Supply: -25% fire rate (red dot: cut-off outpost nearby)"
                                       : "Supply: no effect");
        }
        return lines;
    }

    if (!supplyNetwork.isLoaded() || !supplyOverlay.isVisible()) return {};
    const Dataset& data = supplyNetwork.getData();
    const MstPlanner& planner = supplyNetwork.getPlanner();

    int o = supplyOverlay.outpostAt(supplyNetwork, world);
    if (o >= 0) {
        const Outpost& op = data.outposts[o];
        if (o == data.castle) return {"Castle (supply base)", "Root of the supply tree; convoys start here",
                                      "Population " + std::to_string(op.population)};
        int requested = 0, delivered = 0;
        const PipelineReport& report = supplyNetwork.getReport();
        for (const SupplyItem& it : data.items) if (it.outpost == o) requested++;
        for (size_t i = 0; i < report.deliverable.size(); i++)
            if (report.deliverable[i].outpost == o && report.packing.ok && report.packing.assignment[i] >= 0) delivered++;
        int effect = supplyNetwork.getOutpostEffect(o);
        return {op.name + " outpost",
                "Severity " + std::to_string(op.severity) + "   population " + std::to_string(op.population),
                planner.isConnectedToCastle(o) ? "Connected to the Castle" : "CUT OFF from the Castle",
                "Requests " + std::to_string(requested) + ", delivered " + std::to_string(delivered) + " (last supply phase)",
                effect > 0 ? "Towers within 3 tiles: +25% fire rate"
                           : effect < 0 ? "Towers within 3 tiles: -25% fire rate" : "No effect on nearby towers"};
    }

    int r = supplyOverlay.roadAt(supplyNetwork, world);
    if (r >= 0) {
        const Road& road = planner.getRoads()[r];
        std::string status = road.blocked ? "Destroyed" : planner.isInTree(r) ? "In the spanning tree" : "Unused (would form a cycle)";
        std::vector<std::string> lines = {roadLabel(data, road), "Repair cost " + std::to_string(road.cost) +
                                          "   severity " + std::to_string(data.roadSeverity(road)), "Status: " + status};
        if (r == nextRaidTarget()) lines.push_back("Next raid target");
        return lines;
    }
    return {};
}

HudState GameManager::hudState() const {
    HudState s;
    s.money = playerMoney;
    s.lives = playerLives;
    s.wave = currentWave;
    s.speed = gameSpeed;
    s.muted = sound.isMuted();
    s.selected = selectedTower;
    s.algorithmMenuOpen = algorithmMenuOpen;
    if (!waveActive && enemyManager.allEnemiesDefeated() && currentState == GameState::PLAYING) {
        WavePlan next = wavePlan(currentWave + 1);
        if (currentWave > 0 && intermission > 0.0f)
            s.waveButton = "Wave " + std::to_string(currentWave + 1) + " in " +
                           std::to_string(static_cast<int>(std::ceil(intermission))) + " s  -  Space = now";
        else
            s.waveButton = "Start wave " + std::to_string(currentWave + 1) + ": " + next.name + "  (Space)";
    }
    return s;
}

void GameManager::restartGame() {
    towerManager.reset();
    enemyManager.reset();
    grid.initialize(GRID_WIDTH, GRID_HEIGHT);
    enemyManager.recalculatePaths();

    currentWave = 0;
    intermission = 0.0f;
    playerMoney = STARTING_MONEY;
    playerLives = STARTING_LIVES;
    selectedTower = TowerType::Barrier;
    accumulator = 0.0f;
    gameSpeed = 1;
    kills = 0;
    totalRaids = 0;
    helpOpen = false;
    algorithmMenuOpen = false;
    toastAge = 99.f;
    waveBannerAge = 99.f;
    gameOverAge = 0.f;
    effects.clear();

    if (supplyNetwork.isLoaded()) supplyNetwork.reset();
    supplyOverlay.reset();
    views.close();
    views.clearRaid();
    viewsFromMenu = false;
    waveActive = false;
    waveTime = 0.0f;
    raidsThisWave = 0;

    changeState(GameState::PLAYING);
    std::cout << "Game restarted" << std::endl;
}

GameManager::WavePlan GameManager::wavePlan(int wave) const {
    static const char* names[] = {"Soldiers advancing", "Runners incoming", "Tanks incoming",
                                  "Shielded troops incoming", "Mixed assault"};
    int kind = (wave - 1) % 5;
    WavePlan plan;
    plan.featured = kind < 4 ? kind : -1;
    plan.name = names[kind];
    plan.count = 5 + wave * 3;
    plan.interval = std::max(0.5f, 2.0f - wave * 0.1f);
    return plan;
}

void GameManager::startNextWave() {
    if (!enemyManager.allEnemiesDefeated()) {
        return;
    }

    currentWave++;
    WavePlan plan = wavePlan(currentWave);
    enemyManager.spawnWave(plan.count, plan.interval, plan.featured);
    playerMoney += 100;
    intermission = 0.0f;

    waveActive = true;
    waveTime = 0.0f;
    raidsThisWave = 0;
    waveBannerAge = 0.0f;
    sound.play(SoundManager::Effect::WaveStart);
}

void GameManager::openView(AlgorithmViews::Tab tab) {
    views.open(tab, supplyNetwork);
}

std::vector<std::pair<int, int>> GameManager::enemyPathTiles() const {
    std::vector<std::pair<int, int>> tiles;
    for (Node* node : enemyManager.getSpawnPath())
        tiles.push_back({node->x, node->y});
    return tiles;
}

int GameManager::nextRaidTarget() const {
    if (!supplyNetwork.isLoaded()) return -1;
    return supplyNetwork.nextRoadToDestroy(enemyPathTiles());
}

void GameManager::raidRoad() {
    int road = nextRaidTarget();
    if (road < 0) return;

    RaidTrace trace = traceRaid(supplyNetwork.getPlanner(), road);
    MstPlanner before = supplyNetwork.getPlanner();

    RoadDestroyedEvent e = supplyNetwork.destroyRoad(road);
    supplyOverlay.startRaidAnimation(e);
    views.recordRaid(before, trace, e);
    totalRaids++;

    const Dataset& data = supplyNetwork.getData();
    const std::vector<Road>& roads = supplyNetwork.getPlanner().getRoads();
    sound.play(SoundManager::Effect::Raid);
    std::string ops = std::to_string(e.incrementalOps) + " ops vs " + std::to_string(e.rebuildOps);
    if (!e.wasInTree)
        showToast("Unused road destroyed: network unchanged", Theme::Main, 2.0f);
    else if (e.replacement >= 0)
        showToast("Repaired: " + ops, Theme::Good, 2.0f);
    else
        showToast("No replacement road: outposts cut off  (" + ops + ")", Theme::Bad, 2.0f);

    std::cout << "Raid: " << roadLabel(data, roads[e.road])
              << (e.wasInTree ? "" : " (not in tree)")
              << " -> replacement " << (e.replacement >= 0 ? roadLabel(data, roads[e.replacement]) : "none")
              << " | ops " << e.incrementalOps << " vs rebuild " << e.rebuildOps
              << " | tree cost " << supplyNetwork.getPlanner().getResult().totalCost
              << (e.matchesRebuild ? " (matches rebuild)" : " (DIFFERS FROM REBUILD)") << std::endl;
}

void GameManager::onWaveEnded() {
    intermission = INTERMISSION_TIME;
    if (!supplyNetwork.isLoaded()) return;
    supplyNetwork.runSupplyPhase();
    applySupplyEffects();

    std::cout << "Supply phase after wave " << currentWave << ":" << std::endl;
    const PipelineReport& report = supplyNetwork.getReport();
    printPacking(supplyNetwork.getData(), report.deliverable, report.packing, std::cout);

    int value = report.packing.totalValue;
    if (value > 0)
        showToast("Supplies delivered: " + std::to_string(value) + "  -  press L to see the packing", Theme::Accent, 4.0f);
    else
        showToast("No supplies delivered: outposts cut off  -  press L to see why", Theme::Bad, 4.0f);
}

void GameManager::showToast(const std::string& text, sf::Color color, float seconds) {
    toastText = text;
    toastColor = color;
    toastLength = seconds;
    toastAge = 0.f;
}

std::vector<std::string> GameManager::sidebarTooltip(sf::Vector2f mouse) const {
    HudButton button = hud->buttonAt(mouse, hudState());
    if (button >= HudButton::Tower0 && button <= HudButton::Tower3) {
        int index = static_cast<int>(button) - static_cast<int>(HudButton::Tower0);
        const TowerInfo& info = towerInfo(static_cast<TowerType>(index));
        std::vector<std::string> lines = {info.name + " tower  ($" + std::to_string(info.cost) + ", key " + info.hotkey + ")",
                                          info.description};
        if (info.range > 0.f) lines.push_back("Range " + std::to_string(static_cast<int>(info.range)) + " px");
        return lines;
    }
    if (!supplyNetwork.isLoaded() || !hud->supplyBoxRect().contains(mouse)) return {};

    const Dataset& data = supplyNetwork.getData();
    const MstPlanner& planner = supplyNetwork.getPlanner();
    const MstResult& mst = planner.getResult();
    const std::vector<Road>& roads = planner.getRoads();
    std::vector<std::string> lines = {"Supply network (Kruskal MST)",
        std::to_string(mst.treeRoads.size()) + " roads in the tree, cost " + std::to_string(mst.totalCost) + ", " +
        std::to_string(supplyNetwork.getDestroyedCount()) + " roads destroyed"};

    if (mst.unreachable.empty()) lines.push_back("Cut off: none");
    std::string names;
    for (size_t i = 0; i < mst.unreachable.size(); i++) {
        names += (names.empty() ? "" : ", ") + data.outposts[mst.unreachable[i]].name;
        if (i % 4 == 3 || i + 1 == mst.unreachable.size()) {
            lines.push_back((i < 4 ? "Cut off: " : "   ") + names);
            names.clear();
        }
    }
    if (supplyNetwork.hasLastEvent()) {
        const RoadDestroyedEvent& e = supplyNetwork.getLastEvent();
        std::string result = !e.wasInTree ? " (not in the tree)"
                           : e.replacement >= 0 ? " -> " + roadLabel(data, roads[e.replacement]) : " -> no replacement";
        lines.push_back("Last raid: " + roadLabel(data, roads[e.road]) + result);
        lines.push_back("   " + std::to_string(e.incrementalOps) + " ops incremental vs " + std::to_string(e.rebuildOps) +
                        " for a full rebuild");
    }
    int next = nextRaidTarget();
    lines.push_back("Next raid target: " + (next >= 0 ? roadLabel(data, roads[next]) : std::string("none left")));

    const PackingResult& packing = supplyNetwork.getReport().packing;
    std::string loads;
    for (int j = 0; j < CONVOY_COUNT; j++)
        loads += (j ? ", " : "") + data.convoys[j].name + " " + std::to_string(packing.load[j]) + "/" +
                 std::to_string(data.convoys[j].capacity);
    int leftBehind = 0;
    for (int a : packing.assignment) if (a < 0) leftBehind++;
    lines.push_back("Convoys: " + loads);
    lines.push_back("Value " + std::to_string(packing.totalValue) + ", " + std::to_string(leftBehind) +
                    " items left behind, " + std::to_string(supplyNetwork.getReport().cutOff.size()) + " requests cut off");
    lines.push_back("Step by step: P (Kruskal), V (last raid), L (convoys)");
    return lines;
}

void GameManager::applySupplyEffects() {
    for (auto& tower : towerManager.getTowers()) {
        int effect = 0;
        if (supplyNetwork.isLoaded() && tower->canShoot()) {
            sf::Vector2i tile = worldToGrid(tower->getPosition());
            effect = supplyNetwork.supplyEffectAt(tile.x, tile.y);
        }
        tower->setSupplyEffect(effect);
    }
}

void GameManager::checkLivesLost() {
    int lost = enemyManager.getReachedGoalCount();
    if (lost > 0) {
        playerLives -= lost;
        if (playerLives < 0) playerLives = 0;
    }
}

void GameManager::checkWinLoss() {
    if (playerLives <= 0 && currentState != GameState::GAME_OVER) {
        gameOverAge = 0.f;
        changeState(GameState::GAME_OVER);
    }
}

void GameManager::togglePause() {
    if (currentState == GameState::GAME_OVER) return;
    changeState(currentState == GameState::PAUSED ? GameState::PLAYING : GameState::PAUSED);
}

bool GameManager::tryPlaceTower(TowerType type, sf::Vector2i gridPos) {
    int cost = TOWER_COSTS[static_cast<int>(type)];
    if (playerMoney < cost) return false;

    if (towerManager.placeTower(type, gridPos)) {
        playerMoney -= cost;
        applySupplyEffects();
        return true;
    }
    return false;
}

sf::Vector2f GameManager::windowToWorld(sf::Vector2i pixel) const {
    return window.mapPixelToCoords(pixel, worldView);
}

bool GameManager::isOverGrid(sf::Vector2i pixel) const {
    return pixel.x >= 0 && pixel.x < Theme::GRID_W && pixel.y >= Theme::TOP_BAR_H &&
           pixel.y < Theme::TOP_BAR_H + Theme::GRID_H;
}

sf::Vector2i GameManager::worldToGrid(sf::Vector2f worldPos) const {
    return sf::Vector2i(static_cast<int>(worldPos.x / CELL_SIZE), static_cast<int>(worldPos.y / CELL_SIZE));
}

void GameManager::changeState(GameState newState) {
    currentState = newState;
}

bool GameManager::canAfford(TowerType type) const {
    int cost = TOWER_COSTS[static_cast<int>(type)];
    return playerMoney >= cost;
}

int GameManager::getTowerCost(TowerType type) const {
    return TOWER_COSTS[static_cast<int>(type)];
}
