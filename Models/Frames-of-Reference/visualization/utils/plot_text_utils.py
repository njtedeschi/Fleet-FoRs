# utils/plot_text_utils.py
from __future__ import annotations

import os
from dataclasses import dataclass
from typing import Any, Dict, Iterable, List, Optional, Sequence, Tuple

import numpy as np
import pandas as pd
import yaml
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
from matplotlib.patches import FancyArrowPatch

# Enable LaTeX text rendering
plt.rcParams["text.usetex"] = True

# ----------------------------
# YAML helpers
# ----------------------------

def as_dict(obj: Any) -> Dict[str, Any]:
    """Best-effort: coerce a mapping-like object into a plain dict."""
    return dict(obj or {}) if isinstance(obj, dict) else {}


def parse_text_spec(spec: Any, default_text: str) -> Tuple[str, Dict[str, Any]]:
    """Parse a 'text spec' from YAML into (text, kwargs).

    Accepts:
      - "Some text"                     -> ("Some text", {})
      - {text: "Some text", ...kwargs}  -> ("Some text", {kwargs...})
      - { ...kwargs }                  -> (default_text, {kwargs...})
      - {text: "...", kwargs: {...}}   -> merges kwargs + top-level keys
    """
    if spec is None:
        return default_text, {}
    if isinstance(spec, dict):
        d = dict(spec)
        text = str(d.pop("text", default_text))
        kw: Dict[str, Any] = {}
        kw.update(as_dict(d.pop("kwargs", None)))
        # Remaining keys are treated as matplotlib text kwargs.
        kw.update(d)
        return text, kw
    return str(spec), {}


def parse_kwargs_spec(spec: Any) -> Dict[str, Any]:
    """Parse either {kwargs: {...}} or a dict of kwargs."""
    if spec is None:
        return {}
    if not isinstance(spec, dict):
        return {}
    d = dict(spec)
    kw: Dict[str, Any] = {}
    kw.update(as_dict(d.pop("kwargs", None)))
    kw.update(d)
    return kw


def points_to_fig_frac(fig: plt.Figure, points: float, axis: str = "x") -> float:
    """Convert a point size (1/72 inch) to figure-fraction units."""
    points = float(points or 0.0)
    w_in, h_in = fig.get_size_inches()
    inches = points / 72.0
    if axis.lower().startswith("y"):
        return inches / float(h_in)
    return inches / float(w_in)


def maybe_inject_bbox_kwargs(target_kwargs: Dict[str, Any], box_cfg: Dict[str, Any]) -> None:
    """If `box_cfg` is enabled and target_kwargs has no `bbox`, inject a bbox dict."""
    if not isinstance(box_cfg, dict):
        return
    if not bool(box_cfg.get("enabled", False)):
        return
    if "bbox" in target_kwargs:
        return

    bbox = {k: v for k, v in box_cfg.items() if k != "enabled"}
    # Coerce common numeric keys
    for k in ["alpha", "linewidth"]:
        if k in bbox:
            try:
                bbox[k] = float(bbox[k])
            except Exception:
                pass
    target_kwargs["bbox"] = bbox

def _coerce_percent_like(value: Any) -> Optional[float]:
    """Coerce values like 89, "89", "89%", 0.89 into a float in [0, 1] when plausible."""
    if value is None or (isinstance(value, float) and np.isnan(value)):
        return None
    v = value
    if isinstance(v, str):
        s = v.strip()
        if s.endswith("%"):
            s = s[:-1].strip()
        try:
            v = float(s)
        except Exception:
            return None
    try:
        v = float(v)
    except Exception:
        return None
    # Heuristic: if it's > 1, treat as a percent
    if v > 1.0:
        v = v / 100.0
    return v

def maybe_add_target_line(ax, panel, plot_cfg):
    if not bool(plot_cfg.get("target_line", False)):
        return

    from_col = plot_cfg.get("target_line_from", "%IntrinsicData")
    if from_col not in panel.columns:
        return

    vals = panel[from_col].dropna().unique()
    if len(vals) != 1:
        raise ValueError(
            f"target_line enabled but panel has multiple {from_col} values: {list(vals)}"
        )

    p_int_canonical = _coerce_percent_like(vals[0])  # 89 -> 0.89, "89%" -> 0.89, 0.89 -> 0.89
    if p_int_canonical is None:
        return

    kw = {"color": "black", "linestyle": ":", "alpha": 0.45, "linewidth": 1.0, "zorder": 1}
    kw.update(plot_cfg.get("target_line_kwargs", {}) or {})
    if "label" not in kw:
        kw["label"] = "_nolegend_"

    ax.axhline(p_int_canonical, **kw)

    poco_strength_vals = panel["NoncanonicalOddsReduction"].dropna().unique()
    if len(vals) != 1:
        raise ValueError(
            f"target_line enabled but panel has multiple NoncanonicalOddsReduction values: {list(poco_strength_vals)}"
        )
    poco_strength = coerce_value(poco_strength_vals[0])
    if poco_strength is None or poco_strength == float('inf') or poco_strength == 1:
        return
    p_int_noncanonical = p_int_canonical / (poco_strength - (poco_strength-1)*p_int_canonical)
    ax.axhline(p_int_noncanonical, **kw)

def _add_fig_arrow(fig: plt.Figure, start_xy, end_xy, cfg: Dict[str, Any]) -> None:
    """Add an arrow in figure-fraction coordinates."""
    arrowstyle = str(cfg.get("arrowstyle", "-|>"))
    mutation_scale = float(cfg.get("mutation_scale", 14))
    lw = float(cfg.get("linewidth", 1.4))
    color = cfg.get("color", "0.25")
    alpha = float(cfg.get("alpha", 0.8))
    z = float(cfg.get("zorder", 3))

    arr = FancyArrowPatch(
        posA=start_xy,
        posB=end_xy,
        arrowstyle=arrowstyle,
        mutation_scale=mutation_scale,
        linewidth=lw,
        color=color,
        alpha=alpha,
        transform=fig.transFigure,
        clip_on=False,
        zorder=z,
    )
    fig.add_artist(arr)


def maybe_add_outer_facet_arrows(
    fig: plt.Figure,
    plot_cfg: Dict[str, Any],
    outer_cfg: Dict[str, Any],
    geom: Dict[str, float],
    renderer: Any,
    *,
    inner_enabled: bool,
    row_var: Optional[str],
    col_var: Optional[str],
    nrows_val: int,
    ncols_val: int,
    outer_col_artist: Optional[Any],
    outer_row_artist: Optional[Any],
    row_inner_left_edge: Optional[float],
) -> None:
    """Draw optional arrows between outer facet labels and inner facet labels.

    - Column arrow: horizontal, between outer column label and inner column labels (subplot titles).
    - Row arrow: vertical, between outer row label and inner row labels (left strip labels).
    """
    if not inner_enabled:
        return

    # ------------------
    # Column-axis arrow
    # ------------------
    col_arrow = as_dict(outer_cfg.get("col_arrow"))
    col_pos = str(outer_cfg.get("col_position", "top")).lower()
    if (
        bool(col_arrow.get("enabled", False))
        and col_var
        and ncols_val > 1
        and col_pos.startswith("t")
        and outer_col_artist is not None
    ):
        fig.canvas.draw()
        oc_bb = outer_col_artist.get_window_extent(renderer).transformed(fig.transFigure.inverted())

        # Gap is between:
        #   - top of inner titles: geom["content_top"]
        #   - bottom of outer label: oc_bb.y0
        lower = float(geom["content_top"])
        upper = float(oc_bb.y0)

        pad_pts = float(col_arrow.get("pad_points", 2.0))
        pad = points_to_fig_frac(fig, pad_pts, axis="y")
        lower += pad
        upper -= pad

        if upper > lower:
            y = 0.5 * (lower + upper)

            length = float(col_arrow.get("length", 0.75))
            length = max(0.0, min(1.0, length))

            span = float(geom["axes_right"] - geom["axes_left"])
            half = 0.5 * span * length
            x0 = float(geom["axes_x_center"] - half)
            x1 = float(geom["axes_x_center"] + half)

            direction = str(col_arrow.get("direction", "right")).lower()
            if direction.startswith("l"):
                start, end = (x1, y), (x0, y)  # arrow points left
            else:
                start, end = (x0, y), (x1, y)  # arrow points right

            _add_fig_arrow(fig, start, end, col_arrow)

    # ---------------
    # Row-axis arrow
    # ---------------
    row_arrow = as_dict(outer_cfg.get("row_arrow"))
    if (
        bool(row_arrow.get("enabled", False))
        and row_var
        and nrows_val > 1
        and outer_row_artist is not None
        and row_inner_left_edge is not None
    ):
        fig.canvas.draw()
        or_bb = outer_row_artist.get_window_extent(renderer).transformed(fig.transFigure.inverted())

        # Gap is between:
        #   - right edge of outer row label
        #   - left edge of inner row labels
        left = float(or_bb.x1)
        right = float(row_inner_left_edge)

        pad_pts = float(row_arrow.get("pad_points", 2.0))
        pad = points_to_fig_frac(fig, pad_pts, axis="x")
        left += pad
        right -= pad

        if right > left:
            x = 0.5 * (left + right)

            length = float(row_arrow.get("length", 0.75))
            length = max(0.0, min(1.0, length))

            span = float(geom["axes_top"] - geom["axes_bottom"])
            half = 0.5 * span * length
            y0 = float(geom["axes_y_center"] - half)  # bottom endpoint
            y1 = float(geom["axes_y_center"] + half)  # top endpoint

            direction = str(row_arrow.get("direction", "down")).lower()
            if direction.startswith("u"):
                start, end = (x, y0), (x, y1)  # arrow points up
            else:
                start, end = (x, y1), (x, y0)  # arrow points down

            _add_fig_arrow(fig, start, end, row_arrow)

# ----------------------------
# IO + filtering
# ----------------------------

def load_yaml(path: str) -> Dict[str, Any]:
    with open(path, "r") as f:
        data = yaml.safe_load(f)
    return as_dict(data)


def read_summary(path: str) -> pd.DataFrame:
    if path.lower().endswith(".parquet"):
        return pd.read_parquet(path)
    return pd.read_csv(path)


def parse_filter(expr: str) -> Tuple[str, str]:
    if "=" not in expr:
        raise ValueError(f"Bad --filter {expr!r}. Expected KEY=VALUE.")
    k, v = expr.split("=", 1)
    return k.strip(), v.strip()


def coerce_value(v: str) -> Any:
    """Coerce a CLI/YAML string into int/float/inf/nan when plausible."""
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


def apply_filters(df: pd.DataFrame, filters: Sequence[str]) -> pd.DataFrame:
    """Apply KEY=VALUE filters (repeatable). Numeric columns are compared numerically."""
    out = df.copy()
    for expr in filters:
        k, v_raw = parse_filter(expr)
        if k not in out.columns:
            raise ValueError(f"Filter column {k!r} not found in summary table. Available: {list(out.columns)}")

        v = coerce_value(v_raw)
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
            # String comparison (use the raw string for safety)
            out = out.loc[col.astype(str) == str(v_raw)].copy()
    return out


# ----------------------------
# Style resolution helpers
# ----------------------------

def ensure_font(plot_cfg: Dict[str, Any]) -> None:
    font = as_dict((plot_cfg or {}).get("font"))
    if font:
        plt.rcParams.update({
            "font.family": font.get("family", "DejaVu Sans"),
            "font.size": font.get("size", 10),
        })


def mapping_get(mapping: Optional[Dict[str, Any]], key: Any, default: Any = None) -> Any:
    """Get a value from a mapping, trying both the raw key and str(key)."""
    if not mapping:
        return default
    if key in mapping:
        return mapping[key]
    sk = str(key)
    if sk in mapping:
        return mapping[sk]
    return default


def label_map(val: Any, mapping: Optional[Dict[str, Any]]) -> str:
    if mapping is None:
        return str(val)
    if val in mapping:
        return str(mapping[val])
    sval = str(val)
    if sval in mapping:
        return str(mapping[sval])
    return sval


def pick_order(cfg_list: Sequence[Any], present_values: Iterable[Any]) -> List[Any]:
    """Pick an ordering for facet levels.

    - If cfg_list is non-empty: returns only values present in df, in cfg_list order
      (with a few type-robust matching heuristics).
    - If cfg_list is empty: sorted(present_values) when possible.
    """
    present_values = list(present_values)
    if cfg_list:
        out: List[Any] = []
        for item in cfg_list:
            # Direct match
            if item in present_values:
                out.append(item)
                continue

            # If the config item is a string, try coercions (e.g., "inf" -> inf, "67" -> 67)
            cand = coerce_value(item) if isinstance(item, str) else item
            if cand in present_values:
                out.append(cand)
                continue

            # Try string matches to avoid YAML typing gotchas (e.g., "1.0" vs 1.0)
            s_item = str(item)
            found = None
            for pv in present_values:
                if str(pv) == s_item:
                    found = pv
                    break
            if found is not None:
                out.append(found)
                continue

            s_cand = str(cand)
            found = None
            for pv in present_values:
                if str(pv) == s_cand:
                    found = pv
                    break
            if found is not None:
                out.append(found)

        # If nothing matched (often a typing mismatch), fall back to the present values.
        if out:
            return out

    try:
        return sorted(present_values)
    except Exception:
        return present_values


def var_label(style: Dict[str, Any], var: str) -> str:
    """Pretty label for a facet variable name (row/col key)."""
    mapping = as_dict(style.get("facet_var_labels"))
    if var in mapping:
        return str(mapping[var])
    svar = str(var)
    if svar in mapping:
        return str(mapping[svar])
    return svar


class _SafeFormatDict(dict):
    """format_map dict that leaves unknown placeholders intact."""
    def __missing__(self, key: str) -> str:
        return "{" + key + "}"


def format_template(template: str, ctx: Dict[str, Any]) -> str:
    if template is None:
        return ""
    return str(template).format_map(_SafeFormatDict({k: "" if v is None else v for k, v in ctx.items()}))


def resolve_axis_labels(plot_cfg: Dict[str, Any]) -> Tuple[str, Dict[str, Any], str, Dict[str, Any], str]:
    """Returns (x_text, x_kw, y_text, y_kw, show_on_policy)."""
    axis_cfg = as_dict((plot_cfg or {}).get("axis_labels"))

    x_spec = axis_cfg.get("x", (plot_cfg or {}).get("x_label", None))
    y_spec = axis_cfg.get("y", (plot_cfg or {}).get("y_label", None))

    x_text, x_kw = parse_text_spec(x_spec, default_text="Training examples")
    y_text, y_kw = parse_text_spec(y_spec, default_text="Posterior share")

    # Back-compat: merge any legacy *_kwargs (but prefer the new spec)
    x_kw.update(as_dict((plot_cfg or {}).get("x_label_kwargs")))
    y_kw.update(as_dict((plot_cfg or {}).get("y_label_kwargs")))

    show_on = str(axis_cfg.get("show_on", "edge")).lower()
    return x_text, x_kw, y_text, y_kw, show_on


def axis_label_mask(show_on: str, i: int, j: int, nrows: int, ncols: int) -> Tuple[bool, bool]:
    show_on = str(show_on or "edge").lower()
    if show_on in {"none", "off", "false"}:
        return False, False
    if show_on in {"all", "every"}:
        return True, True
    if show_on in {"upper_left", "topleft", "top_left"}:
        return (i == 0 and j == 0), (i == 0 and j == 0)
    # edge: bottom row x-labels; first column y-labels
    return (i == nrows - 1), (j == 0)


def resolve_facet_outer_labels(
    style: Dict[str, Any],
    plot_cfg: Dict[str, Any],
    row_var: Optional[str],
    col_var: Optional[str],
    nrows_val: int,
    ncols_val: int,
    x_fallback: Tuple[str, Dict[str, Any]],
    y_fallback: Tuple[str, Dict[str, Any]],
) -> Tuple[Tuple[str, Dict[str, Any]], Tuple[str, Dict[str, Any]]]:
    """Resolve (col_outer_label, row_outer_label) -> ((text, kw), (text, kw))."""
    facets = as_dict((plot_cfg or {}).get("facets"))
    outer = as_dict(facets.get("outer"))

    # Defaults from facet_var_labels mapping
    default_col = var_label(style, col_var) if col_var else ""
    default_row = var_label(style, row_var) if row_var else ""

    col_spec = outer.get("col", default_col)
    row_spec = outer.get("row", default_row)

    # Degenerate grids: fall back to data-axis labels
    if (not col_var) or (ncols_val <= 1):
        col_text, col_kw = x_fallback
    else:
        col_text, col_kw = parse_text_spec(col_spec, default_col)

    if (not row_var) or (nrows_val <= 1):
        row_text, row_kw = y_fallback
    else:
        row_text, row_kw = parse_text_spec(row_spec, default_row)

    shared = parse_kwargs_spec(outer.get("kwargs"))
    col_kw = {**shared, **col_kw}
    row_kw = {**shared, **row_kw}
    return (col_text, col_kw), (row_text, row_kw)


def get_level_order(style: Dict[str, Any], var: Optional[str]) -> List[Any]:
    if not var:
        return []
    # Generic mapping can override
    generic = as_dict(style.get("facet_level_orders"))
    if var in generic or str(var) in generic:
        return list(mapping_get(generic, var, default=[] ) or [])
    # Common legacy keys
    key_map = {
        "Language": "language_order",
        "%IntrinsicData": "intrinsic_order",
        "IntrinsicLevel": "intrinsic_order",
        "%CanonicalData": "canonical_order",
        "NoncanonicalOddsReduction": "odds_reduction_order",
        "Canonicity": "canonicity_order",
        "DiffType": "curve_order",
        "Word": "word_order",
        "Date": "date_order",
    }
    k = key_map.get(var)
    return list(style.get(k, [])) if k else []


def get_level_labels(style: Dict[str, Any], var: Optional[str]) -> Optional[Dict[str, Any]]:
    if not var:
        return None
    generic = as_dict(style.get("facet_level_labels"))
    if var in generic or str(var) in generic:
        return as_dict(mapping_get(generic, var, default=None))
    key_map = {
        "Language": "language_labels",
        "%IntrinsicData": "intrinsic_labels",
        "IntrinsicLevel": "intrinsic_labels",
        "%CanonicalData": "canonical_labels",
        "NoncanonicalOddsReduction": "odds_reduction_labels",
        "Canonicity": "canonicity_labels",
        "DiffType": "curve_labels",
        "Word": "word_labels",
        "Date": "date_labels",
    }
    k = key_map.get(var)
    return as_dict(style.get(k)) if k else None


def resolve_series(
    style: Dict[str, Any],
    prefix: str,
    present_values: Iterable[Any],
) -> Tuple[List[Any], Dict[str, Any], Dict[str, Dict[str, Any]]]:
    """Resolve (order, label_map, style_kwargs_map) for a series/curve dimension.

    Supports:
      - {prefix}_order
      - {prefix}_labels
      - {prefix}_plot_kwargs (preferred)
      - fallback: {prefix}_colors, {prefix}_linestyles, {prefix}_markers
    """
    present_values = list(present_values)
    order = pick_order(style.get(f"{prefix}_order", []), present_values)
    labels = as_dict(style.get(f"{prefix}_labels"))

    style_map: Dict[str, Dict[str, Any]] = {}

    # Preferred: full kwargs per series value
    pk = as_dict(style.get(f"{prefix}_plot_kwargs") or style.get(f"{prefix}_styles") or {})
    colors = as_dict(style.get(f"{prefix}_colors"))
    linestyles = as_dict(style.get(f"{prefix}_linestyles"))
    markers = as_dict(style.get(f"{prefix}_markers"))

    for v in order or present_values:
        kw: Dict[str, Any] = {}
        c = mapping_get(colors, v, default=None)
        if c is not None:
            kw["color"] = c
        ls = mapping_get(linestyles, v, default=None)
        if ls is not None:
            kw["linestyle"] = ls
        mk = mapping_get(markers, v, default=None)
        if mk is not None:
            kw["marker"] = mk

        v_pk = mapping_get(pk, v, default=None)
        if isinstance(v_pk, dict):
            # merge; per-series overrides fallback
            kw.update(dict(v_pk))

        style_map[str(v)] = kw  # store string-keyed for robustness

    return order, labels, style_map


# ----------------------------
# Plotting primitives
# ----------------------------

def legend_once(handles: List[Any], labels: List[str], handle: Any, label: str) -> None:
    if label not in labels:
        handles.append(handle)
        labels.append(label)


def apply_panel_style(ax: plt.Axes, plot_cfg: Dict[str, Any]) -> None:
    axes_cfg = as_dict((plot_cfg or {}).get("axes"))
    do_grid = bool(axes_cfg.get("grid", (plot_cfg or {}).get("grid", True)))
    if do_grid:
        ax.grid(True, alpha=float(axes_cfg.get("grid_alpha", (plot_cfg or {}).get("grid_alpha", 0.25))))

    sp = as_dict(axes_cfg.get("spines") or (plot_cfg or {}).get("spines"))
    if sp:
        if sp.get("top", False) is False:
            ax.spines["top"].set_visible(False)
        if sp.get("right", False) is False:
            ax.spines["right"].set_visible(False)

    # Axes scales/limits (support legacy keys)
    xscale = (plot_cfg or {}).get("xscale", axes_cfg.get("xscale", None))
    if xscale:
        ax.set_xscale(str(xscale))

    ylim = (plot_cfg or {}).get("y_lim", (plot_cfg or {}).get("y_limits", None))
    if ylim is not None:
        try:
            ax.set_ylim(float(ylim[0]), float(ylim[1]))
        except Exception:
            pass

    xlim = (plot_cfg or {}).get("x_lim", (plot_cfg or {}).get("x_limits", None))
    if xlim is not None:
        try:
            ax.set_xlim(float(xlim[0]), float(xlim[1]))
        except Exception:
            pass

    # Optional zero reference line (legacy key: zero_line)
    if bool((plot_cfg or {}).get("zero_line", False)):
        zl_kw = as_dict((plot_cfg or {}).get("zero_line_kwargs"))
        defaults = {"color": "black", "linestyle": "--", "alpha": 0.5, "linewidth": 1.0, "zorder": 1}
        defaults.update(zl_kw)
        ax.axhline(0.0, **defaults)


def add_facet_separators(fig: plt.Figure, axes: np.ndarray, plot_cfg: Dict[str, Any]) -> None:
    """Draw figure-level divider lines between facet rows/cols."""
    fs = as_dict((plot_cfg or {}).get("facet_separators"))
    if not fs or not bool(fs.get("enabled", False)):
        return

    nrows, ncols = axes.shape
    if (nrows <= 1 and ncols <= 1) or (nrows <= 1 and not fs.get("cols", False)) or (ncols <= 1 and not fs.get("rows", False)):
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


def grid_geometry(fig: plt.Figure, axes: np.ndarray) -> Tuple[Dict[str, float], Any]:
    """Return geometry + renderer.

    Geometry includes:
      - axes_left/right/top/bottom: from axes positions (plot area only)
      - content_left/top/bottom: from tight bboxes (includes tick labels + axis labels + titles)
    """
    fig.canvas.draw()
    renderer = fig.canvas.get_renderer()

    nrows, ncols = axes.shape
    positions = [axes[i, j].get_position() for i in range(nrows) for j in range(ncols)]
    axes_left = float(min(p.x0 for p in positions))
    axes_right = float(max(p.x1 for p in positions))
    axes_bottom = float(min(p.y0 for p in positions))
    axes_top = float(max(p.y1 for p in positions))

    # Tight bboxes for safer "don't overlap" placement.
    first_col = [axes[i, 0] for i in range(nrows)]
    bottom_row = [axes[nrows - 1, j] for j in range(ncols)]
    top_row = [axes[0, j] for j in range(ncols)]

    def _tight(ax: plt.Axes):
        return ax.get_tightbbox(renderer).transformed(fig.transFigure.inverted())

    content_left = float(min(_tight(ax).x0 for ax in first_col))
    content_bottom = float(min(_tight(ax).y0 for ax in bottom_row))
    content_top = float(max(_tight(ax).y1 for ax in top_row))

    geom = {
        "axes_left": axes_left,
        "axes_right": axes_right,
        "axes_bottom": axes_bottom,
        "axes_top": axes_top,
        "axes_x_center": 0.5 * (axes_left + axes_right),
        "axes_y_center": 0.5 * (axes_bottom + axes_top),
        "content_left": content_left,
        "content_bottom": content_bottom,
        "content_top": content_top,
    }
    return geom, renderer

def reorder_legend_by_series_order(
    handles: List[Any],
    labels: List[str],
    series_order: List[Any],
    series_labels: Dict[str, Any],
) -> Tuple[List[Any], List[str]]:
    """
    Reorder legend entries to follow series_order (after label mapping),
    regardless of the order in which entries were first encountered across panels.
    """
    desired = [label_map(s, series_labels) for s in series_order]
    rank = {lab: i for i, lab in enumerate(desired)}

    # Stable sort: keep original relative order for unknown labels.
    indexed = list(enumerate(zip(handles, labels)))
    indexed.sort(key=lambda it: (rank.get(it[1][1], 10**9), it[0]))

    new_handles = [hl[0] for _, hl in indexed]
    new_labels = [hl[1] for _, hl in indexed]
    return new_handles, new_labels

# ----------------------------
# Higher-level renderers
# ----------------------------

def normalize_facet_var(name: Optional[str]) -> Optional[str]:
    if name is None:
        return None
    s = str(name).strip()
    if s == "" or s.lower() in {"none", "null", "nil"}:
        return None
    return s


def _merge_line_kwargs(curves_cfg: Dict[str, Any], per_series: Dict[str, Any]) -> Dict[str, Any]:
    """Merge global + per-series plot kwargs."""
    out: Dict[str, Any] = {}

    base = parse_kwargs_spec(curves_cfg.get("line_kwargs"))
    out.update(base)

    # Back-compat: line_width -> linewidth
    if "linewidth" not in out and ("line_width" in curves_cfg):
        try:
            out["linewidth"] = float(curves_cfg.get("line_width"))
        except Exception:
            pass

    # Series-specific overrides
    out.update({k: v for k, v in (per_series or {}).items() if v is not None})
    return out


def _ci_kwargs(curves_cfg: Dict[str, Any], color: Optional[Any]) -> Dict[str, Any]:
    out: Dict[str, Any] = {}
    out.update(parse_kwargs_spec(curves_cfg.get("ci_kwargs")))
    if "alpha" not in out and "ci_alpha" in curves_cfg:
        try:
            out["alpha"] = float(curves_cfg.get("ci_alpha"))
        except Exception:
            pass
    if "linewidth" not in out:
        out["linewidth"] = 0
    if ("color" not in out) and (color is not None):
        out["color"] = color
    return out


def plot_curves_on_ax(
    ax: plt.Axes,
    panel: pd.DataFrame,
    series_col: str,
    series_order: Sequence[Any],
    series_labels: Dict[str, Any],
    series_style_map: Dict[str, Dict[str, Any]],
    plot_cfg: Dict[str, Any],
    x_col: str = "TrainingSize",
    y_col: str = "mean",
    lo_col: str = "ci_low",
    hi_col: str = "ci_high",
    legend_handles: Optional[List[Any]] = None,
    legend_labels: Optional[List[str]] = None,
) -> None:
    curves_cfg = as_dict((plot_cfg or {}).get("curves"))
    show_ci = bool(curves_cfg.get("show_ci", (plot_cfg or {}).get("show_ci", True)))
    ci_style = str(curves_cfg.get("ci_style", (plot_cfg or {}).get("ci_style", "band"))).lower()

    for s in series_order:
        d = panel.loc[panel[series_col] == s].sort_values(x_col)
        if len(d) == 0:
            continue
        x = d[x_col].to_numpy()
        y = d[y_col].to_numpy()

        per_series = series_style_map.get(str(s), {})
        line_kw = _merge_line_kwargs(curves_cfg, per_series)
        (line,) = ax.plot(x, y, **line_kw)

        if show_ci and (lo_col in d.columns) and (hi_col in d.columns):
            lo = d[lo_col].to_numpy()
            hi = d[hi_col].to_numpy()
            if ci_style.startswith("b"):  # band
                ax.fill_between(x, lo, hi, **_ci_kwargs(curves_cfg, color=line_kw.get("color", None)))
            else:
                # Bars: draw symmetric-ish errorbars using bounds
                yerr = np.vstack([y - lo, hi - y])
                eb_kw = as_dict(curves_cfg.get("ci_bar_kwargs"))
                if "ecolor" not in eb_kw and line_kw.get("color", None) is not None:
                    eb_kw["ecolor"] = line_kw["color"]
                if "alpha" not in eb_kw and "ci_alpha" in curves_cfg:
                    try:
                        eb_kw["alpha"] = float(curves_cfg.get("ci_alpha"))
                    except Exception:
                        pass
                if "elinewidth" not in eb_kw and "linewidth" in line_kw:
                    eb_kw["elinewidth"] = line_kw["linewidth"]
                ax.errorbar(x, y, yerr=yerr, fmt="none", **eb_kw)

        if legend_handles is not None and legend_labels is not None:
            legend_once(legend_handles, legend_labels, line, label_map(s, series_labels))

def infer_title_ctx_from_df(
    df: pd.DataFrame,
    style: Dict[str, Any],
    base: Optional[Dict[str, Any]] = None,
) -> Dict[str, Any]:
    """
    Infer common title placeholders from a single-panel dataframe.
    Currently fills:
      - {word} from df["Word"] if unique
    """
    ctx = dict(base or {})

    # Fill {word} automatically when possible
    if "word" not in ctx and "Word" in df.columns and df["Word"].nunique() == 1:
        word_labels = as_dict(style.get("word_labels"))
        ctx["word"] = label_map(df["Word"].iloc[0], word_labels)

    return ctx

def maybe_add_figure_title(
    fig: plt.Figure,
    axes: np.ndarray,
    style: Dict[str, Any],
    plot_cfg: Dict[str, Any],
    title_ctx: Optional[Dict[str, Any]] = None,
) -> None:
    title_cfg = as_dict(plot_cfg.get("figure_title"))
    template = title_cfg.get("template", None)
    if not template:
        return

    ctx: Dict[str, Any] = {}
    if title_ctx:
        ctx.update(title_ctx)

    title_text = format_template(str(template), ctx)
    if not title_text:
        return

    # Match grid behavior: place after tight_layout using geometry
    geom, renderer = grid_geometry(fig, axes)

    pad_points = float(title_cfg.get("pad_points", 8.0))
    title_kw = parse_kwargs_spec(title_cfg.get("kwargs"))
    title_kw.update(as_dict(plot_cfg.get("suptitle_kwargs")))

    y = geom["content_top"] + points_to_fig_frac(fig, pad_points, axis="y")
    defaults = {"ha": "center", "va": "bottom"}
    defaults.update(title_kw)

    fig.text(
        geom["axes_x_center"],
        y,
        title_text,
        transform=fig.transFigure,
        **defaults,
    )

def render_single(
    df: pd.DataFrame,
    style: Dict[str, Any],
    series_col: str,
    series_prefix: str,
    outpath: str,
    title: Optional[str] = None, # kept for backward compatibility
    title_ctx: Optional[Dict[str, Any]] = None,
    x_col: str = "TrainingSize",
    y_col: str = "mean",
    lo_col: str = "ci_low",
    hi_col: str = "ci_high",
) -> None:
    plot_cfg = as_dict(style.get("plot"))
    ensure_font(plot_cfg)

    present = df[series_col].unique().tolist()
    series_order, series_labels, series_style_map = resolve_series(style, series_prefix, present)

    fig, ax = plt.subplots(
        figsize=tuple(plot_cfg.get("single_figsize", [4.2, 3.0])),
        dpi=int(plot_cfg.get("dpi", 300)),
    )

    legend_handles: List[Any] = []
    legend_labels: List[str] = []

    plot_curves_on_ax(
        ax=ax,
        panel=df,
        series_col=series_col,
        series_order=series_order,
        series_labels=series_labels,
        series_style_map=series_style_map,
        plot_cfg=plot_cfg,
        x_col=x_col,
        y_col=y_col,
        lo_col=lo_col,
        hi_col=hi_col,
        legend_handles=legend_handles,
        legend_labels=legend_labels,
    )

    apply_panel_style(ax, plot_cfg)
    maybe_add_target_line(ax, panel=df, plot_cfg=plot_cfg)

    x_text, x_kw, y_text, y_kw, _ = resolve_axis_labels(plot_cfg)
    ax.set_xlabel(x_text, **x_kw)
    ax.set_ylabel(y_text, **y_kw)

    # Enforce legend order from config (important when some panels lack some series)
    legend_handles, legend_labels = reorder_legend_by_series_order(
        legend_handles, legend_labels, series_order, series_labels
    )

    # Single-plot legend (inside axes by default).
    leg_cfg = as_dict(plot_cfg.get("legend"))
    if bool(leg_cfg.get("enabled", True)):
        leg_kwargs = {
            "loc": leg_cfg.get("loc_inside", leg_cfg.get("single_location", "best")),
            "frameon": bool(leg_cfg.get("frameon", False)),
        }
        leg_kwargs.update(as_dict(leg_cfg.get("kwargs")))
        ax.legend(legend_handles, legend_labels, **leg_kwargs)

    # # Obsolete behavior: using axis title formatting
    # if title:
    #     title_kw = parse_kwargs_spec(plot_cfg.get("panel_title"))
    #     # Back-compat: allow legacy title_kwargs
    #     title_kw.update(as_dict(plot_cfg.get("title_kwargs")))
    #     ax.set_title(title, **title_kw)

    # # Obsolete continuation
    # fig.tight_layout(pad=float(as_dict(plot_cfg.get("layout")).get("tight_layout_pad", 1.0)))
    # os.makedirs(os.path.dirname(outpath), exist_ok=True)
    # fig.savefig(outpath, bbox_inches="tight")
    # plt.close(fig)

    fig.tight_layout(pad=float(as_dict(plot_cfg.get("layout")).get("tight_layout_pad", 1.0)))
    title_ctx = infer_title_ctx_from_df(df, style, base=title_ctx)

    # Use the SAME figure-title logic as grid mode
    maybe_add_figure_title(
        fig=fig,
        axes=np.array([[ax]]),
        style=style,
        plot_cfg=plot_cfg,
        title_ctx=title_ctx,
    )

    os.makedirs(os.path.dirname(outpath), exist_ok=True)
    fig.savefig(outpath, bbox_inches="tight")
    plt.close(fig)



def render_grid(
    df: pd.DataFrame,
    style: Dict[str, Any],
    row_var: Optional[str],
    col_var: Optional[str],
    series_col: str,
    series_prefix: str,
    outpath: str,
    title_ctx: Optional[Dict[str, Any]] = None,
    x_col: str = "TrainingSize",
    y_col: str = "mean",
    lo_col: str = "ci_low",
    hi_col: str = "ci_high",
) -> None:
    plot_cfg = as_dict(style.get("plot"))
    ensure_font(plot_cfg)

    row_var = normalize_facet_var(row_var)
    col_var = normalize_facet_var(col_var)

    # Determine facet levels
    if row_var:
        row_values = pick_order(get_level_order(style, row_var), df[row_var].unique().tolist())
    else:
        row_values = [None]
    if col_var:
        col_values = pick_order(get_level_order(style, col_var), df[col_var].unique().tolist())
    else:
        col_values = [None]

    nrows_val, ncols_val = len(row_values), len(col_values)
    nrows, ncols = max(nrows_val, 1), max(ncols_val, 1)

    cell_w, cell_h = plot_cfg.get("figsize_per_cell", [3.2, 2.6])
    figsize = (float(cell_w) * ncols, float(cell_h) * nrows)

    fig, axes = plt.subplots(
        nrows=nrows,
        ncols=ncols,
        sharex=bool(plot_cfg.get("sharex", True)),
        sharey=bool(plot_cfg.get("sharey", True)),
        figsize=figsize,
        squeeze=False,
        dpi=int(plot_cfg.get("dpi", 300)),
    )

    present = df[series_col].unique().tolist()
    series_order, series_labels, series_style_map = resolve_series(style, series_prefix, present)

    # Facet value label maps
    row_value_labels = get_level_labels(style, row_var) if row_var else None
    col_value_labels = get_level_labels(style, col_var) if col_var else None

    # Facet config
    facets = as_dict(plot_cfg.get("facets"))
    outer = as_dict(facets.get("outer"))
    inner = as_dict(facets.get("inner"))
    use_outer = bool(outer.get("enabled", False))

    inner_enabled = bool(inner.get("enabled", True))
    row_orient = str(inner.get("row_orientation", "vertical")).lower()
    row_pad_points = float(inner.get("row_pad_points", 8.0))
    inner_col_pad_points = inner.get("col_pad_points", None)

    inner_row_kw = parse_kwargs_spec(inner.get("row"))
    inner_col_kw = parse_kwargs_spec(inner.get("col"))

    # Apply the inner column pad unless the user explicitly set a title pad in inner.col.
    if inner_col_pad_points is not None and "pad" not in inner_col_kw:
        try:
            inner_col_kw["pad"] = float(inner_col_pad_points)
        except Exception:
            pass

    # Optional "strip" boxes for the inner labels
    bbox_cfg = as_dict(inner.get("bbox"))
    row_bbox_cfg = as_dict(bbox_cfg.get("row"))
    col_bbox_cfg = as_dict(bbox_cfg.get("col"))
    maybe_inject_bbox_kwargs(inner_row_kw, row_bbox_cfg)
    maybe_inject_bbox_kwargs(inner_col_kw, col_bbox_cfg)

    # Data axis labels
    x_text, x_kw, y_text, y_kw, show_on = resolve_axis_labels(plot_cfg)

    legend_handles: List[Any] = []
    legend_labels: List[str] = []

    # --- Panels ---
    for i, rv in enumerate(row_values):
        for j, cv in enumerate(col_values):
            ax = axes[i, j]
            panel = df
            if row_var:
                panel = panel.loc[panel[row_var] == rv]
            if col_var:
                panel = panel.loc[panel[col_var] == cv]
            panel = panel.copy()

            plot_curves_on_ax(
                ax=ax,
                panel=panel,
                series_col=series_col,
                series_order=series_order,
                series_labels=series_labels,
                series_style_map=series_style_map,
                plot_cfg=plot_cfg,
                x_col=x_col,
                y_col=y_col,
                lo_col=lo_col,
                hi_col=hi_col,
                legend_handles=legend_handles,
                legend_labels=legend_labels,
            )

            apply_panel_style(ax, plot_cfg)
            maybe_add_target_line(ax, panel=panel, plot_cfg=plot_cfg)

            # Column "inner" labels are subplot titles.
            if inner_enabled and col_var and i == 0 and ncols_val > 1:
                ax.set_title(label_map(cv, col_value_labels), **inner_col_kw)

            # Axis labels:
            if use_outer:
                show_x, show_y = axis_label_mask(show_on, i=i, j=j, nrows=nrows, ncols=ncols)
                if show_x:
                    ax.set_xlabel(x_text, **x_kw)
                if show_y:
                    ax.set_ylabel(y_text, **y_kw)

            # If we're not using outer facet labels, keep classic in-panel row headers.
            if inner_enabled and (not use_outer) and row_var and j == 0 and nrows_val > 1:
                rotation = 90 if row_orient.startswith("v") else 0
                ann_kwargs = {"ha": "right", "va": "center", "rotation": rotation, "fontsize": plt.rcParams["font.size"]}
                ann_kwargs.update(inner_row_kw)
                offset_pts = -ax.yaxis.labelpad - 22
                ax.annotate(
                    label_map(rv, row_value_labels),
                    xy=(0, 0.5),
                    xycoords=("axes fraction", "axes fraction"),
                    xytext=(offset_pts, 0),
                    textcoords="offset points",
                    **ann_kwargs,
                )

    # Figure-level x/y labels if not using outer facet labels
    if not use_outer:
        fig.supxlabel(x_text, **x_kw)
        fig.supylabel(y_text, **y_kw)

    # Layout for panels (only). Figure-level texts are placed *after* this.
    layout_cfg = as_dict(plot_cfg.get("layout"))
    fig.tight_layout(pad=float(layout_cfg.get("tight_layout_pad", 1.0)))

    add_facet_separators(fig, axes=axes, plot_cfg=plot_cfg)

    # --- Geometry (after tight_layout) ---
    geom, renderer = grid_geometry(fig, axes)

    outer_row_artist = None
    outer_col_artist = None

    # --- Facet labels (outer+inner row labels) ---
    if use_outer:
        # Inner row labels (one per row), aligned to a stable left edge.
        if inner_enabled and row_var and nrows_val > 1:
            row_inner_anchor_x = geom["content_left"] - points_to_fig_frac(fig, row_pad_points, axis="x")
            inner_row_rotation = 90 if row_orient.startswith("v") else 0

            row_text_kwargs = {"ha": "right", "va": "center", "rotation": inner_row_rotation, "fontsize": plt.rcParams["font.size"]}
            row_text_kwargs.update(inner_row_kw)

            row_text_artists = []
            for i, rv in enumerate(row_values):
                pos = axes[i, 0].get_position()
                y_center = 0.5 * (pos.y0 + pos.y1)
                t = fig.text(
                    row_inner_anchor_x,
                    y_center,
                    label_map(rv, row_value_labels),
                    transform=fig.transFigure,
                    **row_text_kwargs,
                )
                row_text_artists.append(t)

            fig.canvas.draw()
            row_label_left_edge = min(
                t.get_window_extent(renderer).transformed(fig.transFigure.inverted()).x0
                for t in row_text_artists
            )
        else:
            # No row facet: treat the left edge of content as the anchor.
            row_label_left_edge = geom["content_left"]

        # Outer labels (variable names)
        (outer_col_text, outer_col_kw), (outer_row_text, outer_row_kw) = resolve_facet_outer_labels(
            style=style,
            plot_cfg=plot_cfg,
            row_var=row_var,
            col_var=col_var,
            nrows_val=nrows_val,
            ncols_val=ncols_val,
            x_fallback=(x_text, x_kw),
            y_fallback=(y_text, y_kw),
        )

        # Outer row label placement: to the left of the inner row labels.
        if row_var and nrows_val > 1:
            outer_row_pad_points = float(outer.get("row_pad_points", outer.get("row_gap_points", 18.0)))
            outer_row_x = row_label_left_edge - points_to_fig_frac(fig, outer_row_pad_points, axis="x")

            row_outer_orient = str(outer.get("row_orientation", "vertical")).lower()
            row_outer_rot = 90 if row_outer_orient.startswith("v") else 0

            outer_row_kwargs = {"ha": "right", "va": "center", "rotation": row_outer_rot}
            outer_row_kwargs.update(outer_row_kw)
            ###
            # fig.text(outer_row_x, geom["axes_y_center"], outer_row_text, transform=fig.transFigure, **outer_row_kwargs)
            outer_row_artist = fig.text(
                outer_row_x,
                geom["axes_y_center"],
                outer_row_text,
                transform=fig.transFigure,
                **outer_row_kwargs
            )
            ###
        # Outer column label placement
        if col_var and ncols_val > 1:
            col_pos = str(outer.get("col_position", "top")).lower()
            col_pad_points = float(outer.get("col_pad_points", 0.0))

            if col_pos.startswith("t"):
                y0 = geom["content_top"] + points_to_fig_frac(fig, col_pad_points, axis="y")
                outer_col_kwargs = {"ha": "center", "va": "bottom"}
                outer_col_kwargs.update(outer_col_kw)
                outer_col_artist = fig.text(
                    geom["axes_x_center"],
                    y0,
                    outer_col_text,
                    transform=fig.transFigure,
                    **outer_col_kwargs,
                )
            elif col_pos.startswith("b"):
                fig.supxlabel(outer_col_text, **outer_col_kw)
        ###
        maybe_add_outer_facet_arrows(
            fig=fig,
            plot_cfg=plot_cfg,
            outer_cfg=outer,
            geom=geom,
            renderer=renderer,
            inner_enabled=inner_enabled,
            row_var=row_var,
            col_var=col_var,
            nrows_val=nrows_val,
            ncols_val=ncols_val,
            outer_col_artist=outer_col_artist,
            outer_row_artist=outer_row_artist,
            row_inner_left_edge=row_label_left_edge
        )
        ###
    # --- Figure title ---
    title_cfg = as_dict(plot_cfg.get("figure_title"))
    template = title_cfg.get("template", None)
    title_text = ""
    if template:
        ctx = {}
        if title_ctx:
            ctx.update(title_ctx)
        # Provide common placeholders even if caller didn't
        if "row_var" not in ctx and row_var:
            ctx["row_var"] = var_label(style, row_var)
        if "col_var" not in ctx and col_var:
            ctx["col_var"] = var_label(style, col_var)
        title_text = format_template(str(template), ctx)

    if title_text:
        title_pad_points = float(title_cfg.get("pad_points", 8.0))
        title_kw = parse_kwargs_spec(title_cfg.get("kwargs"))
        # Back-compat: allow legacy suptitle_kwargs
        title_kw.update(as_dict(plot_cfg.get("suptitle_kwargs")))

        base_y = geom["content_top"]
        if outer_col_artist is not None:
            fig.canvas.draw()
            oc_bb = outer_col_artist.get_window_extent(renderer).transformed(fig.transFigure.inverted())
            base_y = float(oc_bb.y1)

        y = base_y + points_to_fig_frac(fig, title_pad_points, axis="y")
        title_defaults = {"ha": "center", "va": "bottom"}
        title_defaults.update(title_kw)
        fig.text(geom["axes_x_center"], y, title_text, transform=fig.transFigure, **title_defaults)

    # --- Legend (grid-level) ---

    # Enforce legend order from config (important when some panels lack some series)
    legend_handles, legend_labels = reorder_legend_by_series_order(
        legend_handles, legend_labels, series_order, series_labels
    )

    leg_cfg = as_dict(plot_cfg.get("legend"))
    if bool(leg_cfg.get("enabled", True)):
        pos = str(leg_cfg.get("position", "top")).lower()
        pad_points = float(leg_cfg.get("pad_points", 8.0))
        frameon = bool(leg_cfg.get("frameon", False))
        ncol = int(leg_cfg.get("ncol", max(1, len(legend_labels))))
        leg_kwargs = {"frameon": frameon, "ncol": ncol}
        leg_kwargs.update(as_dict(leg_cfg.get("kwargs")))

        if pos.startswith("b"):
            loc = leg_cfg.get("loc", "upper center")
            anchor_y = geom["content_bottom"] - points_to_fig_frac(fig, pad_points, axis="y")
            fig.legend(
                legend_handles,
                legend_labels,
                loc=loc,
                bbox_to_anchor=(geom["axes_x_center"], anchor_y),
                bbox_transform=fig.transFigure,
                **leg_kwargs,
            )
        elif pos.startswith("t"):
            loc = leg_cfg.get("loc", "lower center")
            anchor_y = geom["content_top"] + points_to_fig_frac(fig, pad_points, axis="y")
            # If there is a title above, the legend should go above content_top; if the title is above outer_col_label,
            # user can increase pad_points.
            fig.legend(
                legend_handles,
                legend_labels,
                loc=loc,
                bbox_to_anchor=(geom["axes_x_center"], anchor_y),
                bbox_transform=fig.transFigure,
                **leg_kwargs,
            )
        else:
            loc = leg_cfg.get("location", leg_cfg.get("loc", "upper center"))
            bbox = leg_cfg.get("bbox_to_anchor", None)
            if bbox is None:
                fig.legend(legend_handles, legend_labels, loc=loc, **leg_kwargs)
            else:
                fig.legend(
                    legend_handles,
                    legend_labels,
                    loc=loc,
                    bbox_to_anchor=tuple(bbox),
                    bbox_transform=fig.transFigure,
                    **leg_kwargs,
                )

    # Save
    os.makedirs(os.path.dirname(outpath), exist_ok=True)
    fig.savefig(outpath, bbox_inches="tight")
    plt.close(fig)
