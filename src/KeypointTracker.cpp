#include "KeypointTracker.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace {
void requireStream(const std::ofstream& stream, const std::string& filename) {
    if (!stream) throw std::runtime_error("Cannot write keypoint file: " + filename);
}

template <typename Map>
std::vector<std::int64_t> sortedIDs(const Map& observations) {
    std::vector<std::int64_t> ids;
    ids.reserve(observations.size());
    for (const auto& item : observations) ids.push_back(item.first);
    std::sort(ids.begin(), ids.end());
    return ids;
}
}

void KeypointTracker::initialize(std::vector<Branch>& branches,
                                 const SimulationParameters& params,
                                 int sample) {
    enabled_ = params.DumpKeypoints;
    sample_ = sample;
    if (!enabled_) return;

    snapshotFilename_ = params.SimulationName + "-KeypointSnapshots-Sample-" +
                        std::to_string(sample) + ".csv";
    eventFilename_ = params.SimulationName + "-KeypointEvents-Sample-" +
                     std::to_string(sample) + ".csv";
    lineageFilename_ = params.SimulationName + "-KeypointLineage-Sample-" +
                       std::to_string(sample) + ".csv";
    branchingMode_ = params.Bifurcation ? "bifurcation" : "side_branching";

    for (Branch& branch : branches) {
        if (branch.Dynamic) branch.PersistentTipID = nextTipID_++;
        else branch.PersistentJunctionID = nextJunctionID_++;
    }

    std::ofstream snapshots(snapshotFilename_, std::ios::trunc);
    requireStream(snapshots, snapshotFilename_);
    snapshots << "sample,timestep,time,keypoint_type,persistent_id,x,y,z,state,birth_timestep\n";

    std::ofstream events(eventFilename_, std::ios::trunc);
    requireStream(events, eventFilename_);
    events << "sample,timestep,time,keypoint_type,persistent_id,event,x,y,z,state\n";

    std::ofstream lineage(lineageFilename_, std::ios::trunc);
    requireStream(lineage, lineageFilename_);
    lineage << "sample,timestep,time,branching_mode,source_tip_id,junction_id,"
               "continuing_tip_id,new_tip_id,x,y,z\n";
}

void KeypointTracker::assignAfterBranching(std::vector<Branch>& branches,
                                           std::size_t timestep,
                                           double time) {
    if (!enabled_) return;
    for (Branch& parent : branches) {
        if (parent.Dynamic || parent.PersistentTipID < 0) continue;
        if (parent.Points.empty()) continue;
        if (parent.Child1_ID < 0 || parent.Child2_ID < 0) continue;
        if (parent.Child1_ID >= static_cast<int>(branches.size()) ||
            parent.Child2_ID >= static_cast<int>(branches.size())) continue;

        const std::int64_t sourceTipID = parent.PersistentTipID;
        Branch& continuation = branches[parent.Child1_ID];
        Branch& newDaughter = branches[parent.Child2_ID];
        if (continuation.PersistentTipID < 0)
            continuation.PersistentTipID = parent.PersistentTipID;
        if (newDaughter.PersistentTipID < 0)
            newDaughter.PersistentTipID = nextTipID_++;
        if (parent.PersistentJunctionID < 0)
            parent.PersistentJunctionID = nextJunctionID_++;

        std::ofstream lineage(lineageFilename_, std::ios::app);
        requireStream(lineage, lineageFilename_);
        lineage << std::setprecision(17)
                << sample_ << ',' << timestep << ',' << time << ',' << branchingMode_ << ','
                << sourceTipID << ',' << parent.PersistentJunctionID << ','
                << continuation.PersistentTipID << ',' << newDaughter.PersistentTipID << ','
                << parent.Points.back().x << ',' << parent.Points.back().y << ','
                << parent.Points.back().z << '\n';
        parent.PersistentTipID = -1;
    }
}

void KeypointTracker::reconcileAfterTopology(std::vector<Branch>& branches) {
    if (!enabled_) return;
    for (Branch& branch : branches) {
        if (branch.Dynamic) {
            branch.PersistentJunctionID = -1;
            if (branch.PersistentTipID < 0) branch.PersistentTipID = nextTipID_++;
        } else {
            branch.PersistentTipID = -1;
            if (branch.PersistentJunctionID < 0)
                branch.PersistentJunctionID = nextJunctionID_++;
        }
    }
}

KeypointTracker::Observations KeypointTracker::observeTips(const std::vector<Branch>& branches) {
    Observations result;
    for (const Branch& branch : branches) {
        if (!branch.Dynamic || branch.PersistentTipID < 0 || branch.Points.empty()) continue;
        result.emplace(branch.PersistentTipID, Observation{branch.Points.back(), branch.State});
    }
    return result;
}

KeypointTracker::Observations KeypointTracker::observeJunctions(const std::vector<Branch>& branches) {
    Observations result;
    for (const Branch& branch : branches) {
        if (branch.Dynamic || branch.PersistentJunctionID < 0 || branch.Points.empty()) continue;
        result.emplace(branch.PersistentJunctionID, Observation{branch.Points.back(), branch.State});
    }
    return result;
}

void KeypointTracker::recordType(
    const char* type,
    const Observations& current,
    Observations& previous,
    std::unordered_map<std::int64_t, std::size_t>& births,
    std::size_t timestep,
    double time) {
    std::ofstream events(eventFilename_, std::ios::app);
    requireStream(events, eventFilename_);
    events << std::setprecision(17);

    for (const std::int64_t id : sortedIDs(current)) {
        const Observation& observation = current.at(id);
        if (previous.find(id) != previous.end()) continue;
        births.emplace(id, timestep);
        events << sample_ << ',' << timestep << ',' << time << ',' << type << ',' << id
               << ",birth," << observation.position.x << ',' << observation.position.y << ','
               << observation.position.z << ',' << observation.state << '\n';
    }
    for (const std::int64_t id : sortedIDs(previous)) {
        const Observation& observation = previous.at(id);
        if (current.find(id) != current.end()) continue;
        events << sample_ << ',' << timestep << ',' << time << ',' << type << ',' << id
               << ",retirement," << observation.position.x << ',' << observation.position.y << ','
               << observation.position.z << ',' << observation.state << '\n';
    }
    previous = current;
}

void KeypointTracker::recordLifecycle(const std::vector<Branch>& branches,
                                      std::size_t timestep,
                                      double time) {
    if (!enabled_) return;
    const Observations tips = observeTips(branches);
    const Observations junctions = observeJunctions(branches);
    recordType("tip", tips, previousTips_, tipBirths_, timestep, time);
    recordType("junction", junctions, previousJunctions_, junctionBirths_, timestep, time);
}

void KeypointTracker::writeSnapshot(const std::vector<Branch>& branches,
                                    std::size_t timestep,
                                    double time) const {
    if (!enabled_) return;
    std::ofstream snapshots(snapshotFilename_, std::ios::app);
    requireStream(snapshots, snapshotFilename_);
    snapshots << std::setprecision(17);

    const auto write = [&](const char* type, const Observations& observations,
                           const auto& births) {
        for (const std::int64_t id : sortedIDs(observations)) {
            const Observation& observation = observations.at(id);
            const auto birth = births.find(id);
            if (birth == births.end())
                throw std::runtime_error("Missing birth timestep for active keypoint");
            snapshots << sample_ << ',' << timestep << ',' << time << ',' << type << ',' << id
                      << ',' << observation.position.x << ',' << observation.position.y << ','
                      << observation.position.z << ',' << observation.state << ','
                      << birth->second << '\n';
        }
    };
    write("tip", observeTips(branches), tipBirths_);
    write("junction", observeJunctions(branches), junctionBirths_);
}
