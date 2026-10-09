#!/usr/bin/env python3
"""Prepare curve-point tables for Chapter 2 *symmetrically unbalanced* comparison plots.

Goal
----
Produce a formatting-agnostic summary table for figures that compare

  (IntOnly - RelOnly) under the *intrinsic-majority* condition
vs
  (RelOnly - IntOnly) under the *relative-majority* condition.

This is the "mirror" diagnostic described in Chapter 2 (Δ67/33 vs Δ33/67),
but at the *per-training-size curve* level (so it can be graphed directly).

Input
-----
Long/tidy CSV with columns:
  Date, Language, %IntrinsicData, TrainingSize, Iteration, Word, Sense, Share

Output
------
Aggregated CSV (mean + CI across Iteration) with one row per curve point:
  Date, Language, TrainingSize, Word,
  IntrinsicLevel, DiffType, n, mean, sd, ci_low, ci_high, ...

Notes
-----
* This script does NOT rerun any semantic/model inference. It only reshapes
  and aggregates already-computed posterior shares.
* It reuses the same selection YAML (dates, max training size, etc.) as the
  main Chapter 2 figure pipeline.
"""

from __future__ import annotations

import argparse
import os
from typing import Any, Dict, Tuple

import numpy as np
import pandas as pd
import yaml


def _as_list(x):
    if x is None:
        return []
    if isinstance(x, (list, tuple)):
        return list(x)
    return [x]


def load_config(path: str) -> Dict[str, Any]:
    with open(path, "r") as f:
        return yaml.safe_load(f)


def filter_df(df: pd.DataFrame, cfg: Dict[str, Any]) -> pd.DataFrame:
    """Apply the same "main text" selection logic as prepare_ch2_figdata.py."""
    df = df.copy()

    # Normalize dtypes
    df["TrainingSize"] = pd.to_numeric(df["TrainingSize"], errors="coerce").astype(int)
    df["%IntrinsicData"] = pd.to_numeric(df["%IntrinsicData"], errors="coerce").astype(int)
    df["Iteration"] = pd.to_numeric(df["Iteration"], errors="coerce").astype(int)
    df["Share"] = pd.to_numeric(df["Share"], errors="coerce").astype(float)

    # Date selection: per language
    date_by_lang = cfg.get("date_by_language", {})
    if date_by_lang:
        keep_mask = np.zeros(len(df), dtype=bool)
        for lang, dates in date_by_lang.items():
            dates_list = _as_list(dates)
            keep_mask |= ((df["Language"] == lang) & (df["Date"].isin(dates_list)))
        df = df.loc[keep_mask].copy()

    # Max training size
    max_ts = cfg.get("max_training_size", None)
    if max_ts is not None:
        df = df.loc[df["TrainingSize"] <= int(max_ts)].copy()

    # Optional filters
    words = cfg.get("include_words", None)
    if words:
        df = df.loc[df["Word"].isin(words)].copy()

    languages = cfg.get("include_languages", None)
    if languages:
        df = df.loc[df["Language"].isin(languages)].copy()

    return df


def bootstrap_ci(values: np.ndarray, level: float = 0.95, n_boot: int = 2000, seed: int = 0) -> Tuple[float, float]:
    rng = np.random.default_rng(seed)
    values = np.asarray(values, dtype=float)
    if len(values) == 0:
        return (np.nan, np.nan)
    if len(values) == 1:
        v = float(values[0])
        return (v, v)

    idx = rng.integers(0, len(values), size=(n_boot, len(values)))
    means = values[idx].mean(axis=1)
    alpha = (1.0 - level) / 2.0
    lo = float(np.quantile(means, alpha))
    hi = float(np.quantile(means, 1.0 - alpha))
    return lo, hi


def sem_ci(values: np.ndarray, level: float = 0.95) -> Tuple[float, float]:
    from math import sqrt

    values = np.asarray(values, dtype=float)
    n = len(values)
    if n == 0:
        return (np.nan, np.nan)
    if n == 1:
        v = float(values[0])
        return (v, v)
    mean = float(values.mean())
    sd = float(values.std(ddof=1))
    sem = sd / sqrt(n)
    z = 1.96 if abs(level - 0.95) < 1e-9 else 1.96
    return mean - z * sem, mean + z * sem


def summarize_diffs(diff_df: pd.DataFrame, cfg: Dict[str, Any]) -> pd.DataFrame:
    ci_cfg = (cfg.get("ci", {}) or {})
    method = ci_cfg.get("method", "bootstrap")
    level = float(ci_cfg.get("level", 0.95))
    n_boot = int(ci_cfg.get("n_boot", 2000))
    seed = int(ci_cfg.get("seed", 0))

    group_cols = ["Date", "Language", "Word", "TrainingSize", "IntrinsicLevel", "DiffType"]

    out_rows = []
    for keys, g in diff_df.groupby(group_cols, sort=False):
        vals = g["Diff"].to_numpy(dtype=float)
        mean = float(np.mean(vals)) if len(vals) else np.nan
        sd = float(np.std(vals, ddof=1)) if len(vals) > 1 else 0.0

        if method == "bootstrap":
            lo, hi = bootstrap_ci(vals, level=level, n_boot=n_boot, seed=seed)
        elif method == "sem":
            lo, hi = sem_ci(vals, level=level)
        else:
            raise ValueError(f"Unknown ci.method: {method}")

        row = dict(zip(group_cols, keys))
        row.update({
            "n": int(len(vals)),
            "mean": mean,
            "sd": sd,
            "ci_low": lo,
            "ci_high": hi,
            "ci_method": method,
            "ci_level": level,
        })
        out_rows.append(row)

    out = pd.DataFrame(out_rows).sort_values(group_cols).reset_index(drop=True)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--input", default=os.path.join("..", "figdata", "chapter_2_results.csv"),
                    help="Path to the long-format Chapter 2 results CSV.")
    ap.add_argument("--config", default=os.path.join("..", "config", "ch2_selection.yml"),
                    help="YAML config controlling what subset is considered 'main text'.")
    ap.add_argument("--hi_level", type=int, default=67,
                    help="Intrinsic-majority level (default: 67).")
    ap.add_argument("--lo_level", type=int, default=33,
                    help="Relative-majority level (default: 33).")
    ap.add_argument("--out_csv", default=os.path.join("..", "figdata", "ch2_sym_unbalance_summary.csv"),
                    help="Output CSV path.")
    args = ap.parse_args()

    cfg = load_config(args.config)
    df = pd.read_csv(args.input)
    df = filter_df(df, cfg)

    # Only keep the two mirror levels and the two senses we need.
    df = df.loc[df["%IntrinsicData"].isin([args.hi_level, args.lo_level])].copy()
    df = df.loc[df["Sense"].isin(["Int", "Rel"])].copy()
    if len(df) == 0:
        raise SystemExit("No rows remain after filtering to hi/lo levels and Int/Rel senses.")

    # Pivot to get Int and Rel per (datum/iteration)
    idx = ["Date", "Language", "%IntrinsicData", "TrainingSize", "Iteration", "Word"]
    wide = (
        df.pivot_table(index=idx, columns="Sense", values="Share", aggfunc="first")
        .reset_index()
    )
    for col in ["Int", "Rel"]:
        if col not in wide.columns:
            wide[col] = 0.0

    wide = wide.rename(columns={"%IntrinsicData": "IntrinsicLevel"})

    hi_mask = wide["IntrinsicLevel"] == int(args.hi_level)
    lo_mask = wide["IntrinsicLevel"] == int(args.lo_level)

    wide.loc[hi_mask, "Diff"] = wide.loc[hi_mask, "Int"] - wide.loc[hi_mask, "Rel"]
    wide.loc[hi_mask, "DiffType"] = "Int-Rel"

    wide.loc[lo_mask, "Diff"] = wide.loc[lo_mask, "Rel"] - wide.loc[lo_mask, "Int"]
    wide.loc[lo_mask, "DiffType"] = "Rel-Int"

    # Defensive: drop anything unexpected
    wide = wide.loc[wide["DiffType"].isin(["Int-Rel", "Rel-Int"])].copy()

    summary = summarize_diffs(wide[["Date", "Language", "Word", "TrainingSize", "IntrinsicLevel", "DiffType", "Diff"]], cfg)

    os.makedirs(os.path.dirname(args.out_csv), exist_ok=True)
    summary.to_csv(args.out_csv, index=False)
    print(f"Wrote: {args.out_csv}")
    print(f"Rows: {len(summary):,} | (from {len(wide):,} per-iteration diff rows)")


if __name__ == "__main__":
    main()
