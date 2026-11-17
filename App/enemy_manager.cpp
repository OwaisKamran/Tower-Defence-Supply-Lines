#include "enemy_manager.hpp"
#include "asset_manager.hpp"
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <set>

EnemyManager::EnemyManager(Grid *grid, AStarPathfinder *pathfinder, AssetManager *assets)
    : grid(grid), pathfinder(pathfinder), assetManager(assets),
      spawnInterval(2.0f), enemiesToSpawn(0), enemiesSpawned(0)
{
    Node *start = grid->getNode(grid->getStart().x, grid->getStart().y);
    Node *end = grid->getNode(grid->getEnd().x, grid->getEnd().y);
    if (start && end) {
        cachedPath = pathfinder->findPath(start, end);
        std::cout << "Initial path calculated. Path size: " << cachedPath.size() << std::endl;
    } else {
        std::cout << "Error: Start or End node is null!" << std::endl;
    }
}

void EnemyManager::update(float deltaTime)
{
    spawnTimer += deltaTime;
    if (enemiesToSpawn > 0 && spawnTimer >= spawnInterval)
    {
        spawnEnemy();
        spawnTimer = 0.0f;
    }

    for (auto &enemy : enemies)
        enemy->update(deltaTime);

    clearDeadEnemies();
}

void EnemyManager::draw(sf::RenderWindow &window)
{
    for (auto &enemy : enemies)
        enemy->draw(window);
}

void EnemyManager::spawnWave(int count, float interval, int featured)
{
    enemiesToSpawn = count;
    spawnInterval = interval;
    enemiesSpawned = 0;
    featuredType = featured;
    spawnTimer = interval;
}

std::vector<EnemyDeath> EnemyManager::takeDeaths()
{
    std::vector<EnemyDeath> result;
    result.swap(deaths);
    return result;
}

void EnemyManager::spawnEnemy()
{
    if (cachedPath.empty())
    {
        std::cout << "Warning: Cannot spawn enemy - no valid path! Recalculating..." << std::endl;

        Node *start = grid->getNode(grid->getStart().x, grid->getStart().y);
        Node *end = grid->getNode(grid->getEnd().x, grid->getEnd().y);
        if (start && end) {
            cachedPath = pathfinder->findPath(start, end);
            std::cout << "Recalculated path. Path size: " << cachedPath.size() << std::endl;
        }

        if (cachedPath.empty()) {
            std::cout << "Still no path available. Cannot spawn enemies." << std::endl;
            return;
        }
    }

    int type = std::rand() % 4;
    if (featuredType >= 0 && std::rand() % 10 < 7)
        type = featuredType;
    std::unique_ptr<Enemy> e;

    switch (type)
    {
    case 0:
        e = std::make_unique<NormalEnemy>(cachedPath);
        if (assetManager) {
            e->setDirectionalTextures(Direction::North,
                assetManager->getTexture("normal_north_1"),
                assetManager->getTexture("normal_north_2"));
            e->setDirectionalTextures(Direction::South,
                assetManager->getTexture("normal_south_1"),
                assetManager->getTexture("normal_south_2"));
            e->setDirectionalTextures(Direction::East,
                assetManager->getTexture("normal_east_1"),
                assetManager->getTexture("normal_east_2"));
            e->setDirectionalTextures(Direction::West,
                assetManager->getTexture("normal_west_1"),
                assetManager->getTexture("normal_west_2"));
        }
        break;
    case 1:
        e = std::make_unique<FastEnemy>(cachedPath);
        if (assetManager) {
            e->setDirectionalTextures(Direction::North,
                assetManager->getTexture("fast_north_1"),
                assetManager->getTexture("fast_north_2"));
            e->setDirectionalTextures(Direction::South,
                assetManager->getTexture("fast_south_1"),
                assetManager->getTexture("fast_south_2"));
            e->setDirectionalTextures(Direction::East,
                assetManager->getTexture("fast_east_1"),
                assetManager->getTexture("fast_east_2"));
            e->setDirectionalTextures(Direction::West,
                assetManager->getTexture("fast_west_1"),
                assetManager->getTexture("fast_west_2"));
        }
        break;
    case 2:
        e = std::make_unique<TankEnemy>(cachedPath);
        if (assetManager) {
            e->setDirectionalTextures(Direction::North,
                assetManager->getTexture("tank_north_1"),
                assetManager->getTexture("tank_north_2"));
            e->setDirectionalTextures(Direction::South,
                assetManager->getTexture("tank_south_1"),
                assetManager->getTexture("tank_south_2"));
            e->setDirectionalTextures(Direction::East,
                assetManager->getTexture("tank_east_1"),
                assetManager->getTexture("tank_east_2"));
            e->setDirectionalTextures(Direction::West,
                assetManager->getTexture("tank_west_1"),
                assetManager->getTexture("tank_west_2"));
        }
        break;
    case 3:
        e = std::make_unique<ShieldEnemy>(cachedPath);
        if (assetManager) {
            e->setDirectionalTextures(Direction::North,
                assetManager->getTexture("normal_north_1"),
                assetManager->getTexture("normal_north_2"));
            e->setDirectionalTextures(Direction::South,
                assetManager->getTexture("normal_south_1"),
                assetManager->getTexture("normal_south_2"));
            e->setDirectionalTextures(Direction::East,
                assetManager->getTexture("normal_east_1"),
                assetManager->getTexture("normal_east_2"));
            e->setDirectionalTextures(Direction::West,
                assetManager->getTexture("normal_west_1"),
                assetManager->getTexture("normal_west_2"));
        }
        break;
    }

    enemies.push_back(std::move(e));
    enemiesToSpawn--;
    enemiesSpawned++;
}

void EnemyManager::clearDeadEnemies()
{
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
                                 [this](const std::unique_ptr<Enemy> &e)
                                 {
                                     if (e->hasReachedGoal()) {
                                         enemiesReachedGoal++;
                                         return true;
                                     }
                                     if (e->isDead()) {
                                         deaths.push_back({e->getPosition(), e->getReward()});
                                         return true;
                                     }
                                     return false;
                                 }),
                  enemies.end());
}

void EnemyManager::recalculatePaths()
{
    Node *end = grid->getNode(grid->getEnd().x, grid->getEnd().y);
    if (!end)
        return;

    Node *start = grid->getNode(grid->getStart().x, grid->getStart().y);
    if (start)
        cachedPath = pathfinder->findPath(start, end);

    for (auto &enemy : enemies)
    {
        sf::Vector2f enemyPos = enemy->getPosition();
        int gridX = static_cast<int>(enemyPos.x / 48.0f);
        int gridY = static_cast<int>(enemyPos.y / 48.0f);

        Node *enemyNode = grid->getNode(gridX, gridY);
        if (!enemyNode)
            continue;

        std::vector<Node*> newPath = pathfinder->findPath(enemyNode, end);

        if (!newPath.empty())
            enemy->setPath(newPath);
    }
}

static bool pathCrosses(const std::vector<Node*>& path, int fromIndex,
                        const std::set<std::pair<int, int>>& cells)
{
    for (int i = std::max(0, fromIndex); i < static_cast<int>(path.size()); i++)
    {
        if (cells.count({path[i]->x, path[i]->y}))
            return true;
    }
    return false;
}

void EnemyManager::rerouteAffected(const std::vector<sf::Vector2i>& changedCells, bool costsDecreased)
{
    if (costsDecreased)
    {
        recalculatePaths();
        return;
    }
    if (changedCells.empty())
        return;

    std::set<std::pair<int, int>> changed;
    for (const sf::Vector2i& cell : changedCells)
        changed.insert({cell.x, cell.y});

    Node *end = grid->getNode(grid->getEnd().x, grid->getEnd().y);
    if (!end)
        return;

    if (cachedPath.empty() || pathCrosses(cachedPath, 0, changed))
    {
        Node *start = grid->getNode(grid->getStart().x, grid->getStart().y);
        if (start)
            cachedPath = pathfinder->findPath(start, end);
    }

    int rerouted = 0;
    for (auto &enemy : enemies)
    {
        if (!pathCrosses(enemy->getPath(), enemy->getPathIndex(), changed))
            continue;

        sf::Vector2f enemyPos = enemy->getPosition();
        Node *enemyNode = grid->getNode(static_cast<int>(enemyPos.x / 48.0f),
                                        static_cast<int>(enemyPos.y / 48.0f));
        if (!enemyNode)
            continue;

        std::vector<Node*> newPath = pathfinder->findPath(enemyNode, end);
        if (!newPath.empty())
        {
            enemy->setPath(newPath);
            rerouted++;
        }
    }
    std::cout << "Tiles changed: " << changedCells.size() << ", enemies rerouted: "
              << rerouted << " of " << enemies.size() << std::endl;
}

void EnemyManager::reset()
{
    enemies.clear();
    cachedPath.clear();
    enemiesToSpawn = 0;
    enemiesSpawned = 0;
    enemiesReachedGoal = 0;
    deaths.clear();
    spawnTimer = 0.0f;
    featuredType = -1;
}

bool EnemyManager::allEnemiesDefeated() const
{
    return enemies.empty() && enemiesToSpawn == 0;
}

const std::vector<std::unique_ptr<Enemy>> &EnemyManager::getEnemies() const
{
    return enemies;
}

int EnemyManager::getReachedGoalCount()
{
    int count = enemiesReachedGoal;
    enemiesReachedGoal = 0;
    return count;
}
