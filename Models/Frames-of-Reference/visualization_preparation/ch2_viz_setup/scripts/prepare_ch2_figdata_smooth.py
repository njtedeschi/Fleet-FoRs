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
from scipy.signal import savgol_filter
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

def _smooth_1d_ma(y: np.ndarray, window: int) -> np.ndarray:
    """
    Centered moving-average smoothing with edge padding.
    Returns an array of the same length as y.
    """
    y = np.asarray(y, dtype=float)
    n = len(y)
    if window <= 1 or window > n:
        return y
    if window % 2 == 0:
        window += 1  # prefer odd for centering

    pad = window // 2
    ypad = np.pad(y, (pad, pad), mode="edge")
    kernel = np.ones(window, dtype=float) / float(window)
    return np.convolve(ypad, kernel, mode="valid")


def _smooth_1d_median(y: np.ndarray, window: int) -> np.ndarray:
    """
    Centered rolling median smoothing with edge padding.
    Uses a simple loop (window is small, so this is fine).
    """
    y = np.asarray(y, dtype=float)
    n = len(y)
    if window <= 1 or window > n:
        return y
    if window % 2 == 0:
        window += 1

    pad = window // 2
    ypad = np.pad(y, (pad, pad), mode="edge")
    out = np.empty(n, dtype=float)
    for i in range(n):
        out[i] = float(np.median(ypad[i : i + window]))
    return out

def _savgol_1d(y: np.ndarray, window: int, polyorder: int) -> np.ndarray:
    """
    Savitzky–Golay smoothing with safe window adjustment.
    Uses edge-friendly interpolation mode.
    """
    y = np.asarray(y, dtype=float)
    n = len(y)
    if n == 0:
        return y
    # window must be odd and <= n
    window = int(window)
    if window <= 1:
        return y
    if window % 2 == 0:
        window += 1
    if window > n:
        window = n if (n % 2 == 1) else (n - 1)
    if window <= 1:
        return y
    polyorder = int(polyorder)
    polyorder = min(polyorder, window - 1)
    if polyorder < 1:
        return y
    return savgol_filter(y, window_length=window, polyorder=polyorder, mode="interp")


# def smooth_within_iteration(df: pd.DataFrame, cfg: Dict[str, Any]) -> pd.DataFrame:
#     """
#     Smooth Share as a function of TrainingSize *within each Iteration curve*,
#     then return a dataframe with the same rows/columns (Share replaced).
#     """
#     s_cfg = (cfg.get("smoothing", {}) or {})
#     if not bool(s_cfg.get("enabled", False)):
#         return df

#     method = str(s_cfg.get("method", "moving_average")).lower()
#     window = int(s_cfg.get("window", 7))

#     # Each "run" curve
#     gcols = ["Date", "Language", "%IntrinsicData", "Word", "Sense", "Iteration"]

#     df = df.copy()
#     parts = []

#     for _, g in df.groupby(gcols, sort=False):
#         g = g.sort_values("TrainingSize").copy()

#         y = g["Share"].to_numpy(dtype=float)

#         if method in ("moving_average", "ma", "mean"):
#             y_s = _smooth_1d_ma(y, window)
#         elif method in ("median", "moving_median"):
#             y_s = _smooth_1d_median(y, window)
#         else:
#             raise ValueError(f"Unknown smoothing.method: {method}")

#         g["Share"] = y_s
#         parts.append(g)

#     out = pd.concat(parts, ignore_index=True)
#     return out

def smooth_within_iteration(df: pd.DataFrame, cfg: Dict[str, Any]) -> pd.DataFrame:
    """
    Smooth Share as a function of TrainingSize *within each Iteration curve*.
    Uses Savitzky–Golay via SciPy when enabled.
    """
    s_cfg = (cfg.get("smoothing", {}) or {})
    if not bool(s_cfg.get("enabled", False)):
        return df

    window = int(s_cfg.get("window", 9))
    polyorder = int(s_cfg.get("polyorder", 2))

    gcols = ["Date", "Language", "%IntrinsicData", "Word", "Sense", "Iteration"]

    df = df.copy()
    parts = []
    for _, g in df.groupby(gcols, sort=False):
        g = g.sort_values("TrainingSize").copy()
        y = g["Share"].to_numpy(dtype=float)
        g["Share"] = _savgol_1d(y, window=window, polyorder=polyorder)
        parts.append(g)

    return pd.concat(parts, ignore_index=True)


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

def summarize_both(df_raw: pd.DataFrame, df_smooth: pd.DataFrame, cfg: Dict[str, Any]) -> pd.DataFrame:
    group_cols = ["Date", "Language", "%IntrinsicData", "TrainingSize", "Word", "Sense"]

    raw = summarize(df_raw, cfg).rename(columns={
        "n": "n_raw",
        "mean": "mean_raw",
        "sd": "sd_raw",
        "ci_low": "ci_low_raw",
        "ci_high": "ci_high_raw",
    })

    sm = summarize(df_smooth, cfg).rename(columns={
        "n": "n_smooth",
        "mean": "mean_smooth",
        "sd": "sd_smooth",
        "ci_low": "ci_low_smooth",
        "ci_high": "ci_high_smooth",
    })

    # CI metadata columns are identical by construction; keep one set (from raw)
    keep_meta = ["ci_method", "ci_level"]
    sm = sm.drop(columns=[c for c in keep_meta if c in sm.columns], errors="ignore")

    out = raw.merge(sm, on=group_cols, how="inner")

    # Optional: record smoothing parameters
    s_cfg = (cfg.get("smoothing", {}) or {})
    out["smooth_enabled"] = bool(s_cfg.get("enabled", False))
    out["smooth_method"] = str(s_cfg.get("method", "")) if out["smooth_enabled"].any() else ""
    out["smooth_window"] = int(s_cfg.get("window", 0)) if out["smooth_enabled"].any() else 0

    return out.sort_values(group_cols).reset_index(drop=True)


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
    ap.add_argument("--out_csv", default=os.path.join("..", "figdata", "ch2_summary_smooth.csv"),
                    help="Output path for the summary (CSV).")
    ap.add_argument("--out_parquet", default=None,
                    help="Optional Parquet output path (requires pyarrow or fastparquet).")
    args = ap.parse_args()

    cfg = load_config(args.config)
    df = pd.read_csv(args.input)
    df_f = filter_df(df, cfg)

    if len(df_f) == 0:
        raise SystemExit("After filtering, no rows remain. Check your config filters (dates/languages/words).")

    df_raw = df_f
    df_smooth = smooth_within_iteration(df_f, cfg)
    summary = summarize_both(df_raw, df_smooth, cfg)

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
