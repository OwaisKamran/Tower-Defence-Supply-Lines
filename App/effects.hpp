#pragma once
#include <SFML/Graphics.hpp>
#include <random>
#include <string>
#include <vector>

class Effects {
public:
    void muzzleFlash(sf::Vector2f pos, sf::Vector2f dir, bool heavy);
    void explosion(sf::Vector2f pos, float radius);
    void deathPuff(sf::Vector2f pos);

    void update(float dt);
    void draw(sf::RenderTarget& target) const;
    void clear();

private:
    struct Particle {
        sf::Vector2f pos, vel;
        float life, maxLife, size;
        sf::Color color;
        float drag;
    };
    struct Ring {
        sf::Vector2f pos;
        float radius, maxRadius, life, maxLife;
        sf::Color color;
    };
    struct Flash {
        sf::Vector2f pos, dir;
        float life, maxLife, size;
    };

    std::vector<Particle> particles;
    std::vector<Ring> rings;
    std::vector<Flash> flashes;
    std::mt19937 rng{12345};
    static constexpr size_t MAX_PARTICLES = 600;

    float random(float lo, float hi);
    void addParticle(sf::Vector2f pos, sf::Vector2f vel, float life, float size, sf::Color color, float drag);
};
