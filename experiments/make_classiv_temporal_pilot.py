#!/usr/bin/env python3
"""Create the exact temporal pilot config from upstream ClassIV_parameters.in."""

import argparse
import re
from pathlib import Path


REPLACEMENTS = {
    "SimulationName": "ClassIVTemporalPilot",
    "NSample": "3",
    "RunParallel": "false",
    "RandomSeed": "20260908",
    "Dt": "0.02",
    "Time_Start": "960.0",
    "Time_End": "970.0",
    "ReadFromSWC": "false",
    "N_SWC": "6",
    "DumpKeypoints": "true",
    "MAX_IMAGE_SIZE": "60",
}

RENAMED_PARAMETERS = {
    "KSpring": "E_Axial",
    "KBending": "E_Bending",
    "Eta": "DragCoef",
}

# Missing from the upstream ClassIV file. Values are copied from the current
# general parameters.in so no SimulationParameters field remains indeterminate.
FALLBACK_PARAMETERS = {
    "InitialAngle": "90.0",
    "Boundary_SoftLength": "15.0",
    "TerminationRate": "0.0",
    "MaximumRadius": "0.4",
}


def replace_parameter(text: str, key: str, value: str) -> str:
    pattern = re.compile(rf"^(\s*{re.escape(key)}\s*=)[^;]*(;.*)$", re.MULTILINE)
    updated, count = pattern.subn(rf"\g<1>{value}\g<2>", text)
    if count != 1:
        raise ValueError(f"expected exactly one {key!r} assignment, found {count}")
    return updated


def rename_parameter(text: str, old: str, new: str) -> str:
    pattern = re.compile(rf"^(\s*){re.escape(old)}(\s*=)", re.MULTILINE)
    updated, count = pattern.subn(rf"\g<1>{new}\g<2>", text)
    if count != 1:
        raise ValueError(f"expected exactly one {old!r} assignment, found {count}")
    return updated


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    text = args.source.read_text(encoding="utf-8")
    for key, value in REPLACEMENTS.items():
        text = replace_parameter(text, key, value)
    for old, new in RENAMED_PARAMETERS.items():
        text = rename_parameter(text, old, new)
    header = (
        "# GENERATED FILE: engineering pilot, not biologically validated\n"
        "# Source: ClassIV_parameters.in at the checked-out Git commit\n"
        "# Deviation: ReadFromSWC=false because upstream INPUT.swc is not included\n"
        "# Inferred mappings: KSpring->E_Axial, KBending->E_Bending, Eta->DragCoef\n"
        "# Fallbacks from parameters.in: InitialAngle=90, Boundary_SoftLength=15,\n"
        "# TerminationRate=0, MaximumRadius=0.4\n"
    )
    fallback = "".join(f"    {key}={value};\n" for key, value in FALLBACK_PARAMETERS.items())
    args.output.write_text(header + fallback + text, encoding="utf-8")


if __name__ == "__main__":
    main()
