import pandas as pd

class DataProcessor:

    def __init__(self,
                 groupby_cols,
                 index = "TrainingSize",
                 stats=('mean', 'sem', 'std')):
        self.groupby_cols = groupby_cols # Identifiers except for "Iteration"
        self.index = index,
        self.stats = stats

    def filter(self, data, condition):
        """
        Applies filtering conditions to the data using the DataFrame.query method.

        Parameters:
        - data: pandas DataFrame to filter.
        - conditions: Dictionary or string of conditions for filtering.

        Returns:
        - Filtered pandas DataFrame.
        """
        if isinstance(condition, dict):
            query_str = " & ".join([f"`{k}` == {repr(v)}" for k, v in condition.items()])
        elif isinstance(condition, str):
            query_str = condition
        else:
            raise ValueError("Conditions must be either a dictionary or a string.")

        return data.query(query_str)

    def aggregate(self, data, metrics):
        # Ensure metrics is always in list form
        if isinstance(metrics, str):
            metrics = [metrics]  # Convert single metric to a list
        aggregated_data = data.groupby(self.groupby_cols)[metrics].agg(self.stats)
        # Reset index to remove all levels except for 'TrainingSize'
        # Note: This assumes 'TrainingSize' is one of the groupby columns and you want it as the sole index
        aggregated_data = aggregated_data.reset_index().set_index(self.index)
        return aggregated_data

    def resample(self, data):
        return data.groupby(self.groupby_cols, group_keys=False).apply(
            lambda x: x.sample(n=len(x), replace=True)
        )