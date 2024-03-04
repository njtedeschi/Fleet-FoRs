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

    def _filter_data_for_one_curve(self, df, factor_values):
        filtered_df = df
        for factor, value in factor_values.items():
            if factor in filtered_df.columns:
                filtered_df = filtered_df[
                    filtered_df[factor] == value
                ]
        return filtered_df

    def _aggregate_curve_data(self, filtered_df, metric, variability='sem'):
        return (
            filtered_df
            .groupby(
                'TrainingSize'
            )[metric]
            .agg([
                'mean',
                variability
            ])
            .unstack()
        )


class PlotManager(DataManager):

    def __init__(self, cl_args):
        super().__init__(cl_args)

    def _initialize_plotter(self, metric, factor_i):
        plot_configuration = PlotConfiguration(
            ylabel=METRIC_LABELS[metric],
            labels=FACTOR_LABELS.get(factor_i, {}),
            colors=FACTOR_COLORS.get(factor_i, {}),
            linestyles=FACTOR_LINESTYLES.get(factor_i, {}),
        )
        return Plotter(plot_configuration)

    def _determine_title(self, metric, factor_i, factor_values):
        title_format = self.config["plot_formatting"]["title"].get(
            factor_i, ""
        )
        if title_format:
            title = title_format.format(**factor_values)
        else:
            title = f"{metric} Learning Curve for {factor_values}, varying {factor_i}"
        return title

    def _determine_save_path(self, metric, factor_i, factor_values):
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

    ###

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
            self._create_plot(plotter, df, metric, factor_i, factor_values)

    def _create_plot(self, plotter, df, metric, factor_i, factor_values):
        """Prepare data and delegate plotting to Plotter."""
        plotter.initialize_plot()
        self._process_and_plot_curves(plotter, df, metric, factor_i, factor_values)
        title = self._determine_title(metric, factor_i, factor_values)
        if self.cl_args.save:
            save_path = self._determine_save_path(metric, factor_i, factor_values)
        else:
            save_path = None
        plotter.finalize_plot(title, save_path)

    def _process_and_plot_curves(self, plotter, df, metric, factor_i, factor_values):
        for value in self.factors[factor_i]:
            self._plot_single_curve(plotter, df, metric, factor_i, factor_values, value)

    def _plot_single_curve(self, plotter, df, metric, factor_i, factor_values, value):
        specific_factor_values = factor_values.copy()
        specific_factor_values[factor_i] = value
        filtered_df = self._filter_data_for_one_curve(df, specific_factor_values)
        if not filtered_df.empty:
            aggregated_df = self._aggregate_curve_data(filtered_df, metric)
            plotter.plot_curve(
                x=aggregated_df['mean'].index,
                y=aggregated_df['mean'].values,
                yerr=aggregated_df['sem'].values,
                factor_i=factor_i,
                value=value
            )


class StatsManager(DataManager):

    def __init__(self, cl_args):
        super().__init__(cl_args)
