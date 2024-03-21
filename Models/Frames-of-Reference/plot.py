import argparse

from dataprocessing.src.data_manager import PlotManager

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
    parser.add_argument('--title',
                        action='store_true',
                        help="Whether to give title to plots"
    )
    parser.add_argument('--no_legend',
                        action='store_false',
                        dest='legend',
                        help="Disable the legend.")
    parser.add_argument('--save',
                        action='store_true',
                        help="Whether to save plots as pngs"
    )
    parser.add_argument('--xmax',
                        type=int,
                        nargs='?',
                        default=None,
                        help='Optional maximum x value'
    )
    cl_args = parser.parse_args()
    metric = cl_args.metric
    factor = cl_args.factor

    if(cl_args.simple):
        results_type = "simplified_shares"
        # Override choice of metric if provided
        metric = "Share"
    else:
        results_type = "all_metrics"

    plot_manager = PlotManager(cl_args)
    data = plot_manager.load_df(results_type)
    ### UNCOMMENT FOR DOING MORE COMPLEX CURVES
    # data = plot_manager.combine_factor_values(
    #     data,
    #     "Share",
    #     "Sense",
    #     # ["Int", "Rel", "IntRel"],
    #     ["Int", "Rel"],
    #     # lambda shares: shares.sum(),
    #     lambda shares: shares.iloc[0] - shares.iloc[1],
    #     # "Angular"
    #     "Int - Rel"
    # )
    ###
    plot_manager.create_all_plots(data, metric, factor)
