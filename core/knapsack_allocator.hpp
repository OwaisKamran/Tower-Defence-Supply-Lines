#pragma once
#include <string>
#include <vector>
#include "dataset.hpp"

struct PackingResult {
    bool ok = false;
    std::string error;
    int totalValue = 0;
    std::vector<int> assignment;
    int load[CONVOY_COUNT] = {0, 0, 0};
    int value[CONVOY_COUNT] = {0, 0, 0};
    long long states = 0;
    long long operations = 0;
};

PackingResult packConvoysDP(const std::vector<SupplyItem>& items, const std::vector<Convoy>& convoys);

const int BRUTE_FORCE_MAX_ITEMS = 12;
PackingResult packConvoysBruteForce(const std::vector<SupplyItem>& items, const std::vector<Convoy>& convoys);
