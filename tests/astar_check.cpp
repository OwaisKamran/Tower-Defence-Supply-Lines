#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <queue>
#include <random>
#include <string>
#include <vector>

#include "grid.hpp"
#include "a_star_path_finder.hpp"

const int W = 20, H = 15;
const sf::Vector2i START = {0, H / 2};
const sf::Vector2i END = {W - 1, H / 2};
const float EPS = 1e-4f;

struct Frost { int x, y; };

struct Layout {
    std::string name;
    std::vector<sf::Vector2i> walls;
    std::vector<Frost> frost;
};

static void buildGrid(Grid& grid, const Layout& layout) {
    grid.initialize(W, H);
    grid.setStartEnd(START, END);
    for (sf::Vector2i w : layout.walls) grid.setObstacle(w.x, w.y, true);
    for (Frost f : layout.frost) grid.applyFrostEffect(f.x, f.y, 2, 0.5f);
}

static float pathCost(Grid& grid, const std::vector<Node*>& path) {
    if (path.empty()) return -1;
    if (path.front() != grid.getNode(START.x, START.y) || path.back() != grid.getNode(END.x, END.y)) return -1;
    float cost = 0;
    for (size_t i = 1; i < path.size(); i++) {
        int step = std::abs(path[i]->x - path[i - 1]->x) + std::abs(path[i]->y - path[i - 1]->y);
        if (step != 1 || !path[i]->walkable) return -1;
        cost += path[i]->getMovementCost();
    }
    return cost;
}

static float dijkstra(Grid& grid) {
    std::vector<float> dist(W * H, std::numeric_limits<float>::max());
    using Entry = std::pair<float, int>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> pq;
    int start = START.y * W + START.x;
    dist[start] = 0;
    pq.push({0.f, start});
    while (!pq.empty()) {
        auto [d, cell] = pq.top();
        pq.pop();
        if (d > dist[cell]) continue;
        Node* node = grid.getNode(cell % W, cell / W);
        for (Node* next : grid.getNeighbors(node)) {
            if (!next->walkable) continue;
            int id = next->y * W + next->x;
            float nd = d + next->getMovementCost();
            if (nd < dist[id]) {
                dist[id] = nd;
                pq.push({nd, id});
            }
        }
    }
    float result = dist[END.y * W + END.x];
    return result == std::numeric_limits<float>::max() ? -1 : result;
}

static std::vector<Layout> fixedLayouts() {
    std::vector<Layout> list;
    list.push_back({"empty grid", {}, {}});

    Layout wall{"wall with one gap", {}, {}};
    for (int y = 0; y < H; y++) if (y != 2) wall.walls.push_back({10, y});
    list.push_back(wall);

    Layout maze{"serpentine maze", {}, {}};
    for (int x = 3; x < W - 1; x += 4) {
        bool gapAtTop = (x / 4) % 2 == 0;
        for (int y = 0; y < H; y++)
            if (gapAtTop ? y != 0 : y != H - 1) maze.walls.push_back({x, y});
    }
    list.push_back(maze);

    list.push_back({"frost on the straight line", {}, {{6, 7}, {9, 7}, {12, 7}}});

    Layout frostWall{"frost inside a corridor", {}, {{8, 6}, {8, 8}, {13, 7}}};
    for (int x = 5; x < 16; x++) {
        frostWall.walls.push_back({x, 4});
        frostWall.walls.push_back({x, 10});
    }
    list.push_back(frostWall);

    Layout stacked{"stacked frost + barrier detour", {{10, 5}, {10, 6}, {10, 7}, {10, 8}, {10, 9}}, {{8, 7}, {9, 7}, {12, 7}, {12, 5}, {12, 9}}};
    list.push_back(stacked);
    return list;
}

static Layout randomLayout(std::mt19937& rng, int index) {
    Layout layout{"random " + std::to_string(index), {}, {}};
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            sf::Vector2i cell{x, y};
            if (cell == START || cell == END) continue;
            if (rng() % 100 < 28) layout.walls.push_back(cell);
        }
    int frostCount = static_cast<int>(rng() % 7);
    for (int i = 0; i < frostCount; i++)
        layout.frost.push_back({static_cast<int>(rng() % W), static_cast<int>(rng() % H)});
    return layout;
}

int main() {
    Grid grid(W, H);
    AStarPathfinder pathfinder(&grid);

    std::cout << "Fixed layouts (A* cost vs Dijkstra optimum):\n";
    int fixedFail = 0;
    for (const Layout& layout : fixedLayouts()) {
        buildGrid(grid, layout);
        float best = dijkstra(grid);
        float found = pathCost(grid, pathfinder.findPath(grid.getNode(START.x, START.y), grid.getNode(END.x, END.y)));
        bool ok = std::fabs(found - best) < EPS;
        if (!ok) fixedFail++;
        std::cout << "  " << std::left << std::setw(32) << layout.name << std::right << std::fixed << std::setprecision(2)
                  << " A* " << std::setw(6) << found << "   optimal " << std::setw(6) << best
                  << "   " << (ok ? "OK" : "SUBOPTIMAL / WRONG") << "\n";
    }

    const int RANDOM_LAYOUTS = 2000;
    std::mt19937 rng(2026);
    int optimal = 0, worse = 0, wrongNoPath = 0, noPathBoth = 0;
    float worstExcess = 0;
    for (int i = 0; i < RANDOM_LAYOUTS; i++) {
        Layout layout = randomLayout(rng, i);
        buildGrid(grid, layout);
        float best = dijkstra(grid);
        float found = pathCost(grid, pathfinder.findPath(grid.getNode(START.x, START.y), grid.getNode(END.x, END.y)));
        if (best < 0 && found < 0) noPathBoth++;
        else if (best < 0 || found < 0) wrongNoPath++;
        else if (std::fabs(found - best) < EPS) optimal++;
        else {
            worse++;
            worstExcess = std::max(worstExcess, found - best);
        }
    }

    std::cout << "\nRandom layouts (" << RANDOM_LAYOUTS << ", 28% walls, 0-6 frost towers):\n"
              << "  optimal path:            " << optimal << "\n"
              << "  longer than optimal:     " << worse << " (worst excess " << worstExcess << ")\n"
              << "  no path on both sides:   " << noPathBoth << "\n"
              << "  path/no-path mismatch:   " << wrongNoPath << "\n";

    bool pass = fixedFail == 0 && worse == 0 && wrongNoPath == 0;
    std::cout << "\n" << (pass ? "PASS" : "FAIL") << ": A* matches Dijkstra on every layout\n";
    return pass ? 0 : 1;
}
