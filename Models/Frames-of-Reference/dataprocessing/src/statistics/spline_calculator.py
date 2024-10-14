import numpy as np
from scipy.interpolate import splrep, splev

SMOOTHING = 5
SPACING = 200

class SplineCalculator:

    def __init__(self, smoothing=SMOOTHING, spacing=SPACING):
        self.smoothing = smoothing
        self.spacing = spacing

    # Fitting splines
    def calculate_b_spline(self, x, y, standard_deviations):
        weights = self.calculate_weights(standard_deviations)

        tck = splrep(x, y, w=weights, s=self.smoothing)
        return tck

    def calculate_weights(self, standard_deviations):
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
    def evaluate_spline(self, x, tck):
        xs = np.linspace(min(x), max(x), self.spacing)
        ys = splev(xs, tck)
        return xs, ys

    def area(self, xs, ys):
        area = np.trapz(ys, xs) / (max(xs) - min(xs))
        return area

    # def area(self, x, y, standard_deviations):
    #     tck = self.calculate_b_spline(x, y, standard_deviations)
    #     return self._area(tck, min(x), max(x))

    # def _area(self, tck, x_min, x_max):
    #     xs = np.linspace(x_min, x_max, self.spacing)
    #     ys = splev(xs, tck)

    #     area = np.trapz(ys, xs) / (x_max - x_min)
    #     return area