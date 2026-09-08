#include "KeypointTracker.h"

#include <vector>

namespace {
Branch makeTip(int id, Vec3 point) {
    Branch branch{};
    branch.ID = id;
    branch.Dynamic = true;
    branch.Points.push_back(point);
    return branch;
}
}

int main() {
    SimulationParameters params{};
    params.DumpKeypoints = true;
    params.SimulationName = "TrackerUnit";

    std::vector<Branch> branches{makeTip(0, {1.0, 1.0, 0.0})};
    KeypointTracker tracker;
    tracker.initialize(branches, params, 1);
    tracker.recordLifecycle(branches, 0, 0.0);
    tracker.writeSnapshot(branches, 0, 0.0);

    Branch parent = branches[0];
    parent.Dynamic = false;
    parent.Child1_ID = 1;
    parent.Child2_ID = 2;
    parent.Points.back() = {0.5, 0.5, 0.0};
    Branch continuation = makeTip(1, {1.5, 1.0, 0.0});
    Branch daughter = makeTip(2, {0.5, 1.5, 0.0});
    branches = {parent, continuation, daughter};
    tracker.assignAfterBranching(branches);
    tracker.reconcileAfterTopology(branches);
    tracker.recordLifecycle(branches, 1, 1.0);
    tracker.writeSnapshot(branches, 1, 1.0);

    continuation = branches[1];
    continuation.ID = 0;
    continuation.Points.back() = {2.0, 1.0, 0.0};
    branches = {continuation};
    tracker.reconcileAfterTopology(branches);
    tracker.recordLifecycle(branches, 2, 2.0);
    tracker.writeSnapshot(branches, 2, 2.0);
}
