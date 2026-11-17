#pragma once
#include <SFML/Graphics.hpp>
#include <string>

namespace Ui {
void setFonts(sf::Font& regular, sf::Font& bold);
sf::Font& font(bool bold = false);

void roundedRect(sf::RenderTarget& target, sf::FloatRect rect, float radius, sf::Color fill,
                 sf::Color outline = sf::Color::Transparent, float outlineThickness = 0.f);

void panel(sf::RenderTarget& target, sf::FloatRect rect, const std::string& title = "");

float text(sf::RenderTarget& target, const std::string& s, sf::Vector2f pos, unsigned size,
           sf::Color color, bool bold = false);
void textCentered(sf::RenderTarget& target, const std::string& s, sf::Vector2f centre, unsigned size,
                  sf::Color color, bool bold = false);
void textRight(sf::RenderTarget& target, const std::string& s, sf::Vector2f topRight, unsigned size,
               sf::Color color, bool bold = false);
float textWidth(const std::string& s, unsigned size, bool bold = false);

void bar(sf::RenderTarget& target, sf::FloatRect rect, float fraction, sf::Color fill,
         sf::Color back = sf::Color(20, 24, 34, 220));

void button(sf::RenderTarget& target, sf::FloatRect rect, const std::string& label, sf::Vector2f mouse,
            bool active = false, unsigned size = 18);

void slider(sf::RenderTarget& target, sf::FloatRect rect, float value, const std::string& label);

void line(sf::RenderTarget& target, sf::Vector2f a, sf::Vector2f b, float thickness, sf::Color color,
          float dash = 0.f, float gap = 0.f);

void circle(sf::RenderTarget& target, sf::Vector2f centre, float radius, sf::Color fill,
            sf::Color outline = sf::Color::Transparent, float outlineThickness = 0.f);

inline bool contains(sf::FloatRect rect, sf::Vector2f p) { return rect.contains(p); }

inline float clamp01(float t) { return t < 0.f ? 0.f : (t > 1.f ? 1.f : t); }
inline float easeOut(float t) { t = clamp01(t); float u = 1.f - t; return 1.f - u * u * u; }
float distanceToSegment(sf::Vector2f p, sf::Vector2f a, sf::Vector2f b);
}
