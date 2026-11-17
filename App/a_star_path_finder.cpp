#include "a_star_path_finder.hpp"
#include <cmath>
#include <algorithm>

AStarPathfinder::AStarPathfinder(Grid* grid) : grid(grid) {}

float AStarPathfinder::calculateHCost(Node* a, Node* b) {
    return std::abs(a->x - b->x) + std::abs(a->y - b->y);
}

std::vector<Node*> AStarPathfinder::findPath(Node* start, Node* end) {
    std::vector<Node*> path;
    if (!start || !end) return {};
    if (start == end) return {start};

    grid->resetCosts();
    openSet.clear();

    start->gCost = 0;
    start->hCost = calculateHCost(start, end);
    start->opened = true;
    openSet.push(start);

    while (!openSet.empty()) {
        Node* current = openSet.pop();

        if (current->closed) continue;
        current->closed = true;

        if (current == end) {
            reconstructPath(start, end, path);
            return path;
        }

        for (Node* neighbor : grid->getNeighbors(current)) {
            if (!neighbor->walkable || neighbor->closed)
                continue;

            float moveCost = neighbor->getMovementCost();
            float newCostToNeighbor = current->gCost + moveCost;

            if (!neighbor->opened || newCostToNeighbor < neighbor->gCost) {
                neighbor->parent = current;
                neighbor->gCost = newCostToNeighbor;
                neighbor->hCost = calculateHCost(neighbor, end);
                neighbor->opened = true;

                openSet.push(neighbor);
            }
        }
    }

    return path;
}

void AStarPathfinder::reconstructPath(Node* start, Node* end, std::vector<Node*>& path) {
    Node* current = end;
    while (current) {
        path.push_back(current);
        current = current->parent;
    }
    std::reverse(path.begin(), path.end());
}
