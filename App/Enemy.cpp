#include "enemy.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

Enemy::Enemy(const std::vector<Node*>& path, float speed, int health)
    : path(path), baseSpeed(speed), health(health), maxHealth(health),
      currentNodeIndex(0), reachedGoal(false),
      currentDirection(Direction::East), currentFrame(0),
      animationTimer(0.f), frameTime(0.15f)
{
    if (!path.empty()) {
        position = sf::Vector2f(path[0]->x * 48.f + 24.f, path[0]->y * 48.f + 24.f);
    }

    shape.setRadius(12.f);
    shape.setFillColor(sf::Color::Red);
    shape.setOrigin({12.f, 12.f});
    shape.setPosition(position);
}

void Enemy::updateDirection(const sf::Vector2f& movement) {
    Direction previousDirection = currentDirection;

    if (std::abs(movement.x) > std::abs(movement.y)) {
        currentDirection = (movement.x > 0) ? Direction::East : Direction::West;
    } else {
        currentDirection = (movement.y > 0) ? Direction::South : Direction::North;
    }

    if (previousDirection != currentDirection) {
        currentFrame = 0;
        animationTimer = 0.f;
        refreshSpriteTexture();
    }
}

void Enemy::updateAnimation(float deltaTime) {
    if (directionTextures[currentDirection].empty()) return;

    animationTimer += deltaTime;

    if (animationTimer >= frameTime) {
        animationTimer = 0.f;
        currentFrame = (currentFrame + 1) % directionTextures[currentDirection].size();

        if (sprite && directionTextures[currentDirection].size() > 0) {
            sprite->setTexture(*directionTextures[currentDirection][currentFrame]);
        }
    }
}

void Enemy::update(float deltaTime) {
    if (reachedGoal || path.empty()) return;

    Node* target = path[currentNodeIndex];
    sf::Vector2f targetPos(target->x * 48.f + 24.f, target->y * 48.f + 24.f);

    sf::Vector2f dir = targetPos - position;
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);

    if (len < 1.f) {
        currentNodeIndex++;
        if (currentNodeIndex >= static_cast<int>(path.size())) {
            reachedGoal = true;
            return;
        }
        return;
    }

    dir /= len;

    updateDirection(dir);

    float effectiveSpeed = getCurrentSpeed(target);
    sf::Vector2f movement = dir * effectiveSpeed * deltaTime;
    position += movement;
    shape.setPosition(position);

    updateAnimation(deltaTime);

    if (sprite) {
        sprite->setPosition(position);
    }
}

void Enemy::setDirectionalTextures(Direction dir, sf::Texture& frame1, sf::Texture& frame2) {
    directionTextures[dir] = {&frame1, &frame2};

    if (!sprite.has_value()) {
        sprite.emplace(*directionTextures[dir][0]);

        sf::Vector2u texSize = frame1.getSize();
        sprite->setOrigin({texSize.x / 2.f, texSize.y / 2.f});
        sprite->setPosition(position);

        float scaleX = 48.f / texSize.x;
        float scaleY = 48.f / texSize.y;
        sprite->setScale({scaleX, scaleY});
    }

    if (dir == currentDirection && sprite.has_value()) {
        sprite->setTexture(*directionTextures[dir][0]);

        sf::Vector2u texSize = frame1.getSize();
        float scaleX = 48.f / texSize.x;
        float scaleY = 48.f / texSize.y;
        if (dir == Direction::West) {
            sprite->setScale({-scaleX, scaleY});
        } else {
            sprite->setScale({scaleX, scaleY});
        }
    }
}

void Enemy::refreshSpriteTexture() {
    if (sprite.has_value() && !directionTextures[currentDirection].empty()) {
        sprite->setTexture(*directionTextures[currentDirection][currentFrame]);

        if (!directionTextures[currentDirection].empty()) {
            sf::Vector2u texSize = directionTextures[currentDirection][0]->getSize();
            float scaleX = 48.f / texSize.x;
            float scaleY = 48.f / texSize.y;

            if (currentDirection == Direction::West) {
                sprite->setScale({-scaleX, scaleY});
            } else {
                sprite->setScale({scaleX, scaleY});
            }
        }
    }
}

float Enemy::getCurrentSpeed(Node* node) const {
    return baseSpeed / std::max(1.0f, node->slowMultiplier);
}

void Enemy::draw(sf::RenderWindow& window) {
    if (sprite) {
        window.draw(*sprite);
    } else {
        window.draw(shape);
    }
    drawHealthBar(window);
}

static void drawStatusBar(sf::RenderWindow& window, sf::Vector2f centre, float fraction, sf::Color color) {
    const float width = 32.f, height = 5.f;
    sf::RectangleShape frame({width + 2.f, height + 2.f});
    frame.setPosition({centre.x - width / 2.f - 1.f, centre.y - 1.f});
    frame.setFillColor(sf::Color(10, 12, 18, 220));
    window.draw(frame);

    sf::RectangleShape fill({width * std::max(0.f, std::min(1.f, fraction)), height});
    fill.setPosition({centre.x - width / 2.f, centre.y});
    fill.setFillColor(color);
    window.draw(fill);

    sf::RectangleShape shine({fill.getSize().x, 1.f});
    shine.setPosition(fill.getPosition());
    shine.setFillColor(sf::Color(255, 255, 255, 90));
    window.draw(shine);
}

void Enemy::drawHealthBar(sf::RenderWindow& window) const {
    float healthPercent = static_cast<float>(health) / static_cast<float>(maxHealth);

    sf::Color color = healthPercent > 0.6f ? sf::Color(76, 217, 123)
                    : healthPercent > 0.3f ? sf::Color(255, 200, 60)
                                           : sf::Color(240, 86, 86);
    drawStatusBar(window, {position.x, position.y - 22.f}, healthPercent, color);
}

void Enemy::takeDamage(int dmg) {
    health -= dmg;
    if (health < 0) health = 0;
}

void Enemy::setPath(const std::vector<Node*>& newPath) {
    path = newPath;
    reachedGoal = false;

    if (path.empty()) {
        currentNodeIndex = 0;
        return;
    }

    int closestIdx = 0;
    float minDist = std::numeric_limits<float>::max();

    for (int i = 0; i < static_cast<int>(path.size()); ++i) {
        sf::Vector2f nodePos(path[i]->x * 48.f + 24.f, path[i]->y * 48.f + 24.f);
        sf::Vector2f diff = position - nodePos;
        float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);
        if (dist < minDist) {
            minDist = dist;
            closestIdx = i;
        }
    }

    currentNodeIndex = closestIdx;

    if (currentNodeIndex < static_cast<int>(path.size()) - 1) {
        Node* nextNode = path[currentNodeIndex + 1];
        sf::Vector2f nextPos(nextNode->x * 48.f + 24.f, nextNode->y * 48.f + 24.f);
        sf::Vector2f direction = nextPos - position;

        float len = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (len > 0.01f) {
            direction /= len;
            updateDirection(direction);
        }
    }
}

bool Enemy::isDead() const { return health <= 0; }
bool Enemy::hasReachedGoal() const { return reachedGoal; }
const sf::Vector2f& Enemy::getPosition() const { return position; }

NormalEnemy::NormalEnemy(const std::vector<Node*>& path)
    : Enemy(path, 100.f, 100) {
    shape.setFillColor(sf::Color(200, 50, 50));
}

FastEnemy::FastEnemy(const std::vector<Node*>& path)
    : Enemy(path, 160.f, 70) {
    shape.setFillColor(sf::Color(255, 150, 0));
}

TankEnemy::TankEnemy(const std::vector<Node*>& path)
    : Enemy(path, 60.f, 250) {
    shape.setFillColor(sf::Color(80, 80, 80));
}

ShieldEnemy::ShieldEnemy(const std::vector<Node*>& path)
    : Enemy(path, 90.f, 120), shield(80), maxShield(80) {
    shape.setFillColor(sf::Color(0, 100, 255));
}

void ShieldEnemy::draw(sf::RenderWindow& window) {
    if (sprite) {
        window.draw(*sprite);
    } else {
        window.draw(shape);
    }

    drawHealthBar(window);

    if (maxShield > 0 && shield > 0) {
        float shieldPercent = static_cast<float>(shield) / static_cast<float>(maxShield);
        drawStatusBar(window, {position.x, position.y - 29.f}, shieldPercent, sf::Color(90, 200, 255));

        sf::CircleShape bubble(17.f);
        bubble.setOrigin({17.f, 17.f});
        bubble.setPosition(position);
        bubble.setFillColor(sf::Color(90, 200, 255, static_cast<std::uint8_t>(25 + 35 * shieldPercent)));
        bubble.setOutlineColor(sf::Color(150, 225, 255, static_cast<std::uint8_t>(80 + 100 * shieldPercent)));
        bubble.setOutlineThickness(1.5f);
        window.draw(bubble);
    }
}

void ShieldEnemy::takeDamage(int dmg) {
    if (shield > 0) {
        shield -= dmg;
        if (shield < 0) {
            health += shield;
            shield = 0;
        }
    } else {
        Enemy::takeDamage(dmg);
    }
}
