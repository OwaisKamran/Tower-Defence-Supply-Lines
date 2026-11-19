#pragma once
#include "tower.hpp"
#include <SFML/Graphics.hpp>

class BarrierTower : public Tower {
private:
    sf::RectangleShape shape;

public:
    BarrierTower(sf::Vector2f pos);

    void update(float deltaTime, std::vector<std::unique_ptr<Enemy>>& enemies) override;
    void draw(sf::RenderWindow& window) override;
};
