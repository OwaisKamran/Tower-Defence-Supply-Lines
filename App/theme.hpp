#pragma once
#include <SFML/Graphics.hpp>

namespace Theme {
constexpr int WINDOW_W = 1280;
constexpr int WINDOW_H = 768;
constexpr float TOP_BAR_H = 48.f;
constexpr float GRID_W = 960.f;
constexpr float GRID_H = 720.f;
constexpr float SIDEBAR_X = 960.f;
constexpr float SIDEBAR_W = 320.f;
constexpr float CELL = 48.f;

const sf::Color Background(16, 20, 31);
const sf::Color Main(72, 118, 182);
const sf::Color CutOff(128, 132, 142);
const sf::Color Panel(24, 30, 46, 238);
const sf::Color PanelLight(36, 45, 68);
const sf::Color PanelHover(52, 64, 96);
const sf::Color Border(66, 80, 116);
const sf::Color Accent(255, 196, 61);
const sf::Color Text(234, 238, 246);
const sf::Color TextDim(150, 161, 188);
const sf::Color Good(76, 217, 123);
const sf::Color Bad(240, 86, 86);
const sf::Color Info(88, 166, 255);
const sf::Color Ice(140, 215, 255);
const sf::Color SideA(64, 200, 255);
const sf::Color SideB(236, 112, 255);
const sf::Color Skipped(120, 126, 140);

inline sf::Color groupColor(int group) {
    static const sf::Color palette[] = {
        {88, 166, 255}, {255, 140, 66}, {120, 220, 120}, {236, 112, 255}, {255, 214, 64}, {64, 224, 208},
        {255, 99, 132}, {170, 140, 255}, {200, 230, 90}, {255, 170, 200}, {120, 180, 200}, {230, 150, 90}};
    return palette[group % 12];
}

inline sf::Color mix(sf::Color a, sf::Color b, float t) {
    auto lerp = [t](std::uint8_t x, std::uint8_t y) { return static_cast<std::uint8_t>(x + (y - x) * t); };
    return sf::Color(lerp(a.r, b.r), lerp(a.g, b.g), lerp(a.b, b.b), lerp(a.a, b.a));
}

inline sf::Color withAlpha(sf::Color c, std::uint8_t alpha) {
    c.a = alpha;
    return c;
}
}
