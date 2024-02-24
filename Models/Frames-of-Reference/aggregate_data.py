from dataprocessing.src.aggregation.rank_collapser import RankCollapser

import argparse
import os

import pandas as pd
# import yaml

# TODO: get from config_aggregation.yaml file
INPUT_SUBDIRECTORY = "test_results"
OUTPUT_SUBDIRECTORY = "aggregate_results"
OUTPUT_FILENAME_METRICS = "all_metrics.csv"
# Only include shares, and consolidate "SInt" and "MInt"
OUTPUT_FILENAME_SHARES = "simplified_shares.csv"

WORD_GROUPINGS = {
    # English
    "above": "above/below",
    "below": "above/below",
    "front": "front/behind",
    "behind": "front/behind",
    "side": "side",
    "left": "left/right",
    "right": "left/right",
    "near": "near/far",
    "far": "near/far",
    # Mixtec
    "head": "head",
    "belly": "belly",
    "face": "face",
    "back": "back",
    "flank": "flank"
}

EXPERIMENTAL_CONDITIONS = {
    "int_33_lang_0" : {
        "Language": "English",
        "%IntrinsicData": 33
    },
    "int_33_lang_1" : {
        "Language": "Mixtec",
        "%IntrinsicData": 33
    },
    "int_50_lang_0" : {
        "Language": "English",
        "%IntrinsicData": 50
    },
    "int_50_lang_1" : {
        "Language": "Mixtec",
        "%IntrinsicData": 50
    },
    "int_67_lang_0" : {
        "Language": "English",
        "%IntrinsicData": 67
    },
    "int_67_lang_1" : {
        "Language": "Mixtec",
        "%IntrinsicData": 67
    }
}

SENSE_GROUPINGS = {
    ('SInt', 'MInt'): 'Int',
    ('SIntRel', 'MIntRel'): 'IntRel'
}

def full_results_to_metrics(input_directory,
                            word_groupings,
                            experimental_conditions):
    rank_collapser = RankCollapser(word_groupings)
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

           # collapsed_df[subcondition_label] = subcondition_value

        concatenated_df = pd.concat([concatenated_df, collapsed_df], ignore_index=True)
    return concatenated_df

def metrics_to_simplified_shares(df, sense_groupings):
    # Filter the necessary columns
    df_filtered = df[['Language', '%IntrinsicData', 'TrainingSize', 'Iteration', 'Word', 'Sense', 'Share']]

    # Placeholder for the transformed rows
    transformed_rows = []

    # Iterate through the data and apply transformations
    for _, group in df_filtered.groupby(['Language', '%IntrinsicData', 'TrainingSize', 'Iteration', 'Word']):
        for original_senses, new_sense in sense_groupings.items():
            # Filter rows that match the original senses
            sense_rows = group[group['Sense'].isin(original_senses)]
            if not sense_rows.empty:
                # Sum the Share values and create a new row
                new_share = sense_rows['Share'].sum()
                new_row = sense_rows.iloc[0].copy()
                new_row['Sense'] = new_sense
                new_row['Share'] = new_share
                transformed_rows.append(new_row)
        # Remove the original SInt, MInt, SIntRel, MIntRel rows
        group = group[~group['Sense'].isin(['SInt', 'MInt', 'SIntRel', 'MIntRel'])]
        # Append remaining (untransformed) rows
        transformed_rows.extend(group.to_dict('records'))

    # Create a new DataFrame from the transformed rows
    transformed_df = pd.DataFrame(transformed_rows)

    return transformed_df

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('root', help="Path to the root directory") # e.g. results/DATE/
    # parser.add_argument('config', help="Name of config file in root")
    args = parser.parse_args()

    root = args.root
    # config = args.config

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

    metrics_filename = OUTPUT_FILENAME_METRICS
    metrics_filepath = os.path.join(output_directory, metrics_filename)
    shares_filename = OUTPUT_FILENAME_SHARES
    shares_filepath = os.path.join(output_directory, shares_filename)

    word_groupings = WORD_GROUPINGS
    experimental_conditions = EXPERIMENTAL_CONDITIONS
    sense_groupings = SENSE_GROUPINGS

    ###

    metrics_df = full_results_to_metrics(
        input_directory,
        word_groupings,
        experimental_conditions
    )
    shares_df = metrics_to_simplified_shares(
        metrics_df,
        sense_groupings
    )
    metrics_df.to_csv(metrics_filepath, index=False)
    shares_df.to_csv(shares_filepath, index=False)