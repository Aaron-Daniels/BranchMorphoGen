#!/usr/bin/env python3
"""Deterministic functional test for the oracle association baseline."""

import csv
import importlib.util
import tempfile
from pathlib import Path


root = Path(__file__).resolve().parents[1]
module_path = root / "experiments" / "evaluate_association_baseline.py"
spec = importlib.util.spec_from_file_location("association_baseline", module_path)
baseline = importlib.util.module_from_spec(spec)
spec.loader.exec_module(baseline)

rows = [
    (0, "tip", 0, 0.0), (0, "tip", 1, 10.0), (0, "junction", 0, 5.0),
    (1, "tip", 0, 1.0), (1, "tip", 1, 9.0), (1, "tip", 2, 20.0),
    (1, "junction", 0, 5.1),
    (2, "tip", 0, 2.0), (2, "tip", 2, 19.0), (2, "junction", 0, 5.2),
]

with tempfile.TemporaryDirectory() as directory:
    path = Path(directory) / "snapshots.csv"
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow([
            "sample", "timestep", "time", "keypoint_type", "persistent_id",
            "x", "y", "z", "state", "birth_timestep"
        ])
        for step, kind, identity, x in rows:
            writer.writerow([1, step, step, kind, identity, x, 0, 0, 0, 0])

    result = baseline.evaluate_file(path, gate=2.0)
    assert result["tip_correct_matches"] == 4
    assert result["tip_identity_switches"] == 0
    assert result["tip_missed_continuations"] == 0
    assert result["tip_births"] == 1
    assert result["tip_births_incorrectly_linked"] == 0
    assert result["tip_retirements"] == 1
    assert result["tip_retirements_incorrectly_linked"] == 0
    assert result["tip_f1"] == 1.0
    assert result["junction_correct_matches"] == 2
    assert result["junction_f1"] == 1.0
    aggregate = baseline.aggregate_results([result])
    assert aggregate["tip_f1"] == 1.0
    assert aggregate["junction_f1"] == 1.0

print("association baseline functional test passed")
