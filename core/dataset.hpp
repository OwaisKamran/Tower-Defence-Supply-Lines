#pragma once
#include <string>
#include <vector>

const int CONVOY_COUNT = 3;

struct Outpost {
    int id;
    std::string name;
    int gridX, gridY;
    int severity;
    int population;
};

struct Road {
    int id;
    int from, to;
    int cost;
    bool blocked;
};

struct Convoy {
    int id;
    std::string name;
    int capacity;
};

struct SupplyItem {
    int id;
    std::string name;
    int weight;
    int value;
    int outpost;
};

struct Dataset {
    std::vector<Outpost> outposts;
    std::vector<Road> roads;
    std::vector<Convoy> convoys;
    std::vector<SupplyItem> items;
    int castle = -1;

    int roadSeverity(const Road& road) const {
        return outposts[road.from].severity + outposts[road.to].severity;
    }
};
