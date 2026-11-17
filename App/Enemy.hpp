#ifndef ENEMY_HPP
#define ENEMY_HPP

#include <SFML/Graphics.hpp>
#include <vector>
#include <optional>
#include <map>
#include <string>
#include "node.hpp"

enum class Direction {
    North,
    South,
    East,
    West
};
class Enemy {
protected:
    sf::Vector2f position;
    float baseSpeed;
    int health;
    int maxHealth;
    bool reachedGoal;

    std::vector<Node*> path;
    int currentNodeIndex;

    sf::CircleShape shape;

    std::map<Direction, std::vector<sf::Texture*>> directionTextures;
    std::optional<sf::Sprite> sprite;
    Direction currentDirection;
    int currentFrame;
    float animationTimer;
    float frameTime;

    void drawHealthBar(sf::RenderWindow& window) const;
    void updateDirection(const sf::Vector2f& movement);
    void updateAnimation(float deltaTime);

public:
    Enemy(const std::vector<Node*>& path, float speed = 100.f, int health = 100);
    virtual ~Enemy() = default;

    virtual void update(float deltaTime);
    virtual void draw(sf::RenderWindow& window);
    virtual void takeDamage(int dmg);

    void setDirectionalTextures(Direction dir, sf::Texture& frame1, sf::Texture& frame2);
    void refreshSpriteTexture();

    void setPath(const std::vector<Node*>& newPath);

    bool isDead() const;
    bool hasReachedGoal() const;

    const sf::Vector2f& getPosition() const;
    int getHealth() const { return health; }
    int getMaxHealth() const { return maxHealth; }
    float getBaseSpeed() const { return baseSpeed; }
    virtual std::string getName() const = 0;
    virtual int getReward() const = 0;
    const std::vector<Node*>& getPath() const { return path; }
    int getPathIndex() const { return currentNodeIndex; }

    virtual float getCurrentSpeed(Node* node) const;
};

class NormalEnemy : public Enemy {
public:
    NormalEnemy(const std::vector<Node*>& path);
    std::string getName() const override { return "Soldier"; }
    int getReward() const override { return 10; }
};

class FastEnemy : public Enemy {
public:
    FastEnemy(const std::vector<Node*>& path);
    std::string getName() const override { return "Runner"; }
    int getReward() const override { return 8; }
};

class TankEnemy : public Enemy {
public:
    TankEnemy(const std::vector<Node*>& path);
    std::string getName() const override { return "Tank"; }
    int getReward() const override { return 25; }
};

class ShieldEnemy : public Enemy {
    int shield;
    int maxShield;
public:
    ShieldEnemy(const std::vector<Node*>& path);
    void takeDamage(int dmg) override;
    void draw(sf::RenderWindow& window) override;
    int getShield() const { return shield; }
    int getMaxShield() const { return maxShield; }
    std::string getName() const override { return "Shielded"; }
    int getReward() const override { return 15; }
};

#endif
