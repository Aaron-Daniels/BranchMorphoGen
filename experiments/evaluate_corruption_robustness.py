#!/usr/bin/env python3
"""Compare association methods under deterministic detector-like corruption."""

import argparse
import importlib.util
import json
from collections import defaultdict
from pathlib import Path

import numpy as np


module_path = Path(__file__).with_name("evaluate_association_baseline.py")
spec = importlib.util.spec_from_file_location("association_baseline", module_path)
baseline = importlib.util.module_from_spec(spec)
spec.loader.exec_module(baseline)


def corrupt_frames(clean_frames, rng, sigma, miss_probability, false_ratio, gate):
    corrupted = defaultdict(lambda: defaultdict(dict))
    false_counter = 0
    for step in sorted(clean_frames):
        all_positions = [
            position for kind in clean_frames[step].values() for position in kind.values()
        ]
        stacked = np.stack(all_positions)
        minimum = stacked.min(axis=0)
        maximum = stacked.max(axis=0)
        active_dimensions = (maximum - minimum) > 1e-12
        lower = np.where(active_dimensions, minimum - gate, minimum)
        upper = np.where(active_dimensions, maximum + gate, maximum)
        for kind in ("tip", "junction"):
            truth = clean_frames[step][kind]
            for identity, position in truth.items():
                if rng.random() < miss_probability:
                    continue
                noise = rng.normal(0.0, sigma, 3)
                noise[~active_dimensions] = 0.0
                corrupted[step][kind][identity] = position + noise
            false_count = rng.poisson(false_ratio * len(truth))
            for _ in range(false_count):
                false_counter += 1
                corrupted[step][kind][-false_counter] = rng.uniform(lower, upper)
    return corrupted


def evaluate(snapshot_paths, topology_paths, gate, topology_weight, sigma,
             miss_probability, false_ratio, repetitions, seed):
    methods = {"motion": [], "topology": []}
    repetition_results = {"motion": [], "topology": []}
    for repetition in range(repetitions):
        current = {"motion": [], "topology": []}
        for sample_index, (snapshot_path, topology_path) in enumerate(
            zip(snapshot_paths, topology_paths)
        ):
            clean = baseline.load_snapshots(snapshot_path)
            descriptors = baseline.load_topology(topology_path, clean, gate)
            sequence = np.random.SeedSequence([seed, repetition, sample_index])
            corrupted = corrupt_frames(
                clean, np.random.default_rng(sequence), sigma,
                miss_probability, false_ratio, gate,
            )
            motion_result = baseline.evaluate_frames(
                corrupted, gate, reference_frames=clean
            )
            topology_result = baseline.evaluate_frames(
                corrupted, gate, descriptors, topology_weight, reference_frames=clean
            )
            methods["motion"].append(motion_result)
            methods["topology"].append(topology_result)
            current["motion"].append(motion_result)
            current["topology"].append(topology_result)
        for name in methods:
            repetition_results[name].append(baseline.aggregate_results(current[name]))

    summaries = {}
    for name, results in methods.items():
        values = repetition_results[name]
        metrics = {
            "tip_f1": np.array([item["tip_f1"] for item in values]),
            "junction_f1": np.array([item["junction_f1"] for item in values]),
        }
        metrics["macro_f1"] = (metrics["tip_f1"] + metrics["junction_f1"]) / 2
        summaries[name] = {
            "pooled": baseline.aggregate_results(results),
            "repetition_summary": {
                metric: {
                    "mean": float(series.mean()),
                    "sample_std": float(series.std(ddof=1)) if repetitions > 1 else 0.0,
                    "min": float(series.min()),
                    "max": float(series.max()),
                }
                for metric, series in metrics.items()
            },
        }

    motion_macro = np.array([
        (item["tip_f1"] + item["junction_f1"]) / 2
        for item in repetition_results["motion"]
    ])
    topology_macro = np.array([
        (item["tip_f1"] + item["junction_f1"]) / 2
        for item in repetition_results["topology"]
    ])
    delta = topology_macro - motion_macro
    return {
        "methods": summaries,
        "paired_macro_f1_delta": {
            "mean": float(delta.mean()),
            "sample_std": float(delta.std(ddof=1)) if repetitions > 1 else 0.0,
            "min": float(delta.min()),
            "max": float(delta.max()),
            "fraction_positive": float(np.mean(delta > 0)),
        },
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--snapshots", type=Path, nargs="+", required=True)
    parser.add_argument("--topology", type=Path, nargs="+", required=True)
    parser.add_argument("--gate", type=float, required=True)
    parser.add_argument("--topology-weight", type=float, required=True)
    parser.add_argument("--sigma", type=float, required=True)
    parser.add_argument("--miss-probability", type=float, required=True)
    parser.add_argument("--false-ratio", type=float, required=True)
    parser.add_argument("--repetitions", type=int, default=30)
    parser.add_argument("--seed", type=int, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if len(args.snapshots) != len(args.topology):
        parser.error("--snapshots and --topology counts must match")
    if args.gate <= 0 or args.topology_weight < 0 or args.sigma < 0:
        parser.error("gate must be positive; weight and sigma must be non-negative")
    if not 0 <= args.miss_probability <= 1 or args.false_ratio < 0:
        parser.error("miss probability must be in [0,1]; false ratio must be non-negative")
    if args.repetitions <= 0:
        parser.error("repetitions must be positive")

    results = evaluate(
        args.snapshots, args.topology, args.gate, args.topology_weight,
        args.sigma, args.miss_probability, args.false_ratio,
        args.repetitions, args.seed,
    )
    payload = {
        "method": "deterministic detector-like corruption with oracle topology descriptors",
        "gate_spatial_units": args.gate,
        "topology_weight_spatial_units": args.topology_weight,
        "localization_sigma_spatial_units": args.sigma,
        "miss_probability": args.miss_probability,
        "false_detection_ratio": args.false_ratio,
        "repetitions": args.repetitions,
        "random_seed": args.seed,
        "samples": len(args.snapshots),
        "results": results["methods"],
        "paired_macro_f1_delta": results["paired_macro_f1_delta"],
    }
    rendered = json.dumps(payload, indent=2, sort_keys=True) + "\n"
    args.output.write_text(rendered, encoding="utf-8")
    print(rendered, end="")


if __name__ == "__main__":
    main()
