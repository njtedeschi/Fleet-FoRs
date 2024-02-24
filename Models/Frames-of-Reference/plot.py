import argparse
# import os

# import pandas as pd
# import matplotlib.pyplot as plt
# import yaml

from dataprocessing.src.plotting.plotter import Plotter
from dataprocessing.src.plotting.plot_manager import PlotManager
from dataprocessing.src.plotting.constants import (
    LEGEND_INFO,
    METRIC_LABELS,
    ENGLISH_WORD_SENSES,
    MIXTEC_WORD_SENSES_SM_COMBINED,
    MIXTEC_WORD_SENSES_SM_SEPARATE
)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('root', help="Path to the root directory") # e.g. results/DATE/
    parser.add_argument('--metric',
                        default="Share",
                        help="Average value to plot"
    ) # e.g. "Share" or "F1"
    parser.add_argument('--factor',
                        default="Sense",
                        help="Factor to vary for curves on one plot"
    ) # e.g. "Word", "Language", "%IntrinsicData"
    parser.add_argument('--simple',
                        action='store_true',
                        help="Whether to just plot shares with symmetric and meronymic senses combined"
    )
    parser.add_argument('--save',
                        action='store_true',
                        help="Whether to save plots as pngs"
    )
    args = parser.parse_args()

    root = args.root
    metric = args.metric
    factor = args.factor
    # factor = args.factor
    simple = args.simple
    save_results = args.save

    languages = {
        "English" : ENGLISH_WORD_SENSES
        # Mixtec set after processing data
    }
    if(simple):
        languages["Mixtec"] = MIXTEC_WORD_SENSES_SM_COMBINED
        results_type = "simplified_shares"
        # Override choice of metric if provided
        metric = "Share"
    else:
        languages["Mixtec"] = MIXTEC_WORD_SENSES_SM_SEPARATE
        results_type = "all_metrics"

    plotter = Plotter(LEGEND_INFO, METRIC_LABELS)
    plot_manager = PlotManager(root, languages, plotter,
                               simple, save_results)
    data = plot_manager.load_df(results_type)

    if factor == "Sense":
        plot_manager.create_senses_plots(data, metric)
    else:
        print("Error: factor isn't supported")
