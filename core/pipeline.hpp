#pragma once
#include <ostream>
#include <string>
#include <vector>
#include "dataset.hpp"
#include "mst_planner.hpp"
#include "knapsack_allocator.hpp"

struct PipelineReport {
    MstPlanner planner;
    std::vector<SupplyItem> deliverable;
    std::vector<SupplyItem> cutOff;
    PackingResult packing;

    explicit PipelineReport(const Dataset& data) : planner(data) {}
};

PipelineReport runPipeline(const Dataset& data);

void allocateSupplies(const Dataset& data, PipelineReport& report);

std::string roadLabel(const Dataset& data, const Road& road);
void printMst(const Dataset& data, const MstPlanner& planner, std::ostream& out);
void printPacking(const Dataset& data, const std::vector<SupplyItem>& items,
                  const PackingResult& packing, std::ostream& out);
void printPipelineReport(const Dataset& data, const PipelineReport& report, std::ostream& out);
