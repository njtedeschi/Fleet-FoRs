#!/bin/bash

# Direct context probabilities
p_direct_values=(0 0.2)

# Intrinsic description probabilities
# odds_values: 1/16 1/8 1/4 1/2 1 2 4 8 16
# Pitts: 1/16 1/8 1/4
# Montague: 1/2 1 2
# Wittgenstein 4 8 16
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

# Loop over p_direct values
for p_direct in "${p_direct_values[@]}"; do
    # Calculate percentage and format it to two digits
    p_direct_percent=$(printf "%02d" $(echo "$p_direct * 100 / 1" | bc))

    # Loop over p_intrinsic values
    for idx in "${!p_intrinsic_values[@]}"; do
        p_intrinsic=${p_intrinsic_values[idx]}
        odds=${odds_values[idx]}
        
        # Determine file name suffix based on whether odds is greater or less than 1
        if [[ "$odds" == */* ]]; then
            # Odds is a fraction, so use the format "1_x"
            log_file_suffix=$(echo $odds | tr '/' '_')
        else
            # Odds is a whole number, so use the format "x_1"
            log_file_suffix="${odds}_1"
        fi

        log_file="$root_dir/dir_${p_direct_percent}-int_${log_file_suffix}.log"

        # Run the main executable with the specified arguments
        ./main --chains=20 \
               --threads=20 \
               --steps=1000000 \
               --p_frame=0.9 \
               --p_intrinsic="$p_intrinsic" \
               --p_direct="$p_direct" \
               --train_min=10 \
               --train_max=1000 \
               --train_spacing=10 \
               --top=1000 \
               --language="english" \
               --output_dir="$root_dir" 2>&1 | tee "$log_file"

        # Sleep for 1 second to avoid potential issues with identical timestamps
        sleep 1
    done
done