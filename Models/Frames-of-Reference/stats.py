import argparse
import os

import pandas as pd

from dataprocessing.src.statistics.significance_testing import confidence_interval
from dataprocessing.src.data_manager import BootstrapManager

def sense_across_languages(bootstrap_manager, data, output_path, language_1, language_2, n_bootstrap=1000):
        ### BEGIN EDITING ###

    words = {
        "English": ["front/behind", "left/right", "side"],
        "Engtec": ["front/behind", "left/right", "side"],
        "Mixtec": ["front/behind", "left/right", "side"]
    }
    int_frequencies = [33, 50, 67]

    sense = "Rel+IntRel"
    metric = "Share"

    ### END EDITING ###

    # TODO: move more inside of bootstrap_manager
    for word_1, word_2 in zip(words[language_1], words[language_2]):
        for int_frequency in int_frequencies:
            condition_1 = {
                "Language": language_1,
                "%IntrinsicData": int_frequency,
                "Word": word_1,
                "Sense": sense
            }
            condition_2 = {
                "Language": language_2,
                "%IntrinsicData": int_frequency,
                "Word": word_2,
                "Sense": sense
            }

            bootstrapped_areas = bootstrap_manager.sample_across_conditions(
                data,
                condition_1,
                condition_2,
                metric,
                n_bootstrap
            )

            lower, upper = confidence_interval(bootstrapped_areas, alpha=0.05)
            # Construct the row according to the header format
            row = bootstrap_manager.calculate_row(
                metric,
                condition_1,
                condition_2,
                lower,
                upper,
                n_bootstrap
            )
            if cl_args.save:
                with open(output_path, 'a') as file:
                    file.write(row)
            else:
                print(row)

def senses_within_language(bootstrap_manager, data, output_path, language, n_bootstrap=1000):
            ### BEGIN EDITING ###

    # 2023-12-10
    words = {
        "English": ["front/behind", "left/right", "side"],
        "Engtec": ["front/behind", "left/right", "side"],
        "Mixtec": ["front/behind", "left/right", "side"]
    }
    int_frequencies = [50]

    sense = "Int-Rel"
    metric = "Share"
    ### END EDITING ###


    # TODO: move more inside of bootstrap_manager
    for word in words[language]:
        for int_frequency in int_frequencies:
            condition = {
                "Language": language,
                "%IntrinsicData": int_frequency,
                "Word": word,
                "Sense": sense
            }
            # Workaround for printing rows with no entries for condition 2
            blank = {
                "Language": "",
                "%IntrinsicData": "",
                "Word": "",
                "Sense": ""
            }

            bootstrapped_areas = bootstrap_manager.sample_within_condition(
                data,
                condition,
                metric,
                n_bootstrap
            )

            lower, upper = confidence_interval(bootstrapped_areas, alpha=0.05)
            # Construct the row according to the header format
            row = bootstrap_manager.calculate_row(
                metric,
                condition,
                blank,
                lower,
                upper,
                n_bootstrap
            )
            if cl_args.save:
                with open(output_path, 'a') as file:
                    file.write(row)
            else:
                print(row)

def boosts(bootstrap_manager, data, output_path, language, n_bootstrap=1000):
    words = {
        "English": ["front/behind", "left/right", "side"],
        "Engtec": ["front/behind", "left/right", "side"],
        "Mixtec": ["front/behind", "left/right", "side"]
    }

    # TODO: move more inside of bootstrap_manager
    for word in words[language]:
        condition_1 = {
            "Language": language,
            "%IntrinsicData": 67,
            "Word": word,
            "Sense": "Int-Rel"
        }
        # Workaround for printing rows with no entries for condition 2
        condition_2 = {
            "Language": language,
            "%IntrinsicData": 33,
            "Word": word,
            "Sense": "Rel-Int"
        }

        bootstrapped_areas = bootstrap_manager.sample_across_conditions(
            data,
            condition_1,
            condition_2,
            metric,
            n_bootstrap
        )

        lower, upper = confidence_interval(bootstrapped_areas, alpha=0.05)
        # Construct the row according to the header format
        row = bootstrap_manager.calculate_row(
            metric,
            condition_1,
            condition_2,
            lower,
            upper,
            n_bootstrap
        )
        if cl_args.save:
            with open(output_path, 'a') as file:
                file.write(row)
        else:
            print(row)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('root', help="Path to the root directory") # e.g. results/DATE/
    # parser.add_argument('--metric',
    #                     default="Share",
    #                     help="Average value to compare"
    # ) # e.g. "Share" or "F1"
    # parser.add_argument('--factor',
    #                     default="Sense",
    #                     # help="Factor to vary for curves on one plot"
    # ) # e.g. "Word", "Language", "%IntrinsicData"
    parser.add_argument('--simple',
                        action='store_true',
                        help="Whether to just use shares with symmetric and meronymic senses combined"
    )
    parser.add_argument('--save',
                        action='store_true',
                        help="Whether to save results as csv"
    )
    parser.add_argument('--xmax',
                        type=int,
                        nargs='?',
                        default=None,
                        help='Optional maximum x value'
    )
    parser.add_argument('--n_bootstrap',
                        type=int,
                        nargs='?',
                        default=1000,
                        help='Number of bootstrap samples'
    )
    parser.add_argument('--save',
                        action='store_true',
                        help="Whether to save results as csv"
    )
    cl_args = parser.parse_args()

    if(cl_args.simple):
        results_type = "simplified_shares"
        # Override choice of metric if provided
        metric = "Share"
    else:
        results_type = "all_metrics"
        metric = "Share" # TODO: placeholder

    bootstrap_manager = BootstrapManager(cl_args)
    data = bootstrap_manager.load_df(results_type)

    data = bootstrap_manager.combine_factor_values(
        data,
        "Share",
        "Sense",
        ["Rel", "IntRel"],
        lambda shares: shares.sum(),
        "Rel+IntRel"
    )
    data = bootstrap_manager.combine_factor_values(
        data,
        "Share",
        "Sense",
        ["Int", "Rel"],
        lambda shares: shares.iloc[0] - shares.iloc[1],
        "Int-Rel"
    )
    data = bootstrap_manager.combine_factor_values(
        data,
        "Share",
        "Sense",
        ["Int", "Rel"],
        lambda shares: shares.iloc[1] - shares.iloc[0],
        "Rel-Int"
    )

    # output_filename = "confidence_intervals_simple.csv" if cl_args.simple else "confidence_intervals.csv"
    output_filestem = "confidence_intervals_simple" if cl_args.simple else "confidence_intervals"
    if cl_args.xmax:
        output_filestem += f'_{cl_args.xmax}'
    output_path = os.path.join(cl_args.root, "stats", output_filestem + '.csv')
    header = bootstrap_manager.calculate_header()
    if not os.path.exists(output_path):
        with open(output_path, 'w') as file:
            file.write(header)
    else:
        with open(output_path, 'r') as file:
            existing_header = file.readline()
            if existing_header.strip() != header.strip():
                raise ValueError("Existing file's header does not match expected format.")

    n_bootstrap = cl_args.n_bootstrap
    for language in ["English", "Engtec", "Mixtec"]:
        senses_within_language(bootstrap_manager, data, output_path, language, n_bootstrap)
        boosts(bootstrap_manager, data, output_path, language, n_bootstrap)
    sense_across_languages(bootstrap_manager, data, output_path, "English", "Engtec", n_bootstrap)
    sense_across_languages(bootstrap_manager, data, output_path, "Engtec", "Mixtec", n_bootstrap)