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


def load_topology(path: Path, frames, gate):
    rows_by_frame = defaultdict(dict)
    with path.open(newline="", encoding="utf-8") as handle:
        for row in csv.DictReader(handle):
            rows_by_frame[int(row["timestep"])][
                (row["keypoint_type"], int(row["persistent_id"]))
            ] = row

    descriptors = defaultdict(lambda: defaultdict(dict))
    for step, graph in rows_by_frame.items():
        positions = frames[step]
        for (kind, identity), row in graph.items():
            position = positions[kind][identity]
            parent_id = int(row["parent_junction_id"])
            has_parent = float(parent_id >= 0)
            parent_distance = 0.0
            if parent_id >= 0:
                parent_distance = np.linalg.norm(position - positions["junction"][parent_id]) / gate

            child_distances = []
            child_tip_count = 0
            child_vectors = []
            for slot in ("child1", "child2"):
                child_type = row[slot + "_type"]
                child_id = int(row[slot + "_id"])
                if child_id < 0:
                    continue
                child_position = positions[child_type][child_id]
                vector = child_position - position
                child_vectors.append(vector)
                child_distances.append(float(np.linalg.norm(vector)) / gate)
                child_tip_count += child_type == "tip"
            child_distances.sort()
            child_distances += [0.0] * (2 - len(child_distances))

            opening_angle = 0.0
            if len(child_vectors) == 2:
                norms = np.linalg.norm(child_vectors[0]) * np.linalg.norm(child_vectors[1])
                if norms > 0:
                    cosine = np.clip(np.dot(child_vectors[0], child_vectors[1]) / norms, -1, 1)
                    opening_angle = float(np.arccos(cosine) / np.pi)
            descriptors[step][kind][identity] = np.array([
                has_parent, parent_distance, child_distances[0], child_distances[1],
                child_tip_count / 2.0, opening_angle,
            ])
    return descriptors


def match(previous, current, gate, previous_descriptors=None,
          current_descriptors=None, topology_weight=0.0):
    previous_ids = sorted(previous)
    current_ids = sorted(current)
    if not previous_ids or not current_ids:
        return []
    costs = np.array([
        [np.linalg.norm(previous[a] - current[b]) for b in current_ids]
        for a in previous_ids
    ])
    objective = costs.copy()
    if topology_weight:
        descriptor_size = 6
        zero_descriptor = np.zeros(descriptor_size, dtype=float)
        topology_costs = np.array([
            [np.linalg.norm(previous_descriptors.get(a, zero_descriptor) -
                            current_descriptors.get(b, zero_descriptor))
             for b in current_ids]
            for a in previous_ids
        ])
        objective += topology_weight * topology_costs
    finite = objective[costs <= gate]
    invalid_cost = (max(objective.shape) + 1) * ((float(np.max(finite)) if finite.size else gate) + 1)
    objective[costs > gate] = invalid_cost
    rows, columns = linear_sum_assignment(objective)
    return [
        (previous_ids[row], current_ids[column], float(costs[row, column]))
        for row, column in zip(rows, columns)
        if costs[row, column] <= gate
    ]


def evaluate_frames(frames, gate: float, descriptors=None, topology_weight=0.0,
                    reference_frames=None):
    totals = defaultdict(int)
    distances = []
    steps = sorted(frames)
    for previous_step, current_step in zip(steps, steps[1:]):
        for kind in ("tip", "junction"):
            previous = frames[previous_step][kind]
            current = frames[current_step][kind]
            assignments = match(
                previous, current, gate,
                descriptors[previous_step][kind] if descriptors else None,
                descriptors[current_step][kind] if descriptors else None,
                topology_weight,
            )
            assigned_previous = {a for a, _, _ in assignments}
            assigned_current = {b for _, b, _ in assignments}
            reference_previous = (
                reference_frames[previous_step][kind] if reference_frames else previous
            )
            reference_current = (
                reference_frames[current_step][kind] if reference_frames else current
            )
            continuing = set(reference_previous) & set(reference_current)
            births = set(reference_current) - set(reference_previous)
            retirements = set(reference_previous) - set(reference_current)
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

    result = {"gate_spatial_units": gate,
              "topology_weight_spatial_units": topology_weight, **totals}
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


def evaluate_file(path: Path, gate: float, topology_path=None, topology_weight=0.0):
    frames = load_snapshots(path)
    descriptors = load_topology(topology_path, frames, gate) if topology_path else None
    result = evaluate_frames(frames, gate, descriptors, topology_weight)
    result["snapshot_file"] = str(path)
    result["topology_file"] = str(topology_path) if topology_path else None
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
    parser.add_argument("--topology", type=Path, nargs="*")
    parser.add_argument("--topology-weight", type=float, default=0.0)
    args = parser.parse_args()
    if args.gate <= 0:
        parser.error("--gate must be positive")
    if args.topology_weight < 0:
        parser.error("--topology-weight must be non-negative")
    if args.topology_weight and not args.topology:
        parser.error("--topology is required when --topology-weight is non-zero")
    if args.topology and len(args.topology) != len(args.snapshots):
        parser.error("--topology must provide one file per snapshot file")

    topology = args.topology or [None] * len(args.snapshots)
    results = [
        evaluate_file(path, args.gate, topology_path, args.topology_weight)
        for path, topology_path in zip(args.snapshots, topology)
    ]
    payload = {
        "method": "oracle-coordinate motion-gated Hungarian assignment",
        "gate_spatial_units": args.gate,
        "topology_weight_spatial_units": args.topology_weight,
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
