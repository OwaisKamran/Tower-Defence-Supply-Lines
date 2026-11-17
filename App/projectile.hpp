#pragma once
#include <SFML/Graphics.hpp>
#include <optional>

class Enemy;

enum class ProjectileType {
    Gatling,
    Artillery
};

class Projectile {
public:
    sf::Vector2f position;
    sf::Vector2f startPosition;
    sf::Vector2f direction;
    float speed;
    int damage;
    bool active;

    float aoeRadius;
    Enemy* target;
    ProjectileType projectileType;

    sf::CircleShape shape;
    std::optional<sf::Sprite> sprite;
    static const float radius;

    Projectile(sf::Vector2f start, sf::Vector2f dir, float speed, int dmg, float aoe = 0.f, Enemy* target = nullptr);

    void update(float deltaTime);
    void draw(sf::RenderWindow& window);
    void setTexture(sf::Texture& texture);

    bool checkCollision(Enemy* enemy);
};
