import numpy as np

def confidence_interval(bootstrapped_areas, alpha=0.05):
    bottom_percentile = alpha/2
    top_percentile = (1 - alpha/2)

    lower_bound = np.percentile(bootstrapped_areas, bottom_percentile)
    upper_bound = np.percentile(bootstrapped_areas, top_percentile)

    return lower_bound, upper_bound