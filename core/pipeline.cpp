#include "pipeline.hpp"
#include <algorithm>
#include <iomanip>

PipelineReport runPipeline(const Dataset& data) {
    PipelineReport report(data);
    report.planner.build();
    allocateSupplies(data, report);
    return report;
}

void allocateSupplies(const Dataset& data, PipelineReport& report) {
    report.deliverable.clear();
    report.cutOff.clear();
    for (const SupplyItem& item : data.items) {
        if (report.planner.isConnectedToCastle(item.outpost))
            report.deliverable.push_back(item);
        else
            report.cutOff.push_back(item);
    }
    report.packing = packConvoysDP(report.deliverable, data.convoys);
}

std::string roadLabel(const Dataset& data, const Road& road) {
    return "R" + std::to_string(road.id) + " " + data.outposts[road.from].name +
           "-" + data.outposts[road.to].name;
}

void printMst(const Dataset& data, const MstPlanner& planner, std::ostream& out) {
    const std::vector<Road>& roads = planner.getRoads();
    const MstResult& mst = planner.getResult();

    out << "Outposts: " << data.outposts.size() << "   Roads: " << roads.size() << "\n";

    out << "Blocked roads (removed before sorting):";
    bool anyBlocked = false;
    for (const Road& r : roads) {
        if (!r.blocked) continue;
        out << "\n  " << roadLabel(data, r) << " (cost " << r.cost << ")";
        anyBlocked = true;
    }
    out << (anyBlocked ? "\n" : " none\n");

    out << "Kruskal order (cost asc, severity desc, id asc):\n";
    const std::vector<int>& sorted = planner.getSortedRoads();
    for (size_t pos = 0; pos < sorted.size(); pos++) {
        int r = sorted[pos];
        const Road& road = roads[r];
        const char* status;
        if (road.blocked) status = "DESTROYED";
        else if (planner.isInTree(r)) status = "-> in tree";
        else if (static_cast<int>(pos) < planner.getExaminedCount()) status = "skipped (would form a cycle)";
        else status = "not examined (tree already complete)";
        out << "  " << std::left << std::setw(28) << roadLabel(data, road) << std::right
            << " cost " << std::setw(2) << road.cost
            << "  sev " << std::setw(2) << data.roadSeverity(road) << "  " << status << "\n";
    }

    out << "Chosen roads:\n";
    for (int r : mst.treeRoads)
        out << "  " << roadLabel(data, roads[r]) << " (cost " << roads[r].cost << ")\n";
    out << "Total repair cost: " << mst.totalCost << "\n";
    out << "Trees in forest: " << mst.componentCount << "\n";

    out << "Unreachable from the Castle:";
    if (mst.unreachable.empty()) out << " none";
    for (int o : mst.unreachable) out << " " << data.outposts[o].name;
    out << "\n";
}

void printPacking(const Dataset& data, const std::vector<SupplyItem>& items,
                  const PackingResult& packing, std::ostream& out) {
    if (!packing.ok) {
        out << "Packing failed: " << packing.error << "\n";
        return;
    }

    int maxCapacity = 0;
    for (const Convoy& c : data.convoys) maxCapacity = std::max(maxCapacity, c.capacity);

    out << "Item assignment:\n";
    for (size_t i = 0; i < items.size(); i++) {
        const SupplyItem& it = items[i];
        out << "  I" << std::left << std::setw(3) << it.id << std::setw(14) << it.name
            << std::setw(12) << data.outposts[it.outpost].name << std::right
            << " w " << std::setw(2) << it.weight << "  v " << std::setw(3) << it.value << "  -> ";
        int j = packing.assignment[i];
        if (j >= 0)
            out << data.convoys[j].name;
        else if (it.weight > maxCapacity)
            out << "left behind (heavier than every convoy)";
        else
            out << "left behind";
        out << "\n";
    }

    int totalDemand = 0;
    for (const SupplyItem& it : items) totalDemand += it.weight;
    int totalCapacity = 0;
    for (const Convoy& c : data.convoys) totalCapacity += c.capacity;

    out << "Convoy loads:\n";
    for (int j = 0; j < CONVOY_COUNT; j++) {
        out << "  " << std::left << std::setw(8) << data.convoys[j].name << std::right
            << " load " << std::setw(2) << packing.load[j] << " / " << std::setw(2)
            << data.convoys[j].capacity << "   value " << packing.value[j] << "\n";
    }
    out << "Total demand weight " << totalDemand << " vs total capacity " << totalCapacity << "\n";
    out << "Total value delivered: " << packing.totalValue << "\n";
    out << "DP states: " << packing.states << "   operations: " << packing.operations << "\n";
}

void printPipelineReport(const Dataset& data, const PipelineReport& report, std::ostream& out) {
    out << "--- Part A: supply network (Kruskal) ---\n";
    printMst(data, report.planner, out);

    out << "\n--- Part B: convoy packing (multiple knapsack DP) ---\n";
    out << "Requests from connected outposts: " << report.deliverable.size()
        << "   cut off (cannot be delivered): " << report.cutOff.size() << "\n";
    for (const SupplyItem& it : report.cutOff)
        out << "  cut off: I" << it.id << " " << it.name << " for "
            << data.outposts[it.outpost].name << "\n";
    printPacking(data, report.deliverable, report.packing, out);
}
