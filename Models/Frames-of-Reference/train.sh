#!/bin/bash

# Check if at least root directory is provided
if [ "$#" -lt 3 ]; then
    echo "Usage: $0 cpu_name root_directory id1 [id2 ...]"
    exit 1
fi

cpu_name=$1
root_dir=$2
config_file="$root_dir/config_evaluation.txt"
shift 2 # Remove the first two arguments, now $@ contains only the IDs

# Compile the train executable for the specified CPU
make training CPU=$cpu_name

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

            # Create the output subdirectory if it doesn't exist
            output_subdir="$root_dir/trained_models/$subdirectory"
            mkdir -p "$output_subdir"

            # Extract the zip file
            unzip -q "$zip_file" -d "$tmp_dir"

            # Dynamically identify unique training sizes and iterations
            json_dir="$tmp_dir/$subdirectory"
            training_sizes=$(find "$json_dir" -type f -name '*.json' | sed -E 's^.*/([0-9]+)_([0-9]+)\.json^\1^' | sort -u)
            iterations=$(find "$json_dir" -type f -name '*.json' | sed -E 's^.*/([0-9]+)_([0-9]+)\.json^\2^' | sort -u)

            # Loop over files by iteration and then training size
            for iteration in $iterations; do
                for training_size in $training_sizes; do
                # Construct the file name
                input_file="$json_dir/${training_size}_${iteration}.json"
                    if [ -f "$input_file" ]; then  # Check if it's a file
                        output_file="$output_subdir/${input_file##*/}"
                        output_file="${output_file%.json}.txt"

                        # Run the C++ executable
                        ./train_${cpu_name} --chains="$chains" --threads="$threads" --steps="$steps" --top="$top" --language="$language" --input_path="$input_file" --output_path="$output_file"
                        # echo "./train --chains=\"$chains\" --threads=\"$threads\" --steps=\"$steps\" --top=\"$top\" --language=\"$language\" --input_path=\"$input_file\" --output_path=\"$output_file\""
                    fi
                done
            done

            # Zip the processed subdirectory
            processed_zip="${subdirectory}.zip"
            pushd "$root_dir/trained_models" > /dev/null
            zip -r "$processed_zip" "$subdirectory"
            popd > /dev/null

            # Check if the zip operation was successful and delete the original subdirectory
            if [ $? -eq 0 ]; then
                processed_zip_path="$root_dir/trained_models/$processed_zip"
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
