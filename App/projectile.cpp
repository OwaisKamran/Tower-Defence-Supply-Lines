#include "projectile.hpp"
#include "enemy.hpp"
#include <algorithm>
#include <cmath>

const float Projectile::radius = 5.f;

Projectile::Projectile(sf::Vector2f start, sf::Vector2f dir, float spd, int dmg, float aoe, Enemy* target)
    : position(start),
      startPosition(start),
      direction(dir),
      speed(spd),
      damage(dmg),
      active(true),
      aoeRadius(aoe),
      target(target)
{
    shape.setRadius(radius);
    shape.setFillColor(aoeRadius > 0 ? sf::Color::Yellow : sf::Color::Red);
    shape.setOrigin({radius, radius});
    shape.setPosition(position);
}

void Projectile::update(float deltaTime) {
    if (!active) return;

    position += direction * speed * deltaTime;
    shape.setPosition(position);

    if (sprite) {
        sprite->setPosition(position);
    }

    if (position.x < -50 || position.x > 1000 || position.y < -50 || position.y > 800) {
        active = false;
    }
}

void Projectile::draw(sf::RenderWindow& window) {
    if (active) {
        if (sprite) {
            window.draw(*sprite);
        } else {
            window.draw(shape);
        }
    }
}

void Projectile::setTexture(sf::Texture& texture) {
    sprite.emplace(texture);

    sf::Vector2u texSize = texture.getSize();
    sprite->setOrigin({texSize.x / 2.f, texSize.y / 2.f});
    sprite->setPosition(position);

    float desiredWidth = (aoeRadius > 0) ? 24.f : 16.f;
    float scale = desiredWidth / texSize.x;
    sprite->setScale({scale, scale});

    float angle = std::atan2(direction.y, direction.x) * 180.f / 3.14159f;

    sprite->setRotation(sf::degrees(angle + 90.f));
}

bool Projectile::checkCollision(Enemy* enemy) {
    if (!active || !enemy || enemy->isDead()) return false;

    float dist = std::hypot(enemy->getPosition().x - position.x, enemy->getPosition().y - position.y);
    float hitRange = radius + 12.f;

    if (dist <= hitRange) {
        active = false;
        return true;
    }
    return false;
}
