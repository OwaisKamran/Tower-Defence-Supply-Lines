#pragma once
#include <cmath>

class Node {
public:
    int x, y;
    bool walkable;

    float gCost;
    float hCost;
    float baseCost;
    float slowMultiplier;
    Node* parent;
    bool opened;
    bool closed;

    Node(int x = 0, int y = 0, bool walkable = true);

    float getFCost() const;

    float getMovementCost() const;

    bool operator==(const Node& other) const;

    void resetCosts();
};
