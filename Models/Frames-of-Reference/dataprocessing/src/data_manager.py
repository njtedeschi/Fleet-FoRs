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
from .statistics.spline_calculator import SplineCalculator

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

    def combine_factor_values(self, df, metric, factor_i, factor_values, combine_lambda, new_name):
        """
        Combines factor values in a DataFrame, applies a lambda function to metric values,
        and appends the new rows with a custom factor value to the original DataFrame.

        Parameters:
        - df: The original DataFrame.
        - factor_i:
        - factor_values: A list of factor values to combine.
        - combine_lambda: A lambda function defining how to combine `metric` values.
        - new_name: The name for the new `factor` value in the new row.

        Returns:
        - A DataFrame with the new rows appended.
        """
        # Filter the dataset for rows that match any of the senses in the list
        filtered_df = df[df[factor_i].isin(factor_values)]

        # Group by all index columns except `factor` apply the lambda to `metric` values, and create new rows
        factors_except_i = [factor for factor in self.factors.keys() if factor != factor_i]
        grouped = filtered_df.groupby(factors_except_i + ['TrainingSize', 'Iteration'])
        new_rows_list = []

        for _, group in grouped:
            combined_metric = combine_lambda(group[metric])
            new_row = group.iloc[0].copy()  # Copy the first row to retain all other column values
            new_row[factor_i] = new_name  # Set the new "Sense" value
            new_row[metric] = combined_metric  # Set the combined `metric` value using the lambda function
            new_rows_list.append(new_row)

        # Create a DataFrame from the new rows
        new_rows_df = pd.DataFrame(new_rows_list)

        # Append these new rows to the original dataset
        updated_df = pd.concat([df, new_rows_df], ignore_index=True)

        return updated_df

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
        if self.cl_args.mixtec_paired:
            filename = 'mixtec_paired_' + filename
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


class BootstrapManager(DataManager):

    def __init__(self, cl_args):
        super().__init__(cl_args)
        self.spline_calculator = SplineCalculator()

    def _resample(self, df):
        groupby_cols = list(self.factors.keys()) + ["TrainingSize"]
        return df.groupby(groupby_cols, group_keys=False).apply(
            lambda x: x.sample(n=len(x), replace=True)
        )

    def calculate_header(self):
        factor_cols = [
            f"{factor}_1,{factor}_2"
            for factor in self.factors.keys()
        ]
        header_cols = (
            ["Metric"]
            + factor_cols
            + ["Lower", "Upper"]
        )
        header = ",".join(header_cols) + "\n"
        return header

    def calculate_row(self, metric, condition_1, condition_2, lower, upper):
        row_elements = [metric]
        for factor in self.factors.keys():
            row_elements.append(str(condition_1[factor]))
            row_elements.append(str(condition_2[factor]))
        row_elements = row_elements + [str(lower), str(upper)]
        row = ",".join(row_elements) + "\n"
        return row

    def sample_across_conditions(
            self,
            df,
            condition_1,
            condition_2,
            metric,
            num_bootstrap_samples=1000
        ):
        filtered_data_1 = self._filter_data_for_one_curve(df, condition_1)
        filtered_data_2 = self._filter_data_for_one_curve(df, condition_2)

        bootstrapped_areas = []
        for _ in range(num_bootstrap_samples):
            sample_1 = self._resample(filtered_data_1)
            sample_2 = self._resample(filtered_data_2)
            aggregated_1 = self._aggregate_curve_data(sample_1, metric, 'std')
            aggregated_2 = self._aggregate_curve_data(sample_2, metric, 'std')

            # print(aggregated_1)
            # print(aggregated_2)
            # exit()
            area_1 = self.spline_calculator.area(
                aggregated_1['mean'].index,
                aggregated_1['mean'].values,
                aggregated_1['std'].values
            )
            area_2 = self.spline_calculator.area(
                aggregated_2['mean'].index,
                aggregated_2['mean'].values,
                aggregated_2['std'].values
            )
            bootstrapped_areas.append(area_1 - area_2)
        return bootstrapped_areas