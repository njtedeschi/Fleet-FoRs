import argparse
import os

import pandas as pd
import yaml

FULL_RESULTS_DIRECTORY = "results"
CONFIG_FILENAME = "config.yaml"
CONFIG_PATH = os.path.join(
    FULL_RESULTS_DIRECTORY,
    CONFIG_FILENAME
)
FULL_RESULTS_PATH = os.path.join(
    FULL_RESULTS_DIRECTORY,
    "{result_type}.csv"
)

EXPERIMENTS_DIRECTORY = "experiments"
EXPERIMENT_RESULTS_SUBDIRECTORY = "aggregate_results"
EXPERIMENT_RESULTS_PATH = os.path.join(
    EXPERIMENTS_DIRECTORY,
    "{date}",
    EXPERIMENT_RESULTS_SUBDIRECTORY,
    "{result_type}.csv"
)

class ResultsUpdater:

    def __init__(self):
        self.config = self._load_config()

    def _load_config(self):
        try:
            config_path = CONFIG_PATH
            with open(config_path, 'r') as file:
                return yaml.safe_load(file)
        except FileNotFoundError as e:
            print(f"Error: The file {config_path} was not found.")
            raise e
        except yaml.YAMLError as exc:
            print(f"Error parsing the YAML file: {exc}")
            return None

    def _initialize_full_results(self, result_type):
        columns = self.config['index_cols'] + self.config['metrics'][result_type]
        return pd.DataFrame(columns=columns)

    def _adjust_columns_to_config(self, df, date=None):
        """
        Adjusts DataFrame columns according to configuration, adding missing columns with default values.

        Parameters:
        - df (pd.DataFrame): The DataFrame to adjust.
        - date (str, optional): The experiment date. If specified, applies experiment-specific adjustments.
                                If None, assumes adjustment is for the master results DataFrame.

        Returns:
        - pd.DataFrame: The adjusted DataFrame with columns in specified order and missing values filled.
        """
        if date and 'Date' not in df.columns:
            df.insert(0, 'Date', date)

        for factor in self.config['factors']:
            if factor not in df.columns:
                if date:
                    df[factor] = self._set_missing_value(date, factor)
                else:
                    default_values = df['Date'].apply(lambda x: self._set_missing_value(x, factor))
                    df[factor] = default_values

        ordered_columns = [col for col in self.config['index_cols'] if col in df.columns] + \
                        [col for col in df.columns if col not in self.config['index_cols']]
        df = df[ordered_columns]

        return df

    def _set_missing_value(self, date, factor):
        # Check if date is in the config and has a specific value for the factor
        experiment_defaults = self.config["default_factor_values"][date]
        return experiment_defaults.get(factor, "NA")

    def update_results_files(self, result_types=None, dates=None):
        if result_types is None:
            result_types = self.config['result_types']
        if dates is None:
            dates = self.config['dates']

        for result_type in result_types:
            full_results_path = FULL_RESULTS_PATH.format(
                result_type=result_type
            )
            if os.path.exists(full_results_path):
                full_results_df = pd.read_csv(full_results_path)
                # Check that all columns in configuration are present
                # If not, add new factors with their default values
                full_results_df = self._adjust_columns_to_config(full_results_df)
            else:
                full_results_df = self._initialize_full_results(result_type)
            updated_results_df = self._update_results_df(
                full_results_df,
                result_type,
                dates
            )
            updated_results_df.to_csv(full_results_path, index=False)

    def _update_results_df(self, full_results_df, result_type, dates):
        # Remove existing entries for the given date from the full results DataFrame
        full_results_df = full_results_df[~full_results_df['Date'].isin(dates)]
        all_dfs = [full_results_df]
        for date in dates:
            experiment_results_path = EXPERIMENT_RESULTS_PATH.format(
                date=date,
                result_type=result_type
            )
            if os.path.exists(experiment_results_path):
                experiment_df = pd.read_csv(experiment_results_path)
                experiment_df = self._adjust_columns_to_config(experiment_df, date)
                all_dfs.append(experiment_df)
            else:
                print(f"No file at {experiment_results_path}")
        return pd.concat(all_dfs, ignore_index=True)


def main(args):
    results_updater = ResultsUpdater()
    results_updater.update_results_files(
        args.result_types,
        args.dates
    )

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--dates',
        nargs='+',
        type=str,
        default=None,
        help='Specific date to update'
    )
    parser.add_argument(
        '--result_types',
        nargs='+',
        type=str,
        default=None,
        help='Specific result type to update'
    )
    args = parser.parse_args()
    main(args)