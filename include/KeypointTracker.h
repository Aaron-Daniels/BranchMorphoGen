#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "Branch/BranchCommon.h"
#include "SimulationParameters.h"

class KeypointTracker {
public:
    void initialize(std::vector<Branch>& branches,
                    const SimulationParameters& params,
                    int sample);
    void assignAfterBranching(std::vector<Branch>& branches);
    void reconcileAfterTopology(std::vector<Branch>& branches);
    void recordLifecycle(const std::vector<Branch>& branches,
                         std::size_t timestep,
                         double time);
    void writeSnapshot(const std::vector<Branch>& branches,
                       std::size_t timestep,
                       double time) const;

private:
    struct Observation {
        Vec3 position;
        int state = 0;
    };

    using Observations = std::unordered_map<std::int64_t, Observation>;

    std::int64_t nextTipID_ = 0;
    std::int64_t nextJunctionID_ = 0;
    int sample_ = 0;
    bool enabled_ = false;
    std::string snapshotFilename_;
    std::string eventFilename_;
    Observations previousTips_;
    Observations previousJunctions_;
    std::unordered_map<std::int64_t, std::size_t> tipBirths_;
    std::unordered_map<std::int64_t, std::size_t> junctionBirths_;

    static Observations observeTips(const std::vector<Branch>& branches);
    static Observations observeJunctions(const std::vector<Branch>& branches);
    void recordType(const char* type,
                    const Observations& current,
                    Observations& previous,
                    std::unordered_map<std::int64_t, std::size_t>& births,
                    std::size_t timestep,
                    double time);
};
