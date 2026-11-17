#include "algorithm_trace.hpp"
#include "disjoint_set.hpp"
#include <algorithm>
#include <queue>

KruskalTrace traceKruskal(const Dataset& data, const std::vector<Road>& roads) {
    Dataset current = data;
    current.roads = roads;
    MstPlanner planner(current);
    planner.build();

    KruskalTrace trace;
    trace.sortedRoads = planner.getSortedRoads();
    for (int r = 0; r < static_cast<int>(roads.size()); r++)
        if (roads[r].blocked) trace.blockedRoads.push_back(r);

    int n = static_cast<int>(data.outposts.size());
    DisjointSet sets(n);
    for (int o = 0; o < n; o++) trace.startGroup.push_back(o);

    int treeSize = 0;
    long long cost = 0;
    for (int r : trace.sortedRoads) {
        KruskalStep step;
        step.road = r;
        step.added = sets.unite(roads[r].from, roads[r].to);
        if (step.added) {
            treeSize++;
            cost += roads[r].cost;
        }
        step.costSoFar = cost;
        for (int o = 0; o < n; o++) step.group.push_back(sets.find(o));
        trace.steps.push_back(step);
        if (treeSize == n - 1) break;
    }
    trace.totalCost = cost;
    return trace;
}

RaidTrace traceRaid(const MstPlanner& planner, int road) {
    const std::vector<Road>& roads = planner.getRoads();
    const MstResult& mst = planner.getResult();
    int n = static_cast<int>(mst.component.size());

    RaidTrace trace;
    trace.road = road;
    trace.wasInTree = planner.isInTree(road);
    trace.side.assign(n, -1);
    if (!trace.wasInTree) return trace;

    std::vector<std::vector<int>> adj(n);
    for (int t : mst.treeRoads) {
        if (t == road) continue;
        adj[roads[t].from].push_back(roads[t].to);
        adj[roads[t].to].push_back(roads[t].from);
    }

    int ends[2] = {roads[road].from, roads[road].to};
    for (int s = 0; s < 2; s++) {
        std::queue<int> queue;
        queue.push(ends[s]);
        trace.side[ends[s]] = s;
        while (!queue.empty()) {
            int u = queue.front();
            queue.pop();
            for (int w : adj[u]) {
                if (trace.side[w] != -1) continue;
                trace.side[w] = s;
                queue.push(w);
            }
        }
    }

    for (int c : planner.getSortedRoads()) {
        RaidScanStep step;
        step.road = c;
        int a = trace.side[roads[c].from];
        int b = trace.side[roads[c].to];
        if (c == road) step.verdict = ScanVerdict::JustDestroyed;
        else if (planner.isInTree(c)) step.verdict = ScanVerdict::InTree;
        else if (roads[c].blocked) step.verdict = ScanVerdict::Blocked;
        else if (a == -1 || b == -1) step.verdict = ScanVerdict::OtherTree;
        else if (a == b) step.verdict = ScanVerdict::SameSide;
        else step.verdict = ScanVerdict::Chosen;
        trace.scan.push_back(step);
        if (step.verdict == ScanVerdict::Chosen) {
            trace.replacement = c;
            break;
        }
    }
    return trace;
}

std::vector<LoadStep> traceConvoyLoading(const Dataset& data, const PipelineReport& report) {
    int maxCapacity = 0;
    for (const Convoy& c : data.convoys) maxCapacity = std::max(maxCapacity, c.capacity);

    std::vector<LoadStep> steps;
    const PackingResult& packing = report.packing;
    for (size_t i = 0; i < report.deliverable.size(); i++) {
        LoadStep step;
        step.item = report.deliverable[i];
        step.convoy = packing.ok ? packing.assignment[i] : -1;
        if (step.convoy < 0)
            step.reason = step.item.weight > maxCapacity ? LeftReason::TooHeavy : LeftReason::NoSpace;
        steps.push_back(step);
    }
    for (const SupplyItem& item : report.cutOff) {
        LoadStep step;
        step.item = item;
        step.reason = LeftReason::CutOff;
        steps.push_back(step);
    }
    return steps;
}

std::string leftReasonText(LeftReason reason) {
    switch (reason) {
        case LeftReason::TooHeavy: return "too heavy";
        case LeftReason::NoSpace:  return "no space";
        case LeftReason::CutOff:   return "outpost cut off";
        default:                   return "loaded";
    }
}
