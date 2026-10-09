#!/usr/bin/env python3
"""Render Chapter 2 learning-curve figures from a precomputed summary table.

Design goal: formatting-only. This script intentionally does *no* analysis.
It reads curve points (mean + CI) and applies formatting from a style YAML.

Modes:
  - --mode grid   : faceted grids (rows x cols)
  - --mode single : one plot for a single experimental condition (via --filter)

Examples
--------
# Grid (like before)
python render_ch2_learning_curves.py --word front/behind --rows %IntrinsicData --cols Language

# Single plot: front/behind for Engtec at 67% intrinsic
python render_ch2_learning_curves.py --mode single --word front/behind --filter "Language=Engtec" --filter "%IntrinsicData=67"
"""
from __future__ import annotations

import argparse
import os
from typing import Any, Dict, Iterable, List, Optional, Tuple

import numpy as np
import pandas as pd
import yaml
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D


def _as_kwargs(obj: Any) -> Dict[str, Any]:
    """Best-effort: coerce a mapping-like object into a kwargs dict."""
    return dict(obj or {}) if isinstance(obj, dict) else {}


def _get_text_and_kwargs(plot_cfg: Dict[str, Any], key: str, default_text: str) -> Tuple[str, Dict[str, Any]]:
    """Get a text string + kwargs from the style YAML.

    Supports either:
      - key: "Some label"
      - key: {text: "Some label", kwargs: {...}}
      - key_kwargs: {...}

    The kwargs sources are merged (nested dict first, then key_kwargs), with later
    entries overriding earlier ones.
    """
    val = (plot_cfg or {}).get(key, default_text)
    kw: Dict[str, Any] = {}
    if isinstance(val, dict):
        text = str(val.get("text", default_text))
        kw.update(_as_kwargs(val.get("kwargs")))
    else:
        text = str(val) if val is not None else default_text
    kw.update(_as_kwargs((plot_cfg or {}).get(f"{key}_kwargs")))
    return text, kw


def _add_facet_separators(fig: plt.Figure, axes: np.ndarray, plot_cfg: Dict[str, Any]) -> None:
    """Draw figure-level divider lines between facet rows/cols.

    This is separate from per-axis grid lines: it draws a demarcation between panels.

    Style YAML:
      plot:
        facet_separators:
          enabled: true
          rows: true
          cols: false
          linewidth: 0.8
          color: "0.85"
          alpha: 1.0
          linestyle: "-"
          padding: 0.0
    """
    fs = _as_kwargs((plot_cfg or {}).get("facet_separators"))
    if not fs or not bool(fs.get("enabled", False)):
        return

    nrows, ncols = axes.shape
    if (nrows <= 1 and ncols <= 1) or (nrows <= 1 and not fs.get("cols", False)) or (ncols <= 1 and not fs.get("rows", False)):
        # Nothing to separate.
        return

    # After tight_layout(), axes positions are final.
    positions = [axes[i, j].get_position() for i in range(nrows) for j in range(ncols)]
    left = min(p.x0 for p in positions)
    right = max(p.x1 for p in positions)
    bottom = min(p.y0 for p in positions)
    top = max(p.y1 for p in positions)

    pad = float(fs.get("padding", 0.0) or 0.0)
    left += pad
    right -= pad
    bottom += pad
    top -= pad

    lw = float(fs.get("linewidth", fs.get("line_width", 0.8)))
    color = fs.get("color", "0.85")
    alpha = float(fs.get("alpha", 1.0))
    ls = fs.get("linestyle", "-")
    z = float(fs.get("zorder", 2))

    if bool(fs.get("rows", True)) and nrows > 1:
        for i in range(nrows - 1):
            pos_upper = axes[i, 0].get_position()
            pos_lower = axes[i + 1, 0].get_position()
            y = 0.5 * (pos_lower.y1 + pos_upper.y0)
            fig.add_artist(Line2D([left, right], [y, y], transform=fig.transFigure,
                                  linewidth=lw, color=color, alpha=alpha, linestyle=ls, zorder=z))

    if bool(fs.get("cols", False)) and ncols > 1:
        for j in range(ncols - 1):
            pos_left = axes[0, j].get_position()
            pos_right = axes[0, j + 1].get_position()
            x = 0.5 * (pos_left.x1 + pos_right.x0)
            fig.add_artist(Line2D([x, x], [bottom, top], transform=fig.transFigure,
                                  linewidth=lw, color=color, alpha=alpha, linestyle=ls, zorder=z))


def load_yaml(path: str) -> Dict[str, Any]:
    with open(path, "r") as f:
        return yaml.safe_load(f)


def read_summary(path: str) -> pd.DataFrame:
    if path.lower().endswith(".parquet"):
        return pd.read_parquet(path)
    return pd.read_csv(path)


def pick_order(cfg_list: List[Any], present_values: Iterable[Any]) -> List[Any]:
    present_values = list(present_values)
    if cfg_list:
        return [x for x in cfg_list if x in present_values]
    try:
        return sorted(present_values)
    except Exception:
        return present_values


def label_map(val: Any, mapping: Optional[Dict[str, Any]]) -> str:
    if mapping is None:
        return str(val)
    # try exact and string keys
    if val in mapping:
        return str(mapping[val])
    sval = str(val)
    if sval in mapping:
        return str(mapping[sval])
    return sval


def ensure_font(style_plot: Dict[str, Any]):
    font = (style_plot or {}).get("font", {}) or {}
    if font:
        plt.rcParams.update({
            "font.family": font.get("family", "DejaVu Sans"),
            "font.size": font.get("size", 10),
        })


def _parse_filter(expr: str) -> Tuple[str, str]:
    if "=" not in expr:
        raise ValueError(f"Bad --filter {expr!r}. Expected KEY=VALUE.")
    k, v = expr.split("=", 1)
    return k.strip(), v.strip()


def _coerce_value(v: str) -> Any:
    # Try int, then float, else keep as string
    try:
        if v.lower() in {"nan", "none"}:
            return np.nan
        if v.lower() in {"inf", "+inf", "infinity"}:
            return float("inf")
        if v.lower() in {"-inf", "-infinity"}:
            return float("-inf")
        if "." not in v and "e" not in v.lower():
            return int(v)
        return float(v)
    except Exception:
        return v


def apply_filters(df: pd.DataFrame, filters: List[str]) -> pd.DataFrame:
    out = df.copy()
    for expr in filters:
        k, v_raw = _parse_filter(expr)
        if k not in out.columns:
            raise ValueError(f"Filter column {k!r} not found in summary table. Available: {list(out.columns)}")
        v = _coerce_value(v_raw)
        col = out[k]
        if isinstance(v, (int, float, np.floating)) and pd.api.types.is_numeric_dtype(col):
            if np.isnan(v):
                out = out.loc[col.isna()].copy()
            else:
                # exact for ints; isclose for floats
                if pd.api.types.is_integer_dtype(col):
                    out = out.loc[col == int(v)].copy()
                else:
                    out = out.loc[np.isclose(col.astype(float), float(v), rtol=0.0, atol=1e-9)].copy()
        else:
            out = out.loc[col.astype(str) == str(v_raw)].copy()
    return out


def _legend_once(handles: List[Any], labels: List[str], handle: Any, label: str):
    # Avoid duplicates
    if label not in labels:
        handles.append(handle)
        labels.append(label)


def render_single(
    df: pd.DataFrame,
    style: Dict[str, Any],
    outpath: str,
    title: Optional[str] = None,
):
    plot_cfg = style.get("plot", {}) or {}
    ensure_font(plot_cfg)

    sense_order = pick_order(style.get("sense_order", []), df["Sense"].unique().tolist())
    sense_labels = style.get("sense_labels", {}) or {}
    sense_colors = style.get("sense_colors", {}) or {}

    show_ci = bool(plot_cfg.get("show_ci", True))
    ci_alpha = float(plot_cfg.get("ci_alpha", 0.18))
    lw = float(plot_cfg.get("line_width", 2.0))
    do_grid = bool(plot_cfg.get("grid", True))

    fig, ax = plt.subplots(figsize=tuple(plot_cfg.get("single_figsize", [4.2, 3.0])),
                           dpi=int(plot_cfg.get("dpi", 300)))

    legend_handles: List[Any] = []
    legend_labels: List[str] = []

    for s in sense_order:
        d = df.loc[df["Sense"] == s].sort_values("TrainingSize")
        if len(d) == 0:
            continue
        x = d["TrainingSize"].to_numpy()
        y = d["mean"].to_numpy()
        color = sense_colors.get(s, None)

        (line,) = ax.plot(x, y, linewidth=lw, color=color)
        if show_ci and ("ci_low" in d.columns) and ("ci_high" in d.columns):
            lo = d["ci_low"].to_numpy()
            hi = d["ci_high"].to_numpy()
            ax.fill_between(x, lo, hi, alpha=ci_alpha, color=color, linewidth=0)

        _legend_once(legend_handles, legend_labels, line, label_map(s, sense_labels))

    if do_grid:
        ax.grid(True, alpha=0.25)

    sp = plot_cfg.get("spines", {}) or {}
    if sp.get("top", False) is False:
        ax.spines["top"].set_visible(False)
    if sp.get("right", False) is False:
        ax.spines["right"].set_visible(False)

    if "y_lim" in plot_cfg:
        ax.set_ylim(plot_cfg["y_lim"][0], plot_cfg["y_lim"][1])

    x_text, x_kw = _get_text_and_kwargs(plot_cfg, "x_label", "Training examples")
    y_text, y_kw = _get_text_and_kwargs(plot_cfg, "y_label", "Posterior share")
    ax.set_xlabel(x_text, **x_kw)
    ax.set_ylabel(y_text, **y_kw)

    leg_cfg = plot_cfg.get("legend", {}) or {}
    if leg_cfg.get("enabled", True):
        leg_kwargs = {
            "loc": leg_cfg.get("single_location", "best"),
            "frameon": bool(leg_cfg.get("frameon", False)),
        }
        leg_kwargs.update(_as_kwargs(leg_cfg.get("kwargs")))
        ax.legend(legend_handles, legend_labels, **leg_kwargs)

    if title:
        ax.set_title(title, **_as_kwargs(plot_cfg.get("title_kwargs")))

    fig.tight_layout()
    _add_facet_separators(fig, axes=np.asarray([[ax]]), plot_cfg=plot_cfg)
    os.makedirs(os.path.dirname(outpath), exist_ok=True)
    fig.savefig(outpath, bbox_inches="tight")
    plt.close(fig)


def render_grid(
    df: pd.DataFrame,
    style: Dict[str, Any],
    word: str,
    row_var: str,
    col_var: str,
    outpath: str,
    title: Optional[str] = None,
):
    plot_cfg = style.get("plot", {}) or {}
    ensure_font(plot_cfg)

    df_w = df.loc[df["Word"] == word].copy()
    if len(df_w) == 0:
        raise ValueError(f"No rows found for Word={word!r} in summary table after filtering.")

    row_values = pick_order(
        style.get("language_order", []) if row_var == "Language" else
        style.get("intrinsic_order", []) if row_var == "%IntrinsicData" else
        [],
        df_w[row_var].unique().tolist()
    )
    col_values = pick_order(
        style.get("language_order", []) if col_var == "Language" else
        style.get("intrinsic_order", []) if col_var == "%IntrinsicData" else
        [],
        df_w[col_var].unique().tolist()
    )

    nrows, ncols = len(row_values), len(col_values)
    cell_w, cell_h = plot_cfg.get("figsize_per_cell", [3.2, 2.6])
    figsize = (cell_w * max(ncols, 1), cell_h * max(nrows, 1))

    fig, axes = plt.subplots(
        nrows=max(nrows, 1), ncols=max(ncols, 1),
        sharex=bool(plot_cfg.get("sharex", True)),
        sharey=bool(plot_cfg.get("sharey", True)),
        figsize=figsize,
        squeeze=False,
        dpi=int(plot_cfg.get("dpi", 300)),
    )

    sense_order = pick_order(style.get("sense_order", []), df_w["Sense"].unique().tolist())
    sense_labels = style.get("sense_labels", {}) or {}
    sense_colors = style.get("sense_colors", {}) or {}

    show_ci = bool(plot_cfg.get("show_ci", True))
    ci_alpha = float(plot_cfg.get("ci_alpha", 0.18))
    lw = float(plot_cfg.get("line_width", 2.0))
    do_grid = bool(plot_cfg.get("grid", True))

    lang_labels = style.get("language_labels", {}) or {}
    intrinsic_labels = style.get("intrinsic_labels", {}) or {}

    legend_handles: List[Any] = []
    legend_labels: List[str] = []

    col_title_kwargs = _as_kwargs(plot_cfg.get("col_title_kwargs"))
    row_label_kwargs = _as_kwargs(plot_cfg.get("row_label_kwargs"))

    for i, rv in enumerate(row_values):
        for j, cv in enumerate(col_values):
            ax = axes[i, j]
            panel = df_w.loc[(df_w[row_var] == rv) & (df_w[col_var] == cv)].copy()

            for s in sense_order:
                d = panel.loc[panel["Sense"] == s].sort_values("TrainingSize")
                if len(d) == 0:
                    continue
                x = d["TrainingSize"].to_numpy()
                y = d["mean"].to_numpy()
                color = sense_colors.get(s, None)

                (line,) = ax.plot(x, y, linewidth=lw, color=color)
                if show_ci and ("ci_low" in d.columns) and ("ci_high" in d.columns):
                    lo = d["ci_low"].to_numpy()
                    hi = d["ci_high"].to_numpy()
                    ax.fill_between(x, lo, hi, alpha=ci_alpha, color=color, linewidth=0)

                # collect legend if not already
                _legend_once(legend_handles, legend_labels, line, label_map(s, sense_labels))

            if do_grid:
                ax.grid(True, alpha=0.25)

            sp = plot_cfg.get("spines", {}) or {}
            if sp.get("top", False) is False:
                ax.spines["top"].set_visible(False)
            if sp.get("right", False) is False:
                ax.spines["right"].set_visible(False)

            if i == 0:
                col_title = label_map(
                    cv,
                    lang_labels if col_var == "Language" else intrinsic_labels if col_var == "%IntrinsicData" else None,
                )
                ax.set_title(col_title, **col_title_kwargs)

            if j == 0:
                row_label = label_map(
                    rv,
                    lang_labels if row_var == "Language" else intrinsic_labels if row_var == "%IntrinsicData" else None,
                )
                ann_kwargs = {
                    "ha": "right",
                    "va": "center",
                    "rotation": 90,
                    "fontsize": plt.rcParams["font.size"],
                }
                ann_kwargs.update(row_label_kwargs)
                ax.annotate(
                    row_label,
                    xy=(0, 0.5),
                    xycoords=("axes fraction", "axes fraction"),
                    xytext=(-ax.yaxis.labelpad - 22, 0),
                    textcoords="offset points",
                    **ann_kwargs,
                )

            if "y_lim" in plot_cfg:
                ax.set_ylim(plot_cfg["y_lim"][0], plot_cfg["y_lim"][1])

    x_text, x_kw = _get_text_and_kwargs(plot_cfg, "x_label", "Training examples")
    y_text, y_kw = _get_text_and_kwargs(plot_cfg, "y_label", "Posterior share")
    fig.supxlabel(x_text, **x_kw)
    fig.supylabel(y_text, **y_kw)

    leg_cfg = plot_cfg.get("legend", {}) or {}
    if leg_cfg.get("enabled", True):
        leg_kwargs = {
            "loc": leg_cfg.get("location", "upper center"),
            "ncol": int(leg_cfg.get("ncol", 3)),
            "frameon": bool(leg_cfg.get("frameon", False)),
            "bbox_to_anchor": tuple(leg_cfg.get("bbox_to_anchor", [0.5, 1.02])),
        }
        leg_kwargs.update(_as_kwargs(leg_cfg.get("kwargs")))
        fig.legend(legend_handles, legend_labels, **leg_kwargs)

    if title:
        st_kwargs = {"y": 1.08}
        st_kwargs.update(_as_kwargs(plot_cfg.get("suptitle_kwargs")))
        fig.suptitle(title, **st_kwargs)

    fig.tight_layout()
    _add_facet_separators(fig, axes=axes, plot_cfg=plot_cfg)
    os.makedirs(os.path.dirname(outpath), exist_ok=True)
    fig.savefig(outpath, bbox_inches="tight")
    plt.close(fig)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--summary", default=os.path.join("..", "figdata", "ch2_summary.csv"),
                    help="Path to the prepared summary table (CSV by default).")
    ap.add_argument("--style", default=os.path.join("..", "config", "style.yml"),
                    help="YAML style mapping controlling labels/colors/layout.")
    ap.add_argument("--mode", default="grid", choices=["grid", "single"],
                    help="Render a grid of panels or a single plot.")
    ap.add_argument("--word", default="front/behind",
                    help="Which Word pair to plot (front/behind, left/right, above/below, side, or 'all').")
    ap.add_argument("--rows", default="%IntrinsicData",
                    help="Row facet variable (grid mode).")
    ap.add_argument("--cols", default="Language",
                    help="Column facet variable (grid mode).")
    ap.add_argument("--filter", action="append", default=[],
                    help="Filter expression(s) KEY=VALUE (repeatable). Used in both modes.")
    ap.add_argument("--outdir", default=os.path.join("..", "figures", "out"),
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

    words = sorted(df["Word"].unique().tolist())
    to_plot = words if args.word == "all" else [args.word]

    if args.mode == "grid":
        for w in to_plot:
            outname = f"ch2_{w.replace('/', '-')}_grid_r-{args.rows}_c-{args.cols}.{args.fmt}"
            outpath = args.out or os.path.join(args.outdir, outname)
            render_grid(
                df=df,
                style=style,
                word=w,
                row_var=args.rows,
                col_var=args.cols,
                outpath=outpath,
                title=style.get("word_labels", {}).get(w, w),
            )
            print(f"Wrote: {outpath}")
    else:
        if len(to_plot) != 1:
            raise SystemExit("--mode single requires a single --word (not 'all').")
        w = to_plot[0]
        d = df.loc[df["Word"] == w].copy()
        if len(d) == 0:
            raise SystemExit(f"No rows remain for Word={w!r} after filtering.")
        # Build a compact title from remaining constant columns (excluding the curve axes)
        title_bits: List[str] = [style.get("word_labels", {}).get(w, w)]
        for col in ["Language", "%IntrinsicData", "Date"]:
            if col in d.columns and d[col].nunique() == 1:
                title_bits.append(f"{col}={d[col].iloc[0]}")
        outname = f"ch2_{w.replace('/', '-')}_single.{args.fmt}"
        outpath = args.out or os.path.join(args.outdir, outname)
        render_single(d, style=style, outpath=outpath, title=" • ".join(title_bits))
        print(f"Wrote: {outpath}")


if __name__ == "__main__":
    main()
