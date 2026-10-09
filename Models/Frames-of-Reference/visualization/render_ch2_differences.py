#!/usr/bin/env python3
"""
render_ch2_differences.py

Render Chapter 2 *symmetrically unbalanced* "mirror difference" learning curves.

Reads:
  figdata/summary_ch2_differences.csv

Uses:
  config/style_ch2_differences.yml

Outputs:
  figures/ch2_differences/  (by default)

Notes
-----
- This script intentionally does *no* analysis: it only reads the prepared
  summary table and draws curves.
- The summary table contains:
    DiffType in {Int-Rel, Rel-Int}
  where each corresponds to a complementary intrinsic-frequency condition.

New in this version
-------------------
If you set --rows Word and/or --cols Word in grid mode, this script will render
a *single* multi-word grid (or one per --group value), instead of forcing one
figure per word pair.
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
    ap.add_argument("--summary", default=os.path.join(HERE, "figdata", "summary_ch2_differences.csv"),
                    help="Path to the prepared Chapter 2 differences summary table (CSV or Parquet).")
    ap.add_argument("--style", default=os.path.join(HERE, "config", "style_ch2_differences.yml"),
                    help="YAML style mapping controlling labels/colors/layout.")
    ap.add_argument("--mode", default="grid", choices=["grid", "single"],
                    help="Render a grid of panels or a single plot.")
    ap.add_argument("--word", default="front/behind",
                    help="Which Word pair to plot (front/behind, left/right, above/below, side, or 'all').")
    ap.add_argument("--rows", default="",
                    help="Row facet variable (grid mode). Set '' for none.")
    ap.add_argument("--cols", default="Language",
                    help="Column facet variable (grid mode). Set '' for none.")
    ap.add_argument("--filter", action="append", default=[],
                    help="Filter expression(s) KEY=VALUE (repeatable). Used in both modes.")
    ap.add_argument("--group", default=None,
                    help="Optional column to group by: renders one figure per unique value (grid mode).")
    ap.add_argument("--outdir", default=os.path.join(HERE, "figures", "ch2_differences"),
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
        raise SystemExit("Expected a 'Word' column in summary_ch2_differences.csv.")

    words = sorted(df["Word"].unique().tolist())
    to_plot = words if args.word == "all" else [args.word]

    word_labels = style.get("word_labels", {}) or {}

    word_is_facet = (args.rows == "Word") or (args.cols == "Word")

    if args.mode == "grid":
        # ---------------------------------------------------------------------
        # Special case: Word is used as a facet variable. In that case, render
        # a single multi-word grid (or one per --group value), rather than one
        # figure per word.
        # ---------------------------------------------------------------------
        if word_is_facet:
            d0 = df.loc[df["Word"].isin(to_plot)].copy() if args.word != "all" else df.copy()
            if len(d0) == 0:
                raise SystemExit("No rows remain for the requested --word selection.")

            def _render_one(d: pd.DataFrame, suffix_bits: List[str], title_ctx: Dict[str, Any]) -> None:
                outname = "ch2_differences_multiword_grid"
                if args.rows:
                    outname += f"_r-{args.rows}"
                if args.cols:
                    outname += f"_c-{args.cols}"
                if suffix_bits:
                    outname += "_" + "_".join(suffix_bits)
                outname += f".{args.fmt}"
                outpath = args.out or os.path.join(args.outdir, outname)

                render_grid(
                    df=d,
                    style=style,
                    row_var=args.rows,
                    col_var=args.cols,
                    series_col="DiffType",
                    series_prefix="curve",
                    outpath=outpath,
                    title_ctx=title_ctx,
                )
                print(f"Wrote: {outpath}")

            if args.group:
                if args.group not in d0.columns:
                    raise SystemExit(f"--group={args.group!r} not in columns: {list(d0.columns)}")
                for gv in sorted(d0[args.group].unique().tolist()):
                    d_g = d0.loc[d0[args.group] == gv].copy()
                    if len(d_g) == 0:
                        continue
                    _render_one(
                        d=d_g,
                        suffix_bits=[f"{args.group}={str(gv)}"],
                        title_ctx={"word": "all word pairs", "group": f"{args.group}={gv}"},
                    )
            else:
                title_word = "all word pairs" if args.word == "all" else label_map(args.word, word_labels)
                _render_one(
                    d=d0,
                    suffix_bits=[],
                    title_ctx={"word": title_word},
                )

        # ---------------------------------------------------------------------
        # Default: render one figure per word (original behavior).
        # ---------------------------------------------------------------------
        else:
            for w in to_plot:
                df_w = df.loc[df["Word"] == w].copy()
                if len(df_w) == 0:
                    continue

                def _render_one(d: pd.DataFrame, suffix_bits: List[str], title_ctx: Dict[str, Any]) -> None:
                    outname = f"ch2_differences_{w.replace('/', '-')}_grid"
                    if args.rows:
                        outname += f"_r-{args.rows}"
                    if args.cols:
                        outname += f"_c-{args.cols}"
                    if suffix_bits:
                        outname += "_" + "_".join(suffix_bits)
                    outname += f".{args.fmt}"
                    outpath = args.out or os.path.join(args.outdir, outname)

                    render_grid(
                        df=d,
                        style=style,
                        row_var=args.rows,
                        col_var=args.cols,
                        series_col="DiffType",
                        series_prefix="curve",
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
        # Single-plot mode (unchanged): one output per selected word.
        for w in to_plot:
            d = df.loc[df["Word"] == w].copy()
            if len(d) == 0:
                continue

            title_bits: List[str] = [label_map(w, word_labels)]
            for col in ["Language", "Date"]:
                if col in d.columns and d[col].nunique() == 1:
                    title_bits.append(f"{col}={d[col].iloc[0]}")

            outname = f"ch2_differences_{w.replace('/', '-')}_single.{args.fmt}"
            outpath = args.out or os.path.join(args.outdir, outname)
            render_single(
                df=d,
                style=style,
                series_col="DiffType",
                series_prefix="curve",
                outpath=outpath,
                title=" • ".join(title_bits),
            )
            print(f"Wrote: {outpath}")


if __name__ == "__main__":
    main()
