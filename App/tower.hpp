#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <optional>
#include "enemy.hpp"

enum class TowerType {
    Barrier,
    Gatling,
    Frost,
    Artillery
};

class Tower {
protected:
    sf::Vector2f position;
    float range;
    float fireRate;
    float cooldown;
    int cost;
    bool isBlocking;
    TowerType type;

    std::optional<sf::Sprite> baseSprite;
    std::optional<sf::Sprite> shooterSprite;
    float currentRotation = -90.f;

    int supplyEffect = 0;
    float fireRateMultiplier = 1.0f;

public:
    Tower(sf::Vector2f pos, float range, float fireRate, int cost, bool isBlocking, TowerType type);
    virtual ~Tower() = default;

    virtual void update(float deltaTime, std::vector<std::unique_ptr<Enemy>>& enemies) = 0;
    virtual void draw(sf::RenderWindow& window) = 0;

    void setBaseTexture(sf::Texture& texture);
    void setShooterTexture(sf::Texture& texture);

    bool canAttack(Enemy* enemy);

    void rotateToward(const sf::Vector2f& targetPos);

    static constexpr float SUPPLY_BUFF = 1.25f;
    static constexpr float SUPPLY_PENALTY = 0.75f;
    void setSupplyEffect(int effect) {
        supplyEffect = effect;
        fireRateMultiplier = effect > 0 ? SUPPLY_BUFF : effect < 0 ? SUPPLY_PENALTY : 1.0f;
    }
    int getSupplyEffect() const { return supplyEffect; }
    bool canShoot() const { return type == TowerType::Gatling || type == TowerType::Artillery; }

    const sf::Vector2f& getPosition() const { return position; }
    float getRange() const { return range; }
    float getFireRate() const { return fireRate * fireRateMultiplier; }
    bool getIsBlocking() const { return isBlocking; }
    int getCost() const { return cost; }
    TowerType getType() const { return type; }
};
