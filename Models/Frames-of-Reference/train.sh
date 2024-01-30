#!/bin/bash

# Check if at least root directory is provided
if [ "$#" -lt 2 ]; then
    echo "Usage: $0 root_directory id1 [id2 ...]"
    exit 1
fi

root_dir=$1
config_file="$root_dir/trained_models/config.txt"
shift  # Remove the first argument, now $@ contains only the IDs

# Convert the IDs into an array for easier searching
declare -A id_map
for id in "$@"; do
    id_map[$id]=1
done

# Read configuration values from the config file
chains=$(grep 'chains' $config_file | cut -d ':' -f2 | xargs)
threads=$(grep 'threads' $config_file | cut -d ':' -f2 | xargs)
steps=$(grep 'steps' $config_file | cut -d ':' -f2 | xargs)
top=$(grep 'top' $config_file | cut -d ':' -f2 | xargs)

# Loop over subdirectory-language pairs in the config file
while IFS=: read -r id subdirectory language; do
    # Check if the current id is in the list of IDs to process
    if [[ ${id_map[$id]} ]]; then
        # Loop over all json files in the current subdirectory
        for input_file in "$root_dir/training_data/$subdirectory"/*.json; do
            if [ -f "$input_file" ]; then  # Check if it's a file
                # Correct the output file path
                output_file="$root_dir/trained_models/$subdirectory/${input_file##*/}"
                output_file="${output_file%.json}.txt"

                # Run the C++ executable
                ./train --chains="$chains" --threads="$threads" --steps="$steps" --top="$top" --language="$language" --input_file="$input_file" --output_file="$output_file"
                # echo "./train --chains=\"$chains\" --threads=\"$threads\" --steps=\"$steps\" --top=\"$top\" --language=\"$language\" --input_file=\"$input_file\" --output_file=\"$output_file\""
            fi
        done
    fi
done < "$config_file"