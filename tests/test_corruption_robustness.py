#!/usr/bin/env python3
"""Determinism and no-corruption checks for robustness evaluation."""

import importlib.util
from collections import defaultdict
from pathlib import Path

import numpy as np


root = Path(__file__).resolve().parents[1]
module_path = root / "experiments" / "evaluate_corruption_robustness.py"
spec = importlib.util.spec_from_file_location("corruption_robustness", module_path)
robustness = importlib.util.module_from_spec(spec)
spec.loader.exec_module(robustness)

clean = defaultdict(lambda: defaultdict(dict))
clean[0]["tip"][0] = np.array([0.0, 0.0, 0.0])
clean[1]["tip"][0] = np.array([1.0, 0.0, 0.0])

first = robustness.corrupt_frames(
    clean, np.random.default_rng(123), sigma=0.5,
    miss_probability=0.2, false_ratio=0.5, gate=5.0,
)
second = robustness.corrupt_frames(
    clean, np.random.default_rng(123), sigma=0.5,
    miss_probability=0.2, false_ratio=0.5, gate=5.0,
)
assert first.keys() == second.keys()
for step in first:
    for kind in first[step]:
        assert first[step][kind].keys() == second[step][kind].keys()
        for identity in first[step][kind]:
            np.testing.assert_array_equal(
                first[step][kind][identity], second[step][kind][identity]
            )

uncorrupted = robustness.corrupt_frames(
    clean, np.random.default_rng(456), sigma=0.0,
    miss_probability=0.0, false_ratio=0.0, gate=5.0,
)
for step in clean:
    np.testing.assert_array_equal(uncorrupted[step]["tip"][0], clean[step]["tip"][0])

print("corruption robustness functional test passed")
