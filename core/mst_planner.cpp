#include "mst_planner.hpp"
#include "disjoint_set.hpp"
#include <algorithm>
#include <queue>

MstPlanner::MstPlanner(const Dataset& data)
    : outposts(data.outposts), roads(data.roads), castle(data.castle) {
}

bool MstPlanner::comesBefore(int a, int b) const {
    const Road& ra = roads[a];
    const Road& rb = roads[b];
    if (ra.cost != rb.cost) return ra.cost < rb.cost;

    int severityA = outposts[ra.from].severity + outposts[ra.to].severity;
    int severityB = outposts[rb.from].severity + outposts[rb.to].severity;
    if (severityA != severityB) return severityA > severityB;

    return ra.id < rb.id;
}

const MstResult& MstPlanner::build() {
    int n = static_cast<int>(outposts.size());
    int m = static_cast<int>(roads.size());
    current = MstResult();
    long long ops = 0;

    sortedRoads.clear();
    for (int r = 0; r < m; r++) {
        ops++;
        if (!roads[r].blocked) sortedRoads.push_back(r);
    }

    std::sort(sortedRoads.begin(), sortedRoads.end(), [&](int a, int b) {
        ops++;
        return comesBefore(a, b);
    });

    DisjointSet sets(n);
    inTree.assign(m, false);
    treeAdj.assign(n, {});
    examinedCount = 0;
    for (int r : sortedRoads) {
        ops++;
        examinedCount++;
        if (sets.unite(roads[r].from, roads[r].to)) {
            addToTree(r);
            if (static_cast<int>(current.treeRoads.size()) == n - 1) break;
        }
    }
    ops += sets.getSteps();

    labelComponents(ops);
    current.operations = ops;
    return current;
}

RoadDestroyedReport MstPlanner::destroyRoad(int road) {
    RoadDestroyedReport report;
    report.road = road;
    long long ops = 1;

    roads[road].blocked = true;
    report.wasInTree = inTree[road];

    if (!inTree[road]) {
        report.operations = ops;
        return report;
    }

    int oldComponent = current.component[roads[road].from];
    removeFromTree(road);

    std::vector<bool> sideA(outposts.size(), false);
    std::vector<int> sideAList;
    std::queue<int> queue;
    queue.push(roads[road].from);
    sideA[roads[road].from] = true;
    while (!queue.empty()) {
        int u = queue.front();
        queue.pop();
        sideAList.push_back(u);
        ops++;
        for (int t : treeAdj[u]) {
            ops++;
            int w = otherEnd(t, u);
            if (!sideA[w]) {
                sideA[w] = true;
                queue.push(w);
            }
        }
    }

    for (int c : sortedRoads) {
        ops++;
        if (inTree[c] || roads[c].blocked) continue;
        int a = roads[c].from;
        int b = roads[c].to;
        bool sameOldTree = current.component[a] == oldComponent && current.component[b] == oldComponent;
        if (sameOldTree && sideA[a] != sideA[b]) {
            addToTree(c);
            report.replacement = c;
            break;
        }
    }
    if (report.replacement < 0) {
        int newLabel = current.componentCount++;
        for (int u : sideAList) {
            ops++;
            current.component[u] = newLabel;
        }
        updateUnreachable(ops);
    }

    report.operations = ops;
    return report;
}

bool MstPlanner::isConnectedToCastle(int outpost) const {
    return current.component[outpost] == current.component[castle];
}

int MstPlanner::otherEnd(int road, int outpost) const {
    return roads[road].from == outpost ? roads[road].to : roads[road].from;
}

void MstPlanner::addToTree(int road) {
    inTree[road] = true;
    treeAdj[roads[road].from].push_back(road);
    treeAdj[roads[road].to].push_back(road);
    current.treeRoads.push_back(road);
    current.totalCost += roads[road].cost;
}

void MstPlanner::removeFromTree(int road) {
    inTree[road] = false;
    auto removeFrom = [road](std::vector<int>& list) {
        list.erase(std::remove(list.begin(), list.end(), road), list.end());
    };
    removeFrom(treeAdj[roads[road].from]);
    removeFrom(treeAdj[roads[road].to]);
    removeFrom(current.treeRoads);
    current.totalCost -= roads[road].cost;
}

void MstPlanner::labelComponents(long long& ops) {
    int n = static_cast<int>(outposts.size());
    current.component.assign(n, -1);
    current.componentCount = 0;

    for (int start = 0; start < n; start++) {
        if (current.component[start] != -1) continue;
        int label = current.componentCount++;
        std::queue<int> queue;
        queue.push(start);
        current.component[start] = label;
        while (!queue.empty()) {
            int u = queue.front();
            queue.pop();
            ops++;
            for (int t : treeAdj[u]) {
                ops++;
                int w = otherEnd(t, u);
                if (current.component[w] == -1) {
                    current.component[w] = label;
                    queue.push(w);
                }
            }
        }
    }

    updateUnreachable(ops);
}

void MstPlanner::updateUnreachable(long long& ops) {
    current.unreachable.clear();
    for (int i = 0; i < static_cast<int>(outposts.size()); i++) {
        ops++;
        if (current.component[i] != current.component[castle])
            current.unreachable.push_back(i);
    }
}
