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

    def finalize_plot(self, title=None, save_path=None, include_legend=True):
        """Finalize and display or save the plot."""
        plt.ylim(self.config.ylim)
        if title:
            plt.title(title)
        if include_legend:
            plt.legend()
        plt.xlabel(self.config.xlabel)
        plt.ylabel(self.config.ylabel)
        if save_path:
            plt.savefig(save_path)
            plt.close()
        else:
            plt.show()

    def plot_curve_on_ax(self, ax, x, y, yerr=None, factor_i="", value=""):
        """Plot a single curve with error bars."""
        ax.errorbar(
            x,
            y,
            yerr=yerr,
            label=self.labels.get(value, f"{factor_i}-{value}"),
            # Use color from FACTOR_COLORS if available, otherwise use the next color from the default cycle
            color=self.colors.get(value, next(self.default_colors)),
            linestyle=self.linestyles.get(value, "solid")
        )
