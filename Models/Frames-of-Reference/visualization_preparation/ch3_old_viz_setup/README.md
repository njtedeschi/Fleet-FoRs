# Chapter 3 (old model) "formatting-agnostic" plotting setup

This folder parallels the Chapter 2 setup, but for the **old model** POCO experiments,
where the hypothesis classes are:

- `NoPOCO`
- `POCO`
- `StrongPOCO` (labeled as **AbsPOCO** in the style file)
- `NoInt`

The goal is the same: change formatting and layouts *without rerunning inference*.

---

## Raw inputs (as uploaded)

Raw outputs live in `figdata/raw/` and are named like:

- `shares_2024-10-14.csv`
- `shares_2024-11-12.csv`

These are long/tidy posterior-share tables.

---

## Step 1: Consolidate + prepare curve points (fast)

From `scripts/` run:

```bash
python prepare_ch3_old_figdata.py
```

Outputs:

- `figdata/chapter_3_old_results.csv`  (consolidated, now includes a `Date` column)
- `figdata/ch3_old_summary.csv`        (mean + CI over `Iteration`)

Selection settings for main-text plots live in `config/ch3_old_selection.yml`.

---

## Step 2: Render figures (formatting only)

From `scripts/` run:

### Grid figures

Experiment 1.2-style grid (rows = %CanonicalData, cols = %IntrinsicData; k=∞):

```bash
python render_ch3_old_learning_curves.py --word front/behind \
  --rows %CanonicalData --cols %IntrinsicData --filter "NoncanonicalOddsReduction=inf"
```

Experiment 1.1-style grid (rows = odds reduction k; %IntrinsicData is usually fixed):

```bash
python render_ch3_old_learning_curves.py --word front/behind \
  --rows NoncanonicalOddsReduction --cols %IntrinsicData --filter "Date=2024-10-14"
```

### Single figure (one word pair + one condition)

```bash
python render_ch3_old_learning_curves.py --mode single --word front/behind \
  --filter "NoncanonicalOddsReduction=inf" --filter "%CanonicalData=50" --filter "%IntrinsicData=67"
```

Outputs go to `figures/out/`.

---

## Formatting changes

All formatting lives in `config/style.yml`:

- `sense_colors`, `sense_labels`
- facet label maps (canonical / intrinsic / k)
- legend placement and figure sizes

