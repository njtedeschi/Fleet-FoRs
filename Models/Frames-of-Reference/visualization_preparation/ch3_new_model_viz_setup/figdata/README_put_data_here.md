# Put your Chapter 3 (new model) consolidated CSV here

Expected filename (default):

- `ch3_new_model_consolidated_preprocessed.csv`

This is the *frozen* curve-points table produced by:

```bash
python ../scripts/consolidate_ch3_new_model_results.py <experiments_root> \
  -o ch3_new_model_consolidated_preprocessed.csv \
  --max_training_size 1000
```

Then run:

```bash
cd ../scripts
python prepare_ch3_new_model_figdata.py
python render_ch3_new_model_learning_curves.py --mode grid --rows NoncanonicalOddsReduction --cols %CanonicalData
```
