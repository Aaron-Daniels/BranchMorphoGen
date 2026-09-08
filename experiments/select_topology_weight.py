#!/usr/bin/env python3
"""Select a topology cost weight on development arbors only."""

import argparse
import importlib.util
import json
from pathlib import Path


module_path = Path(__file__).with_name("evaluate_association_baseline.py")
spec = importlib.util.spec_from_file_location("association_baseline", module_path)
baseline = importlib.util.module_from_spec(spec)
spec.loader.exec_module(baseline)


def select(weights, snapshots, topology, gate):
    candidates = []
    for weight in weights:
        samples = [
            baseline.evaluate_file(snapshot, gate, graph, weight)
            for snapshot, graph in zip(snapshots, topology)
        ]
        aggregate = baseline.aggregate_results(samples)
        macro_f1 = (aggregate["tip_f1"] + aggregate["junction_f1"]) / 2
        candidates.append({
            "topology_weight_spatial_units": weight,
            "macro_f1": macro_f1,
            "aggregate": aggregate,
        })
    selected = max(candidates, key=lambda item: (item["macro_f1"], -item["topology_weight_spatial_units"]))
    return {
        "selection_rule": "maximum development macro F1; smallest weight breaks ties",
        "gate_spatial_units": gate,
        "weights_searched": weights,
        "candidates": candidates,
        "selected_topology_weight_spatial_units": selected["topology_weight_spatial_units"],
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--snapshots", type=Path, nargs="+", required=True)
    parser.add_argument("--topology", type=Path, nargs="+", required=True)
    parser.add_argument("--gate", type=float, required=True)
    parser.add_argument("--weights", type=float, nargs="+", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if len(args.snapshots) != len(args.topology):
        parser.error("--snapshots and --topology counts must match")
    if args.gate <= 0 or any(weight < 0 for weight in args.weights):
        parser.error("gate must be positive and weights must be non-negative")
    payload = select(args.weights, args.snapshots, args.topology, args.gate)
    rendered = json.dumps(payload, indent=2, sort_keys=True) + "\n"
    args.output.write_text(rendered, encoding="utf-8")
    print(rendered, end="")


if __name__ == "__main__":
    main()
