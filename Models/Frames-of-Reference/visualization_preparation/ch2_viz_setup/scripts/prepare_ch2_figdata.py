#!/usr/bin/env python3
"""
Prepare "formatting-agnostic" curve-point tables for Chapter 2 learning curves.

Input:  a long/tidy table with columns:
  Date, Language, %IntrinsicData, TrainingSize, Iteration, Word, Sense, Share

Output: an aggregated table with one row per curve point:
  Date, Language, %IntrinsicData, TrainingSize, Word, Sense,
  n, mean, sd, ci_low, ci_high

This script is designed to work out-of-the-box with *no optional parquet dependencies*.
It always writes CSV. Optionally, it can also write Parquet if you have pyarrow installed.
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

    levels = cfg.get("include_intrinsic_levels", None)
    if levels:
        df = df.loc[df["%IntrinsicData"].isin([int(x) for x in levels])].copy()

    languages = cfg.get("include_languages", None)
    if languages:
        df = df.loc[df["Language"].isin(languages)].copy()

    senses = cfg.get("include_senses", None)
    if senses:
        df = df.loc[df["Sense"].isin(senses)].copy()

    return df


def bootstrap_ci(values: np.ndarray, level: float = 0.95, n_boot: int = 2000, seed: int = 0) -> Tuple[float, float]:
    rng = np.random.default_rng(seed)
    values = np.asarray(values, dtype=float)
    if len(values) == 0:
        return (np.nan, np.nan)
    if len(values) == 1:
        return (float(values[0]), float(values[0]))

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
        return (float(values[0]), float(values[0]))
    mean = float(values.mean())
    sd = float(values.std(ddof=1))
    sem = sd / sqrt(n)
    z = 1.96 if abs(level - 0.95) < 1e-9 else 1.96
    return mean - z * sem, mean + z * sem


def summarize(df: pd.DataFrame, cfg: Dict[str, Any]) -> pd.DataFrame:
    ci_cfg = (cfg.get("ci", {}) or {})
    method = ci_cfg.get("method", "bootstrap")
    level = float(ci_cfg.get("level", 0.95))
    n_boot = int(ci_cfg.get("n_boot", 2000))
    seed = int(ci_cfg.get("seed", 0))

    group_cols = ["Date", "Language", "%IntrinsicData", "TrainingSize", "Word", "Sense"]

    out_rows = []
    for keys, g in df.groupby(group_cols, sort=False):
        vals = g["Share"].to_numpy(dtype=float)
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


def try_write_parquet(df: pd.DataFrame, path: str) -> bool:
    try:
        df.to_parquet(path, index=False)
        return True
    except Exception as e:
        print(f"[warn] Could not write parquet ({path}): {e}")
        return False


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--input", default=os.path.join("..", "figdata", "chapter_2_results.csv"),
                    help="Path to the long-format Chapter 2 results CSV.")
    ap.add_argument("--config", default=os.path.join("..", "config", "ch2_selection.yml"),
                    help="YAML config controlling what subset to export for plotting.")
    ap.add_argument("--out_csv", default=os.path.join("..", "figdata", "ch2_summary.csv"),
                    help="Output path for the summary (CSV).")
    ap.add_argument("--out_parquet", default=None,
                    help="Optional Parquet output path (requires pyarrow or fastparquet).")
    args = ap.parse_args()

    cfg = load_config(args.config)
    df = pd.read_csv(args.input)
    df_f = filter_df(df, cfg)

    if len(df_f) == 0:
        raise SystemExit("After filtering, no rows remain. Check your config filters (dates/languages/words).")

    summary = summarize(df_f, cfg)

    os.makedirs(os.path.dirname(args.out_csv), exist_ok=True)
    summary.to_csv(args.out_csv, index=False)
    print(f"Wrote: {args.out_csv}")

    if args.out_parquet:
        os.makedirs(os.path.dirname(args.out_parquet), exist_ok=True)
        if try_write_parquet(summary, args.out_parquet):
            print(f"Wrote: {args.out_parquet}")

    print(f"Rows: {len(summary):,} (from {len(df_f):,} raw rows after filtering)")


if __name__ == "__main__":
    main()
