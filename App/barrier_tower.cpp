#include "barrier_tower.hpp"

BarrierTower::BarrierTower(sf::Vector2f pos)
    : Tower(pos, 0.f, 0.f, 50, true, TowerType::Barrier)
{
    shape.setSize(sf::Vector2f(40.f, 40.f));
    shape.setFillColor(sf::Color(139, 69, 19));
    shape.setOrigin({shape.getSize().x / 2.f, shape.getSize().y / 2.f});
    shape.setPosition(position);
}

void BarrierTower::update(float deltaTime, std::vector<std::unique_ptr<Enemy>>& enemies) {
}

void BarrierTower::draw(sf::RenderWindow& window) {
    if (baseSprite) {
        window.draw(*baseSprite);
    } else {
        window.draw(shape);
    }
}
