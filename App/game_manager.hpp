#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include <string>
#include <vector>
#include "grid.hpp"
#include "a_star_path_finder.hpp"
#include "enemy_manager.hpp"
#include "tower_manager.hpp"
#include "asset_manager.hpp"
#include "tower.hpp"
#include "supply_network.hpp"
#include "supply_overlay.hpp"
#include "algorithm_views.hpp"
#include "effects.hpp"
#include "sound_manager.hpp"
#include "hud.hpp"
#include "menus.hpp"

enum class GameState {
    MENU,
    HOW_TO_PLAY,
    PLAYING,
    PAUSED,
    GAME_OVER
};

class GameManager {
private:
    sf::RenderWindow window;
    sf::View worldView;
    AssetManager assetManager;

    Grid grid;
    AStarPathfinder pathfinder;
    EnemyManager enemyManager;
    TowerManager towerManager;

    Effects effects;
    SoundManager sound;
    std::optional<Hud> hud;
    Menus menus;
    SupplyOverlay supplyOverlay;
    AlgorithmViews views;

    SupplyNetwork supplyNetwork;
    bool waveActive = false;
    float waveTime = 0.0f;
    int raidsThisWave = 0;
    int totalRaids = 0;
    static constexpr int RAIDS_PER_WAVE = 3;
    static constexpr float RAID_INTERVAL = 4.0f;
    static constexpr float RAID_SLOW_MOTION = 0.35f;

    struct Bomb {
        sf::Vector2f from;
        float age = 0.f;
    };
    std::optional<Bomb> waveBomb;
    std::optional<Bomb> manualBomb;
    static constexpr float BOMB_FLIGHT = 1.0f;

    sf::Clock clock;
    sf::Clock focusClock;
    static constexpr float FOCUS_CLICK_GUARD = 0.25f;
    float accumulator = 0.0f;
    const float FIXED_TIMESTEP = 1.0f / 60.0f;
    float uiTime = 0.0f;
    int gameSpeed = 1;
    int fpsFrames = 0;
    float fpsTime = 0.0f;

    GameState currentState = GameState::MENU;
    GameState howToPlayReturn = GameState::MENU;
    bool viewsFromMenu = false;
    int currentWave = 0;
    float intermission = 0.0f;
    static constexpr float INTERMISSION_TIME = 6.0f;
    float waveBannerAge = 99.f;
    float pauseAnim = 0.f, helpAnim = 0.f, gameOverAge = 0.f;
    bool helpOpen = false;
    bool algorithmMenuOpen = false;
    int kills = 0;

    std::string toastText;
    sf::Color toastColor;
    float toastAge = 99.f, toastLength = 2.f;

    static constexpr int STARTING_MONEY = 1000;
    static constexpr int STARTING_LIVES = 20;
    int playerMoney = STARTING_MONEY;
    int playerLives = STARTING_LIVES;
    const int TOWER_COSTS[4] = {50, 100, 150, 250};

    TowerType selectedTower = TowerType::Barrier;

    static constexpr int GRID_WIDTH = 20;
    static constexpr int GRID_HEIGHT = 15;
    static constexpr int CELL_SIZE = 48;

public:
    GameManager();
    void run();

private:
    void handleInput();
    void handleGameEvent(const sf::Event& event, sf::Vector2f mouse);
    void update(float dt);
    void updateUi(float realDt);
    void render();
    void renderBattlefield(sf::Vector2f mouse);

    struct WavePlan { int featured; std::string name; int count; float interval; };
    WavePlan wavePlan(int wave) const;
    void startNextWave();
    void checkLivesLost();
    void checkWinLoss();
    void togglePause();
    void restartGame();
    void openView(AlgorithmViews::Tab tab);
    HudState hudState() const;
    std::vector<std::string> tooltipAt(sf::Vector2f mouse) const;
    std::vector<std::string> sidebarTooltip(sf::Vector2f mouse) const;
    void showToast(const std::string& text, sf::Color color, float seconds);

    std::vector<std::pair<int, int>> enemyPathTiles() const;
    int nextRaidTarget() const;
    void raidRoad();
    Bomb throwBomb(int road);
    void bombLands(int road);
    sf::Vector2f roadMiddle(int road) const;
    void onWaveEnded();
    void applySupplyEffects();

    bool tryPlaceTower(TowerType type, sf::Vector2i gridPos);
    bool canAfford(TowerType type) const;
    int getTowerCost(TowerType type) const;

    sf::Vector2f windowToWorld(sf::Vector2i pixel) const;
    bool isOverGrid(sf::Vector2i pixel) const;
    sf::Vector2i worldToGrid(sf::Vector2f worldPos) const;

    void changeState(GameState newState);
};
