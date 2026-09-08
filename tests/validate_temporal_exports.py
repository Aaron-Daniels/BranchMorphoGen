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


def validate(events_path, snapshots_path):
    events = rows(events_path)
    snapshots = rows(snapshots_path)
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

    print(
        "valid:", f"events={len(events)}", f"births={len(births)}",
        f"retirements={len(retirements)}", f"snapshot_rows={len(snapshots)}",
        f"frames={len(frames)}"
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("events", type=Path)
    parser.add_argument("snapshots", type=Path)
    args = parser.parse_args()
    validate(args.events, args.snapshots)
