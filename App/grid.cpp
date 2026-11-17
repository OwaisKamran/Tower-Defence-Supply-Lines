#include "grid.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

Grid::Grid(int w, int h) : gridTexture(nullptr), startTexture(nullptr), endTexture(nullptr) {
    initialize(w, h);
}

void Grid::initialize(int w, int h) {
    width = w;
    height = h;
    nodes.resize(height, std::vector<Node>(width));

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            nodes[y][x] = Node(x, y, true);
            nodes[y][x].baseCost = 1.0f;
            nodes[y][x].slowMultiplier = 1.0f;
        }
    }
}

void Grid::setTexture(sf::Texture& texture) {
    gridTexture = &texture;

    std::cout << "Grid texture set: " << texture.getSize().x << "x" << texture.getSize().y << std::endl;
}

void Grid::setStartTexture(sf::Texture& texture) {
    startTexture = &texture;
    std::cout << "Start texture set: " << texture.getSize().x << "x" << texture.getSize().y << std::endl;
}

void Grid::setEndTexture(sf::Texture& texture) {
    endTexture = &texture;
    std::cout << "End texture set: " << texture.getSize().x << "x" << texture.getSize().y << std::endl;
}

Node* Grid::getNode(int x, int y) {
    if (x >= 0 && x < width && y >= 0 && y < height)
        return &nodes[y][x];
    return nullptr;
}

std::vector<Node*> Grid::getNeighbors(Node* node) {
    std::vector<Node*> neighbors;
    int x = node->x;
    int y = node->y;

    if (y > 0) neighbors.push_back(&nodes[y - 1][x]);
    if (y < height - 1) neighbors.push_back(&nodes[y + 1][x]);
    if (x > 0) neighbors.push_back(&nodes[y][x - 1]);
    if (x < width - 1) neighbors.push_back(&nodes[y][x + 1]);

    return neighbors;
}

bool Grid::isWalkable(int x, int y) {
    Node* node = getNode(x, y);
    return node && node->walkable;
}

void Grid::setObstacle(int x, int y, bool blocked) {
    Node* node = getNode(x, y);
    if (node) node->walkable = !blocked;
}

void Grid::setStartEnd(sf::Vector2i start, sf::Vector2i end) {
    startCell = start;
    endCell = end;
}

std::vector<sf::Vector2i> Grid::applyFrostEffect(int centerX, int centerY, int radius, float slowMultiplier) {
    std::vector<sf::Vector2i> changed;
    for (int y = centerY - radius; y <= centerY + radius; y++) {
        for (int x = centerX - radius; x <= centerX + radius; x++) {
            Node* node = getNode(x, y);
            if (!node || !node->walkable) continue;

            float before = node->slowMultiplier;
            node->slowMultiplier = std::min(before + slowMultiplier, 1.7f);
            if (node->slowMultiplier != before)
                changed.push_back({x, y});
        }
    }
    return changed;
}
void Grid::resetCosts() {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            nodes[y][x].resetCosts();
        }
    }
}

void Grid::draw(sf::RenderWindow& window) {
    float cellSize = 48.f;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            Node& node = nodes[y][x];
            sf::Vector2i currentPos(x, y);

            if (gridTexture) {
                sf::Sprite gridSprite(*gridTexture);
                gridSprite.setPosition({x * cellSize, y * cellSize});

                sf::Vector2u texSize = gridTexture->getSize();
                float scaleX = cellSize / texSize.x;
                float scaleY = cellSize / texSize.y;
                gridSprite.setScale({scaleX, scaleY});

                if (node.slowMultiplier > 1.0f) {
                    gridSprite.setColor(sf::Color(185, 220, 255));
                } else {
                    gridSprite.setColor(sf::Color::White);
                }

                window.draw(gridSprite);

                if (currentPos == startCell && startTexture) {
                    sf::Sprite startSprite(*startTexture);
                    startSprite.setPosition({x * cellSize, y * cellSize});

                    sf::Vector2u startTexSize = startTexture->getSize();
                    float startScaleX = cellSize / startTexSize.x;
                    float startScaleY = cellSize / startTexSize.y;
                    startSprite.setScale({startScaleX, startScaleY});

                    window.draw(startSprite);
                } else if (currentPos == endCell && endTexture) {
                    sf::Sprite endSprite(*endTexture);
                    endSprite.setPosition({x * cellSize, y * cellSize});

                    sf::Vector2u endTexSize = endTexture->getSize();
                    float endScaleX = cellSize / endTexSize.x;
                    float endScaleY = cellSize / endTexSize.y;
                    endSprite.setScale({endScaleX, endScaleY});

                    window.draw(endSprite);
                }
            } else {
                sf::RectangleShape rect({cellSize - 1, cellSize - 1});

                if (!node.walkable)
                    rect.setFillColor(sf::Color::Black);
                else if (sf::Vector2i(x, y) == startCell)
                    rect.setFillColor(sf::Color::Green);
                else if (sf::Vector2i(x, y) == endCell)
                    rect.setFillColor(sf::Color::Red);
                else if (node.slowMultiplier > 1.0f)
                    rect.setFillColor(sf::Color(100, 180, 255));
                else
                    rect.setFillColor(sf::Color(200, 200, 200));

                rect.setPosition({x * cellSize, y * cellSize});
                window.draw(rect);
            }
        }
    }
    drawIce(window);
}

void Grid::drawIce(sf::RenderWindow& window) {
    const float cellSize = 48.f;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const Node& node = nodes[y][x];
            if (node.slowMultiplier <= 1.0f) continue;
            float strength = std::min(1.f, (node.slowMultiplier - 1.f) / 0.7f);
            sf::RectangleShape ice({cellSize, cellSize});
            ice.setPosition({x * cellSize, y * cellSize});
            ice.setFillColor(sf::Color(200, 235, 255, static_cast<std::uint8_t>(60 + 45 * strength)));
            window.draw(ice);
        }
    }
}
