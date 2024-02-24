import pandas as pd
import matplotlib.pyplot as plt

class Plotter:

    def __init__(self, legend_info, metric_labels):
        self.legend_info = legend_info
        self.metric_labels = metric_labels

    def plot_mean_values(self,
                         df,
                         metric,
                         factor_col,
                         factor_vals,
                         filter_conditions,
                         title,
                         save_path=None):
        """
        High-level function to plot mean values of a specified metric, for different factor values, with data filtered by given conditions.

        Parameters are described in the context of the overall plotting process.
        """
        filtered_df = self._apply_filters(df, filter_conditions)
        if filtered_df.empty:
            raise ValueError("No data left after applying filter conditions. Please adjust your filters.")

        grouped_data = self._group_and_aggregate(filtered_df, metric, factor_col)
        self._plot_curves(grouped_data, metric, factor_col, factor_vals, title, save_path)

    def _apply_filters(self, df, filter_conditions):
        """
        Filter the DataFrame based on given conditions.

        Parameters:
        - df: The DataFrame to filter.
        - filter_conditions: Dictionary with conditions used to filter the DataFrame.

        Returns:
        - Filtered DataFrame.
        """
        for column, value in filter_conditions.items():
            df = df[df[column] == value]
        return df

    def _group_and_aggregate(self, df, metric, factor_col):
        """
        Group by 'TrainingSize' and 'factor_col', then aggregate by mean and SEM.

        Parameters:
        - df: DataFrame to aggregate.
        - metric: The metric column to aggregate.
        - factor_col: The column name used to group the data alongside 'TrainingSize'.

        Returns:
        - Grouped and aggregated DataFrame with multi-level columns for mean and SEM.
        """
        return df.groupby(['TrainingSize', factor_col])[metric].agg(['mean', 'sem']).unstack()

    def _plot_curves(self, grouped_data, metric, factor_col, factor_vals, title, save_path=None):
        """
        Plot curves for the specified factor values.

        Parameters:
        - grouped_data: The grouped and aggregated DataFrame.
        - metric: The metric column to plot.
        - factor_values: Values within the factor column to plot as separate curves.
        - title: Plot title.
        - save_path: If provided, path to save the plot, otherwise it is shown.
        """
        plt.figure(figsize=(12, 6))

        for factor_val in factor_vals:
            info = self.legend_info.get(factor_col, {'label': factor_val, 'color': 'grey', 'linestyle': '-'})[factor_val]
            if factor_val in grouped_data['mean']:
                means = grouped_data['mean'][factor_val]
                sems = grouped_data['sem'][factor_val]
                training_sizes = grouped_data.index

                plt.errorbar(training_sizes, means, yerr=sems,
                             label=info['label'],
                             color=info['color'],
                             linestyle=info['linestyle'])
            else:
                print(f"Warning: {factor_val} not found. Skipping.")

        plt.ylim(0, 1)
        plt.xlabel('Amount of Training Data')
        plt.ylabel(f'Average {self.metric_labels.get(metric, metric)}')
        plt.title(title)
        plt.legend()
        plt.grid(True)
        if save_path:
            plt.savefig(save_path)
            plt.close()
        else:
            plt.show()