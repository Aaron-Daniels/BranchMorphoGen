# Temporal association pilot

This directory contains a reproducible engineering pilot for temporal keypoint
association. It is not a biologically validated Class IV simulation and must not
be described as reproducing an unpublished dataset.

## Pilot configuration

Generate the input file from the checked-out upstream configuration:

```sh
python experiments/make_classiv_temporal_pilot.py \
  ClassIV_parameters.in /path/to/output/pilot.in
```

The generator uses the upstream Class-IV-labeled dynamic parameters but makes
these explicit changes:

- 3 serial samples;
- base seed `20260908`;
- 960–970 minutes at the upstream 0.02-minute step;
- 6 output frames;
- generated initialization because upstream `INPUT.swc` is absent;
- 60-micron maximum rendered image size; and
- persistent keypoint exports enabled.

The upstream Class-IV file also uses three legacy parameter names and omits four
fields expected by the current parser. To prevent indeterminate C++ values, the
generator records and applies these engineering assumptions:

- `KSpring → E_Axial`, preserving value 1.0;
- `KBending → E_Bending`, preserving value 5.0;
- `Eta → DragCoef`, preserving value 5.0;
- `InitialAngle=90.0` degrees;
- `Boundary_SoftLength=15.0` microns;
- `TerminationRate=0.0`; and
- `MaximumRadius=0.4` microns.

The four fallback values come from the repository's general `parameters.in`.
Sabya should confirm the legacy-name mappings and missing Class-IV values before
the configuration is interpreted biologically.

## Association baseline

Install the pinned analysis dependencies in an isolated environment, then run:

```sh
python experiments/evaluate_association_baseline.py \
  --gate 5 \
  --output association-gate-5.json \
  /path/to/output/*KeypointSnapshots*.csv
```

The baseline performs Euclidean, motion-gated Hungarian assignment separately
for tips and junctions. It uses exact simulator coordinates, not image-model
predictions. The 5-unit gate is fixed before evaluation and is interpreted in
the simulation's configured spatial unit (microns for this pilot).

Reported precision is correct identity matches divided by all accepted
assignments. Recall is correct identity matches divided by true continuing
identities. The output also reports identity switches, missed continuations,
births or retirements incorrectly linked, and correct-match distances. Results
on this pilot are development evidence only; they are not train/test estimates
and the gate was not validated on held-out simulated or real data.

## Topology-aware comparison

With `KeypointTopology` files available, select a structural-cost weight using
development arbors only:

```sh
python experiments/select_topology_weight.py \
  --gate 5 --weights 0 0.25 0.5 1 2 5 \
  --snapshots /path/to/dev/*KeypointSnapshots*.csv \
  --topology /path/to/dev/*KeypointTopology*.csv \
  --output topology-weight-selection.json
```

The selection rule is maximum development macro F1, with the smallest weight
breaking ties. Evaluate the selected weight once on separately seeded held-out
arbors using `evaluate_association_baseline.py --topology ...
--topology-weight ...`.

The topology descriptor contains only anonymous structural measurements:
presence and distance of a parent junction, sorted child-edge lengths, fraction
of immediate children that are tips, and child opening angle. Persistent IDs are
used to retrieve connected coordinates while constructing the simulator
fixture, but ID values and equality are not included in assignment costs. IDs
are consulted only after assignment to calculate evaluation metrics.
