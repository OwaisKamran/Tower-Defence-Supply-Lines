#include "ui_draw.hpp"
#include "theme.hpp"
#include <algorithm>
#include <cmath>

namespace Ui {
static sf::Font* regularFont = nullptr;
static sf::Font* boldFont = nullptr;

void setFonts(sf::Font& regular, sf::Font& bold) {
    regularFont = &regular;
    boldFont = &bold;
}

sf::Font& font(bool bold) {
    return bold ? *boldFont : *regularFont;
}

void roundedRect(sf::RenderTarget& target, sf::FloatRect rect, float radius, sf::Color fill,
                 sf::Color outline, float outlineThickness) {
    const int perCorner = 6;
    radius = std::min(radius, std::min(rect.size.x, rect.size.y) / 2.f);
    sf::ConvexShape shape(perCorner * 4);
    sf::Vector2f centres[4] = {
        {rect.position.x + rect.size.x - radius, rect.position.y + radius},
        {rect.position.x + rect.size.x - radius, rect.position.y + rect.size.y - radius},
        {rect.position.x + radius, rect.position.y + rect.size.y - radius},
        {rect.position.x + radius, rect.position.y + radius}};
    const float pi = 3.14159265f;
    for (int c = 0; c < 4; c++) {
        for (int i = 0; i < perCorner; i++) {
            float angle = -pi / 2.f + c * pi / 2.f + (pi / 2.f) * i / (perCorner - 1);
            shape.setPoint(c * perCorner + i, centres[c] + sf::Vector2f(std::cos(angle), std::sin(angle)) * radius);
        }
    }
    shape.setFillColor(fill);
    shape.setOutlineColor(outline);
    shape.setOutlineThickness(outlineThickness);
    target.draw(shape);
}

void panel(sf::RenderTarget& target, sf::FloatRect rect, const std::string& title) {
    roundedRect(target, rect, 8.f, Theme::Panel, Theme::Border, 1.5f);
    if (!title.empty())
        text(target, title, {rect.position.x + 12.f, rect.position.y + 8.f}, 15, Theme::Accent, true);
}

float text(sf::RenderTarget& target, const std::string& s, sf::Vector2f pos, unsigned size,
           sf::Color color, bool bold) {
    sf::Text t(font(bold), s, size);
    t.setFillColor(color);
    t.setPosition({std::round(pos.x), std::round(pos.y)});
    target.draw(t);
    return t.getLocalBounds().size.x;
}

float textWidth(const std::string& s, unsigned size, bool bold) {
    sf::Text t(font(bold), s, size);
    return t.getLocalBounds().size.x;
}

void textCentered(sf::RenderTarget& target, const std::string& s, sf::Vector2f centre, unsigned size,
                  sf::Color color, bool bold) {
    sf::Text t(font(bold), s, size);
    sf::FloatRect b = t.getLocalBounds();
    t.setFillColor(color);
    t.setPosition({std::round(centre.x - b.size.x / 2.f - b.position.x),
                   std::round(centre.y - b.size.y / 2.f - b.position.y)});
    target.draw(t);
}

void textRight(sf::RenderTarget& target, const std::string& s, sf::Vector2f topRight, unsigned size,
               sf::Color color, bool bold) {
    float w = textWidth(s, size, bold);
    text(target, s, {topRight.x - w, topRight.y}, size, color, bold);
}

void bar(sf::RenderTarget& target, sf::FloatRect rect, float fraction, sf::Color fill, sf::Color back) {
    roundedRect(target, rect, rect.size.y / 2.f, back);
    fraction = clamp01(fraction);
    if (fraction > 0.f) {
        sf::FloatRect filled = rect;
        filled.size.x = std::max(rect.size.y, rect.size.x * fraction);
        roundedRect(target, filled, rect.size.y / 2.f, fill);
    }
}

void button(sf::RenderTarget& target, sf::FloatRect rect, const std::string& label, sf::Vector2f mouse,
            bool active, unsigned size) {
    bool hover = rect.contains(mouse);
    sf::Color fill = active ? Theme::Accent : (hover ? Theme::PanelHover : Theme::PanelLight);
    sf::Color textColor = active ? sf::Color(30, 24, 10) : Theme::Text;
    roundedRect(target, rect, 7.f, fill, hover ? Theme::Accent : Theme::Border, 1.5f);
    textCentered(target, label, {rect.position.x + rect.size.x / 2.f, rect.position.y + rect.size.y / 2.f},
                 size, textColor, true);
}

void slider(sf::RenderTarget& target, sf::FloatRect rect, float value, const std::string& label) {
    text(target, label, {rect.position.x, rect.position.y - 20.f}, 13, Theme::TextDim);
    float midY = rect.position.y + rect.size.y / 2.f;
    roundedRect(target, {{rect.position.x, midY - 3.f}, {rect.size.x, 6.f}}, 3.f, Theme::PanelLight);
    roundedRect(target, {{rect.position.x, midY - 3.f}, {rect.size.x * clamp01(value), 6.f}}, 3.f, Theme::Accent);
    circle(target, {rect.position.x + rect.size.x * clamp01(value), midY}, 8.f, Theme::Text, Theme::Accent, 2.f);
}

void line(sf::RenderTarget& target, sf::Vector2f a, sf::Vector2f b, float thickness, sf::Color color,
          float dash, float gap) {
    sf::Vector2f d = b - a;
    float length = std::sqrt(d.x * d.x + d.y * d.y);
    if (length < 0.5f) return;
    sf::Vector2f dir = d / length;
    sf::Angle angle = sf::radians(std::atan2(d.y, d.x));
    float step = dash > 0.f ? dash : length;
    for (float start = 0.f; start < length; start += step + gap) {
        sf::RectangleShape piece({std::min(step, length - start), thickness});
        piece.setOrigin({0.f, thickness / 2.f});
        piece.setPosition(a + dir * start);
        piece.setRotation(angle);
        piece.setFillColor(color);
        target.draw(piece);
    }
}

void circle(sf::RenderTarget& target, sf::Vector2f centre, float radius, sf::Color fill,
            sf::Color outline, float outlineThickness) {
    sf::CircleShape c(radius, 40);
    c.setOrigin({radius, radius});
    c.setPosition(centre);
    c.setFillColor(fill);
    c.setOutlineColor(outline);
    c.setOutlineThickness(outlineThickness);
    target.draw(c);
}

float distanceToSegment(sf::Vector2f p, sf::Vector2f a, sf::Vector2f b) {
    sf::Vector2f d = b - a;
    float lengthSq = d.x * d.x + d.y * d.y;
    float t = lengthSq > 0.f ? ((p.x - a.x) * d.x + (p.y - a.y) * d.y) / lengthSq : 0.f;
    t = clamp01(t);
    sf::Vector2f c = a + d * t;
    return std::hypot(p.x - c.x, p.y - c.y);
}
}
