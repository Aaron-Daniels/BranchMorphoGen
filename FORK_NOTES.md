# Temporal keypoint identity fork

This fork extends BranchMorphoGen with machine-readable identities for simulated
tip and junction keypoints across time. It is based on upstream commit
`44c56cba9eb97f03d7ff7fb396d7fe8dd1242399` from
`SabyasachiSutradhar/BranchMorphoGen` and retains the upstream MIT license.

## Identity semantics

- Tip and junction IDs occupy separate, sample-scoped namespaces.
- IDs increase monotonically and are never reused within a sample.
- At side branching or bifurcation, the first/top continuation inherits the
  original tip ID; the second daughter receives a new tip ID.
- The new junction receives a new junction ID.
- When retraction removes a daughter and collapses its junction, the surviving
  sibling tip ID follows the surviving geometry.
- If both daughters disappear and the parent becomes a tip again, it receives a
  new tip ID. The retired junction ID is not reused.
- Mutable branch-vector indices and per-file SWC node IDs are not temporal IDs.

These are explicit computational conventions. Whether they match the desired
biological identity definition must be reviewed before the exports are treated
as biological ground truth.

## Configuration and outputs

Set `DumpKeypoints=true` to write:

- `<SimulationName>-KeypointSnapshots-Sample-<N>.csv`
- `<SimulationName>-KeypointEvents-Sample-<N>.csv`
- `<SimulationName>-KeypointLineage-Sample-<N>.csv`
- `<SimulationName>-KeypointTopology-Sample-<N>.csv`

Snapshot rows contain sample, timestep, simulation time, keypoint type,
persistent ID, x/y/z coordinates, dynamic state, and birth timestep. Event rows
record each birth and retirement. Coordinate and time units are the
`SpatialUnit` and `TemporalUnit` selected in the input file.

Each lineage row records one branching event using:

```text
sample,timestep,time,branching_mode,source_tip_id,junction_id,continuing_tip_id,new_tip_id,x,y,z
```

`source_tip_id` equals `continuing_tip_id` under this fork's continuation
convention. `junction_id` and `new_tip_id` must be born at the lineage event's
timestep. The row also records whether the simulator was configured for side
branching or bifurcation and the new junction's position.

Each topology row records an active tip or junction and its immediate graph
relationships at an exported frame:

```text
sample,timestep,time,keypoint_type,persistent_id,parent_junction_id,child1_type,child1_id,child2_type,child2_id
```

Tips have no children. Every junction has two immediate children, each typed as
a tip or junction. The validator requires complete active-frame coverage and
reciprocal parent/child references.

`RandomSeed` is the deterministic base seed. Sample index `i` (zero-based for
seed derivation) uses `RandomSeed + i`; therefore serial and parallel execution
have the same intended per-sample seed assignment.

## Validation

Run:

```sh
tests/run_temporal_smoke_test.sh
```

The test compiles a debug executable, runs a forced topology lifecycle fixture,
runs two identical two-sample simulations, byte-compares their keypoint CSVs,
and validates unique births, retirement ordering, non-reuse, birth timestamps,
active identities at every exported frame, and the referential integrity of
every branching-lineage and per-frame topology relationship.

On the initial development machine, CMake was unavailable, so validation used
Apple Clang with the Homebrew libTIFF include/library paths. The upstream CMake
configuration was not independently exercised there.

## Limitations before dataset generation

- No claim has been made that the included parameter files reproduce the
  unpublished Dataset-2 generation settings.
- The short smoke simulation tests determinism and file consistency, not
  biological realism or the full stochastic event distribution.
- The lineage export records keypoint relationships at branching events but does
  not yet provide a persistent ID for every non-keypoint segment in the full
  morphology graph.
- TIFF rendering and biological parameters require separate calibration and
  validation against an authorized reference dataset.
- No Yale images, Dataset-2 files, unpublished parameters, or other restricted
  research materials are included in this fork.
