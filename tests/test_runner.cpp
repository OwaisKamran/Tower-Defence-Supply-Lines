#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "algorithm_trace.hpp"
#include "dataset_loader.hpp"
#include "disjoint_set.hpp"
#include "knapsack_allocator.hpp"
#include "mst_planner.hpp"
#include "pipeline.hpp"
#include "supply_network.hpp"

static int passed = 0;
static int failed = 0;
static std::string dataFolder = "data";

static void check(bool condition, const std::string& what) {
    std::cout << (condition ? "  [PASS] " : "  [FAIL] ") << what << "\n";
    if (condition) passed++;
    else failed++;
}

static void title(const std::string& text) {
    std::cout << "\n" << std::string(78, '=') << "\n" << text << "\n" << std::string(78, '=') << "\n";
}

static void subtitle(const std::string& text) {
    std::cout << "\n--- " << text << " ---\n";
}

static Dataset load(const std::string& file) {
    Dataset data;
    std::string error;
    if (!loadDataset(dataFolder + "/" + file, data, error)) {
        std::cout << "Cannot load " << file << ": " << error << "\n";
        std::exit(1);
    }
    return data;
}

static int roadIndexById(const Dataset& data, int id) {
    for (size_t i = 0; i < data.roads.size(); i++)
        if (data.roads[i].id == id) return static_cast<int>(i);
    return -1;
}

static int positionInSortedOrder(const MstPlanner& planner, int road) {
    const std::vector<int>& sorted = planner.getSortedRoads();
    return static_cast<int>(std::find(sorted.begin(), sorted.end(), road) - sorted.begin());
}

static std::vector<int> sortedCopy(std::vector<int> v) {
    std::sort(v.begin(), v.end());
    return v;
}

static std::set<std::string> unreachableNames(const Dataset& data, const MstPlanner& planner) {
    std::set<std::string> names;
    for (int o : planner.getResult().unreachable) names.insert(data.outposts[o].name);
    return names;
}

static bool packingIsValid(const std::vector<SupplyItem>& items, const std::vector<Convoy>& convoys,
                           const PackingResult& p) {
    if (!p.ok || p.assignment.size() != items.size()) return false;
    int load[CONVOY_COUNT] = {0, 0, 0};
    int value = 0;
    for (size_t i = 0; i < items.size(); i++) {
        int j = p.assignment[i];
        if (j < -1 || j >= CONVOY_COUNT) return false;
        if (j >= 0) {
            load[j] += items[i].weight;
            value += items[i].value;
        }
    }
    for (int j = 0; j < CONVOY_COUNT; j++)
        if (load[j] > convoys[j].capacity || load[j] != p.load[j]) return false;
    return value == p.totalValue;
}

static int totalWeight(const std::vector<SupplyItem>& items) {
    int sum = 0;
    for (const SupplyItem& it : items) sum += it.weight;
    return sum;
}

static int totalCapacity(const std::vector<Convoy>& convoys) {
    int sum = 0;
    for (const Convoy& c : convoys) sum += c.capacity;
    return sum;
}

static double microsSince(std::chrono::steady_clock::time_point start) {
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::micro>(end - start).count();
}

static std::vector<Convoy> standardConvoys() {
    return {{0, "Wagon", 20}, {1, "Cart", 15}, {2, "Mule", 10}};
}

static Dataset randomDataset(int n, int m, unsigned seed) {
    std::mt19937 rng(seed);
    Dataset d;
    d.castle = 0;
    for (int i = 0; i < n; i++) {
        int severity = (i == 0) ? 0 : 1 + static_cast<int>(rng() % 10);
        d.outposts.push_back({i, "O" + std::to_string(i), 0, 0, severity, 100});
    }
    for (int i = 0; i < m; i++) {
        int a, b;
        if (i < n - 1) {
            a = static_cast<int>(rng() % (i + 1));
            b = i + 1;
        } else {
            a = static_cast<int>(rng() % n);
            b = static_cast<int>(rng() % n);
            if (a == b) b = (b + 1) % n;
        }
        d.roads.push_back({i, a, b, 1 + static_cast<int>(rng() % 50), false});
    }
    d.convoys = standardConvoys();
    return d;
}

static std::vector<SupplyItem> randomItems(int count, unsigned seed) {
    std::mt19937 rng(seed);
    std::vector<SupplyItem> items;
    for (int i = 0; i < count; i++) {
        int weight = 1 + static_cast<int>(rng() % 10);
        int value = 5 + static_cast<int>(rng() % 46);
        items.push_back({i, "Item" + std::to_string(i), weight, value, 0});
    }
    return items;
}

struct IncrementalRow {
    std::string road;
    bool wasInTree;
    std::string replacement;
    long long incrementalOps, rebuildOps;
    double incrementalMicros, rebuildMicros;
    long long incrementalCost, rebuildCost;
    bool sameTree;
};

static std::vector<IncrementalRow> compareIncremental(const Dataset& data, MstPlanner& planner,
                                                      const std::vector<int>& destroyOrder, int timingRuns) {
    std::vector<IncrementalRow> rows;
    for (int road : destroyOrder) {
        IncrementalRow row;
        row.road = roadLabel(data, data.roads[road]);

        double incTime = 0;
        for (int run = 0; run < timingRuns; run++) {
            MstPlanner copy = planner;
            auto start = std::chrono::steady_clock::now();
            copy.destroyRoad(road);
            incTime += microsSince(start);
        }
        RoadDestroyedReport report = planner.destroyRoad(road);

        Dataset after = data;
        after.roads = planner.getRoads();
        double rebuildTime = 0;
        for (int run = 0; run < timingRuns; run++) {
            MstPlanner fresh(after);
            auto start = std::chrono::steady_clock::now();
            fresh.build();
            rebuildTime += microsSince(start);
        }
        MstPlanner fresh(after);
        fresh.build();

        row.wasInTree = report.wasInTree;
        row.replacement = report.replacement >= 0 ? roadLabel(data, data.roads[report.replacement]) : "none";
        row.incrementalOps = report.operations;
        row.rebuildOps = fresh.getResult().operations;
        row.incrementalMicros = incTime / timingRuns;
        row.rebuildMicros = rebuildTime / timingRuns;
        row.incrementalCost = planner.getResult().totalCost;
        row.rebuildCost = fresh.getResult().totalCost;
        row.sameTree = sortedCopy(planner.getResult().treeRoads) == sortedCopy(fresh.getResult().treeRoads) &&
                       planner.getResult().unreachable == fresh.getResult().unreachable;
        rows.push_back(row);
    }
    return rows;
}

static void printIncrementalRows(const std::vector<IncrementalRow>& rows, bool showNames) {
    long long incOps = 0, rebOps = 0;
    double incUs = 0, rebUs = 0;
    std::cout << std::fixed << std::setprecision(2);
    for (const IncrementalRow& r : rows) {
        if (showNames) {
            std::cout << "  Destroyed " << r.road << (r.wasInTree ? " (tree road)" : " (not in tree)")
                      << " -> replacement: " << r.replacement << "\n";
        }
        std::cout << "    incremental: " << std::setw(6) << r.incrementalOps << " ops " << std::setw(10)
                  << r.incrementalMicros << " us   cost " << r.incrementalCost << "\n"
                  << "    rebuild:     " << std::setw(6) << r.rebuildOps << " ops " << std::setw(10)
                  << r.rebuildMicros << " us   cost " << r.rebuildCost << "\n";
        incOps += r.incrementalOps;
        rebOps += r.rebuildOps;
        incUs += r.incrementalMicros;
        rebUs += r.rebuildMicros;
    }
    std::cout << "  TOTAL incremental " << incOps << " ops, " << incUs << " us   |   rebuild " << rebOps
              << " ops, " << rebUs << " us   |   rebuild / incremental = "
              << (incOps > 0 ? static_cast<double>(rebOps) / incOps : 0.0) << "x (ops)\n";
    std::cout.unsetf(std::ios::floatfield);
    std::cout << std::setprecision(6);
}

static bool allSame(const std::vector<IncrementalRow>& rows) {
    for (const IncrementalRow& r : rows)
        if (!r.sameTree || r.incrementalCost != r.rebuildCost) return false;
    return true;
}

static void testWorkedExample() {
    title("TEST 1: Worked example - full pipeline (Part C)");
    Dataset data = load("worked_example.txt");
    PipelineReport report = runPipeline(data);
    printPipelineReport(data, report, std::cout);

    const MstResult& mst = report.planner.getResult();
    subtitle("checks");
    check(data.outposts.size() >= 10 && data.roads.size() >= 20, "dataset has >= 10 outposts and >= 20 roads");
    check(mst.treeRoads.size() == data.outposts.size() - 1, "tree has n-1 roads (all outposts connected)");
    bool noBlocked = true;
    for (int r : mst.treeRoads) noBlocked = noBlocked && !data.roads[r].blocked;
    check(noBlocked, "no blocked road is in the tree");
    check(totalWeight(report.deliverable) > totalCapacity(data.convoys), "demand exceeds capacity (partial packing needed)");
    check(packingIsValid(report.deliverable, data.convoys, report.packing), "packing is valid (loads <= capacity, totals add up)");
}

static void testTieBreak() {
    title("TEST 2: Tie-break rule (cost asc, combined severity desc, road id asc)");
    Dataset data = load("worked_example.txt");
    MstPlanner planner(data);
    planner.build();

    std::cout << "All cost-4 roads in Kruskal order:\n";
    for (int r : planner.getSortedRoads()) {
        const Road& road = data.roads[r];
        if (road.cost != 4) continue;
        std::cout << "  " << std::left << std::setw(28) << roadLabel(data, road) << std::right
                  << " sev " << std::setw(2) << data.roadSeverity(road)
                  << (planner.isInTree(r) ? "  -> in tree" : "  skipped (would form a cycle)") << "\n";
    }

    int r5 = roadIndexById(data, 5), r9 = roadIndexById(data, 9);
    int r10 = roadIndexById(data, 10), r13 = roadIndexById(data, 13);
    std::cout << "\nR5 Stonebridge-Millbrook and R9 Millbrook-Thornvale both cost 4 and would close the\n"
                 "same cycle (Millbrook-Stonebridge-Ashford-Thornvale). R5 serves more severe outposts\n"
                 "(10 vs 7), so it is taken first; when R9 is reached its ends are already joined.\n"
                 "R10 and R13 tie on cost AND severity (11), so the lower road id goes first.\n";

    subtitle("checks");
    check(data.outposts[data.castle].severity == 0, "Castle severity is 0 (does not boost its own roads)");
    check(positionInSortedOrder(planner, r5) < positionInSortedOrder(planner, r9), "R5 (sev 10) sorted before R9 (sev 7)");
    check(planner.isInTree(r5) && !planner.isInTree(r9), "R5 in tree, R9 skipped");
    check(positionInSortedOrder(planner, r10) < positionInSortedOrder(planner, r13), "R10 before R13 (equal cost and severity, lower id)");
}

static void testDisconnected() {
    title("TEST 3: Edge case - disconnected graph (minimum spanning forest)");
    Dataset data = load("disconnected.txt");
    PipelineReport report = runPipeline(data);
    printPipelineReport(data, report, std::cout);

    const MstResult& mst = report.planner.getResult();
    std::set<std::string> expected = {"Duskmoor", "Fenmarsh", "Westgate", "Eaglecrest"};
    bool cutOffOk = true;
    for (const SupplyItem& it : report.cutOff)
        cutOffOk = cutOffOk && !report.planner.isConnectedToCastle(it.outpost);
    bool deliverableOk = true;
    for (const SupplyItem& it : report.deliverable)
        deliverableOk = deliverableOk && report.planner.isConnectedToCastle(it.outpost);

    subtitle("checks");
    check(mst.componentCount == 4, "forest has 4 trees");
    check(unreachableNames(data, report.planner) == expected, "unreachable = Duskmoor, Fenmarsh, Westgate, Eaglecrest");
    check(mst.totalCost == 14, "forest cost = 12 (main tree) + 2 (Duskmoor-Fenmarsh) = 14");
    check(cutOffOk && deliverableOk, "only requests from Castle-connected outposts reach the DP");
    check(packingIsValid(report.deliverable, data.convoys, report.packing), "packing is valid");
}

static void testOverDemand() {
    title("TEST 4: Edge case - over-demand (best partial packing)");
    Dataset data = load("over_demand.txt");
    PipelineReport report = runPipeline(data);
    printPipelineReport(data, report, std::cout);

    PackingResult brute = packConvoysBruteForce(report.deliverable, data.convoys);
    int leftBehind = static_cast<int>(std::count(report.packing.assignment.begin(), report.packing.assignment.end(), -1));
    std::cout << "Brute force: value " << brute.totalValue << " after checking " << brute.states << " assignments\n";

    subtitle("checks");
    check(totalWeight(report.deliverable) > totalCapacity(data.convoys), "total demand exceeds total capacity");
    check(report.packing.ok && report.packing.totalValue > 0, "DP returns a non-zero value (does not fail)");
    check(leftBehind > 0, "some items are left behind");
    check(packingIsValid(report.deliverable, data.convoys, report.packing), "packing is valid");
    check(brute.ok && brute.totalValue == report.packing.totalValue, "DP value equals brute-force optimum");
}

static void testTooHeavy() {
    title("TEST 5: Edge case - items heavier than every convoy");
    Dataset data = load("too_heavy.txt");
    PipelineReport report = runPipeline(data);
    printPipelineReport(data, report, std::cout);

    int maxCapacity = 0;
    for (const Convoy& c : data.convoys) maxCapacity = std::max(maxCapacity, c.capacity);
    bool heavyLeft = true;
    std::vector<SupplyItem> withoutHeavy;
    for (size_t i = 0; i < report.deliverable.size(); i++) {
        if (report.deliverable[i].weight > maxCapacity)
            heavyLeft = heavyLeft && report.packing.assignment[i] == -1;
        else
            withoutHeavy.push_back(report.deliverable[i]);
    }
    PackingResult brute = packConvoysBruteForce(report.deliverable, data.convoys);
    PackingResult filtered = packConvoysDP(withoutHeavy, data.convoys);
    std::cout << "Brute force value: " << brute.totalValue
              << "   DP with heavy items removed by hand: " << filtered.totalValue << "\n";

    subtitle("checks");
    check(heavyLeft, "Siege_Engine and Ballista are left behind");
    check(packingIsValid(report.deliverable, data.convoys, report.packing), "packing is valid");
    check(brute.ok && brute.totalValue == report.packing.totalValue, "DP value equals brute-force optimum");
    check(filtered.totalValue == report.packing.totalValue, "same value as removing the heavy items first (recurrence unaffected)");
}

static void testDpVsBruteForce() {
    title("TEST 6: DP vs brute force (k+1)^N");
    Dataset data = load("worked_example.txt");

    subtitle("worked example, first 10 items");
    std::vector<SupplyItem> first10(data.items.begin(), data.items.begin() + 10);
    PackingResult dp = packConvoysDP(first10, data.convoys);
    PackingResult brute = packConvoysBruteForce(first10, data.convoys);
    std::cout << "DP value " << dp.totalValue << " (" << dp.states << " states, " << dp.operations << " ops)\n"
              << "Brute force value " << brute.totalValue << " (" << brute.states << " assignments)\n";
    check(dp.totalValue == brute.totalValue, "values match");
    check(packingIsValid(first10, data.convoys, dp), "DP packing is valid");

    subtitle("growth with N (random items, capacities 20/15/10)");
    std::cout << "   N   DP ops      DP us   brute assignments   brute us   match\n";
    std::vector<Convoy> convoys = standardConvoys();
    bool allMatch = true;
    for (int n = 4; n <= BRUTE_FORCE_MAX_ITEMS; n += 2) {
        std::vector<SupplyItem> items = randomItems(n, 100 + n);

        const int runs = 5;
        double dpTime = 0;
        PackingResult d;
        for (int run = 0; run < runs; run++) {
            auto start = std::chrono::steady_clock::now();
            d = packConvoysDP(items, convoys);
            dpTime += microsSince(start);
        }
        auto start = std::chrono::steady_clock::now();
        PackingResult b = packConvoysBruteForce(items, convoys);
        double bruteTime = microsSince(start);

        bool match = d.totalValue == b.totalValue && packingIsValid(items, convoys, d);
        allMatch = allMatch && match;
        std::cout << std::fixed << std::setprecision(0)
                  << std::setw(4) << n << std::setw(9) << d.operations << std::setw(11) << dpTime / runs
                  << std::setw(20) << b.states << std::setw(11) << bruteTime
                  << "   " << (match ? "yes" : "NO") << "\n";
        std::cout.unsetf(std::ios::floatfield);
    }
    std::cout << "DP ops grow linearly in N (N * 4 * 21*16*11); brute force multiplies by 4 per item.\n";
    check(allMatch, "DP equals brute force for every N");
}

static void testIncremental() {
    title("TEST 7: Incremental MST update vs full rebuild");
    std::cout << "Note: the incremental update reuses the road list sorted once in build(),\n"
                 "so it never sorts again; the rebuild pays for the sort every time.\n";

    subtitle("worked example: 3 tree roads destroyed during a wave");
    Dataset data = load("worked_example.txt");
    MstPlanner planner(data);
    planner.build();
    std::vector<int> tree = planner.getResult().treeRoads;
    std::vector<int> order = {tree[0], tree[1], tree[2]};
    std::vector<IncrementalRow> rows = compareIncremental(data, planner, order, 200);
    printIncrementalRows(rows, true);
    check(allSame(rows), "after every destroyed road: same tree and cost as a full rebuild");
    check(planner.getResult().unreachable.empty(), "every outpost still reachable (replacements found)");

    subtitle("worked example: road not in tree, then Westgate cut off");
    MstPlanner planner2(data);
    planner2.build();
    std::vector<int> order2 = {roadIndexById(data, 14), roadIndexById(data, 17), roadIndexById(data, 18)};
    std::vector<IncrementalRow> rows2 = compareIncremental(data, planner2, order2, 200);
    printIncrementalRows(rows2, true);
    check(!rows2[0].wasInTree && rows2[0].incrementalOps == 1, "non-tree road: O(1), nothing to do");
    check(rows2[2].replacement == "none", "last road has no replacement: tree splits");
    check(unreachableNames(data, planner2) == std::set<std::string>{"Westgate"}, "Westgate reported unreachable");
    check(allSame(rows2), "matches full rebuild after every road");

    struct Size { int n, m, runs; };
    for (Size s : {Size{200, 1000, 50}, Size{1000, 5000, 20}}) {
        subtitle("random graph n=" + std::to_string(s.n) + " m=" + std::to_string(s.m) +
                 ", 3 tree roads destroyed (time averaged over " + std::to_string(s.runs) + " runs)");
        Dataset big = randomDataset(s.n, s.m, 7);
        MstPlanner bigPlanner(big);
        bigPlanner.build();
        std::vector<int> t = bigPlanner.getResult().treeRoads;
        std::vector<int> bigOrder = {t[t.size() / 4], t[t.size() / 2], t[3 * t.size() / 4]};
        std::vector<IncrementalRow> bigRows = compareIncremental(big, bigPlanner, bigOrder, s.runs);
        printIncrementalRows(bigRows, false);
        check(allSame(bigRows), "same tree and cost as a full rebuild");
    }
}

static void testSensitivity() {
    title("TEST 8: Sensitivity - remove the 2 highest-severity outposts");
    Dataset data = load("worked_example.txt");
    PipelineReport before = runPipeline(data);

    std::vector<int> ranked;
    for (int i = 0; i < static_cast<int>(data.outposts.size()); i++)
        if (i != data.castle) ranked.push_back(i);
    std::sort(ranked.begin(), ranked.end(), [&](int a, int b) {
        if (data.outposts[a].severity != data.outposts[b].severity)
            return data.outposts[a].severity > data.outposts[b].severity;
        return data.outposts[a].id < data.outposts[b].id;
    });
    std::vector<int> removed = {ranked[0], ranked[1]};
    std::cout << "Removing: " << data.outposts[removed[0]].name << " (severity " << data.outposts[removed[0]].severity
              << ") and " << data.outposts[removed[1]].name << " (severity " << data.outposts[removed[1]].severity << ")\n";

    Dataset smaller = withoutOutposts(data, removed);
    PipelineReport after = runPipeline(smaller);
    subtitle("pipeline after removal");
    printPipelineReport(smaller, after, std::cout);

    subtitle("before vs after");
    const MstResult& a = before.planner.getResult();
    const MstResult& b = after.planner.getResult();
    std::cout << "                     before   after\n"
              << "Outposts           " << std::setw(8) << data.outposts.size() << std::setw(8) << smaller.outposts.size() << "\n"
              << "Roads              " << std::setw(8) << data.roads.size() << std::setw(8) << smaller.roads.size() << "\n"
              << "MST cost           " << std::setw(8) << a.totalCost << std::setw(8) << b.totalCost << "\n"
              << "Unreachable        " << std::setw(8) << a.unreachable.size() << std::setw(8) << b.unreachable.size() << "\n"
              << "Requests           " << std::setw(8) << before.deliverable.size() << std::setw(8) << after.deliverable.size() << "\n"
              << "Demand weight      " << std::setw(8) << totalWeight(before.deliverable) << std::setw(8) << totalWeight(after.deliverable) << "\n"
              << "Value delivered    " << std::setw(8) << before.packing.totalValue << std::setw(8) << after.packing.totalValue << "\n";
    for (int j = 0; j < CONVOY_COUNT; j++)
        std::cout << std::left << std::setw(19) << (data.convoys[j].name + " load") << std::right
                  << std::setw(8) << before.packing.load[j] << std::setw(8) << after.packing.load[j] << "\n";

    std::set<int> idsBefore, idsAfter;
    for (int r : a.treeRoads) idsBefore.insert(data.roads[r].id);
    for (int r : b.treeRoads) idsAfter.insert(smaller.roads[r].id);
    auto printDifference = [](const std::string& label, const std::set<int>& from, const std::set<int>& other) {
        std::cout << label;
        bool any = false;
        for (int id : from) {
            if (other.count(id)) continue;
            std::cout << " R" << id;
            any = true;
        }
        std::cout << (any ? "\n" : " none\n");
    };
    printDifference("Tree roads dropped:", idsBefore, idsAfter);
    printDifference("Tree roads added:  ", idsAfter, idsBefore);

    subtitle("checks");
    check(smaller.outposts.size() == data.outposts.size() - 2, "2 outposts removed");
    check(smaller.outposts[smaller.castle].name == "Castle", "Castle kept");
    check(packingIsValid(after.deliverable, smaller.convoys, after.packing), "packing after removal is valid");
}

static void testTableLimit() {
    title("TEST 9: DP table over the 50M-cell limit");
    std::vector<Convoy> huge = {{0, "Wagon", 400}, {1, "Cart", 400}, {2, "Mule", 400}};
    std::vector<SupplyItem> items = randomItems(5, 9);
    long long cells = (static_cast<long long>(items.size()) + 1) * 401 * 401 * 401;
    std::cout << "Capacities 400/400/400 with " << items.size() << " items need " << cells << " cells\n";

    PackingResult result = packConvoysDP(items, huge);
    std::cout << "Result: " << (result.ok ? "ok" : "refused") << " - " << result.error << "\n";

    subtitle("checks");
    check(!result.ok, "DP refuses instead of crashing");
    check(result.error.find("too large") != std::string::npos, "error message explains the table is too large");
}

static void testInputValidation() {
    title("TEST 10: Input validation (bad data files are rejected with a line number)");
    struct Case { std::string what, text; };
    std::vector<Case> cases = {
        {"road to an unknown outpost",
         "CASTLE 0\nOUTPOSTS\n0 Castle 19 7 0 800\n1 Ironhold 16 3 7 300\n"
         "ROADS\n0 0 1 4 0\n1 1 99 3 0\nCONVOYS\n0 A 5\n1 B 4\n2 C 3\n"},
        {"Castle with severity above 0",
         "CASTLE 0\nOUTPOSTS\n0 Castle 19 7 5 800\nCONVOYS\n0 A 5\n1 B 4\n2 C 3\n"},
        {"only two convoys",
         "CASTLE 0\nOUTPOSTS\n0 Castle 19 7 0 800\nCONVOYS\n0 A 5\n1 B 4\n"},
    };
    subtitle("checks");
    for (const Case& c : cases) {
        std::istringstream in(c.text);
        Dataset d;
        std::string error;
        bool ok = parseDataset(in, d, error);
        check(!ok, c.what + " -> \"" + error + "\"");
    }
}

static SupplyNetwork loadNetwork() {
    SupplyNetwork network;
    std::string error;
    if (!network.load(dataFolder + "/worked_example.txt", error)) {
        std::cout << "Cannot load worked_example.txt: " << error << "\n";
        std::exit(1);
    }
    return network;
}

static std::vector<std::pair<int, int>> straightPath() {
    std::vector<std::pair<int, int>> path;
    for (int x = 0; x <= 19; x++) path.push_back({x, 7});
    return path;
}

static double distanceToPath(const Dataset& data, const Road& road, const std::vector<std::pair<int, int>>& path) {
    double ax = data.outposts[road.from].gridX, ay = data.outposts[road.from].gridY;
    double bx = data.outposts[road.to].gridX, by = data.outposts[road.to].gridY;
    double best = 1e18;
    for (auto [px, py] : path) {
        double dx = bx - ax, dy = by - ay;
        double t = std::max(0.0, std::min(1.0, ((px - ax) * dx + (py - ay) * dy) / (dx * dx + dy * dy)));
        best = std::min(best, std::hypot(px - (ax + t * dx), py - (ay + t * dy)));
    }
    return best;
}

static const char* effectName(int effect) {
    return effect > 0 ? "BUFF +25%" : effect < 0 ? "PENALTY -25%" : "none";
}

static void testRaidRule() {
    title("TEST 11: Raid rule - Castle roads protected, then closest to the enemy path");
    std::cout << "Enemy path: straight along row 7 from (0,7) to the Castle (19,7).\n"
                 "Tiers: 0 = tree road not touching the Castle, 1 = tree road touching the Castle,\n"
                 "       2 = unused road not touching the Castle, 3 = unused road touching the Castle.\n\n";
    SupplyNetwork network = loadNetwork();
    const Dataset& data = network.getData();
    std::vector<std::pair<int, int>> path = straightPath();

    int unblockedAtStart = 0;
    for (const Road& r : data.roads) if (!r.blocked) unblockedAtStart++;

    bool ruleOk = true, rebuildOk = true;
    int firstCastleRaid = -1, castleCutOffAt = -1, raids = 0;
    std::cout << "  #  destroyed                  tier  dist  replacement                cut off\n";
    for (int road = network.nextRoadToDestroy(path); road >= 0; road = network.nextRoadToDestroy(path)) {
        const std::vector<Road>& roads = network.getPlanner().getRoads();
        int tier = network.raidTier(road);
        double dist = distanceToPath(data, roads[road], path);

        for (int other = 0; other < static_cast<int>(roads.size()); other++) {
            if (other == road || roads[other].blocked) continue;
            int t = network.raidTier(other);
            double d = distanceToPath(data, roads[other], path);
            bool beats = t < tier || (t == tier && d < dist - 1e-9) ||
                         (t == tier && std::fabs(d - dist) <= 1e-9 && roads[other].id < roads[road].id);
            if (beats) ruleOk = false;
        }
        if (tier == 1 && firstCastleRaid < 0) firstCastleRaid = raids + 1;

        RoadDestroyedEvent e = network.destroyRoad(road);
        rebuildOk = rebuildOk && e.matchesRebuild;
        raids++;
        int cutOff = static_cast<int>(network.getPlanner().getResult().unreachable.size());
        if (cutOff == static_cast<int>(data.outposts.size()) - 1 && castleCutOffAt < 0) castleCutOffAt = raids;

        std::string replacement = e.replacement >= 0 ? roadLabel(data, roads[e.replacement]) : (e.wasInTree ? "none (split)" : "-");
        std::cout << std::setw(3) << raids << "  " << std::left << std::setw(27) << roadLabel(data, roads[road])
                  << std::right << std::setw(4) << tier << std::fixed << std::setprecision(2) << std::setw(6) << dist
                  << "  " << std::left << std::setw(27) << replacement << std::right << std::setw(4) << cutOff << "\n";
        std::cout.unsetf(std::ios::floatfield);
    }
    std::cout << "\nFirst Castle road raided at raid #" << firstCastleRaid
              << "; Castle cut off from every outpost after raid #" << castleCutOffAt << "\n";

    subtitle("checks");
    check(ruleOk, "every raid picked the lowest (tier, distance, road id)");
    check(firstCastleRaid > 0, "Castle roads are raided only once no other tree road is left");
    check(rebuildOk, "every incremental update matches a full rebuild");
    check(raids == unblockedAtStart, "raids continue until all " + std::to_string(unblockedAtStart) + " unblocked roads are destroyed");
}

static void testBuffRule() {
    title("TEST 12: Buff / penalty rule (nearest outpost within 3 tiles, Castle excluded)");
    SupplyNetwork network = loadNetwork();
    const Dataset& data = network.getData();

    struct Probe { int x, y; int expected; std::string why; };
    std::vector<Probe> probes = {
        {16, 4, SupplyNetwork::EFFECT_BUFF, "next to Ironhold (supplied)"},
        {16, 0, SupplyNetwork::EFFECT_BUFF, "exactly 3 tiles from Ironhold (radius is inclusive)"},
        {6, 6, SupplyNetwork::EFFECT_NONE, "next to Duskmoor (connected, nothing delivered)"},
        {18, 7, SupplyNetwork::EFFECT_NONE, "next to the Castle only (Castle does not count)"},
        {0, 14, SupplyNetwork::EFFECT_NONE, "no outpost within 3 tiles"},
        {8, 12, SupplyNetwork::EFFECT_BUFF, "next to Fenmarsh (supplied with a Medkit)"},
    };
    subtitle("fresh map");
    for (const Probe& p : probes) {
        int effect = network.supplyEffectAt(p.x, p.y);
        check(effect == p.expected, "tile (" + std::to_string(p.x) + "," + std::to_string(p.y) + ") " + p.why +
                                    " -> " + effectName(effect));
    }

    subtitle("Fenmarsh cut off (R15, R16 destroyed)");
    network.destroyRoad(roadIndexById(data, 15));
    network.destroyRoad(roadIndexById(data, 16));
    int fenmarsh = 10;
    check(!network.getPlanner().isConnectedToCastle(fenmarsh), "Fenmarsh is unreachable");
    check(network.supplyEffectAt(8, 12) == SupplyNetwork::EFFECT_BUFF,
          "before the next supply phase the tower keeps its BUFF (effects last a whole wave)");
    network.runSupplyPhase();
    check(network.supplyEffectAt(8, 12) == SupplyNetwork::EFFECT_PENALTY, "after the supply phase -> PENALTY -25%");
}

static void testSupplyPhaseFilter() {
    title("TEST 13: Supply phase uses only requests from outposts connected to the Castle");
    SupplyNetwork network = loadNetwork();
    const Dataset& data = network.getData();
    for (int id : {15, 16, 17, 18}) network.destroyRoad(roadIndexById(data, id));
    network.runSupplyPhase();
    const PipelineReport& report = network.getReport();
    printPipelineReport(data, report, std::cout);

    std::set<int> cutOffIds;
    for (const SupplyItem& it : report.cutOff) cutOffIds.insert(it.id);
    bool deliverableOk = true;
    for (const SupplyItem& it : report.deliverable)
        deliverableOk = deliverableOk && network.getPlanner().isConnectedToCastle(it.outpost);
    std::vector<SupplyItem> manual;
    for (const SupplyItem& it : data.items)
        if (network.getPlanner().isConnectedToCastle(it.outpost)) manual.push_back(it);
    PackingResult expected = packConvoysDP(manual, data.convoys);

    subtitle("checks");
    check(unreachableNames(data, network.getPlanner()) == std::set<std::string>{"Fenmarsh", "Westgate"},
          "unreachable = Fenmarsh, Westgate");
    check(cutOffIds == std::set<int>{13, 14, 15, 16}, "their 4 requests (I13-I16) are cut off");
    check(deliverableOk, "every request given to the DP comes from a connected outpost");
    check(packingIsValid(report.deliverable, data.convoys, report.packing), "packing is valid");
    check(report.packing.totalValue == expected.totalValue, "value equals the DP on a hand-filtered list (" +
                                                            std::to_string(expected.totalValue) + ")");
    check(network.getOutpostEffect(10) == SupplyNetwork::EFFECT_PENALTY &&
          network.getOutpostEffect(11) == SupplyNetwork::EFFECT_PENALTY, "both cut-off outposts give a PENALTY");
}

static void testNetworkReset() {
    title("TEST 14: Restart resets the supply map");
    SupplyNetwork network = loadNetwork();
    SupplyNetwork fresh = loadNetwork();
    const Dataset& data = network.getData();
    std::vector<std::pair<int, int>> path = straightPath();
    for (int i = 0; i < 8; i++) network.destroyRoad(network.nextRoadToDestroy(path));
    network.runSupplyPhase();
    std::cout << "After 8 raids: tree cost " << network.getPlanner().getResult().totalCost << ", "
              << network.getPlanner().getResult().unreachable.size() << " cut off, value "
              << network.getReport().packing.totalValue << "\n";

    network.reset();
    std::cout << "After reset:   tree cost " << network.getPlanner().getResult().totalCost << ", "
              << network.getPlanner().getResult().unreachable.size() << " cut off, value "
              << network.getReport().packing.totalValue << "\n";

    int blocked = 0;
    for (const Road& r : network.getPlanner().getRoads()) if (r.blocked) blocked++;
    bool sameEffects = true;
    for (int o = 0; o < static_cast<int>(data.outposts.size()); o++)
        sameEffects = sameEffects && network.getOutpostEffect(o) == fresh.getOutpostEffect(o);

    subtitle("checks");
    check(network.getPlanner().getResult().totalCost == 45, "tree cost back to 45");
    check(blocked == 2, "only the 2 roads blocked in the data file are blocked");
    check(network.getDestroyedCount() == 0 && !network.hasLastEvent(), "raid counter and last raid cleared");
    check(network.getReport().packing.totalValue == 269, "supply value back to 269");
    check(sameEffects, "every outpost has the same effect as on a freshly loaded map");
}

static void testTraces() {
    title("TEST 15: Visualisation traces agree with the algorithms");
    std::cout << "The game's Kruskal / raid / convoy views replay these traces; they must\n"
                 "show exactly what MstPlanner and the DP did.\n";

    bool kruskalOk = true;
    for (const std::string& file : {std::string("worked_example.txt"), std::string("disconnected.txt")}) {
        Dataset data = load(file);
        MstPlanner planner(data);
        planner.build();
        KruskalTrace trace = traceKruskal(data, planner.getRoads());
        std::vector<int> added;
        for (const KruskalStep& s : trace.steps) if (s.added) added.push_back(s.road);
        std::set<int> groups(trace.steps.back().group.begin(), trace.steps.back().group.end());
        std::cout << "  " << file << ": " << trace.steps.size() << " steps, " << added.size()
                  << " added, cost " << trace.totalCost << ", " << groups.size() << " group(s) at the end\n";
        kruskalOk = kruskalOk && sortedCopy(added) == sortedCopy(planner.getResult().treeRoads) &&
                    trace.totalCost == planner.getResult().totalCost &&
                    static_cast<int>(trace.steps.size()) == planner.getExaminedCount() &&
                    static_cast<int>(groups.size()) == planner.getResult().componentCount;
    }

    SupplyNetwork network = loadNetwork();
    std::vector<std::pair<int, int>> path = straightPath();
    bool raidOk = true;
    int raids = 0;
    for (int road = network.nextRoadToDestroy(path); road >= 0; road = network.nextRoadToDestroy(path)) {
        RaidTrace trace = traceRaid(network.getPlanner(), road);
        RoadDestroyedEvent e = network.destroyRoad(road);
        raidOk = raidOk && trace.wasInTree == e.wasInTree && trace.replacement == e.replacement;
        raids++;
    }
    std::cout << "  " << raids << " raids replayed\n";

    SupplyNetwork fresh = loadNetwork();
    std::vector<LoadStep> load = traceConvoyLoading(fresh.getData(), fresh.getReport());
    int value = 0, loaded = 0;
    for (const LoadStep& s : load)
        if (s.convoy >= 0) { value += s.item.value; loaded++; }
    std::cout << "  convoy trace: " << loaded << " items loaded, value " << value << "\n";

    subtitle("checks");
    check(kruskalOk, "Kruskal trace adds the same roads, cost and groups as MstPlanner::build()");
    check(raidOk, "raid trace picks the same replacement as MstPlanner::destroyRoad() in all " +
                  std::to_string(raids) + " raids");
    check(value == fresh.getReport().packing.totalValue && value == 269, "convoy trace loads value 269 like the DP");
}

int main(int argc, char** argv) {
    if (argc > 1) dataFolder = argv[1];

    testWorkedExample();
    testTieBreak();
    testDisconnected();
    testOverDemand();
    testTooHeavy();
    testDpVsBruteForce();
    testIncremental();
    testSensitivity();
    testTableLimit();
    testInputValidation();
    testRaidRule();
    testBuffRule();
    testSupplyPhaseFilter();
    testNetworkReset();
    testTraces();

    title("SUMMARY");
    std::cout << passed << " checks passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
