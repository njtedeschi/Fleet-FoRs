#!/bin/bash
set -e # Exit immediately if a command exits with a non-zero status

# Check if at least root directory is provided
if [ "$#" -lt 3 ]; then
    echo "Usage: $0 cpu_name root_directory id1 [id2 ...]"
    exit 1
fi

cpu_name=$1
root_dir=$2
config_file="$root_dir/config_evaluation.txt"
shift 2 # Remove the first two arguments, now $@ contains only the IDs

# Compile the test executable for the specified CPU
make testing CPU=$cpu_name

# Convert the IDs into an array for easier searching
declare -A id_map
for id in "$@"; do
    id_map[$id]=1
done

# Loop over subdirectory-language pairs in the config file
while IFS=: read -r id subdirectory language; do
    # Check if the current id is in the list of IDs to process
    if [[ ${id_map[$id]} ]]; then
        # Define the path to the zipped file in trained_models instead of training_data
        zip_file="$root_dir/trained_models/${subdirectory}.zip"

        # Check if the zip file exists
        if [ -f "$zip_file" ]; then
            # Create a temporary directory for extraction
            tmp_dir=$(mktemp -d)

            output_dir="$root_dir/test_results"

            # Extract the zip file
            unzip "$zip_file" -d "$tmp_dir"

            testing_data_file="$root_dir/testing_data/${subdirectory}.json"
            if [ -f "$testing_data_file" ]; then  # Check if the json file exists
                # Run the C++ executable for testing instead of training
                ./test_${cpu_name} --language="$language" --testing_data_path="$testing_data_file" --model_directory="$tmp_dir/$subdirectory" --output_directory="$output_dir" --output_filename_stem="$subdirectory"
                # echo "./test --language=\"$language\" --testing_data_path=\"$testing_data_file\" --model_directory=\"$tmp_dir/$subdirectory\" --output_directory=\"$output_dir\" --output_filename_stem=\"$subdirectory\""
            else
                echo "Testing data file not found: $testing_data_file"
            fi

            # Clean up the temporary directory
            rm -rf "$tmp_dir"
        else
            echo "Zip file not found: $zip_file"
        fi
    fi
done < "$config_file"
