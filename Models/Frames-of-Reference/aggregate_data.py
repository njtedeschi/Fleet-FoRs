from dataprocessing.src.aggregation.rank_collapser import RankCollapser

import argparse
import os

import pandas as pd
import yaml

# INPUT_SUBDIRECTORY = "test_results"
INPUT_SUBDIRECTORY = "test_results_sorted"
OUTPUT_SUBDIRECTORY = "aggregate_results"
OUTPUT_FILENAME_METRICS = "all_metrics.csv"
# Only include shares, and consolidate "SInt" and "MInt"
OUTPUT_FILENAME_SHARES = "simplified_shares.csv"

WORD_GROUPINGS = {
    "above": "above/below",
    "below": "above/below",
    "front": "front/behind",
    "behind": "front/behind",
    "left": "left/right",
    "right": "left/right",
    "near": "near/far",
    "far": "near/far"
}
WORD_GROUPINGS_SIMPLE = {
    # English
    "above": "above/below",
    "below": "above/below",
    "front": "front/behind",
    "behind": "front/behind",
    "left": "left/right",
    "right": "left/right",
    "near": "near/far",
    "far": "near/far",
    # Mixtec
    "head": "above/below",
    "belly": "above/below",
    "face": "front/behind",
    "back": "front/behind",
    "flank": "side"
}
SENSE_GROUPINGS_SIMPLE = {
    "SInt": "Int",
    "MInt": "Int",
    "SIntRel": "IntRel",
    "MIntRel": "IntRel",
    "SIntAbs": "IntAbs",
    "MIntAbs": "IntAbs",
    "SIntRelAbs": "IntRelAbs",
    "MIntRelAbs": "IntRelAbs"
}

def load_config(root):
    try:
        # Construct the path to the config.yaml file
        config_path = os.path.join(
            root,
            'config_analysis.yaml'
        )
        with open(config_path, 'r') as file:
            return yaml.safe_load(file)
    except FileNotFoundError:
        print(f"Error: The file {config_path} was not found.")
        return None
    except yaml.YAMLError as exc:
        print(f"Error parsing the YAML file: {exc}")
        return None

def process_results(input_directory,
                            word_groupings,
                            sense_groupings,
                            experimental_conditions):
    rank_collapser = RankCollapser(word_groupings, sense_groupings)
    concatenated_df = pd.DataFrame()
    for condition_label, subconditions in experimental_conditions.items():
        counts_path = os.path.join(input_directory,
                                   (condition_label + ".csv"))
        posterior_lookup_path = os.path.join(input_directory,
                                   (condition_label + "_lookup.csv"))
        collapsed_df = rank_collapser.process_experimental_condition(counts_path, posterior_lookup_path)

        # Initialize a variable to track the position for inserting new columns
        insert_position = 0
        for subcondition_label, subcondition_value in subconditions.items():
            # Insert each subcondition as a new column at the beginning of the DataFrame
            collapsed_df.insert(insert_position, subcondition_label, subcondition_value)
            # Increment the position for the next column to be inserted
            insert_position += 1
        concatenated_df = pd.concat([concatenated_df, collapsed_df], ignore_index=True)
    return concatenated_df

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('root', help="Path to the root directory") # e.g. results/DATE/
    parser.add_argument('--simple',
                        action='store_true',
                        help=""
    )
    # parser.add_argument('config', help="Name of config file in root")
    args = parser.parse_args()

    root = args.root
    config = load_config(root)

    if args.simple:
        word_groupings = WORD_GROUPINGS_SIMPLE
        sense_groupings = SENSE_GROUPINGS_SIMPLE
        output_filename = OUTPUT_FILENAME_SHARES
    else:
        word_groupings = WORD_GROUPINGS
        sense_groupings = {}
        output_filename = OUTPUT_FILENAME_METRICS
    conditions_by_filename = config["conditions_by_filename"]

    ###
    # TODO: Set constant values from config
    # Set input directory path
    input_directory = os.path.join(
        root,
        INPUT_SUBDIRECTORY
    )

    # Set output directory path
    output_directory = os.path.join(
        root,
        OUTPUT_SUBDIRECTORY
    )
    # Make sure the output directory exists; if not, create it
    if not os.path.exists(output_directory):
        os.makedirs(output_directory)

    output_filepath = os.path.join(output_directory, output_filename)

    ###

    results_df = process_results(
        input_directory,
        word_groupings,
        sense_groupings,
        conditions_by_filename
    )
    results_df.to_csv(output_filepath, index=False)
