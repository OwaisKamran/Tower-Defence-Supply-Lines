#pragma once
#include <string>
#include <vector>
#include "dataset.hpp"
#include "mst_planner.hpp"
#include "pipeline.hpp"

struct KruskalStep {
    int road = -1;
    bool added = false;
    long long costSoFar = 0;
    std::vector<int> group;
};

struct KruskalTrace {
    std::vector<int> sortedRoads;
    std::vector<int> blockedRoads;
    std::vector<int> startGroup;
    std::vector<KruskalStep> steps;
    long long totalCost = 0;
};

KruskalTrace traceKruskal(const Dataset& data, const std::vector<Road>& roads);

enum class ScanVerdict {
    JustDestroyed,
    InTree,
    Blocked,
    SameSide,
    OtherTree,
    Chosen
};

struct RaidScanStep {
    int road = -1;
    ScanVerdict verdict = ScanVerdict::InTree;
};

struct RaidTrace {
    int road = -1;
    bool wasInTree = false;
    std::vector<int> side;
    std::vector<RaidScanStep> scan;
    int replacement = -1;
};

RaidTrace traceRaid(const MstPlanner& planner, int road);

enum class LeftReason {
    None,
    TooHeavy,
    NoSpace,
    CutOff
};

struct LoadStep {
    SupplyItem item;
    int convoy = -1;
    LeftReason reason = LeftReason::None;
};

std::vector<LoadStep> traceConvoyLoading(const Dataset& data, const PipelineReport& report);

std::string leftReasonText(LeftReason reason);
