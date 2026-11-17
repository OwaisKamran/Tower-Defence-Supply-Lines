#include "effects.hpp"
#include "theme.hpp"
#include "ui_draw.hpp"
#include <algorithm>
#include <cmath>

float Effects::random(float lo, float hi) {
    return std::uniform_real_distribution<float>(lo, hi)(rng);
}

void Effects::addParticle(sf::Vector2f pos, sf::Vector2f vel, float life, float size, sf::Color color, float drag) {
    if (particles.size() >= MAX_PARTICLES) return;
    particles.push_back({pos, vel, life, life, size, color, drag});
}

void Effects::muzzleFlash(sf::Vector2f pos, sf::Vector2f dir, bool heavy) {
    flashes.push_back({pos, dir, heavy ? 0.1f : 0.06f, heavy ? 0.1f : 0.06f, heavy ? 18.f : 10.f});
}

void Effects::explosion(sf::Vector2f pos, float radius) {
    rings.push_back({pos, 4.f, radius, 0.35f, 0.35f, sf::Color(255, 210, 120)});
    rings.push_back({pos, 2.f, radius * 0.6f, 0.25f, 0.25f, sf::Color(255, 120, 40)});
    for (int i = 0; i < 10; i++) {
        float a = random(0.f, 6.2832f);
        float speed = random(30.f, radius * 2.f);
        sf::Color c = Theme::mix(sf::Color(255, 230, 120), sf::Color(230, 70, 20), random(0.f, 1.f));
        addParticle(pos, {std::cos(a) * speed, std::sin(a) * speed}, random(0.2f, 0.4f), random(2.f, 4.f), c, 3.f);
    }
}

void Effects::deathPuff(sf::Vector2f pos) {
    rings.push_back({pos, 4.f, 22.f, 0.25f, 0.25f, sf::Color(255, 255, 255, 140)});
}

void Effects::update(float dt) {
    for (Particle& p : particles) {
        p.life -= dt;
        p.pos += p.vel * dt;
        p.vel *= std::max(0.f, 1.f - p.drag * dt);
    }
    for (Ring& r : rings) {
        r.life -= dt;
        float t = 1.f - r.life / r.maxLife;
        r.radius = r.maxRadius * Ui::easeOut(t);
    }
    for (Flash& f : flashes) f.life -= dt;
    particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p) { return p.life <= 0.f; }), particles.end());
    rings.erase(std::remove_if(rings.begin(), rings.end(), [](const Ring& r) { return r.life <= 0.f; }), rings.end());
    flashes.erase(std::remove_if(flashes.begin(), flashes.end(), [](const Flash& f) { return f.life <= 0.f; }), flashes.end());
}

void Effects::draw(sf::RenderTarget& target) const {
    sf::VertexArray quads(sf::PrimitiveType::Triangles);
    for (const Particle& p : particles) {
        float t = p.life / p.maxLife;
        sf::Color c = p.color;
        c.a = static_cast<std::uint8_t>(c.a * t);
        float s = p.size * (0.5f + 0.5f * t);
        sf::Vector2f tl = p.pos - sf::Vector2f(s, s), tr = p.pos + sf::Vector2f(s, -s);
        sf::Vector2f bl = p.pos + sf::Vector2f(-s, s), br = p.pos + sf::Vector2f(s, s);
        for (sf::Vector2f v : {tl, tr, br, tl, br, bl}) quads.append(sf::Vertex{v, c});
    }
    target.draw(quads, sf::BlendAdd);

    for (const Ring& r : rings) {
        float t = r.life / r.maxLife;
        Ui::circle(target, r.pos, r.radius, sf::Color::Transparent, Theme::withAlpha(r.color, static_cast<std::uint8_t>(220 * t)), 3.f * t + 1.f);
    }

    for (const Flash& f : flashes) {
        float t = f.life / f.maxLife;
        sf::Vector2f side(-f.dir.y, f.dir.x);
        sf::ConvexShape cone(3);
        cone.setPoint(0, f.pos + side * (f.size * 0.35f));
        cone.setPoint(1, f.pos - side * (f.size * 0.35f));
        cone.setPoint(2, f.pos + f.dir * f.size);
        cone.setFillColor(sf::Color(255, 240, 160, static_cast<std::uint8_t>(230 * t)));
        target.draw(cone, sf::BlendAdd);
        Ui::circle(target, f.pos, f.size * 0.3f, sf::Color(255, 255, 220, static_cast<std::uint8_t>(200 * t)));
    }
}

void Effects::clear() {
    particles.clear();
    rings.clear();
    flashes.clear();
}
