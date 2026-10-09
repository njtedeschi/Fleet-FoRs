# Experimental steps

## Set-up

1. Create directory *./experiments/DATE/* according to specifications in **Experimental directory structure**: `setup.sh experiments/DATE`
2. Complete configuration files

## Training

1. Generate training data: `python data_generation/generate_training_data.py experiments/DATE`
2. Compile *Train.cpp* into *train*: `make training`
3. Train models: `train.sh experiments/DATE`

## Testing

1. Generate testing data: `python data_generation/generate_testing_data.py experiments/DATE`
2. Compile *Test.cpp* into *test*: `make testing`
3. Test models: `test.sh experiments/DATE`

## Analysis

1. Aggregate test results: `python results_analysis/aggregate_results.py experiments/DATE`
2. Analyze aggregate results
    * Plot: `plot.py` experiments/DATE {experimental_condition(s)} {word(s)}

# Experimental directory structure

## Initial
```
├── ./experiments/DATE/
│   ├── config_data_generation.yaml
│   ├── config_training.txt
│   ├── config_testing.txt
│   ├── training/
│   │   ├── training_data/
│   │   ├── trained_models/
│   ├── testing/
│   │   ├── testing_data/
│   │   ├── test_results/
│   ├── analysis/
│   │   ├── plots/
│   │   ├── stats/
```

## Final

```
├── ./experiments/DATE/
│   ├── training/
│   │   ├── training_data/
│   │   │   ├── {experimental_condition}/
│   │   │   │   ├── {training_size}_{instance}.json
│   │   │   │   ├── ...
│   │   │   │   ├── {training_size}_{instance}.json
│   │   ├── trained_models/
│   │   │   ├── {experimental_condition}/
│   │   │   │   ├── {training_size_}_{instance}.txt
│   │   │   │   ├── ...
│   │   │   │   ├── {training_size}_{instance}.txt
│   ├── testing/
│   │   ├── testing_data/
│   │   │   ├── {experimental_condition}.json
│   │   │   ├── ...
│   │   │   ├── {experimental_condition}.json
│   │   ├── test_results/
│   │   │   ├── {experimental_condition}/
│   │   │   │   ├── lookup.json
│   │   │   │   ├── {training_size}_{instance}.csv
│   │   │   │   ├── ...
│   │   │   │   ├── {training_size}_{instance}.csv
│   ├── analysis/
│   │   ├── aggregate_data.csv
│   │   ├── plots/
│   │   │   ├── ...
│   │   ├── stats/
│   │   │   ├── ...
```

# Configuration files

## Data generation

## Training

## Testing