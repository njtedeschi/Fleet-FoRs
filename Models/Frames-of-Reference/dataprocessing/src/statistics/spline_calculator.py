import numpy as np
from scipy.interpolate import splrep, splev

SMOOTHING = 5
SPACING = 200

class SplineCalculator:

    def __init__(self, smoothing=SMOOTHING, spacing=SPACING):
        self.smoothing = smoothing
        self.spacing = spacing

    # Fitting splines
    def calculate_b_spline(self, aggregate_data, metric):
        y = aggregate_data[metric]['mean']
        x = aggregate_data.index
        weights = self.calculate_weights(aggregate_data, metric)

        tck = splrep(x, y, w=weights, s=self.smoothing)
        return tck

    def calculate_weights(self, aggregate_data, metric):
        standard_deviations = aggregate_data[metric]['std']
        min_std = 1e-3
        weights = []
        for std in standard_deviations:
            if std < min_std:
                weight = 1/min_std
            else:
                weight = 1/std
            weights.append(weight)
        return weights

    # Evaluating splines
    def area(self, aggregate_data, metric):
        tck = self.calculate_b_spline(aggregate_data, metric)
        x = aggregate_data.index
        return self._area(tck, min(x), max(x))

    def _area(self, tck, x_min, x_max):
        xs = np.linspace(x_min, x_max, self.spacing)
        ys = splev(xs, tck)

        area = np.trapz(ys, xs) / (x_max - x_min)
        return area