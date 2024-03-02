import os

import matplotlib.pyplot as plt
import pandas as pd
import yaml

from .plotting.constants import (
    METRIC_LABELS,
    FACTOR_LABELS,
    FACTOR_COLORS,
    FACTOR_LINESTYLES
)
from .plotting.plot_configuration import PlotConfiguration
from .plotting.plotter import Plotter

class DataManager:

    def __init__(self, cl_args):
        self.cl_args = cl_args
        self.root = cl_args.root
        self.config = self._load_config()
        self.factors = self.config['factors']

    def _load_config(self):
        try:
            # Construct the path to the config.yaml file
            config_path = os.path.join(
                self.root,
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

    def load_df(self, results_type):
        results_file = self.config["results_files"][results_type]
        csv_path = os.path.join(
            self.root,
            "aggregate_results",
            results_file
        )
        return pd.read_csv(csv_path)

    ###

    def filter_data_for_one_curve(self, df, factor_values):
        filtered_df = df
        for factor, value in factor_values.items():
            if factor in filtered_df.columns:
                filtered_df = filtered_df[
                    filtered_df[factor] == value
                ]
        return filtered_df

    def aggregate_curve_data(self, filtered_df, metric):
        return (
            filtered_df
            .groupby(
                'TrainingSize'
            )[metric]
            .agg([
                'mean',
                'sem'
            ])
            .unstack()
        )


class PlotManager(DataManager):

    def __init__(self, cl_args):
        super().__init__(cl_args)

    def _save_path(self, metric, factor_i, factor_values):
        filename_format=self.config["plot_formatting"]["filename"].get(
                factor_i, ""
            )
        filestem = filename_format.format(**factor_values)
        if self.cl_args.simple:
            filename = filestem + '_simple.png'
        else:
            filename = filestem + f'_{metric}.png'
        filename = filename.replace("/", "_").lower()
        save_path = os.path.join(
            self.root,
            "plots",
            filename
        )
        return save_path

    def _initialize_plotter(self, metric, factor_i):
        plot_configuration = PlotConfiguration(
            ylabel=METRIC_LABELS[metric],
            labels=FACTOR_LABELS.get(factor_i, {}),
            colors=FACTOR_COLORS.get(factor_i, {}),
            linestyles=FACTOR_LINESTYLES.get(factor_i, {}),
            title_format=self.config["plot_formatting"]["title"].get(
                factor_i, ""
            ),
            filename_format=self.config["plot_formatting"]["filename"].get(
                factor_i, ""
            )
        )
        return Plotter(plot_configuration)

    def plot_data(self, plotter, df, metric, factor_i, factor_values):
        """Prepare data and delegate plotting to Plotter."""
        plotter.initialize_plot()
        for value in self.factors[factor_i]:
            specific_factor_values = factor_values.copy()
            specific_factor_values[factor_i] = value
            filtered_df = self.filter_data_for_one_curve(df, specific_factor_values)
            if not filtered_df.empty:
                aggregated_df = self.aggregate_curve_data(filtered_df, metric)
                plotter.plot_curve(
                    x=aggregated_df['mean'].index,
                    y=aggregated_df['mean'].values,
                    yerr=aggregated_df['sem'].values,
                    factor_i=factor_i,
                    value=value
                )
        # Adjust to pass the correct parameters for finalize_plot
        if self.cl_args.save:
            save_path = self._save_path(metric, factor_i, factor_values)
        else:
            save_path = None
        plotter.finalize_plot(metric, factor_i, factor_values, save_path)

    def create_all_plots(self, df, metric, factor_i):
        """Create all plots for a given metric and varying factor."""
        factors_except_i = {factor: self.factors[factor] for factor in self.factors if factor != factor_i}
        unique_combinations = df.drop_duplicates(subset=factors_except_i.keys())

        for _, row in unique_combinations.iterrows():
            factor_values = {factor: row[factor] for factor in factors_except_i if factor in row}
            valid = all(factor_values.get(factor, None) in self.factors.get(factor, []) for factor in factor_values)
            if not valid:
                continue  # Skip combinations not specified in config

            plotter = self._initialize_plotter(metric, factor_i)
            self.plot_data(plotter, df, metric, factor_i, factor_values)


class StatsManager(DataManager):
    pass