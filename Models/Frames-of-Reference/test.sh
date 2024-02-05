#!/bin/bash

# Check if at least root directory is provided
if [ "$#" -lt 2 ]; then
    echo "Usage: $0 root_directory id1 [id2 ...]"
    exit 1
fi

root_dir=$1
config_file="$root_dir/config_evaluation.txt"
shift  # Remove the first argument, now $@ contains only the IDs

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
        zip_file="$root_dir/training/trained_models/${subdirectory}.zip"

        # Check if the zip file exists
        if [ -f "$zip_file" ]; then
            # Create a temporary directory for extraction
            tmp_dir=$(mktemp -d)

            # Create the output subdirectory in testing/test_results instead of training/trained_models
            output_subdir="$root_dir/testing/test_results/$subdirectory"
            mkdir -p "$output_subdir"

            # Extract the zip file
            unzip "$zip_file" -d "$tmp_dir"

            # Find the single json file with the same name as the directory in testing/testing_data
            testing_data_file="$root_dir/testing/testing_data/${subdirectory}.json"
            if [ -f "$testing_data_file" ]; then  # Check if the json file exists
                # Run the C++ executable for testing instead of training
                ./test --language="$language" --testing_data_path="$testing_data_file" --model_directory="$tmp_dir/$subdirectory" --output_directory="$output_subdir"
            else
                echo "Testing data file not found: $testing_data_file"
            fi

            # Zip the processed output directory
            processed_zip="${subdirectory}.zip"
            pushd "$root_dir/testing/test_results" > /dev/null
            zip -r "$processed_zip" "$subdirectory"
            popd > /dev/null

            # Check if the zip operation was successful and delete the original output subdirectory
            if [ $? -eq 0 ]; then
                processed_zip_path="$root_dir/testing/test_results/$processed_zip"
                if [ -f "$processed_zip_path" ]; then
                    rm -rf "$output_subdir"
                else
                    echo "Failed to create zip: $processed_zip_path"
                fi
            else
                echo "Failed to create zip: $processed_zip"
            fi

            # Clean up the temporary directory
            rm -rf "$tmp_dir"
        else
            echo "Zip file not found: $zip_file"
        fi
    fi
done < "$config_file"
