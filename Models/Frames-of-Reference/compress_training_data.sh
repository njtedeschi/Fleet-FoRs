#!/bin/bash

# Check if an argument is provided
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <experiment>"
    exit 1
fi

experiment=$1
target_dir="./experiments/${experiment}/training_data/"

# Check if the target directory exists
if [ ! -d "$target_dir" ]; then
    echo "Directory $target_dir does not exist."
    exit 1
fi

# Change to the target directory
cd "$target_dir"

# Loop over subdirectories, zip them, and then delete the original directory if the zip file is created successfully
for dir in */; do
    zip_file="${dir%/}.zip"
    zip -r "$zip_file" "$dir"
    if [ $? -eq 0 ] && [ -f "$zip_file" ]; then
        rm -rf "$dir"
    else
        echo "Failed to create zip for $dir"
    fi
done