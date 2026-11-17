#pragma once
#include "tower.hpp"
#include "grid.hpp"

class FrostTower : public Tower {
private:
    float slowMultiplier;
    int aoeRangeCells;
    float pathCostIncrease;
    sf::RectangleShape aoeVisual;
    Grid* grid;

public:
    FrostTower(sf::Vector2f pos, Grid* grid, float range, float fireRate,
               int cost, float slowMultiplier, int aoeRangeCells);

    void update(float deltaTime, std::vector<std::unique_ptr<Enemy>>& enemies) override;
    void draw(sf::RenderWindow& window) override;

    std::vector<sf::Vector2i> applyFrostEffects();
};
