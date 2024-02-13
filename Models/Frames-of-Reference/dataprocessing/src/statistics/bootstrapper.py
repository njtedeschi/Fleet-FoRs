
class Bootstrapper:

    def __init__(self, data_processor, spline_calculator):
        self.data_processor = data_processor
        self.spline_calculator = spline_calculator

    def sample_within_condition(self, data, condition, metric_1, metric_2, num_bootstrap_samples=1000):
        filtered_data = self.data_processor.filter(data, condition)

        bootstrapped_areas = []
        for _ in range(num_bootstrap_samples):
            sample = self.data_processor.resample(filtered_data)
            aggregated = self.data_processor.aggregate(sample, [metric_1, metric_2])

            area_1 = self.spline_calculator.area(aggregated, metric_1)
            area_2 = self.spline_calculator.area(aggregated, metric_2)
            bootstrapped_areas.append(area_1 - area_2)
        return bootstrapped_areas

    def sample_across_conditions(self, data, condition_1, condition_2, metric, num_bootstrap_samples=1000):
        filtered_data_1 = self.data_processor.filter(data, condition_1)
        filtered_data_2 = self.data_processor.filter(data, condition_2)

        bootstrapped_areas = []
        for _ in range(num_bootstrap_samples):
            sample_1 = self.data_processor.resample(filtered_data_1)
            sample_2 = self.data_processor.resample(filtered_data_2)
            aggregated_1 = self.data_processor.aggregate(sample_1, metric)
            aggregated_2 = self.data_processor.aggregate(sample_2, metric)

            area_1 = self.spline_calculator.area(aggregated_1, metric)
            area_2 = self.spline_calculator.area(aggregated_2, metric)
            bootstrapped_areas.append(area_1 - area_2)
        return bootstrapped_areas