#!/usr/bin/env python3
"""
render_ch3_old.py

Render Chapter 3 (old model) POCO posterior-share learning curves (formatting-only).

Reads:
  figdata/summary_ch3_old.csv

Uses:
  config/style_ch3_old.yml

Outputs:
  figures/ch3_old/  (by default)

Notes
-----
- No analysis is performed here; we only plot precomputed summary curves.
- Use --filter KEY=VALUE (repeatable) to select experimental conditions.
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
    label_map,
)

HERE = os.path.dirname(os.path.abspath(__file__))


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--summary", default=os.path.join(HERE, "figdata", "summary_ch3_old.csv"),
                    help="Path to the prepared Chapter 3 (old model) summary table (CSV or Parquet).")
    ap.add_argument("--style", default=os.path.join(HERE, "config", "style_ch3_old.yml"),
                    help="YAML style mapping controlling labels/colors/layout.")
    ap.add_argument("--mode", default="grid", choices=["grid", "single"],
                    help="Render a faceted grid or a single plot.")
    ap.add_argument("--word", default="front/behind",
                    help="Which Word pair to plot (front/behind, left/right, above/below, side, or 'all').")
    ap.add_argument("--rows", default="%IntrinsicData",
                    help="Row facet variable (grid mode).")
    ap.add_argument("--cols", default="%CanonicalData",
                    help="Column facet variable (grid mode).")
    ap.add_argument("--filter", action="append", default=[],
                    help="Filter expression(s) KEY=VALUE (repeatable). Used in both modes.")
    ap.add_argument("--group", default=None,
                    help="Optional column to group by: renders one figure per unique value (grid mode).")
    ap.add_argument("--outdir", default=os.path.join(HERE, "figures", "ch3_old"),
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

    if "Word" not in df.columns:
        raise SystemExit("Expected a 'Word' column in summary_ch3_old.csv.")

    words = sorted(df["Word"].unique().tolist())
    to_plot = words if args.word == "all" else [args.word]

    word_labels = style.get("word_labels", {}) or {}

    if args.mode == "grid":
        for w in to_plot:
            df_w = df.loc[df["Word"] == w].copy()
            if len(df_w) == 0:
                continue

            def _render_one(d: pd.DataFrame, suffix_bits: List[str], title_ctx: Dict[str, Any]) -> None:
                outname = f"ch3_old_{w.replace('/', '-')}_grid_r-{args.rows}_c-{args.cols}"
                if suffix_bits:
                    outname += "_" + "_".join(suffix_bits)
                outname += f".{args.fmt}"
                outpath = args.out or os.path.join(args.outdir, outname)
                render_grid(
                    df=d,
                    style=style,
                    row_var=args.rows,
                    col_var=args.cols,
                    series_col="Sense",
                    series_prefix="sense",
                    outpath=outpath,
                    title_ctx=title_ctx,
                )
                print(f"Wrote: {outpath}")

            if args.group:
                if args.group not in df_w.columns:
                    raise SystemExit(f"--group={args.group!r} not in columns: {list(df_w.columns)}")
                for gv in sorted(df_w[args.group].unique().tolist()):
                    d_g = df_w.loc[df_w[args.group] == gv].copy()
                    if len(d_g) == 0:
                        continue
                    _render_one(
                        d=d_g,
                        suffix_bits=[f"{args.group}={str(gv)}"],
                        title_ctx={"word": label_map(w, word_labels), "group": f"{args.group}={gv}"},
                    )
            else:
                _render_one(
                    d=df_w,
                    suffix_bits=[],
                    title_ctx={"word": label_map(w, word_labels)},
                )

    else:
        for w in to_plot:
            d = df.loc[df["Word"] == w].copy()
            if len(d) == 0:
                continue

            title_bits: List[str] = [label_map(w, word_labels)]
            for col in ["Language", "%CanonicalData", "%IntrinsicData", "NoncanonicalOddsReduction", "Date"]:
                if col in d.columns and d[col].nunique() == 1:
                    title_bits.append(f"{col}={d[col].iloc[0]}")

            outname = f"ch3_old_{w.replace('/', '-')}_single.{args.fmt}"
            outpath = args.out or os.path.join(args.outdir, outname)
            render_single(
                df=d,
                style=style,
                series_col="Sense",
                series_prefix="sense",
                outpath=outpath,
                title=" • ".join(title_bits),
            )
            print(f"Wrote: {outpath}")


if __name__ == "__main__":
    main()
