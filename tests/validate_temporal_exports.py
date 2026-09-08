#!/usr/bin/env python3
"""Validate persistent-keypoint snapshot and event CSV exports."""

import argparse
import csv
from collections import defaultdict
from pathlib import Path


def rows(path):
    with path.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def key(row):
    return int(row["sample"]), row["keypoint_type"], int(row["persistent_id"])


def validate(events_path, snapshots_path, lineage_path):
    events = rows(events_path)
    snapshots = rows(snapshots_path)
    lineage = rows(lineage_path)
    births = {}
    retirements = {}
    previous_step = -1

    for row in events:
        step = int(row["timestep"])
        assert step >= previous_step, "events are not ordered by timestep"
        previous_step = step
        item = key(row)
        if row["event"] == "birth":
            assert item not in births, f"duplicate birth: {item}"
            births[item] = step
        elif row["event"] == "retirement":
            assert item in births, f"retirement before birth: {item}"
            assert item not in retirements, f"duplicate retirement: {item}"
            assert step >= births[item], f"retirement precedes birth: {item}"
            retirements[item] = step
        else:
            raise AssertionError(f"unknown event: {row['event']}")

    frames = defaultdict(set)
    for row in snapshots:
        step = int(row["timestep"])
        item = key(row)
        frame = int(row["sample"]), step
        assert item not in frames[frame], f"duplicate identity in frame: {item}"
        frames[frame].add(item)
        assert item in births, f"snapshot identity without birth: {item}"
        assert births[item] <= step, f"identity appears before birth: {item}"
        assert item not in retirements or retirements[item] > step, \
            f"retired identity appears in snapshot: {item}"
        assert int(row["birth_timestep"]) == births[item], \
            f"incorrect birth timestep: {item}"

    for (sample, step), observed in sorted(frames.items()):
        expected = {
            item for item, birth in births.items()
            if item[0] == sample and birth <= step
            and (item not in retirements or retirements[item] > step)
        }
        assert observed == expected, (
            f"active-set mismatch sample={sample} step={step}; "
            f"missing={sorted(expected-observed)[:5]} extra={sorted(observed-expected)[:5]}"
        )

    seen_junctions = set()
    seen_new_tips = set()
    for row in lineage:
        sample = int(row["sample"])
        step = int(row["timestep"])
        source = (sample, "tip", int(row["source_tip_id"]))
        junction = (sample, "junction", int(row["junction_id"]))
        continuing = (sample, "tip", int(row["continuing_tip_id"]))
        new_tip = (sample, "tip", int(row["new_tip_id"]))
        assert row["branching_mode"] in {"side_branching", "bifurcation"}, \
            f"unknown branching mode: {row['branching_mode']}"
        assert source == continuing, "continuation must inherit source tip identity"
        assert source in births and births[source] <= step, \
            f"source tip missing at lineage event: {source}"
        assert junction in births and births[junction] == step, \
            f"junction birth does not match lineage event: {junction}"
        assert new_tip in births and births[new_tip] == step, \
            f"new-tip birth does not match lineage event: {new_tip}"
        assert junction not in seen_junctions, f"junction reused in lineage: {junction}"
        assert new_tip not in seen_new_tips, f"new tip reused in lineage: {new_tip}"
        seen_junctions.add(junction)
        seen_new_tips.add(new_tip)

    print(
        "valid:", f"events={len(events)}", f"births={len(births)}",
        f"retirements={len(retirements)}", f"snapshot_rows={len(snapshots)}",
        f"frames={len(frames)}", f"lineage_events={len(lineage)}"
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("events", type=Path)
    parser.add_argument("snapshots", type=Path)
    parser.add_argument("lineage", type=Path)
    args = parser.parse_args()
    validate(args.events, args.snapshots, args.lineage)
