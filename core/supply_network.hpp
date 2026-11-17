#pragma once
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include "dataset.hpp"
#include "pipeline.hpp"

struct RoadDestroyedEvent {
    int road = -1;
    int replacement = -1;
    bool wasInTree = false;
    long long incrementalOps = 0;
    long long rebuildOps = 0;
    bool matchesRebuild = false;
};

class SupplyNetwork {
public:
    static constexpr int EFFECT_PENALTY = -1;
    static constexpr int EFFECT_NONE = 0;
    static constexpr int EFFECT_BUFF = 1;
    static constexpr double EFFECT_RADIUS = 3.0;

    bool load(const std::string& path, std::string& error);
    bool isLoaded() const { return report.has_value(); }

    void reset();

    int nextRoadToDestroy(const std::vector<std::pair<int, int>>& enemyPath) const;
    int raidTier(int road) const;

    RoadDestroyedEvent destroyRoad(int road);

    void runSupplyPhase();

    int supplyEffectAt(int gridX, int gridY) const;

    const Dataset& getData() const { return data; }
    const PipelineReport& getReport() const { return *report; }
    const MstPlanner& getPlanner() const { return report->planner; }
    int getOutpostEffect(int outpost) const { return outpostEffect[outpost]; }
    bool hasLastEvent() const { return lastEvent.road >= 0; }
    const RoadDestroyedEvent& getLastEvent() const { return lastEvent; }
    int getDestroyedCount() const { return destroyedCount; }

private:
    Dataset data;
    std::optional<PipelineReport> report;
    std::vector<int> outpostEffect;
    RoadDestroyedEvent lastEvent;
    int destroyedCount = 0;
};
