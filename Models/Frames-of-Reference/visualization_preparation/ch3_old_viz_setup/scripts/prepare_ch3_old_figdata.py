#!/usr/bin/env python3
"""
Prepare "formatting-agnostic" curve-point tables for Chapter 3 (old model).

Inputs:
  - One or more raw files named shares_YYYY-MM-DD.csv (long format), OR
  - A previously consolidated long-format file chapter_3_old_results.csv

Raw schema (per shares file):
  Language, %IntrinsicData, %CanonicalData (optional), NoncanonicalOddsReduction,
  TrainingSize, Iteration, Word, Sense, Share

Outputs:
  1) A consolidated long-format table with a Date column:
       figdata/chapter_3_old_results.csv
  2) A summarized curve-point table (mean + CI across Iteration):
       figdata/ch3_old_summary.csv

This script is fast and does *no* inference—it's just aggregating already-computed shares.
"""
from __future__ import annotations

import argparse
import glob
import os
import re
from typing import Any, Dict, List, Tuple

import numpy as np
import pandas as pd
import yaml


DATE_RE = re.compile(r"shares_(\d{4}-\d{2}-\d{2})\.csv$")


def _as_list(x):
    if x is None:
        return []
    if isinstance(x, (list, tuple)):
        return list(x)
    return [x]


def load_config(path: str) -> Dict[str, Any]:
    with open(path, "r") as f:
        return yaml.safe_load(f)


def infer_date_from_filename(path: str) -> str:
    m = DATE_RE.search(os.path.basename(path))
    if not m:
        raise ValueError(
            f"Could not infer date from filename {path!r}. Expected shares_YYYY-MM-DD.csv"
        )
    return m.group(1)


def consolidate_raw(raw_glob: str, default_canonical: int = 17) -> pd.DataFrame:
    files = sorted(glob.glob(raw_glob))
    if not files:
        raise SystemExit(f"No raw files matched: {raw_glob}")

    parts = []
    for path in files:
        date = infer_date_from_filename(path)
        df = pd.read_csv(path)
        df["Date"] = date

        if "%CanonicalData" not in df.columns:
            # Match the "uniform orientations" setting when missing
            df["%CanonicalData"] = int(default_canonical)

        parts.append(df)

    out = pd.concat(parts, ignore_index=True)

    # Normalize column order (keep any extras at end)
    base_cols = [
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
    extras = [c for c in out.columns if c not in base_cols]
    out = out[base_cols + extras]
    return out


def filter_df(df: pd.DataFrame, cfg: Dict[str, Any]) -> pd.DataFrame:
    df = df.copy()

    # Normalize dtypes
    df["TrainingSize"] = pd.to_numeric(df["TrainingSize"], errors="coerce").astype(int)
    df["%IntrinsicData"] = pd.to_numeric(df["%IntrinsicData"], errors="coerce").astype(int)
    df["%CanonicalData"] = pd.to_numeric(df["%CanonicalData"], errors="coerce").astype(int)
    df["Iteration"] = pd.to_numeric(df["Iteration"], errors="coerce").astype(int)
    # Allow "inf" in odds reduction
    df["NoncanonicalOddsReduction"] = pd.to_numeric(df["NoncanonicalOddsReduction"], errors="coerce")
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

    canon_levels = cfg.get("include_canonical_levels", None)
    if canon_levels:
        df = df.loc[df["%CanonicalData"].isin([int(x) for x in canon_levels])].copy()

    odds_levels = cfg.get("include_odds_reduction", None)
    if odds_levels:
        # accept inf in yaml
        parsed = []
        for x in odds_levels:
            if isinstance(x, str) and x.lower() == "inf":
                parsed.append(float("inf"))
            else:
                parsed.append(float(x))
        df = df.loc[df["NoncanonicalOddsReduction"].isin(parsed)].copy()

    senses = cfg.get("include_senses", None)
    if senses:
        df = df.loc[df["Sense"].isin(senses)].copy()

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

    group_cols = [
        "Date",
        "Language",
        "%IntrinsicData",
        "%CanonicalData",
        "NoncanonicalOddsReduction",
        "TrainingSize",
        "Word",
        "Sense",
    ]

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


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--raw_glob", default=os.path.join("..", "figdata", "raw", "shares_*.csv"),
                    help="Glob for raw shares_YYYY-MM-DD.csv files.")
    ap.add_argument("--config", default=os.path.join("..", "config", "ch3_old_selection.yml"),
                    help="YAML config controlling what subset to export for plotting.")
    ap.add_argument("--default_canonical", type=int, default=17,
                    help="Fill value for %CanonicalData when missing in a raw file.")
    ap.add_argument("--out_consolidated", default=os.path.join("..", "figdata", "chapter_3_old_results.csv"),
                    help="Output consolidated long-format CSV (with Date column).")
    ap.add_argument("--out_summary", default=os.path.join("..", "figdata", "ch3_old_summary.csv"),
                    help="Output summary table (CSV).")
    args = ap.parse_args()

    cfg = load_config(args.config)

    # Consolidate raw (always; it's fast and ensures the 'Date' column exists)
    df_long = consolidate_raw(args.raw_glob, default_canonical=args.default_canonical)
    os.makedirs(os.path.dirname(args.out_consolidated), exist_ok=True)
    df_long.to_csv(args.out_consolidated, index=False)
    print(f"Wrote consolidated: {args.out_consolidated} ({len(df_long):,} rows)")

    # Filter + summarize for plotting
    df_f = filter_df(df_long, cfg)
    if len(df_f) == 0:
        raise SystemExit("After filtering, no rows remain. Check your config filters (dates/levels).")

    summary = summarize(df_f, cfg)
    os.makedirs(os.path.dirname(args.out_summary), exist_ok=True)
    summary.to_csv(args.out_summary, index=False)
    print(f"Wrote summary: {args.out_summary} ({len(summary):,} rows from {len(df_f):,} filtered rows)")


if __name__ == "__main__":
    main()
