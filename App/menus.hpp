#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include <string>

class Menus {
public:
    enum class Action { None, Play, HowToPlay, AlgorithmView, Quit, Resume, Restart, MainMenu, Back };

    void drawMain(sf::RenderTarget& target, sf::Vector2f mouse, float time) const;
    Action clickMain(sf::Vector2f mouse) const;

    void drawHowToPlay(sf::RenderTarget& target, sf::Vector2f mouse) const;
    Action clickHowToPlay(sf::Vector2f mouse) const;

    void drawPause(sf::RenderTarget& target, sf::Vector2f mouse, float openAmount) const;
    Action clickPause(sf::Vector2f mouse) const;

    void drawGameOver(sf::RenderTarget& target, sf::Vector2f mouse, float age, int wave, int kills, int raids) const;
    Action clickGameOver(sf::Vector2f mouse) const;

private:
    sf::FloatRect mainButton(int index) const;
    sf::FloatRect pauseButton(int index) const;
    sf::FloatRect backButton() const;
    sf::FloatRect gameOverButton(int index) const;
    void drawBackground(sf::RenderTarget& target, std::uint8_t darken) const;
};
