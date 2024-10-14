import argparse
import os

import pandas as pd

INPUT_SUBDIRECTORY = "test_results"
COLS_TO_SORT = ['TrainingSize', 'Iteration', 'Rank', 'Word', 'Sense']

def sort_csv_and_save(input_file, output_file, columns_to_sort):
    """
    Reads a CSV file, checks for the existence of specified columns, sorts it based on the existing specified columns, 
    and saves the sorted DataFrame to a new CSV.
    """
    df = pd.read_csv(input_file)

    # Filter out the columns that do not exist in the DataFrame
    existing_cols_to_sort = [col for col in columns_to_sort if col in df.columns]

    if existing_cols_to_sort:
        df_sorted = df.sort_values(by=existing_cols_to_sort)
    else:
        # If none of the COLS_TO_SORT exist, do not sort and print a message
        df_sorted = df
        print(f"None of the specified columns to sort by exist in {input_file}. Skipping sort.")

    df_sorted.to_csv(output_file, index=False)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('root', help="Path to the root directory")  # e.g., "results/DATE/"
    args = parser.parse_args()

    input_directory = os.path.join(args.root, INPUT_SUBDIRECTORY)
    output_directory = os.path.join(args.root, INPUT_SUBDIRECTORY + "_sorted")  # Creates a separate directory for sorted files

    if not os.path.exists(output_directory):
        os.makedirs(output_directory)  # Create the output directory if it doesn't exist

    # Loop over all CSV files in the input directory
    for filename in os.listdir(input_directory):
        if filename.endswith(".csv"):
            input_file_path = os.path.join(input_directory, filename)
            output_file_path = os.path.join(output_directory, filename)

            print(f"Sorting {input_file_path} and saving to {output_file_path}")
            sort_csv_and_save(input_file_path, output_file_path, COLS_TO_SORT)
