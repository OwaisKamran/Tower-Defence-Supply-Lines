#pragma once
#include "tower.hpp"
#include <SFML/Graphics.hpp>

class BarrierTower : public Tower {
private:
    sf::RectangleShape shape;
    int durability;

public:
    BarrierTower(sf::Vector2f pos);

    void update(float deltaTime, std::vector<std::unique_ptr<Enemy>>& enemies) override;
    void draw(sf::RenderWindow& window) override;

    void takeDamage(int damage);
    bool isDestroyed() const;
    int getDurability() const { return durability; }
};
