#include "supply_network.hpp"
#include "dataset_loader.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

bool SupplyNetwork::load(const std::string& path, std::string& error) {
    if (!loadDataset(path, data, error)) {
        report.reset();
        return false;
    }
    reset();
    return true;
}

void SupplyNetwork::reset() {
    report.emplace(data);
    report->planner.build();
    lastEvent = RoadDestroyedEvent();
    destroyedCount = 0;
    runSupplyPhase();
}

static double pointToSegment(double px, double py, double ax, double ay, double bx, double by) {
    double dx = bx - ax, dy = by - ay;
    double lengthSq = dx * dx + dy * dy;
    double t = lengthSq > 0 ? ((px - ax) * dx + (py - ay) * dy) / lengthSq : 0;
    t = std::max(0.0, std::min(1.0, t));
    double cx = ax + t * dx, cy = ay + t * dy;
    return std::hypot(px - cx, py - cy);
}

int SupplyNetwork::raidTier(int road) const {
    const Road& r = report->planner.getRoads()[road];
    bool touchesCastle = r.from == data.castle || r.to == data.castle;
    if (report->planner.isInTree(road)) return touchesCastle ? 1 : 0;
    return touchesCastle ? 3 : 2;
}

int SupplyNetwork::nextRoadToDestroy(const std::vector<std::pair<int, int>>& enemyPath) const {
    const std::vector<Road>& roads = report->planner.getRoads();

    int best = -1, bestTier = 0;
    double bestDistance = 0;
    for (int r = 0; r < static_cast<int>(roads.size()); r++) {
        if (roads[r].blocked) continue;

        const Outpost& a = data.outposts[roads[r].from];
        const Outpost& b = data.outposts[roads[r].to];
        double distance = enemyPath.empty() ? 0 : std::numeric_limits<double>::max();
        for (const auto& tile : enemyPath)
            distance = std::min(distance, pointToSegment(tile.first, tile.second, a.gridX, a.gridY, b.gridX, b.gridY));

        int tier = raidTier(r);
        bool better = best < 0 || tier < bestTier ||
                      (tier == bestTier && distance < bestDistance - 1e-9) ||
                      (tier == bestTier && std::fabs(distance - bestDistance) <= 1e-9 && roads[r].id < roads[best].id);
        if (better) {
            best = r;
            bestTier = tier;
            bestDistance = distance;
        }
    }
    return best;
}

RoadDestroyedEvent SupplyNetwork::destroyRoad(int road) {
    RoadDestroyedEvent event;
    RoadDestroyedReport update = report->planner.destroyRoad(road);
    event.road = road;
    event.replacement = update.replacement;
    event.wasInTree = update.wasInTree;
    event.incrementalOps = update.operations;

    Dataset after = data;
    after.roads = report->planner.getRoads();
    MstPlanner fresh(after);
    fresh.build();
    event.rebuildOps = fresh.getResult().operations;
    event.matchesRebuild = fresh.getResult().totalCost == report->planner.getResult().totalCost;

    lastEvent = event;
    destroyedCount++;
    return event;
}

void SupplyNetwork::runSupplyPhase() {
    allocateSupplies(data, *report);

    const MstPlanner& planner = report->planner;
    outpostEffect.assign(data.outposts.size(), EFFECT_NONE);
    for (int o = 0; o < static_cast<int>(data.outposts.size()); o++)
        if (!planner.isConnectedToCastle(o)) outpostEffect[o] = EFFECT_PENALTY;

    const PackingResult& packing = report->packing;
    for (size_t i = 0; i < report->deliverable.size(); i++)
        if (packing.ok && packing.assignment[i] >= 0)
            outpostEffect[report->deliverable[i].outpost] = EFFECT_BUFF;
}

int SupplyNetwork::supplyEffectAt(int gridX, int gridY) const {
    int nearest = -1;
    double nearestDistance = EFFECT_RADIUS;
    for (int o = 0; o < static_cast<int>(data.outposts.size()); o++) {
        if (o == data.castle) continue;
        double d = std::hypot(data.outposts[o].gridX - gridX, data.outposts[o].gridY - gridY);
        if (d <= nearestDistance && (nearest < 0 || d < nearestDistance)) {
            nearest = o;
            nearestDistance = d;
        }
    }
    return nearest < 0 ? EFFECT_NONE : outpostEffect[nearest];
}
