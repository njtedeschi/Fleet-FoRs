# Dissertation visualization setup (unified)

This directory is a **single home** for the *formatting-agnostic* visualization pipelines you’ve been building across:

- **Chapter 2** (Int / Rel / IntRel posterior-share learning curves)
- **Chapter 2 mirror diagnostic** for the **symmetrically unbalanced** conditions (Δ67/33 vs Δ33/67)
- **Chapter 3 (old model)** POCO experiments (posterior shares of hypothesis types like NoInt / NoPOCO / POCO / AbsPOCO)
- **Chapter 3 (new model)** Experiment 2 (learning curves of **IntP − RelP** by **Canonicity**)

The key idea is always the same:

1. Keep *analysis outputs* frozen as **long/tidy tables** in `figdata/`.
2. Run a **fast summarization step** once to produce small `*_summary.csv` curve-point tables.
3. Re-render figures as many times as you want by editing only YAML configs (labels, colors, grids, etc.).

No model re-training / inference reruns are needed to tweak visuals.

---

## Requirements

- Python 3.9+ (3.11 is fine)
- Packages: `pandas`, `numpy`, `matplotlib`, `pyyaml`

If needed:
```bash
pip install pandas numpy matplotlib pyyaml
```

---

## Where to put your data

You said you already have consolidated Chapter 2 + Chapter 3 old-model files, and a pre-consolidated Chapter 3 new-model file.

Put them here (replace any placeholders if present):

### Chapter 2
- `ch2_viz_setup/figdata/chapter_2_results.csv`
  - long/tidy with columns:
    `Date, Language, %IntrinsicData, TrainingSize, Iteration, Word, Sense, Share`

### Chapter 3 old model (POCO)
- Put **raw** shares files here:
  - `ch3_old_viz_setup/figdata/raw/shares_YYYY-MM-DD.csv`

The prep script will automatically:
- infer the `Date` column from the filename,
- fill `%CanonicalData` with a default when missing,
- write a consolidated long table to `ch3_old_viz_setup/figdata/chapter_3_old_results.csv`.

### Chapter 3 new model (Experiment 2)
- `ch3_new_model_viz_setup/figdata/ch3_new_model_consolidated_preprocessed.csv`
  - This is the output of the *one-time* consolidation/preprocessing step that mimics your old `load_and_process_data` work.
  - Required columns are listed at the top of:
    `ch3_new_model_viz_setup/scripts/prepare_ch3_new_model_figdata.py`

If you only have the **raw per-condition CSVs** arranged in date subfolders, you can *first* build that consolidated file with:
```bash
python ch3_new_model_viz_setup/scripts/consolidate_ch3_new_model_results.py <experiments_root> \
  -o ch3_new_model_viz_setup/figdata/ch3_new_model_consolidated_preprocessed.csv \
  --max_training_size 1000
```

---

## One-time preprocessing (fast)

From the **root of this directory**:

```bash
python scripts/prepare_all.py
```

This generates (or refreshes) the small plot-ready summary tables:

- `ch2_viz_setup/figdata/ch2_summary.csv`
- `ch2_viz_setup/figdata/ch2_sym_unbalance_summary.csv`
- `ch3_old_viz_setup/figdata/ch3_old_summary.csv`
- `ch3_new_model_viz_setup/figdata/ch3_new_model_summary.csv`

After that, all plotting is formatting-only.

---

## Plotting (repeatable + fast)

All plotters support:
- **grids** (faceting by condition variables)
- **single plots** (filter down to one condition)

### Chapter 2: Int / Rel / IntRel posterior shares
```bash
cd ch2_viz_setup/scripts
python render_ch2_learning_curves.py --word front/behind --rows %IntrinsicData --cols Language

# single plot for one condition
python render_ch2_learning_curves.py --mode single --word front/behind \
  --filter "Language=English" --filter "%IntrinsicData=50"
```

### Chapter 2: symmetrically unbalanced “mirror” plot
This compares:
- `(Int − Rel)` at **67% intrinsic**
vs
- `(Rel − Int)` at **33% intrinsic**

```bash
cd ch2_viz_setup/scripts
python render_ch2_sym_unbalance.py --word front/behind --rows Language --cols Date

# single plot
python render_ch2_sym_unbalance.py --mode single --word front/behind \
  --filter "Language=English" --filter "Date=2024-02-08"
```

### Chapter 3 old model: POCO hypothesis-type posterior shares
```bash
cd ch3_old_viz_setup/scripts
python render_ch3_old_learning_curves.py --word front/behind \
  --rows %CanonicalData --cols %IntrinsicData --filter "NoncanonicalOddsReduction=inf"

# single plot
python render_ch3_old_learning_curves.py --mode single --word front/behind \
  --filter "NoncanonicalOddsReduction=inf" --filter "%CanonicalData=50" --filter "%IntrinsicData=67"
```

### Chapter 3 new model: IntP − RelP by canonicity
```bash
cd ch3_new_model_viz_setup/scripts
python render_ch3_new_model_learning_curves.py --mode grid \
  --rows NoncanonicalOddsReduction --cols %CanonicalData

# single plot
python render_ch3_new_model_learning_curves.py --mode single \
  --filter "NoncanonicalOddsReduction=inf" --filter "%CanonicalData=17" --filter "%IntrinsicData=50" --filter "Date=2024-11-04"
```

---

## What to edit when you’re revamping visuals

Every module has two config files:

- `config/*_selection.yml` — *data inclusion* (dates, max training size, which levels to include)
- `config/style.yml` — *formatting only* (colors, legend labels, facet ordering, axis labels, figure sizes)

Because the analysis outputs are frozen in `figdata/`, you can iterate on `style.yml` rapidly.
