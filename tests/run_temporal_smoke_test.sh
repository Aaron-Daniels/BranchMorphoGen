#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project_dir=$(CDPATH= cd -- "$script_dir/.." && pwd)
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/branchmorphogen-test.XXXXXX")
trap 'rm -rf "$test_dir"' EXIT INT TERM

mkdir "$test_dir/build"
clang++ -std=c++17 -O0 -g -Wall -Wextra -Wno-unused-parameter \
    -I"$project_dir/include" -I"$project_dir/include/Branch" \
    "$project_dir/tests/keypoint_tracker_test.cpp" \
    "$project_dir/src/KeypointTracker.cpp" \
    -o "$test_dir/build/keypoint_tracker_test"
clang++ -std=c++17 -O0 -g -Wall -Wextra -Wno-unused-parameter \
    -I"$project_dir/include" -I"$project_dir/include/Branch" \
    -I"$project_dir/third_party/eigen" -I/opt/homebrew/include \
    "$project_dir"/src/*.cpp "$project_dir"/src/Branch/*.cpp \
    -L/opt/homebrew/lib -ltiff -pthread -o "$test_dir/build/BranchMorphoGen"

(cd "$test_dir" && "$test_dir/build/keypoint_tracker_test")
python3 "$script_dir/validate_temporal_exports.py" \
    "$test_dir/TrackerUnit-KeypointEvents-Sample-1.csv" \
    "$test_dir/TrackerUnit-KeypointSnapshots-Sample-1.csv" \
    "$test_dir/TrackerUnit-KeypointLineage-Sample-1.csv"

sed \
    -e 's/^    NSample=.*/    NSample=2;/' \
    -e 's/^    RunParallel=.*/    RunParallel=false;/' \
    -e 's/^    RandomSeed=.*/    RandomSeed=1729;/' \
    -e 's/^    Time_Start =.*/    Time_Start = 0.0;/' \
    -e 's/^    Dt =.*/    Dt = 0.05;/' \
    -e 's/^    Time_End =.*/    Time_End = 0.10;/' \
    -e 's/^    N_SWC=.*/    N_SWC=3;/' \
    -e 's/^    DumpKeypoints=.*/    DumpKeypoints=true;/' \
    -e 's/^    SimulationName=.*/    SimulationName=TemporalTest;/' \
    -e 's/^    SelfAvoiding=.*/    SelfAvoiding=false;/' \
    -e 's/^    StraightenBranches=.*/    StraightenBranches=false;/' \
    -e 's/^    MAX_IMAGE_SIZE=.*/    MAX_IMAGE_SIZE=20;/' \
    -e 's/^    pixelsize=.*/    pixelsize=0.5;/' \
    "$project_dir/parameters.in" > "$test_dir/parameters.test.in"

mkdir "$test_dir/run-a" "$test_dir/run-b"
(cd "$test_dir/run-a" && "$test_dir/build/BranchMorphoGen" "$test_dir/parameters.test.in")
(cd "$test_dir/run-b" && "$test_dir/build/BranchMorphoGen" "$test_dir/parameters.test.in")

for sample in 1 2; do
    for stem in KeypointEvents KeypointSnapshots KeypointLineage; do
        file="TemporalTest-${stem}-Sample-${sample}.csv"
        cmp "$test_dir/run-a/$file" "$test_dir/run-b/$file"
    done
    python3 "$script_dir/validate_temporal_exports.py" \
        "$test_dir/run-a/TemporalTest-KeypointEvents-Sample-${sample}.csv" \
        "$test_dir/run-a/TemporalTest-KeypointSnapshots-Sample-${sample}.csv" \
        "$test_dir/run-a/TemporalTest-KeypointLineage-Sample-${sample}.csv"
done

printf '%s\n' "deterministic temporal smoke test passed"
