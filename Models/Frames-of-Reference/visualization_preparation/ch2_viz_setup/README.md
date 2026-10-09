# Chapter 2 “formatting-agnostic” plotting setup

This folder gives you a clean separation between:

1) **analysis output** (your posterior shares per run, already computed), and
2) **visual formatting** (colors/labels/layout) that you can change instantly.

You should be able to regenerate dissertation-ready figures *without re-running MCMC*.

---

## Frozen input table

`figdata/chapter_2_results.csv` is your consolidated long-format table with columns:

- `Date`
- `Language` (English / Engtec / Mixtec)
- `%IntrinsicData` (33 / 50 / 67)
- `TrainingSize` (25 … 8000)
- `Iteration` (trial index)
- `Word` (front/behind, left/right, above/below, side)
- `Sense` (Int, Rel, IntRel)
- `Share` (posterior mass for that class at that x-value)

This repo treats that as the **frozen base**: it never changes unless you rerun analysis.

---

## Step 1: prepare plot-ready curve points (fast)

From `scripts/`:

```bash
python prepare_ch2_figdata.py
```

This writes:
- `figdata/ch2_summary.csv`

What gets included in the summary is controlled by:
- `config/ch2_selection.yml` (dates per language, max training size, etc.)

---

## Step 1b: prepare the “symmetrically unbalanced mirror” curves (fast)

If you want the Chapter 2 diagnostic that compares:

- `(Int − Rel)` at **67% intrinsic**
vs
- `(Rel − Int)` at **33% intrinsic**

run:

```bash
python prepare_ch2_sym_unbalance_figdata.py
```

This writes:
- `figdata/ch2_sym_unbalance_summary.csv`

Its parameters live in:
- `config/sym_unbalance.yml`

---

## Step 2: render figures (formatting only)

Outputs go to `figures/out/` (vector PDFs).

### Standard learning-curve plots

Grid figures (facets):
```bash
python render_ch2_learning_curves.py --word front/behind --rows %IntrinsicData --cols Language
```

Single figure (one condition):
```bash
python render_ch2_learning_curves.py --mode single --word front/behind \
  --filter "Language=Engtec" --filter "%IntrinsicData=67"
```

### Symmetrically unbalanced mirror plots

Grid:
```bash
python render_ch2_sym_unbalance.py --word front/behind --rows Language --cols Date
```

Single:
```bash
python render_ch2_sym_unbalance.py --mode single --word front/behind \
  --filter "Language=English" --filter "Date=2024-02-08"
```

---

## Where to change things

- **Data inclusion**: `config/ch2_selection.yml` (main learning curves)
- **Mirror diagnostic parameters**: `config/sym_unbalance.yml`
- **All formatting**: `config/style.yml`

Because the renderer reads YAML on every run, you can iterate quickly on:
- colors
- legend labels
- facet ordering
- axis labels and limits
- grid cell sizes

without touching the underlying results.
