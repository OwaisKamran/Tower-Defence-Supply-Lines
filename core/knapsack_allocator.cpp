#include "knapsack_allocator.hpp"
#include <algorithm>

static const long long MAX_TABLE_CELLS = 50000000;

static void sumConvoyTotals(const std::vector<SupplyItem>& items, PackingResult& result) {
    for (int j = 0; j < CONVOY_COUNT; j++) {
        result.load[j] = 0;
        result.value[j] = 0;
    }
    for (size_t i = 0; i < items.size(); i++) {
        int j = result.assignment[i];
        if (j < 0) continue;
        result.load[j] += items[i].weight;
        result.value[j] += items[i].value;
    }
}

PackingResult packConvoysDP(const std::vector<SupplyItem>& items, const std::vector<Convoy>& convoys) {
    PackingResult result;
    if (static_cast<int>(convoys.size()) != CONVOY_COUNT) {
        result.error = "exactly 3 convoys are required";
        return result;
    }

    const int N = static_cast<int>(items.size());
    const int C1 = convoys[0].capacity;
    const int C2 = convoys[1].capacity;
    const int C3 = convoys[2].capacity;

    const long long layer = 1LL * (C1 + 1) * (C2 + 1) * (C3 + 1);
    if ((N + 1) * layer > MAX_TABLE_CELLS) {
        result.error = "DP table too large; reduce capacities or items";
        return result;
    }

    auto cell = [&](int i, int c1, int c2, int c3) {
        return ((1LL * i * (C1 + 1) + c1) * (C2 + 1) + c2) * (C3 + 1) + c3;
    };

    std::vector<int> dp((N + 1) * layer, 0);
    result.states = (N + 1) * layer;

    for (int i = N - 1; i >= 0; i--) {
        const int w = items[i].weight;
        const int v = items[i].value;
        for (int c1 = 0; c1 <= C1; c1++) {
            for (int c2 = 0; c2 <= C2; c2++) {
                for (int c3 = 0; c3 <= C3; c3++) {
                    int best = dp[cell(i + 1, c1, c2, c3)];
                    if (w <= c1) best = std::max(best, v + dp[cell(i + 1, c1 - w, c2, c3)]);
                    if (w <= c2) best = std::max(best, v + dp[cell(i + 1, c1, c2 - w, c3)]);
                    if (w <= c3) best = std::max(best, v + dp[cell(i + 1, c1, c2, c3 - w)]);
                    dp[cell(i, c1, c2, c3)] = best;
                    result.operations += CONVOY_COUNT + 1;
                }
            }
        }
    }

    result.totalValue = dp[cell(0, C1, C2, C3)];

    result.assignment.assign(N, -1);
    int left[CONVOY_COUNT] = {C1, C2, C3};
    for (int i = 0; i < N; i++) {
        int here = dp[cell(i, left[0], left[1], left[2])];
        if (here == dp[cell(i + 1, left[0], left[1], left[2])]) continue;

        for (int j = 0; j < CONVOY_COUNT; j++) {
            if (items[i].weight > left[j]) continue;
            int next[CONVOY_COUNT] = {left[0], left[1], left[2]};
            next[j] -= items[i].weight;
            if (here == items[i].value + dp[cell(i + 1, next[0], next[1], next[2])]) {
                result.assignment[i] = j;
                left[j] = next[j];
                break;
            }
        }
    }

    sumConvoyTotals(items, result);
    result.ok = true;
    return result;
}

static void tryAll(const std::vector<SupplyItem>& items, int i, int left[], int value,
                   std::vector<int>& current, PackingResult& best) {
    best.operations++;
    if (i == static_cast<int>(items.size())) {
        best.states++;
        bool fits = left[0] >= 0 && left[1] >= 0 && left[2] >= 0;
        if (fits && value > best.totalValue) {
            best.totalValue = value;
            best.assignment = current;
        }
        return;
    }

    current[i] = -1;
    tryAll(items, i + 1, left, value, current, best);

    for (int j = 0; j < CONVOY_COUNT; j++) {
        current[i] = j;
        left[j] -= items[i].weight;
        tryAll(items, i + 1, left, value + items[i].value, current, best);
        left[j] += items[i].weight;
    }
    current[i] = -1;
}

PackingResult packConvoysBruteForce(const std::vector<SupplyItem>& items, const std::vector<Convoy>& convoys) {
    PackingResult result;
    if (static_cast<int>(convoys.size()) != CONVOY_COUNT) {
        result.error = "exactly 3 convoys are required";
        return result;
    }
    if (static_cast<int>(items.size()) > BRUTE_FORCE_MAX_ITEMS) {
        result.error = "too many items for brute force";
        return result;
    }

    int left[CONVOY_COUNT] = {convoys[0].capacity, convoys[1].capacity, convoys[2].capacity};
    std::vector<int> current(items.size(), -1);
    result.assignment.assign(items.size(), -1);
    tryAll(items, 0, left, 0, current, result);

    sumConvoyTotals(items, result);
    result.ok = true;
    return result;
}
