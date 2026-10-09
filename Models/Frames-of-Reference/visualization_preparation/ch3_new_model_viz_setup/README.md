# Chapter 3 (New Model) – Plotting From a Frozen “Curve-Points” CSV

This folder mirrors the Chapter 2 + Chapter 3 (old model) workflow:

1) **You already produced** a single consolidated “curve-points” CSV using
   `consolidate_ch3_new_model_results.py` (the script from the earlier step).
   That CSV is the **formatting-agnostic base**.

2) `prepare_ch3_new_model_figdata.py` takes that base and creates a small
   **summary table** (mean + CI across Iterations) that is fast to re-plot.

3) `render_ch3_new_model_learning_curves.py` renders either:
   - **grids** (row × col facets over condition variables), or
   - a **single** plot (one experimental condition)

The plotting script is formatting-only: you can tweak legends, colors, axis labels,
and grid layouts without touching any expensive preprocessing.

---

## Expected inputs

A consolidated CSV with (at least) these columns:

- `Date`
- `%CanonicalData`
- `%IntrinsicData`
- `NoncanonicalOddsReduction`
- `TrainingSize`
- `Iteration`
- `Canonicity`  (0.0 / 0.5 / 1.0)
- `IntP_mean`, `RelP_mean`  (per Iteration × TrainingSize × Canonicity)

(Extra columns are fine; they’ll be ignored.)

Put it in `figdata/` (recommended) as:

- `figdata/ch3_new_model_consolidated_preprocessed.csv`

or pass `--input` to the prepare script.

---

## Step 1: Prepare the summary table (mean + CI across Iterations)

From inside `ch3_new_model_viz_setup/`:

```bash
python scripts/prepare_ch3_new_model_figdata.py
```

By default, that reads:

- `figdata/ch3_new_model_consolidated_preprocessed.csv`

uses selection rules in:

- `config/ch3_new_model_selection.yml`

and writes:

- `figdata/ch3_new_model_summary.csv`

You can override paths:

```bash
python scripts/prepare_ch3_new_model_figdata.py \
  --input /path/to/ch3_new_model_consolidated_preprocessed.csv \
  --config config/ch3_new_model_selection.yml \
  --out_csv figdata/ch3_new_model_summary.csv
```

---

## Step 2A: Render a grid of plots

Example: rows = `%CanonicalData`, cols = `%IntrinsicData`, holding `NoncanonicalOddsReduction=inf`.

```bash
python scripts/render_ch3_new_model_learning_curves.py \
  --mode grid \
  --rows %CanonicalData \
  --cols %IntrinsicData \
  --filter "NoncanonicalOddsReduction=inf"
```

You can also pin to a single date:

```bash
python scripts/render_ch3_new_model_learning_curves.py \
  --mode grid \
  --rows %CanonicalData \
  --cols %IntrinsicData \
  --filter "Date=2024-12-17" \
  --filter "NoncanonicalOddsReduction=inf"
```

Outputs go to `figures/` by default.

---

## Step 2B: Render a single plot

Single plot = one experimental condition (so filter down to one value per factor).

```bash
python scripts/render_ch3_new_model_learning_curves.py \
  --mode single \
  --filter "Date=2024-11-04" \
  --filter "NoncanonicalOddsReduction=16" \
  --filter "%CanonicalData=17" \
  --filter "%IntrinsicData=50"
```

---

## Formatting / consistency

Edit `config/style.yml` to change:

- legend labels and colors (by canonicity)
- axis labels, limits, grid lines
- subplot sizes, DPI, legend placement

No re-preprocessing needed.

---
