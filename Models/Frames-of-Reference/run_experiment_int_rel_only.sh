#!/bin/bash

# Get current datetime in the required format
datetime=$(date +"%Y-%m-%d_%H-%M-%S")

# Create directory named results/datetime
root_dir="results/$datetime"
mkdir -p "$root_dir"

# Declare arrays for train_size and repetitions pairs
train_sizes=(8 16 32 64 128 256 512)
repetitions=(5 5 5 5 5 5 5)

# Declare arrays for p_direct and p_intrinsic values
p_direct_values=(0 0.2)

# Loop over p_direct values
for p_direct in "${p_direct_values[@]}"; do
    # Create a sub-directory for this combination of hyperparameters based on datetime
    hyper_dir="$root_dir/$(date +"%Y-%m-%d_%H-%M-%S")"
    mkdir -p "$hyper_dir"

    # Loop over train_size and repetitions pairs
    for i in "${!train_sizes[@]}"; do
        train_size="${train_sizes[$i]}"
        repetition="${repetitions[$i]}"

        # Create a log file for this combination of values
        log_file="$hyper_dir/train_$train_size-rep_$repetition.log"

        # Run the main executable with the specified arguments
        ./main --chains=20 \
                --threads=20 \
                --steps=1000000 \
                --p_frame=0.9 \
                --p_intrinsic=1.0 \
                --p_direct="$p_direct" \
                --train_size="$train_size" \
                --repetitions="$repetition" \
                --top=1000 \
                --language="english_int_only" \
                --output_dir="$hyper_dir" 2>&1 | tee "$log_file"

        # Sleep for 1 second to ensure the next directory (if any) has a different datetime
        sleep 1
    done
done

# Loop over p_direct values
for p_direct in "${p_direct_values[@]}"; do
    # Create a sub-directory for this combination of hyperparameters based on datetime
    hyper_dir="$root_dir/$(date +"%Y-%m-%d_%H-%M-%S")"
    mkdir -p "$hyper_dir"

    # Loop over train_size and repetitions pairs
    for i in "${!train_sizes[@]}"; do
        train_size="${train_sizes[$i]}"
        repetition="${repetitions[$i]}"

        # Create a log file for this combination of values
        log_file="$hyper_dir/train_$train_size-rep_$repetition.log"

        # Run the main executable with the specified arguments
        ./main --chains=20 \
                --threads=20 \
                --steps=1000000 \
                --p_frame=0.9 \
                --p_intrinsic=0.0 \
                --p_direct="$p_direct" \
                --train_size="$train_size" \
                --repetitions="$repetition" \
                --top=1000 \
                --language="english_rel_only" \
                --output_dir="$hyper_dir" 2>&1 | tee "$log_file"

        # Sleep for 1 second to ensure the next directory (if any) has a different datetime
        sleep 1
    done
done
