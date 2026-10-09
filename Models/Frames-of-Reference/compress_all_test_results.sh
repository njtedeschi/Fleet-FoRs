#!/bin/bash

# Set the directory containing your experiment folders
EXPERIMENTS_DIR="./experiments"

# Loop through each folder in the experiments directory
for experiment_folder in "$EXPERIMENTS_DIR"/*; do
  # Check if it's a directory
  if [ -d "$experiment_folder" ]; then
    # Define the path to the test_results subfolder
    TEST_RESULTS_DIR="$experiment_folder/test_results"

    # Check if the test_results folder exists
    if [ -d "$TEST_RESULTS_DIR" ]; then
      # Navigate to the test_results folder
      pushd "$TEST_RESULTS_DIR" > /dev/null || continue

      # Check if there are any CSV files to zip
      if ls *.csv > /dev/null 2>&1; then
          # Create a results.zip file containing all CSV files
          zip -r results.zip *.csv

          # Check if the zip file was created successfully
          if [ -f results.zip ]; then
              # Safely delete the original CSV files
              rm -f *.csv
          else
              echo "Failed to create results.zip in $TEST_RESULTS_DIR, skipping deletion of CSV files..."
          fi
      else
          echo "No CSV files found in $TEST_RESULTS_DIR, skipping..."
      fi

      # Return to the previous directory
      popd > /dev/null || continue
    fi
  fi
done

echo "Compression completed."
