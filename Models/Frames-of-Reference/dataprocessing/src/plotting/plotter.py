from itertools import cycle

import pandas as pd
import matplotlib.pyplot as plt

class Plotter:

    def __init__(self, config):
        self.config = config
        self.labels = config.labels
        self.colors = config.colors
        self.linestyles = config.linestyles
        self.default_colors = cycle(
            plt.rcParams['axes.prop_cycle']
            .by_key()['color']
        )

    def initialize_plot(self):
        plt.figure(figsize=self.config.figsize)

    def plot_curve(self, x, y, yerr=None, factor_i="", value=""):
        """Plot a single curve with error bars."""
        plt.errorbar(
            x,
            y,
            yerr=yerr,
            label=self.labels.get(value, f"{factor_i}-{value}"),
            # Use color from FACTOR_COLORS if available, otherwise use the next color from the default cycle
            color=self.colors.get(value, next(self.default_colors)),
            linestyle=self.linestyles.get(value, "solid")
        )

    def finalize_plot(self, metric, factor_i, factor_values, save_path=None):
        """Finalize and display or save the plot."""
        if self.config.title_format:
            title = self.config.title_format.format(**factor_values)
        else:
            title = f"{metric} Learning Curve for {factor_values}, varying {factor_i}"

        plt.ylim(self.config.ylim)
        plt.title(title)
        plt.xlabel(self.config.xlabel)
        plt.ylabel(self.config.ylabel or metric)
        plt.legend()
        if save_path:
            plt.savefig(save_path)
            plt.close()
        else:
            plt.show()
