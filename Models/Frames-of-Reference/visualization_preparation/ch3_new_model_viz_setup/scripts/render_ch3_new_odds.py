#!/usr/bin/env python3
"""Render Chapter 3 (new model) learning-curve figures from a prepared summary table.

Design goal: formatting-only.
This script reads a prepared summary table (mean + CI per curve point) and applies
formatting from a style YAML. No expensive preprocessing here.

Modes
-----
  - --mode grid   : faceted grids (rows x cols)
  - --mode single : one plot for a single experimental condition (via --filter)

The summary table is produced by:
  scripts/prepare_ch3_new_model_figdata.py

"""
from __future__ import annotations

import argparse
import os
import re
from typing import Any, Dict, Iterable, List, Optional, Tuple

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D

try:
    import yaml
except Exception as e:  # pragma: no cover
    raise SystemExit("PyYAML is required. Install with: pip install pyyaml") from e


SUMMARY_REQUIRED = [
    "Date",
    "%CanonicalData",
    "%IntrinsicData",
    "NoncanonicalOddsReduction",
    "TrainingSize",
    "Canonicity",
    "n",
    "mean",
    "ci_low",
    "ci_high",
]


def load_yaml(path: str) -> Dict[str, Any]:
    with open(path, "r", encoding="utf-8") as f:
        obj = yaml.safe_load(f) or {}
    if not isinstance(obj, dict):
        raise ValueError(f"YAML must parse to a dict; got {type(obj)}")
    return obj


def ensure_font(plot_cfg: Dict[str, Any]) -> None:
    font_cfg = (plot_cfg.get("font", {}) or {})
    if not font_cfg:
        return
    # Keep it simple: just set family/size if provided
    if "family" in font_cfg:
        plt.rcParams["font.family"] = font_cfg["family"]
    if "size" in font_cfg:
        plt.rcParams["font.size"] = font_cfg["size"]


def _as_kwargs(obj: Any) -> Dict[str, Any]:
    """Best-effort: coerce a mapping-like object into a kwargs dict."""
    return dict(obj or {}) if isinstance(obj, dict) else {}


def _get_text_and_kwargs(plot_cfg: Dict[str, Any], key: str, default_text: str) -> Tuple[str, Dict[str, Any]]:
    """Get a text string + kwargs from the style YAML.

    Supports either:
      - key: "Some label"
      - key: {text: "Some label", kwargs: {...}}
      - key_kwargs: {...}
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
    """Draw figure-level divider lines between facet rows/cols."""
    fs = _as_kwargs((plot_cfg or {}).get("facet_separators"))
    if not fs or not bool(fs.get("enabled", False)):
        return

    nrows, ncols = axes.shape
    if nrows <= 1 and ncols <= 1:
        return

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


def read_summary(path: str) -> pd.DataFrame:
    df = pd.read_csv(path)
    missing = [c for c in SUMMARY_REQUIRED if c not in df.columns]
    if missing:
        raise ValueError(
            "Summary table missing required columns: " + ", ".join(missing)
            + f"\nAvailable columns: {list(df.columns)}"
        )
    return df


def _coerce_value(s: str) -> Any:
    s = str(s).strip()
    if s.lower() in {"inf", "+inf", "infinity", "+infinity"}:
        return float("inf")
    if s.lower() == "nan":
        return float("nan")
    if re.match(r"^[+-]?\d+$", s):
        try:
            return int(s)
        except Exception:
            pass
    try:
        return float(s)
    except Exception:
        return s


def _parse_filter(expr: str) -> Tuple[str, str]:
    if "=" not in expr:
        raise ValueError(f"Filter must look like Column=Value; got {expr!r}")
    k, v = expr.split("=", 1)
    return k.strip(), v.strip()


def apply_filters(df: pd.DataFrame, filters: List[str]) -> pd.DataFrame:
    out = df.copy()
    for expr in filters:
        k, v_raw = _parse_filter(expr)
        if k not in out.columns:
            raise ValueError(f"Filter column {k!r} not found. Available: {list(out.columns)}")
        v = _coerce_value(v_raw)
        col = out[k]

        # Numeric compare when possible
        if isinstance(v, (int, float, np.floating)) and pd.api.types.is_numeric_dtype(col):
            if isinstance(v, float) and np.isnan(v):
                out = out.loc[col.isna()].copy()
            else:
                out = out.loc[col.astype(float) == float(v)].copy()
        else:
            out = out.loc[col.astype(str) == str(v_raw)].copy()
    return out


def label_map(val: Any, mapping: Optional[Dict[str, Any]]) -> str:
    if mapping is None:
        return str(val)
    if val in mapping:
        return str(mapping[val])
    sval = str(val)
    if sval in mapping:
        return str(mapping[sval])
    return sval


def pick_order(cfg_list: List[Any], present_values: Iterable[Any]) -> List[Any]:
    present_values = list(present_values)
    if cfg_list:
        # Keep only values that actually appear
        return [x for x in cfg_list if x in present_values]
    try:
        return sorted(present_values)
    except Exception:
        return present_values


def _facet_mapping(style: Dict[str, Any], var: str) -> Tuple[List[Any], Optional[Dict[str, Any]]]:
    # returns (order, label_map)
    if var == "%IntrinsicData":
        return style.get("intrinsic_order", []) or [], style.get("intrinsic_labels", {}) or {}
    if var == "%CanonicalData":
        return style.get("canonical_order", []) or [], style.get("canonical_labels", {}) or {}
    if var == "NoncanonicalOddsReduction":
        return style.get("odds_reduction_order", []) or [], style.get("odds_reduction_labels", {}) or {}
    if var == "Date":
        return style.get("date_order", []) or [], style.get("date_labels", {}) or {}
    return [], None


def _legend_once(handles: List[Any], labels: List[str], handle: Any, label: str) -> None:
    if label not in labels:
        handles.append(handle)
        labels.append(label)


def _plot_cell(
    ax: plt.Axes,
    df_cell: pd.DataFrame,
    *,
    canonicity_order: List[Any],
    canonicity_labels: Dict[str, str],
    canonicity_colors: Dict[str, str],
    plot_cfg: Dict[str, Any],
    legend_handles: List[Any],
    legend_labels: List[str],
    legend_collect: bool,
) -> None:
    show_ci = bool(plot_cfg.get("show_ci", True))
    ci_style = str(plot_cfg.get("ci_style", "band")).strip().lower()
    ci_alpha = float(plot_cfg.get("ci_alpha", 0.18))
    lw = float(plot_cfg.get("line_width", 2.0))
    marker = plot_cfg.get("marker", "o")
    markersize = float(plot_cfg.get("markersize", 3.5))

    for c in canonicity_order:
        d = df_cell.loc[df_cell["Canonicity"] == c].copy()
        if len(d) == 0:
            continue
        d = d.sort_values("TrainingSize", kind="mergesort")

        x = d["TrainingSize"].astype(float).to_numpy()
        y = d["mean"].astype(float).to_numpy()
        lo = d["ci_low"].astype(float).to_numpy()
        hi = d["ci_high"].astype(float).to_numpy()

        key = str(c)
        color = canonicity_colors.get(key, None)
        label = canonicity_labels.get(key, key)

        (line,) = ax.plot(
            x,
            y,
            marker=marker,
            markersize=markersize,
            linewidth=lw,
            label=label,
            color=color,
            alpha=0.9,
        )
        if show_ci:
            if ci_style == "bars":
                yerr = np.vstack([y - lo, hi - y])
                ax.errorbar(x, y, yerr=yerr, fmt="none", ecolor=line.get_color(), alpha=0.55, linewidth=1.0, capsize=0)
            else:
                ax.fill_between(x, lo, hi, alpha=ci_alpha, color=line.get_color(), linewidth=0)

        if legend_collect:
            _legend_once(legend_handles, legend_labels, line, label)


def render_single(df: pd.DataFrame, style: Dict[str, Any], outpath: str, title: Optional[str] = None) -> None:
    plot_cfg = style.get("plot", {}) or {}
    ensure_font(plot_cfg)

    can_order = style.get("canonicity_order", []) or sorted(df["Canonicity"].unique().tolist())
    can_labels = style.get("canonicity_labels", {}) or {}
    can_colors = style.get("canonicity_colors", {}) or {}

    fig, ax = plt.subplots(
        figsize=tuple(plot_cfg.get("single_figsize", [5.2, 3.6])),
        dpi=int(plot_cfg.get("dpi", 300)),
    )

    legend_handles: List[Any] = []
    legend_labels: List[str] = []

    _plot_cell(
        ax,
        df,
        canonicity_order=can_order,
        canonicity_labels=can_labels,
        canonicity_colors=can_colors,
        plot_cfg=plot_cfg,
        legend_handles=legend_handles,
        legend_labels=legend_labels,
        legend_collect=True,
    )

    # axes formatting
    xscale = str(plot_cfg.get("xscale", "log"))
    if xscale:
        ax.set_xscale(xscale)

    if plot_cfg.get("y_limits") is not None:
        y0, y1 = plot_cfg["y_limits"]
        ax.set_ylim(float(y0), float(y1))
    elif plot_cfg.get("y_lim") is not None:
        y0, y1 = plot_cfg["y_lim"]
        ax.set_ylim(float(y0), float(y1))

    x_text, x_kw = _get_text_and_kwargs(plot_cfg, "x_label", "Training Size")
    y_text, y_kw = _get_text_and_kwargs(plot_cfg, "y_label", "Intrinsic − Relative probability")
    ax.set_xlabel(x_text, **x_kw)
    ax.set_ylabel(y_text, **y_kw)

    if bool(plot_cfg.get("zero_line", True)):
        ax.axhline(0.0, color="black", linestyle="--", linewidth=1.0, alpha=0.5)

    if bool(plot_cfg.get("grid", True)):
        ax.grid(True, which="both", axis="both", alpha=0.25)

    if title:
        ax.set_title(title, **_as_kwargs(plot_cfg.get("title_kwargs")))

    leg_cfg = style.get("legend", {}) or {}
    if leg_cfg.get("enabled", True):
        leg_kwargs = {
            "loc": leg_cfg.get("single_location", "best"),
            "frameon": bool(leg_cfg.get("frameon", False)),
        }
        leg_kwargs.update(_as_kwargs(leg_cfg.get("kwargs")))
        ax.legend(legend_handles, legend_labels, **leg_kwargs)

    os.makedirs(os.path.dirname(outpath), exist_ok=True)
    fig.tight_layout()
    _add_facet_separators(fig, axes=np.asarray([[ax]]), plot_cfg=plot_cfg)
    fig.savefig(outpath)
    plt.close(fig)


def render_grid(
    df: pd.DataFrame,
    style: Dict[str, Any],
    row_var: str,
    col_var: str,
    outpath: str,
    title: Optional[str] = None,
) -> None:
    plot_cfg = style.get("plot", {}) or {}
    ensure_font(plot_cfg)

    # Determine facet values + ordering
    row_order, row_labels = _facet_mapping(style, row_var)
    col_order, col_labels = _facet_mapping(style, col_var)

    row_values = pick_order(row_order, df[row_var].unique().tolist()) if row_var else [None]
    col_values = pick_order(col_order, df[col_var].unique().tolist()) if col_var else [None]

    nrows, ncols = len(row_values), len(col_values)
    cell_w, cell_h = plot_cfg.get("figsize_per_cell", [3.2, 2.7])
    figsize = (cell_w * max(ncols, 1), cell_h * max(nrows, 1))

    fig, axes = plt.subplots(
        nrows=max(nrows, 1),
        ncols=max(ncols, 1),
        figsize=figsize,
        squeeze=False,
        dpi=int(plot_cfg.get("dpi", 300)),
        sharex=bool(plot_cfg.get("sharex", True)),
        sharey=bool(plot_cfg.get("sharey", True)),
    )

    can_order = style.get("canonicity_order", []) or sorted(df["Canonicity"].unique().tolist())
    can_labels = style.get("canonicity_labels", {}) or {}
    can_colors = style.get("canonicity_colors", {}) or {}

    col_title_kwargs = _as_kwargs(plot_cfg.get("col_title_kwargs"))
    row_label_kwargs = _as_kwargs(plot_cfg.get("row_label_kwargs"))

    legend_handles: List[Any] = []
    legend_labels_list: List[str] = []

    for i, rv in enumerate(row_values):
        for j, cv in enumerate(col_values):
            ax = axes[i][j]

            d = df.copy()
            if row_var:
                d = d.loc[d[row_var] == rv]
            if col_var:
                d = d.loc[d[col_var] == cv]

            if len(d) == 0:
                ax.axis("off")
                continue

            _plot_cell(
                ax,
                d,
                canonicity_order=can_order,
                canonicity_labels=can_labels,
                canonicity_colors=can_colors,
                plot_cfg=plot_cfg,
                legend_handles=legend_handles,
                legend_labels=legend_labels_list,
                legend_collect=(i == 0 and j == 0),
            )

            xscale = str(plot_cfg.get("xscale", "log"))
            if xscale:
                ax.set_xscale(xscale)

            if plot_cfg.get("y_limits") is not None:
                y0, y1 = plot_cfg["y_limits"]
                ax.set_ylim(float(y0), float(y1))
            elif plot_cfg.get("y_lim") is not None:
                y0, y1 = plot_cfg["y_lim"]
                ax.set_ylim(float(y0), float(y1))

            if bool(plot_cfg.get("zero_line", True)):
                ax.axhline(0.0, color="black", linestyle="--", linewidth=1.0, alpha=0.5)

            if bool(plot_cfg.get("grid", True)):
                ax.grid(True, which="both", axis="both", alpha=0.25)

            # Facet labels
            if bool(plot_cfg.get("facet_titles", True)):
                if i == 0 and col_var:
                    ax.set_title(label_map(cv, col_labels), **col_title_kwargs)
                if j == 0 and row_var:
                    ann_kwargs = {
                        "ha": "right",
                        "va": "center",
                        "rotation": 90,
                        "fontsize": plt.rcParams["font.size"],
                    }
                    ann_kwargs.update(row_label_kwargs)
                    ax.annotate(
                        label_map(rv, row_labels),
                        xy=(0, 0.5),
                        xycoords=("axes fraction", "axes fraction"),
                        xytext=(-ax.yaxis.labelpad - 22, 0),
                        textcoords="offset points",
                        **ann_kwargs,
                    )

    x_text, x_kw = _get_text_and_kwargs(plot_cfg, "x_label", "Training Size")
    y_text, y_kw = _get_text_and_kwargs(plot_cfg, "y_label", "Intrinsic − Relative probability")
    fig.supxlabel(x_text, **x_kw)
    fig.supylabel(y_text, **y_kw)

    if title:
        st_kwargs = {"y": 0.99}
        st_kwargs.update(_as_kwargs(plot_cfg.get("suptitle_kwargs")))
        fig.suptitle(title, **st_kwargs)

    leg_cfg = style.get("legend", {}) or {}
    if leg_cfg.get("enabled", True):
        leg_kwargs = {
            "loc": leg_cfg.get("location", "upper center"),
            "ncol": int(leg_cfg.get("ncol", 3)),
            "frameon": bool(leg_cfg.get("frameon", False)),
            "bbox_to_anchor": tuple(leg_cfg.get("bbox_to_anchor", [0.5, 1.06])),
        }
        leg_kwargs.update(_as_kwargs(leg_cfg.get("kwargs")))
        fig.legend(legend_handles, legend_labels_list, **leg_kwargs)

    os.makedirs(os.path.dirname(outpath), exist_ok=True)
    fig.tight_layout()
    _add_facet_separators(fig, axes=axes, plot_cfg=plot_cfg)
    fig.savefig(outpath)
    plt.close(fig)


def _default_title_from_constants(df: pd.DataFrame) -> str:
    # Build a compact title from columns that are constant in df
    bits: List[str] = []
    for col in ["Date", "NoncanonicalOddsReduction", "%CanonicalData", "%IntrinsicData"]:
        if col in df.columns and df[col].nunique() == 1:
            bits.append(f"{col}={df[col].iloc[0]}")
    return " • ".join(bits)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "--summary",
        default=os.path.join("..", "figdata", "ch3_new_summary_odds.csv"),
        help="Path to the prepared summary table (CSV).",
    )
    ap.add_argument(
        "--style",
        default=os.path.join("..", "config", "style_odds.yml"),
        help="YAML style mapping controlling labels/colors/layout.",
    )
    ap.add_argument(
        "--mode",
        default="grid",
        choices=["grid", "single"],
        help="Render a grid of panels or a single plot.",
    )
    ap.add_argument(
        "--rows",
        default="%CanonicalData",
        help="Row facet variable (grid mode). Set '' for none.",
    )
    ap.add_argument(
        "--cols",
        default="%IntrinsicData",
        help="Column facet variable (grid mode). Set '' for none.",
    )
    ap.add_argument(
        "--group",
        default=None,
        help="Optional grouping column: render one figure per unique value.",
    )
    ap.add_argument(
        "--filter",
        action="append",
        default=[],
        help="Filter expression Column=Value (repeatable).",
    )
    ap.add_argument(
        "--outdir",
        default=os.path.join("..", "figures"),
        help="Output directory.",
    )
    ap.add_argument(
        "--out",
        default=None,
        help="Optional explicit output filepath (overrides naming).",
    )
    ap.add_argument(
        "--fmt",
        default="pdf",
        choices=["pdf", "png", "svg"],
        help="Output format.",
    )
    ap.add_argument(
        "--title",
        default=None,
        help="Optional explicit title (overrides auto title).",

    )
    args = ap.parse_args()

    style = load_yaml(args.style)
    df = read_summary(args.summary)
    df = apply_filters(df, args.filter)

    if len(df) == 0:
        raise SystemExit("No rows remain after applying --filter constraints.")

    row_var = args.rows.strip() or ""
    col_var = args.cols.strip() or ""

    def render_one(d: pd.DataFrame, outpath: str) -> None:
        title = args.title if args.title is not None else _default_title_from_constants(d)
        if args.mode == "grid":
            if not row_var or row_var not in d.columns:
                raise SystemExit(f"Row facet column {row_var!r} not found.")
            if not col_var or col_var not in d.columns:
                raise SystemExit(f"Col facet column {col_var!r} not found.")
            render_grid(d, style=style, row_var=row_var, col_var=col_var, outpath=outpath, title=title)
        else:
            # For single plots, we strongly recommend filtering down to one condition.
            for col in ["Date", "%CanonicalData", "%IntrinsicData", "NoncanonicalOddsReduction"]:
                if col in d.columns and d[col].nunique() > 1:
                    raise SystemExit(
                        f"Single-plot mode requires one value for {col!r}; found {d[col].nunique()} values. "
                        f"Use --filter to pin it down, or use --mode grid, or use --group {col}."
                    )
            render_single(d, style=style, outpath=outpath, title=title)

    if args.group:
        if args.group not in df.columns:
            raise SystemExit(f"--group column {args.group!r} not found in summary table.")
        for gv in pick_order([], df[args.group].unique().tolist()):
            d = df.loc[df[args.group] == gv].copy()
            safe_gv = str(gv).replace("/", "-")
            outname = f"ch3_new_model_{args.mode}_g-{args.group}_{safe_gv}_r-{row_var}_c-{col_var}.{args.fmt}" if args.mode == "grid" else f"ch3_new_model_single_g-{args.group}_{safe_gv}.{args.fmt}"
            outpath = args.out or os.path.join(args.outdir, outname)
            render_one(d, outpath)
            print(f"Wrote: {outpath}")
    else:
        outname = f"ch3_new_{args.mode}_r-{row_var}_c-{col_var}_odds.{args.fmt}" if args.mode == "grid" else f"ch3_new_model_single.{args.fmt}"
        outpath = args.out or os.path.join(args.outdir, outname)
        render_one(df, outpath)
        print(f"Wrote: {outpath}")


if __name__ == "__main__":
    main()
