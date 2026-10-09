#!/usr/bin/env python3
"""
consolidate_ch3_new_model_results.py

Goal
----
Turn a directory tree like:

  chapter_3_new_model_experiments/
    2024-11-04/
      nc_16_unique_scenes.csv
      nc_01_unique_scenes.csv
    2024-12-17/
      c_50_int_50_unique_scenes.csv
      ...

into ONE consolidated "plot-ready" CSV that contains the *pre-processed*
learning-curve points (the work done in `load_and_process_data`) plus
factor columns extracted from folder/file names.

Output format
-------------
One row per:
  (Date, %CanonicalData, %IntrinsicData, NoncanonicalOddsReduction,
   TrainingSize, Iteration, Canonicity)

with columns:
  Date, %CanonicalData, %IntrinsicData, NoncanonicalOddsReduction,
  TrainingSize, Iteration, Canonicity,
  IntP_mean, IntP_std, RelP_mean, RelP_std, n, IntP_se, RelP_se

Notes
-----
- This script intentionally does *not* plot. It "freezes" the curve points so
  you can restyle plots without re-running the expensive preprocessing.
- Canonicity is computed exactly as in your plotting script.
- Defaults (when missing from filename):
    - NoncanonicalOddsReduction = inf   (if "nc_<...>" absent)
    - %CanonicalData = 17               (if "c_<...>" absent)
    - %IntrinsicData = 50              (if "int_<...>" absent)
"""

from __future__ import annotations

import argparse
import math
import re
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple

import numpy as np
import pandas as pd


DATE_RE = re.compile(r"^\d{4}-\d{2}-\d{2}$")


def find_date_in_path(path: Path) -> str:
    """
    Finds the nearest YYYY-MM-DD folder name in the file's path.
    """
    for part in reversed(path.parts):
        if DATE_RE.match(part):
            return part
    return ""


def parse_conditions_from_filename(filename: str) -> Dict[str, float]:
    """
    Parses condition values out of a filename like:
      - nc_16_unique_scenes.csv
      - c_50_int_89_unique_scenes.csv
      - nc_16_c_89_int_50_unique_scenes.csv   (also supported)

    Returns defaults when tokens are absent.
    """
    # Defaults described by user
    cond = {
        "NoncanonicalOddsReduction": float("inf"),
        "%CanonicalData": 17.0,
        "%IntrinsicData": 50.0,
    }

    stem = Path(filename).stem  # drop .csv
    parts = stem.split("_")

    i = 0
    while i < len(parts):
        tok = parts[i].lower()
        nxt = parts[i + 1] if (i + 1) < len(parts) else None

        if tok in {"nc", "c", "int"} and nxt is not None:
            try:
                val = float(nxt) if nxt.lower() not in {"inf", "infty", "infinity"} else float("inf")
            except ValueError:
                # If it isn't numeric, skip gracefully.
                i += 1
                continue

            if tok == "nc":
                cond["NoncanonicalOddsReduction"] = val
            elif tok == "c":
                cond["%CanonicalData"] = val
            elif tok == "int":
                cond["%IntrinsicData"] = val

            i += 2
            continue

        i += 1

    return cond


def _chunk_aggregate_groups(chunk: pd.DataFrame) -> pd.DataFrame:
    """
    For a raw chunk, compute group-level IntSum/RelSum and keep
    the fields needed to compute canonicity.

    Returns a DF indexed by (TrainingSize, Iteration, TestID).
    """
    # Probability is exp(LogProbability) in the original script
    prob = np.exp(chunk["LogProbability"].to_numpy(dtype=np.float64))

    word = chunk["Word"].astype(str).to_numpy()
    int_desc = chunk["IntDescription"].astype(str).to_numpy()
    rel_desc = chunk["RelDescription"].astype(str).to_numpy()

    chunk = chunk.copy()
    chunk["Prob"] = prob
    chunk["IntContrib"] = np.where(word == int_desc, prob, 0.0)
    chunk["RelContrib"] = np.where(word == rel_desc, prob, 0.0)

    keys = ["TrainingSize", "Iteration", "TestID"]
    ag = chunk.groupby(keys, sort=False).agg(
        IntSum=("IntContrib", "sum"),
        RelSum=("RelContrib", "sum"),
        CanonicalX=("CanonicalX", "first"),
        CanonicalY=("CanonicalY", "first"),
        CanonicalZ=("CanonicalZ", "first"),
        IntDescription=("IntDescription", "first"),
    )
    return ag


def preprocess_one_file(
    csv_path: Path,
    *,
    chunksize: int = 400_000,
    max_training_size: Optional[int] = None,
) -> pd.DataFrame:
    """
    Runs the "load_and_process_data"-equivalent preprocessing for ONE raw csv,
    but returns a per-iteration learning-curve table:

      (TrainingSize, Iteration, Canonicity) -> means/stds/SEs across TestIDs
    """
    usecols = [
        "TrainingSize",
        "Iteration",
        "TestID",
        "IntDescription",
        "RelDescription",
        "CanonicalX",
        "CanonicalY",
        "CanonicalZ",
        "Word",
        "Sense",
        "LogProbability",
    ]

    dtype = {
        "TrainingSize": np.int32,
        "Iteration": np.int16,
        "TestID": np.int32,
        "CanonicalX": np.int8,
        "CanonicalY": np.int8,
        "CanonicalZ": np.int8,
        "LogProbability": np.float64,
    }

    partial_aggs: List[pd.DataFrame] = []

    reader = pd.read_csv(csv_path, usecols=usecols, dtype=dtype, chunksize=chunksize)

    for chunk in reader:
        if max_training_size is not None:
            chunk = chunk[chunk["TrainingSize"] <= max_training_size]
            if chunk.empty:
                continue

        partial_aggs.append(_chunk_aggregate_groups(chunk))

    if not partial_aggs:
        return pd.DataFrame()

    # Combine partial aggregates (needed when groups span chunks)
    keys = ["TrainingSize", "Iteration", "TestID"]
    cons = (
        pd.concat(partial_aggs)
        .groupby(keys, sort=False)
        .agg(
            IntSum=("IntSum", "sum"),
            RelSum=("RelSum", "sum"),
            CanonicalX=("CanonicalX", "first"),
            CanonicalY=("CanonicalY", "first"),
            CanonicalZ=("CanonicalZ", "first"),
            IntDescription=("IntDescription", "first"),
        )
        .reset_index()
    )

    total = cons["IntSum"] + cons["RelSum"]
    cons["IntP"] = np.where(total > 0, cons["IntSum"] / total, np.nan)
    cons["RelP"] = np.where(total > 0, cons["RelSum"] / total, np.nan)

    # Canonicity rule (same as in plot_results_ch_3_new_model.py)
    full_canonical = (cons["CanonicalX"] == 1) & (cons["CanonicalY"] == 1) & (cons["CanonicalZ"] == 1)

    axis = cons["IntDescription"].astype(str).str[0].str.upper()
    axis_canonical = np.select(
        [axis == "X", axis == "Y", axis == "Z"],
        [cons["CanonicalX"], cons["CanonicalY"], cons["CanonicalZ"]],
        default=np.nan,
    )

    cons["Canonicity"] = np.where(full_canonical, 1.0, np.where(axis_canonical == 0, 0.0, 0.5))

    # Aggregate across TestIDs, but keep Iteration (model) separate
    curve = (
        cons.groupby(["TrainingSize", "Iteration", "Canonicity"], sort=False)
        .agg(
            IntP_mean=("IntP", "mean"),
            IntP_std=("IntP", "std"),
            RelP_mean=("RelP", "mean"),
            RelP_std=("RelP", "std"),
            n=("IntP", "size"),
        )
        .reset_index()
    )

    # std is NaN when n==1; treat as 0.
    curve["IntP_std"] = curve["IntP_std"].fillna(0.0)
    curve["RelP_std"] = curve["RelP_std"].fillna(0.0)

    curve["IntP_se"] = curve["IntP_std"] / np.sqrt(curve["n"])
    curve["RelP_se"] = curve["RelP_std"] / np.sqrt(curve["n"])

    return curve


def iter_csv_files(root_dir: Path) -> Iterable[Path]:
    """
    Yields CSV files under root_dir, skipping obvious macOS metadata dirs.
    """
    for p in root_dir.rglob("*.csv"):
        if "__MACOSX" in p.parts:
            continue
        if p.name.startswith("._"):
            continue
        yield p


def build_consolidated_table(
    experiments_root: Path,
    *,
    chunksize: int = 400_000,
    max_training_size: Optional[int] = None,
    verbose: bool = True,
) -> pd.DataFrame:
    rows: List[pd.DataFrame] = []

    for csv_path in sorted(iter_csv_files(experiments_root)):
        # Skip "example rows" if it's sitting in the root
        if csv_path.name.startswith("example_rows"):
            continue

        date = find_date_in_path(csv_path)
        cond = parse_conditions_from_filename(csv_path.name)

        if verbose:
            print(f"[INFO] Processing: {csv_path}")
            print(
                f"       Date={date or '(none)'} | "
                f"%CanonicalData={cond['%CanonicalData']} | "
                f"%IntrinsicData={cond['%IntrinsicData']} | "
                f"NoncanonicalOddsReduction={cond['NoncanonicalOddsReduction']}"
            )

        curve = preprocess_one_file(csv_path, chunksize=chunksize, max_training_size=max_training_size)
        if curve.empty:
            if verbose:
                print("       -> (no rows after filtering; skipped)")
            continue

        curve.insert(0, "Date", date)
        curve.insert(1, "%CanonicalData", cond["%CanonicalData"])
        curve.insert(2, "%IntrinsicData", cond["%IntrinsicData"])
        curve.insert(3, "NoncanonicalOddsReduction", cond["NoncanonicalOddsReduction"])
        curve.insert(4, "SourceFile", csv_path.name)

        rows.append(curve)

    if not rows:
        return pd.DataFrame()

    out = pd.concat(rows, ignore_index=True)

    # Stable, useful sort order
    out = out.sort_values(
        ["Date", "%CanonicalData", "%IntrinsicData", "NoncanonicalOddsReduction", "TrainingSize", "Iteration", "Canonicity"],
        kind="mergesort",
    ).reset_index(drop=True)

    return out


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "experiments_root",
        type=Path,
        help="Directory containing date subdirectories, each with condition CSVs.",
    )
    ap.add_argument(
        "-o",
        "--output_csv",
        type=Path,
        default=Path("ch3_new_model_consolidated.csv"),
        help="Where to write the consolidated CSV.",
    )
    ap.add_argument(
        "--chunksize",
        type=int,
        default=400_000,
        help="Rows per chunk when reading each raw CSV (lower if you hit memory limits).",
    )
    ap.add_argument(
        "--max_training_size",
        type=int,
        default=None,
        help="Optional: keep only TrainingSize <= this value (e.g., 1000).",
    )
    ap.add_argument(
        "--quiet",
        action="store_true",
        help="Suppress per-file logging.",
    )

    args = ap.parse_args()

    df = build_consolidated_table(
        args.experiments_root,
        chunksize=args.chunksize,
        max_training_size=args.max_training_size,
        verbose=not args.quiet,
    )

    if df.empty:
        raise SystemExit("No CSVs found / no rows produced. Check the input directory and filters.")

    # Drop SourceFile if you want a *strict* match to the old intermediate format:
    # df = df.drop(columns=["SourceFile"])

    args.output_csv.parent.mkdir(parents=True, exist_ok=True)
    df.to_csv(args.output_csv, index=False)
    print(f"[DONE] Wrote {len(df):,} rows to: {args.output_csv.resolve()}")


if __name__ == "__main__":
    main()
