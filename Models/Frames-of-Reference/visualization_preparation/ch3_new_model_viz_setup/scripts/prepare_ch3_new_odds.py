#!/usr/bin/env python3
"""Prepare plot-ready summary curves for Chapter 3 (new model).

Input
-----
A *frozen* consolidated CSV (output of your consolidation/preprocessing step) with
one row per:

    Date × %CanonicalData × %IntrinsicData × NoncanonicalOddsReduction
    × TrainingSize × Iteration × Canonicity

and columns `IntP_mean`, `RelP_mean`.

This script does *not* touch the expensive raw model outputs. It only:
  1) filters rows (dates/levels/max size) via a YAML config,
  2) computes the plotted metric:  diff = IntP_mean - RelP_mean,
  3) aggregates across Iterations to get mean + CI for each curve point.

Output
------
A small summary CSV with one row per curve point:

    Date, %CanonicalData, %IntrinsicData, NoncanonicalOddsReduction,
    TrainingSize, Canonicity,
    n, mean, sd, ci_low, ci_high, ci_method, ci_level

where `mean` is the mean of (IntP_mean - RelP_mean) across Iterations.

"""
from __future__ import annotations

import argparse
import math
import os
from typing import Any, Dict, Iterable, List, Optional, Tuple

import numpy as np
import pandas as pd

try:
    import yaml
except Exception as e:  # pragma: no cover
    raise SystemExit(
        "PyYAML is required for this script. Install with: pip install pyyaml"
    ) from e


REQUIRED_COLS = [
    "Date",
    "%CanonicalData",
    "%IntrinsicData",
    "NoncanonicalOddsReduction",
    "TrainingSize",
    "Iteration",
    "Canonicity",
    "IntP_mean",
    "RelP_mean",
]


def load_yaml(path: str) -> Dict[str, Any]:
    with open(path, "r", encoding="utf-8") as f:
        obj = yaml.safe_load(f) or {}
    if not isinstance(obj, dict):
        raise ValueError(f"YAML config must parse to a dict; got {type(obj)}")
    return obj


def _coerce_scalar(v: Any) -> Any:
    """Coerce config values like 'inf' / '16' into numeric when appropriate."""
    if v is None:
        return None
    if isinstance(v, (int, float, np.floating, np.integer)):
        return float(v) if isinstance(v, (np.floating, float)) else int(v)
    s = str(v).strip()
    if s.lower() in {"inf", "+inf", "infinity", "+infinity"}:
        return float("inf")
    # allow 'nan'
    if s.lower() == "nan":
        return float("nan")
    # int?
    try:
        if re.match(r"^[+-]?\d+$", s):
            return int(s)
    except Exception:
        pass
    # float?
    try:
        return float(s)
    except Exception:
        return s


import re


def _coerce_list(vals: Optional[Iterable[Any]]) -> Optional[List[Any]]:
    if vals is None:
        return None
    out: List[Any] = []
    for v in vals:
        out.append(_coerce_scalar(v))
    return out


def filter_df(df: pd.DataFrame, cfg: Dict[str, Any]) -> pd.DataFrame:
    out = df.copy()

    # Dates
    include_dates = cfg.get("include_dates", None)
    if include_dates:
        include_dates = [str(d) for d in include_dates]
        out = out.loc[out["Date"].astype(str).isin(include_dates)].copy()

    # Max training size
    max_training_size = cfg.get("max_training_size", None)
    if max_training_size is not None:
        out = out.loc[out["TrainingSize"].astype(float) <= float(max_training_size)].copy()

    # Optional factor-level filters
    inc_c = _coerce_list(cfg.get("include_canonical_data_levels", None))
    if inc_c:
        out = out.loc[out["%CanonicalData"].astype(float).isin([float(x) for x in inc_c])].copy()

    inc_i = _coerce_list(cfg.get("include_intrinsic_data_levels", None))
    if inc_i:
        out = out.loc[out["%IntrinsicData"].astype(float).isin([float(x) for x in inc_i])].copy()

    inc_k = _coerce_list(cfg.get("include_odds_reduction", None))
    if inc_k:
        # Column may be numeric with inf, or string. Handle both.
        col = out["NoncanonicalOddsReduction"]
        if pd.api.types.is_numeric_dtype(col):
            keep = []
            for v in inc_k:
                if isinstance(v, str):
                    continue
                keep.append(float(v))
            out = out.loc[col.astype(float).isin(keep)].copy()
        else:
            keep = ["inf" if (isinstance(v, float) and np.isinf(v)) else str(v) for v in inc_k]
            out = out.loc[col.astype(str).isin(keep)].copy()

    return out


def bootstrap_ci(
    values: np.ndarray,
    *,
    level: float = 0.95,
    n_boot: int = 2000,
    rng: np.random.Generator,
) -> Tuple[float, float]:
    values = np.asarray(values, dtype=float)
    n = len(values)
    if n == 0:
        return (float("nan"), float("nan"))
    if n == 1:
        return (float(values[0]), float(values[0]))

    # indices bootstrap for speed
    idx = rng.integers(0, n, size=(n_boot, n))
    boots = values[idx].mean(axis=1)

    alpha = (1.0 - level) / 2.0
    lo = float(np.quantile(boots, alpha))
    hi = float(np.quantile(boots, 1.0 - alpha))
    return (lo, hi)


def sem_ci(values: np.ndarray, *, level: float = 0.95) -> Tuple[float, float]:
    values = np.asarray(values, dtype=float)
    n = len(values)
    if n == 0:
        return (float("nan"), float("nan"))
    if n == 1:
        return (float(values[0]), float(values[0]))
    mean = float(values.mean())
    sd = float(values.std(ddof=1))
    sem = sd / math.sqrt(n)
    # For now, only support 95% z; can expand later if you want t-based CI.
    z = 1.96 if abs(level - 0.95) < 1e-9 else 1.96
    return (mean - z * sem, mean + z * sem)

def log_ratio(int_p: pd.Series, rel_p: pd.Series, *, eps: float = 1e-9) -> pd.Series:
    """Compute log(P(int)/P(rel)) with clipping to avoid log(0)."""
    i = int_p.astype(float).clip(eps, 1.0 - eps)
    r = rel_p.astype(float).clip(eps, 1.0 - eps)
    return np.log(i) - np.log(r)


def summarize(df: pd.DataFrame, cfg: Dict[str, Any]) -> pd.DataFrame:
    missing = [c for c in REQUIRED_COLS if c not in df.columns]
    if missing:
        raise ValueError(
            "Input is missing required columns: " + ", ".join(missing)
            + f"\nAvailable columns: {list(df.columns)}"
        )

    # metric per Iteration row
    work = df.copy()
    eps = float(cfg.get("metric_eps", 1e-9))  # optional config knob
    # work["logIR"] = log_ratio(work["IntP_mean"], work["RelP_mean"], eps=eps)
    work["logIR"] = log_ratio(work["IntP_mean"], 1 - work["IntP_mean"], eps=eps)

    ci_cfg = cfg.get("ci", {}) or {}
    method = str(ci_cfg.get("method", "bootstrap")).strip().lower()
    level = float(ci_cfg.get("level", 0.95))
    n_boot = int(ci_cfg.get("n_boot", 2000))
    seed = int(ci_cfg.get("seed", 0))

    rng = np.random.default_rng(seed)

    group_cols = [
        "Date",
        "%CanonicalData",
        "%IntrinsicData",
        "NoncanonicalOddsReduction",
        "TrainingSize",
        "Canonicity",
    ]

    rows: List[Dict[str, Any]] = []

    for key, g in work.groupby(group_cols, sort=False):
        vals = g["logIR"].to_numpy(dtype=float)
        n = int(len(vals))
        mean = float(np.mean(vals)) if n else float("nan")
        sd = float(np.std(vals, ddof=1)) if n > 1 else 0.0

        if method == "bootstrap":
            lo, hi = bootstrap_ci(vals, level=level, n_boot=n_boot, rng=rng)
        elif method == "sem":
            lo, hi = sem_ci(vals, level=level)
        else:
            raise ValueError(f"Unknown ci.method={method!r} (expected 'bootstrap' or 'sem').")

        row = {col: val for col, val in zip(group_cols, key)}
        row.update(
            {
                "n": n,
                "mean": mean,
                "sd": sd,
                "ci_low": lo,
                "ci_high": hi,
                "ci_method": method,
                "ci_level": level,
            }
        )
        rows.append(row)

    out = pd.DataFrame(rows)

    # stable sorting for readability
    sort_cols = [
        "Date",
        "NoncanonicalOddsReduction",
        "%CanonicalData",
        "%IntrinsicData",
        "Canonicity",
        "TrainingSize",
    ]
    sort_cols = [c for c in sort_cols if c in out.columns]
    out = out.sort_values(sort_cols, kind="mergesort").reset_index(drop=True)
    return out


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "--input",
        default=os.path.join("..", "figdata", "ch3_new_model_consolidated_preprocessed.csv"),
        help="Path to consolidated (preprocessed) new-model results CSV.",
    )
    ap.add_argument(
        "--config",
        default=os.path.join("..", "config", "ch3_new_model_selection.yml"),
        help="YAML selection config controlling what subset to export.",
    )
    ap.add_argument(
        "--out_csv",
        default=os.path.join("..", "figdata", "ch3_new_summary_odds.csv"),
        help="Output path for the summary CSV.",
    )
    args = ap.parse_args()

    cfg = load_yaml(args.config)

    df = pd.read_csv(args.input)
    df_f = filter_df(df, cfg)

    if len(df_f) == 0:
        raise SystemExit(
            "After filtering, no rows remain. Check your selection config (dates/levels/max size)."

        )

    summary = summarize(df_f, cfg)

    os.makedirs(os.path.dirname(args.out_csv), exist_ok=True)
    summary.to_csv(args.out_csv, index=False)
    print(f"Wrote: {args.out_csv}")


if __name__ == "__main__":
    main()
