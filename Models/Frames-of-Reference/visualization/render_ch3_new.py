#!/usr/bin/env python3
"""
render_ch3_new.py

Render Chapter 3 (new model) Experiment 2 learning curves (formatting-only).

Reads:
  figdata/summary_ch3_new.csv

Uses:
  config/style_ch3_new.yml

Outputs:
  figures/ch3_new/  (by default)

Notes
-----
- No analysis is performed here; we only plot precomputed summary curves.
- Use --filter KEY=VALUE (repeatable) to select experimental conditions.
- Use --group COLUMN to render one grid per unique value of a third condition,
  e.g. --group NoncanonicalOddsReduction or --group Date.
"""

from __future__ import annotations

import argparse
import os
from typing import List, Dict, Any

import pandas as pd

from utils.plot_text_utils import (
    load_yaml,
    read_summary,
    apply_filters,
    render_grid,
    render_single,
)

HERE = os.path.dirname(os.path.abspath(__file__))


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--summary", default=os.path.join(HERE, "figdata", "summary_ch3_new.csv"),
                    help="Path to the prepared Chapter 3 (new model) summary table (CSV or Parquet).")
    ap.add_argument("--style", default=os.path.join(HERE, "config", "style_ch3_new.yml"),
                    help="YAML style mapping controlling labels/colors/layout.")
    ap.add_argument("--mode", default="grid", choices=["grid", "single"],
                    help="Render a faceted grid or a single plot.")
    ap.add_argument("--rows", default="%CanonicalData",
                    help="Row facet variable (grid mode). Set '' for none.")
    ap.add_argument("--cols", default="%IntrinsicData",
                    help="Column facet variable (grid mode). Set '' for none.")
    ap.add_argument("--filter", action="append", default=[],
                    help="Filter expression(s) KEY=VALUE (repeatable). Used in both modes.")
    ap.add_argument("--group", default=None,
                    help="Optional column to group by: renders one figure per unique value (grid mode).")
    ap.add_argument("--outdir", default=os.path.join(HERE, "figures", "ch3_new"),
                    help="Output directory for rendered figures.")
    ap.add_argument("--out", default=None,
                    help="Optional explicit output filepath (overrides --outdir naming).")
    ap.add_argument("--fmt", default="pdf", choices=["pdf", "png", "svg"],
                    help="Output format.")
    args = ap.parse_args()

    style = load_yaml(args.style)
    df = read_summary(args.summary)
    df = apply_filters(df, args.filter)

    if len(df) == 0:
        raise SystemExit("No rows remain after applying --filter constraints.")

    if args.mode == "grid":
        def _render_one(d: pd.DataFrame, suffix_bits: List[str], title_ctx: Dict[str, Any]) -> None:
            outname = f"ch3_new_grid_r-{args.rows}_c-{args.cols}"
            if suffix_bits:
                outname += "_" + "_".join(suffix_bits)
            outname += f".{args.fmt}"
            outpath = args.out or os.path.join(args.outdir, outname)
            render_grid(
                df=d,
                style=style,
                row_var=args.rows,
                col_var=args.cols,
                series_col="Canonicity",
                series_prefix="canonicity",
                outpath=outpath,
                title_ctx=title_ctx,
            )
            print(f"Wrote: {outpath}")

        if args.group:
            if args.group not in df.columns:
                raise SystemExit(f"--group={args.group!r} not in columns: {list(df.columns)}")
            # Sort by numeric if possible
            vals = sorted(df[args.group].unique().tolist(), key=lambda x: (str(type(x)), str(x)))
            for gv in vals:
                d_g = df.loc[df[args.group] == gv].copy()
                if len(d_g) == 0:
                    continue
                _render_one(
                    d=d_g,
                    suffix_bits=[f"{args.group}={str(gv)}"],
                    title_ctx={"group": f"{args.group}={gv}"},
                )
        else:
            _render_one(
                d=df,
                suffix_bits=[],
                title_ctx={},
            )

    else:
        title_bits: List[str] = []
        for col in ["%CanonicalData", "%IntrinsicData", "NoncanonicalOddsReduction", "Date"]:
            if col in df.columns and df[col].nunique() == 1:
                title_bits.append(f"{col}={df[col].iloc[0]}")

        outname = f"ch3_new_single.{args.fmt}"
        outpath = args.out or os.path.join(args.outdir, outname)
        render_single(
            df=df,
            style=style,
            series_col="Canonicity",
            series_prefix="canonicity",
            outpath=outpath,
            title=" • ".join(title_bits) if title_bits else None,
        )
        print(f"Wrote: {outpath}")


if __name__ == "__main__":
    main()
