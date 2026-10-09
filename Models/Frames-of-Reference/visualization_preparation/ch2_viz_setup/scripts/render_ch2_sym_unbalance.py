#!/usr/bin/env python3
"""Render Chapter 2 *symmetrically unbalanced* comparison figures.

These figures plot two "mirror" difference curves:

  • Int-Rel under the intrinsic-majority condition (e.g., 67% intrinsic)
  • Rel-Int under the relative-majority condition (e.g., 33% intrinsic)

The idea is that if the only effect is raw frequency, these two curves should
largely overlap.

Input
-----
Reads the precomputed summary table created by:
  scripts/prepare_ch2_sym_unbalance_figdata.py

Output
------
PDF/PNG/SVG figures, either:
  --mode grid   : one row of panels (by Language) for a given Word
  --mode single : one panel (after --filter constraints)
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
    """Draw figure-level divider lines between facet columns.

    Symmetric-unbalance grids are typically 1×N, but column separators can still help.
    """
    fs = _as_kwargs((plot_cfg or {}).get("facet_separators"))
    if not fs or not bool(fs.get("enabled", False)):
        return

    nrows, ncols = axes.shape
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
    if val in mapping:
        return str(mapping[val])
    sval = str(val)
    if sval in mapping:
        return str(mapping[sval])
    return sval


def ensure_font(plot_cfg: Dict[str, Any]):
    font = (plot_cfg or {}).get("font", {}) or {}
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
            raise ValueError(f"Filter column {k!r} not found. Available: {list(out.columns)}")
        v = _coerce_value(v_raw)
        col = out[k]
        if isinstance(v, (int, float, np.floating)) and pd.api.types.is_numeric_dtype(col):
            if np.isnan(v):
                out = out.loc[col.isna()].copy()
            else:
                if pd.api.types.is_integer_dtype(col):
                    out = out.loc[col == int(v)].copy()
                else:
                    out = out.loc[np.isclose(col.astype(float), float(v), rtol=0.0, atol=1e-9)].copy()
        else:
            out = out.loc[col.astype(str) == str(v_raw)].copy()
    return out


def _legend_once(handles: List[Any], labels: List[str], handle: Any, label: str):
    if label not in labels:
        handles.append(handle)
        labels.append(label)


def _plot_panel(ax: plt.Axes, panel: pd.DataFrame, cfg: Dict[str, Any]):
    plot_cfg = cfg.get("plot", {}) or {}
    curve_order = cfg.get("curve_order", ["Int-Rel", "Rel-Int"])
    curve_labels = cfg.get("curve_labels", {}) or {}
    curve_colors = cfg.get("curve_colors", {}) or {}

    show_ci = bool(plot_cfg.get("show_ci", True))
    ci_alpha = float(plot_cfg.get("ci_alpha", 0.18))
    lw = float(plot_cfg.get("line_width", 2.0))

    legend_handles: List[Any] = []
    legend_labels: List[str] = []

    for c in curve_order:
        d = panel.loc[panel["DiffType"] == c].sort_values("TrainingSize")
        if len(d) == 0:
            continue
        x = d["TrainingSize"].to_numpy()
        y = d["mean"].to_numpy()
        color = curve_colors.get(c, None)
        (line,) = ax.plot(x, y, linewidth=lw, color=color)
        if show_ci and ("ci_low" in d.columns) and ("ci_high" in d.columns):
            lo = d["ci_low"].to_numpy()
            hi = d["ci_high"].to_numpy()
            ax.fill_between(x, lo, hi, alpha=ci_alpha, color=color, linewidth=0)
        _legend_once(legend_handles, legend_labels, line, label_map(c, curve_labels))

    # Zero baseline
    ax.axhline(0.0, linewidth=1.0, alpha=0.5)

    if bool(plot_cfg.get("grid", True)):
        ax.grid(True, alpha=0.25)

    sp = plot_cfg.get("spines", {}) or {}
    if sp.get("top", False) is False:
        ax.spines["top"].set_visible(False)
    if sp.get("right", False) is False:
        ax.spines["right"].set_visible(False)

    if "y_lim" in plot_cfg:
        ax.set_ylim(plot_cfg["y_lim"][0], plot_cfg["y_lim"][1])

    return legend_handles, legend_labels


def render_single(df: pd.DataFrame, style: Dict[str, Any], outpath: str, title: Optional[str] = None):
    plot_cfg = style.get("plot", {}) or {}
    ensure_font(plot_cfg)

    fig, ax = plt.subplots(
        figsize=tuple(plot_cfg.get("single_figsize", [4.6, 3.2])),
        dpi=int(plot_cfg.get("dpi", 300)),
    )

    handles, labels = _plot_panel(ax, df, style)
    x_text, x_kw = _get_text_and_kwargs(plot_cfg, "x_label", "Training examples")
    y_text, y_kw = _get_text_and_kwargs(plot_cfg, "y_label", "Posterior difference")
    ax.set_xlabel(x_text, **x_kw)
    ax.set_ylabel(y_text, **y_kw)
    if title:
        ax.set_title(title, **_as_kwargs(plot_cfg.get("title_kwargs")))

    leg_cfg = plot_cfg.get("legend", {}) or {}
    if leg_cfg.get("enabled", True):
        leg_kwargs = {
            "loc": leg_cfg.get("single_location", "best"),
            "frameon": bool(leg_cfg.get("frameon", False)),
        }
        leg_kwargs.update(_as_kwargs(leg_cfg.get("kwargs")))
        ax.legend(handles, labels, **leg_kwargs)

    fig.tight_layout()
    _add_facet_separators(fig, axes=np.asarray([[ax]]), plot_cfg=plot_cfg)
    os.makedirs(os.path.dirname(outpath), exist_ok=True)
    fig.savefig(outpath, bbox_inches="tight")
    plt.close(fig)


def render_grid_by_language(df: pd.DataFrame, style: Dict[str, Any], word: str, outpath: str, title: Optional[str] = None):
    plot_cfg = style.get("plot", {}) or {}
    ensure_font(plot_cfg)

    df_w = df.loc[df["Word"] == word].copy()
    if len(df_w) == 0:
        raise SystemExit(f"No rows found for Word={word!r} after filtering.")

    lang_order = pick_order(style.get("language_order", []), df_w["Language"].unique().tolist())
    lang_labels = style.get("language_labels", {}) or {}

    ncols = max(len(lang_order), 1)
    fig, axes = plt.subplots(
        nrows=1,
        ncols=ncols,
        sharex=bool(plot_cfg.get("sharex", True)),
        sharey=bool(plot_cfg.get("sharey", True)),
        figsize=(float(plot_cfg.get("figsize_per_cell", [3.4, 2.8])[0]) * ncols,
                 float(plot_cfg.get("figsize_per_cell", [3.4, 2.8])[1])),
        squeeze=False,
        dpi=int(plot_cfg.get("dpi", 300)),
    )

    legend_handles: List[Any] = []
    legend_labels: List[str] = []

    for j, lang in enumerate(lang_order):
        ax = axes[0, j]
        panel = df_w.loc[df_w["Language"] == lang].copy()
        h, l = _plot_panel(ax, panel, style)
        # Keep legend handles from first non-empty panel
        if not legend_handles and h:
            legend_handles, legend_labels = h, l
        ax.set_title(label_map(lang, lang_labels))

    x_text, x_kw = _get_text_and_kwargs(plot_cfg, "x_label", "Training examples")
    y_text, y_kw = _get_text_and_kwargs(plot_cfg, "y_label", "Posterior difference")
    fig.supxlabel(x_text, **x_kw)
    fig.supylabel(y_text, **y_kw)

    leg_cfg = plot_cfg.get("legend", {}) or {}
    if leg_cfg.get("enabled", True):
        leg_kwargs = {
            "loc": leg_cfg.get("location", "upper center"),
            "ncol": int(leg_cfg.get("ncol", 2)),
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
    ap.add_argument("--summary", default=os.path.join("..", "figdata", "ch2_sym_unbalance_summary.csv"),
                    help="Path to the prepared sym-unbalance summary table (CSV by default).")
    ap.add_argument("--style", default=os.path.join("..", "config", "sym_unbalance.yml"),
                    help="YAML style mapping controlling labels/colors/layout for these plots.")
    ap.add_argument("--mode", default="grid", choices=["grid", "single"],
                    help="Render one panel per language (grid) or one panel after filtering (single).")
    ap.add_argument("--word", default="left/right",
                    help="Which Word pair to plot (front/behind, left/right, above/below, side, or 'all').")
    ap.add_argument("--filter", action="append", default=[],
                    help="Filter expression(s) KEY=VALUE (repeatable).")
    ap.add_argument("--outdir", default=os.path.join("..", "figures", "out"),
                    help="Output directory.")
    ap.add_argument("--out", default=None,
                    help="Optional explicit output filepath.")
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

    word_labels = style.get("word_labels", {}) or {}

    for w in to_plot:
        if args.mode == "grid":
            outname = f"ch2_{w.replace('/', '-')}_sym-unbalance_by-Language.{args.fmt}"
            outpath = args.out or os.path.join(args.outdir, outname)
            render_grid_by_language(
                df=df,
                style=style,
                word=w,
                outpath=outpath,
                title=word_labels.get(w, w),
            )
            print(f"Wrote: {outpath}")
        else:
            d = df.loc[df["Word"] == w].copy()
            if len(d) == 0:
                raise SystemExit(f"No rows remain for Word={w!r} after filtering.")
            title_bits: List[str] = [word_labels.get(w, w)]
            for col in ["Language", "Date"]:
                if col in d.columns and d[col].nunique() == 1:
                    title_bits.append(f"{col}={d[col].iloc[0]}")
            outname = f"ch2_{w.replace('/', '-')}_sym-unbalance_single.{args.fmt}"
            outpath = args.out or os.path.join(args.outdir, outname)
            render_single(d, style=style, outpath=outpath, title=" • ".join(title_bits))
            print(f"Wrote: {outpath}")


if __name__ == "__main__":
    main()
