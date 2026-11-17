#pragma once
#include <vector>
#include <SFML/Graphics.hpp>
#include "node.hpp"

class Grid {
private:
    int width, height;
    std::vector<std::vector<Node>> nodes;
    sf::Texture* gridTexture;
    sf::Texture* startTexture;
    sf::Texture* endTexture;

public:
    sf::Vector2i startCell, endCell;

    Grid(int w, int h);
    void initialize(int w, int h);

    void setTexture(sf::Texture& texture);
    void setStartTexture(sf::Texture& texture);
    void setEndTexture(sf::Texture& texture);

    Node* getNode(int x, int y);
    std::vector<Node*> getNeighbors(Node* node);
    bool isWalkable(int x, int y);
    void setObstacle(int x, int y, bool blocked);
    void setStartEnd(sf::Vector2i start, sf::Vector2i end);

    std::vector<sf::Vector2i> applyFrostEffect(int centerX, int centerY, int radius, float slowMultiplier);
    void resetCosts();

    void draw(sf::RenderWindow& window);
    void drawIce(sf::RenderWindow& window);

    sf::Vector2i getStart() const { return startCell; }
    sf::Vector2i getEnd() const { return endCell; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
};
