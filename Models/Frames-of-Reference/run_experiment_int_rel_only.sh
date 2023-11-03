#!/bin/bash

# Get current datetime in the required format
datetime=$(date +"%Y-%m-%d_%H-%M-%S")

# Create directory named results/datetime
root_dir="results/$datetime"
mkdir -p "$root_dir"

# Declare arrays for train_size and repetitions pairs
#!/bin/bash

# Check if a command line argument is given (non-empty)
if [[ -z "$1" ]]; then
    echo "Usage: $0 path_to_csv_file"
    exit 1
fi

# Get the CSV file path from the command line argument
csv_file="$1"

# Check if the CSV file exists
if [[ ! -f "$csv_file" ]]; then
    echo "CSV file not found: $csv_file"
    exit 1
fi

# Read training sizes and repetitions from the CSV file
IFS=, # set comma as internal field separator for the read command
train_sizes=() # Initialize empty array for training sizes
repetitions=() # Initialize empty array for repetitions
while read -r train_size repetition; do
    if [[ $train_size != "TrainSize" ]]; then # skip the header line
        train_sizes+=($train_size)
        repetitions+=($repetition)
    fi
done < "$csv_file"

# The rest of the original script follows, we will not modify this part.

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
