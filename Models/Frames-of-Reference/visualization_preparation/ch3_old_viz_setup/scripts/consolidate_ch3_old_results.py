#!/usr/bin/env python3
"""Consolidate Chapter 3 (old model) posterior-share CSVs into one long table.

This script creates a single "frozen" figure-data table by concatenating
multiple results files and adding a Date column inferred from each filename.

Expected input filename pattern:
  shares_YYYY-MM-DD.csv

Typical input columns:
  Language, %IntrinsicData, %CanonicalData (optional), NoncanonicalOddsReduction,
  TrainingSize, Iteration, Word, Sense, Share

If an input file lacks %CanonicalData, it will be filled with a default
(17 by default), matching the "uniform ground-orientation" setting.

The output is written as CSV.
"""

from __future__ import annotations

import argparse
import glob
import os
import re
from typing import List

import pandas as pd


DATE_RE = re.compile(r"shares_(\d{4}-\d{2}-\d{2})\.csv$")


def infer_date_from_filename(path: str) -> str:
    m = DATE_RE.search(os.path.basename(path))
    if not m:
        raise ValueError(
            f"Could not infer date from filename {path!r}. Expected pattern shares_YYYY-MM-DD.csv"
        )
    return m.group(1)


def collect_inputs(inputs: List[str]) -> List[str]:
    expanded: List[str] = []
    for item in inputs:
        if any(ch in item for ch in "*?[]"):
            expanded.extend(sorted(glob.glob(item)))
        else:
            expanded.append(item)
    expanded = [p for p in expanded if p]
    if not expanded:
        raise SystemExit("No input files found.")
    return expanded


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "--inputs",
        nargs="+",
        default=[os.path.join("..", "figdata", "raw", "shares_*.csv")],
        help="Input CSV paths or globs (default: ../figdata/raw/shares_*.csv).",
    )
    ap.add_argument(
        "--default_canonical",
        type=int,
        default=17,
        help="Value to fill for %CanonicalData when missing (default: 17).",
    )
    ap.add_argument(
        "--out",
        default=os.path.join("..", "figdata", "chapter_3_old_results.csv"),
        help="Output consolidated CSV path.",
    )
    args = ap.parse_args()

    files = collect_inputs(args.inputs)

    parts = []
    for path in files:
        df = pd.read_csv(path)
        df.insert(0, "Date", infer_date_from_filename(path))

        if "%CanonicalData" not in df.columns:
            df["%CanonicalData"] = int(args.default_canonical)

        parts.append(df)

    out = pd.concat(parts, ignore_index=True, sort=False)

    # Reorder columns (keep unknown extras at end)
    front = [
        "Date",
        "Language",
        "%IntrinsicData",
        "%CanonicalData",
        "NoncanonicalOddsReduction",
        "TrainingSize",
        "Iteration",
        "Word",
        "Sense",
        "Share",
    ]
    existing_front = [c for c in front if c in out.columns]
    rest = [c for c in out.columns if c not in existing_front]
    out = out[existing_front + rest]

    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    out.to_csv(args.out, index=False)
    print(f"Wrote: {args.out}")
    print(f"Rows: {len(out):,}  Cols: {len(out.columns)}")


if __name__ == "__main__":
    main()
