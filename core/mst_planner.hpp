#pragma once
#include <vector>
#include "dataset.hpp"

struct MstResult {
    std::vector<int> treeRoads;
    long long totalCost = 0;
    int componentCount = 0;
    std::vector<int> component;
    std::vector<int> unreachable;
    long long operations = 0;
};

struct RoadDestroyedReport {
    int road = -1;
    bool wasInTree = false;
    int replacement = -1;
    long long operations = 0;
};

class MstPlanner {
public:
    explicit MstPlanner(const Dataset& data);

    const MstResult& build();

    RoadDestroyedReport destroyRoad(int road);

    const MstResult& getResult() const { return current; }
    const std::vector<Road>& getRoads() const { return roads; }
    const std::vector<int>& getSortedRoads() const { return sortedRoads; }
    int getExaminedCount() const { return examinedCount; }
    bool isInTree(int road) const { return inTree[road]; }
    bool isConnectedToCastle(int outpost) const;

private:
    std::vector<Outpost> outposts;
    std::vector<Road> roads;
    int castle;

    std::vector<int> sortedRoads;
    int examinedCount = 0;
    std::vector<bool> inTree;
    std::vector<std::vector<int>> treeAdj;
    MstResult current;

    bool comesBefore(int a, int b) const;
    int otherEnd(int road, int outpost) const;
    void addToTree(int road);
    void removeFromTree(int road);
    void labelComponents(long long& ops);
    void updateUnreachable(long long& ops);
};
