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
        # Define the path to the zipped file
        zip_file="$root_dir/training_data/${subdirectory}.zip"

        # Check if the zip file exists
        if [ -f "$zip_file" ]; then
            # Create a temporary directory for extraction
            tmp_dir=$(mktemp -d)

            # Extract the zip file
            unzip "$zip_file" -d "$tmp_dir"

            # Loop over all json files in the extracted directory
            for input_file in "$tmp_dir/$subdirectory"/*.json; do
                if [ -f "$input_file" ]; then  # Check if it's a file
                    output_file="$root_dir/trained_models/$subdirectory/${input_file##*/}"
                    output_file="${output_file%.json}.txt"

                    # Run the C++ executable
                    # ./train --chains="$chains" --threads="$threads" --steps="$steps" --top="$top" --language="$language" --input_file="$input_file" --output_file="$output_file"
                    echo "./train --chains=\"$chains\" --threads=\"$threads\" --steps=\"$steps\" --top=\"$top\" --language=\"$language\" --input_file=\"$input_file\" --output_file=\"$output_file\""
                fi
            done

            # Clean up the temporary directory
            rm -rf "$tmp_dir"
        else
            echo "Zip file not found: $zip_file"
        fi
    fi
done < "$config_file"
