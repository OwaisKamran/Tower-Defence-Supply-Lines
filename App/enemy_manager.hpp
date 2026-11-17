#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include "enemy.hpp"
#include "a_star_path_finder.hpp"
#include "grid.hpp"

class AssetManager;

struct EnemyDeath {
    sf::Vector2f position;
    int reward;
};

class EnemyManager
{
private:
    Grid *grid;
    AStarPathfinder *pathfinder;
    AssetManager *assetManager;
    std::vector<std::unique_ptr<Enemy>> enemies;
    std::vector<Node *> cachedPath;

    float spawnTimer = 0.0f;
    float spawnInterval;
    int enemiesToSpawn;
    int enemiesSpawned;
    int featuredType = -1;

    int enemiesReachedGoal = 0;
    std::vector<EnemyDeath> deaths;

public:
    EnemyManager(Grid *grid, AStarPathfinder *pathfinder, AssetManager *assets = nullptr);

    void update(float deltaTime);
    void draw(sf::RenderWindow &window);

    void spawnEnemy();
    void spawnWave(int count, float interval, int featured = -1);
    int getEnemiesLeft() const { return static_cast<int>(enemies.size()) + enemiesToSpawn; }
    std::vector<EnemyDeath> takeDeaths();
    void clearDeadEnemies();
    void recalculatePaths();
    void rerouteAffected(const std::vector<sf::Vector2i>& changedCells, bool costsDecreased = false);
    const std::vector<Node*>& getSpawnPath() const { return cachedPath; }
    void reset();

    bool allEnemiesDefeated() const;
    const std::vector<std::unique_ptr<Enemy>> &getEnemies() const;
    int getReachedGoalCount();
};
