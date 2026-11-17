#include "node.hpp"

Node::Node(int x, int y, bool walkable)
    : x(x),
      y(y),
      walkable(walkable),
      gCost(0),
      hCost(0),
      baseCost(1.0f),
      slowMultiplier(1.0f),
      parent(nullptr),
      opened(false),
      closed(false) {}

float Node::getFCost() const {
    return gCost + hCost;
}

float Node::getMovementCost() const {
    return baseCost * slowMultiplier;
}

bool Node::operator==(const Node& other) const {
    return x == other.x && y == other.y;
}

void Node::resetCosts() {
    gCost = 0;
    hCost = 0;
    parent = nullptr;
    opened = false;
    closed = false;
}
