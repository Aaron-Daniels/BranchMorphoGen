#!/usr/bin/env python3
"""Evaluate motion-gated Hungarian matching on oracle keypoint coordinates."""

import argparse
import csv
import json
from collections import defaultdict
from pathlib import Path

import numpy as np
import scipy
from scipy.optimize import linear_sum_assignment


def load_snapshots(path: Path):
    frames = defaultdict(lambda: defaultdict(dict))
    with path.open(newline="", encoding="utf-8") as handle:
        for row in csv.DictReader(handle):
            step = int(row["timestep"])
            kind = row["keypoint_type"]
            identity = int(row["persistent_id"])
            frames[step][kind][identity] = np.array(
                [float(row["x"]), float(row["y"]), float(row["z"])], dtype=float
            )
    return frames


def match(previous, current, gate):
    previous_ids = sorted(previous)
    current_ids = sorted(current)
    if not previous_ids or not current_ids:
        return []
    costs = np.array([
        [np.linalg.norm(previous[a] - current[b]) for b in current_ids]
        for a in previous_ids
    ])
    rows, columns = linear_sum_assignment(costs)
    return [
        (previous_ids[row], current_ids[column], float(costs[row, column]))
        for row, column in zip(rows, columns)
        if costs[row, column] <= gate
    ]


def evaluate_file(path: Path, gate: float):
    frames = load_snapshots(path)
    totals = defaultdict(int)
    distances = []
    steps = sorted(frames)
    for previous_step, current_step in zip(steps, steps[1:]):
        for kind in ("tip", "junction"):
            previous = frames[previous_step][kind]
            current = frames[current_step][kind]
            assignments = match(previous, current, gate)
            assigned_previous = {a for a, _, _ in assignments}
            assigned_current = {b for _, b, _ in assignments}
            continuing = set(previous) & set(current)
            births = set(current) - set(previous)
            retirements = set(previous) - set(current)
            correct = {(a, b) for a, b, _ in assignments if a == b}
            correct_ids = {a for a, _ in correct}

            prefix = kind + "_"
            totals[prefix + "expected_continuations"] += len(continuing)
            totals[prefix + "assignments"] += len(assignments)
            totals[prefix + "correct_matches"] += len(correct)
            totals[prefix + "identity_switches"] += sum(a != b for a, b, _ in assignments)
            totals[prefix + "missed_continuations"] += len(continuing - correct_ids)
            totals[prefix + "births"] += len(births)
            totals[prefix + "births_incorrectly_linked"] += len(births & assigned_current)
            totals[prefix + "retirements"] += len(retirements)
            totals[prefix + "retirements_incorrectly_linked"] += len(
                retirements & assigned_previous
            )
            distances.extend(distance for a, b, distance in assignments if a == b)

    result = {"snapshot_file": str(path), "gate_spatial_units": gate, **totals}
    for kind in ("tip", "junction"):
        prefix = kind + "_"
        correct = totals[prefix + "correct_matches"]
        assignments = totals[prefix + "assignments"]
        expected = totals[prefix + "expected_continuations"]
        precision = correct / assignments if assignments else 0.0
        recall = correct / expected if expected else 0.0
        result[prefix + "precision"] = precision
        result[prefix + "recall"] = recall
        result[prefix + "f1"] = (
            2 * precision * recall / (precision + recall)
            if precision + recall else 0.0
        )
    result["correct_match_mean_distance"] = float(np.mean(distances)) if distances else None
    result["correct_match_max_distance"] = float(np.max(distances)) if distances else None
    return result


def aggregate_results(results):
    aggregate = {}
    count_suffixes = (
        "expected_continuations", "assignments", "correct_matches", "identity_switches",
        "missed_continuations", "births", "births_incorrectly_linked", "retirements",
        "retirements_incorrectly_linked",
    )
    for kind in ("tip", "junction"):
        prefix = kind + "_"
        for suffix in count_suffixes:
            aggregate[prefix + suffix] = sum(item[prefix + suffix] for item in results)
        correct = aggregate[prefix + "correct_matches"]
        assignments = aggregate[prefix + "assignments"]
        expected = aggregate[prefix + "expected_continuations"]
        precision = correct / assignments if assignments else 0.0
        recall = correct / expected if expected else 0.0
        aggregate[prefix + "precision"] = precision
        aggregate[prefix + "recall"] = recall
        aggregate[prefix + "f1"] = (
            2 * precision * recall / (precision + recall)
            if precision + recall else 0.0
        )

    weighted_distances = [
        (item["correct_match_mean_distance"],
         item["tip_correct_matches"] + item["junction_correct_matches"])
        for item in results if item["correct_match_mean_distance"] is not None
    ]
    distance_count = sum(count for _, count in weighted_distances)
    aggregate["correct_match_mean_distance"] = (
        sum(mean * count for mean, count in weighted_distances) / distance_count
        if distance_count else None
    )
    maxima = [
        item["correct_match_max_distance"] for item in results
        if item["correct_match_max_distance"] is not None
    ]
    aggregate["correct_match_max_distance"] = max(maxima) if maxima else None
    return aggregate


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("snapshots", type=Path, nargs="+")
    parser.add_argument("--gate", type=float, required=True)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.gate < 0:
        parser.error("--gate must be non-negative")

    results = [evaluate_file(path, args.gate) for path in args.snapshots]
    payload = {
        "method": "oracle-coordinate motion-gated Hungarian assignment",
        "gate_spatial_units": args.gate,
        "numpy_version": np.__version__,
        "scipy_version": scipy.__version__,
        "samples": results,
        "aggregate": aggregate_results(results),
    }
    rendered = json.dumps(payload, indent=2, sort_keys=True)
    if args.output:
        args.output.write_text(rendered + "\n", encoding="utf-8")
    print(rendered)


if __name__ == "__main__":
    main()
