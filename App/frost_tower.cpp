#include "frost_tower.hpp"
#include <cmath>

FrostTower::FrostTower(sf::Vector2f pos, Grid* grid, float range, float fireRate,
                       int cost, float slowMultiplier, int aoeRangeCells)
    : Tower(pos, range, fireRate, cost, false, TowerType::Frost),
      slowMultiplier(slowMultiplier),
      aoeRangeCells(aoeRangeCells),
      pathCostIncrease(0.0f),
      grid(grid)
{
    aoeVisual.setSize(sf::Vector2f(aoeRangeCells * 2 * 48.f, aoeRangeCells * 2 * 48.f));
    aoeVisual.setOrigin(aoeVisual.getSize() / 2.0f);
    aoeVisual.setPosition(position);
    aoeVisual.setFillColor(sf::Color(0, 150, 255, 60));
}

void FrostTower::update(float deltaTime, std::vector<std::unique_ptr<Enemy>>& enemies) {
    if (cooldown > 0)
        cooldown -= deltaTime;
}

void FrostTower::draw(sf::RenderWindow& window) {
    if (baseSprite) {
        window.draw(*baseSprite);
    } else {
        sf::CircleShape towerBase(16);
        towerBase.setOrigin({16.f, 16.f});
        towerBase.setPosition(position);
        towerBase.setFillColor(sf::Color(100, 180, 255));
        window.draw(towerBase);
    }
}

std::vector<sf::Vector2i> FrostTower::applyFrostEffects() {
    if (!grid) return {};

    int centerX = static_cast<int>(position.x / 48);
    int centerY = static_cast<int>(position.y / 48);

    return grid->applyFrostEffect(centerX, centerY, aoeRangeCells, slowMultiplier);
}
