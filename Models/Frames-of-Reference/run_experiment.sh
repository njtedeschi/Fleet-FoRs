#!/bin/bash

# Direct context probabilities
p_direct_values=(0 0.2)

# Intrinsic description probabilities
# Accept odds_values from command-line arguments
odds_values=("$@")

# Check if odds_values is provided
if [ ${#odds_values[@]} -eq 0 ]; then
  echo "Error: No odds values provided. Please pass odds values as command-line arguments."
  exit 1
fi

# Calculate probabilities from full odds
for odds in "${odds_values[@]}"; do
    # Check if odds is a fraction or a whole number
    if [[ "$odds" == */* ]]; then
        # Odds is a fraction, so calculate the probability accordingly
        numerator=$(echo $odds | cut -d'/' -f1)
        denominator=$(echo $odds | cut -d'/' -f2)
        p_intrinsic_values+=( $(bc <<< "scale=9; $numerator / ($numerator + $denominator)") )
    else
        # Odds is a whole number, so calculate the probability accordingly
        p_intrinsic_values+=( $(bc <<< "scale=9; $odds / (1 + $odds)") )
    fi
done

# Get current datetime in the required format
datetime=$(date +"%Y-%m-%d_%H-%M-%S")

# Create directory named results/datetime
root_dir="results/$datetime"
mkdir -p "$root_dir"

# Loop all conditions the specified number of times
for n in {0..9}; do
    # Loop over p_direct values
    for p_direct in "${p_direct_values[@]}"; do
        # Calculate percentage and format it to two digits
        p_direct_percent=$(printf "%02d" $(echo "$p_direct * 100 / 1" | bc))

        # Loop over p_intrinsic values
        for idx in "${!p_intrinsic_values[@]}"; do
            p_intrinsic=${p_intrinsic_values[idx]}

            # Calculate percentage for p_intrinsic and format it to two digits
            p_intrinsic_percent=$(printf "%02d" $(echo "$p_intrinsic * 100 / 1" | bc))

            # Define log file using the formatted percentages
            log_file="$root_dir/${n}-dir_${p_direct_percent}-int_${p_intrinsic_percent}.log"

            # Run the main executable with the specified arguments
            ./main --chains=20 \
                --threads=20 \
                --steps=1000000 \
                --p_frame=0.9 \
                --p_intrinsic="$p_intrinsic" \
                --p_direct="$p_direct" \
                --train_min=25 \
                --train_max=1000 \
                --train_spacing=25 \
                --top=1 \
                --language="english" \
                --output_dir="$root_dir" 2>&1 | tee "$log_file"

            # Sleep for 1 second to avoid potential issues with identical timestamps
            sleep 1
        done
    done
done