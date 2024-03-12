import numpy as np
import pandas as pd
from tqdm import tqdm

class RankCollapser:

    def __init__(self, word_groupings, sense_groupings):
        self.word_groupings = word_groupings
        self.sense_groupings = sense_groupings
        self.chunksize = 10000

    def process_experimental_condition(self, counts_path, posterior_lookup_path):
        counts_df = self.prepare_counts_df(counts_path)
        posterior_lookup_df = pd.read_csv(posterior_lookup_path)

        counts_groups = counts_df.groupby(['TrainingSize', 'Iteration'])
        posterior_groups = posterior_lookup_df.groupby(['TrainingSize', 'Iteration'])

        models = []
        for ((counts_ts, counts_it), counts_group), ((posterior_ts, posterior_it), posterior_group) in tqdm(zip(counts_groups, posterior_groups), total=counts_groups.ngroups, desc="Processing Groups"):
            # Ensure matching groups are processed together
            assert counts_ts == posterior_ts and counts_it == posterior_it, "Mismatched groups"
            model_df = pd.merge(counts_group, posterior_group[['Rank', 'Posterior']], on='Rank', how='left')
            model_df.set_index(['TrainingSize', 'Iteration', 'Rank', 'Word', 'Sense'], inplace=True)
            model_df = self.collapse_model_ranks(model_df)
            models.append(model_df)
        collapsed_df = pd.concat(models, ignore_index=True)
        return collapsed_df

    #####

    def prepare_counts_df(self, counts_path):
        chunks = []
        for chunk in tqdm(pd.read_csv(counts_path, chunksize=self.chunksize),
                          desc="Input chunks read"):
            chunks.append(chunk)
        df = pd.concat(chunks, ignore_index=True)
        # Calculate whether accuracy is perfect for sense
        df['PerfectAccuracy'] = ((df['FP'] == 0) & (df['FN'] == 0)).astype(int)
        # Combine senses by group and set "PerfectAccuracy" by disjunction
        if self.sense_groupings:
            df['Sense'] = df['Sense'].replace(self.sense_groupings)
            df = df.groupby(
                    ['TrainingSize', 'Iteration', 'Rank', 'Word', 'Sense']
                ).agg(
                    {'PerfectAccuracy': 'max'}
                ).reset_index()
        # Combine words by group and average their metrics
        df['Word'] = df['Word'].replace(self.word_groupings)
        df = df.groupby(
                ['TrainingSize', 'Iteration', 'Rank', 'Word', 'Sense'],
                as_index=False
            ).mean()
        return df

    # Takes group with fixed "TrainingSize" and "Iteration", and "Posterior" values merged in
    def collapse_model_ranks(self, model_df):
        # Apply the log-sum-exp trick
        # Subtract the max log posterior from each log posterior to make numbers smaller
        max_log_posterior = model_df['Posterior'].max()
        model_df['StableLogProb'] = model_df['Posterior'] - max_log_posterior

        # Convert these stabilized log probabilities to unnormalized probabilities
        model_df['UnnormalizedProb'] = np.exp(model_df['StableLogProb'])

        # Normalize the probabilities
        model_df['NormalizedProb'] = model_df.groupby(
            ['Word', 'Sense']
        )['UnnormalizedProb'].transform(
            lambda x: x / x.sum()
        )

        # Sum out ranks to get expected values using .agg() instead of .apply()
        metric_dict = {
            'Share': 'PerfectAccuracy'
        }
        # Only calculate more advanced metrics if no sense grouping simplifications were made
        calculate_advanced_metrics = not self.sense_groupings
        if calculate_advanced_metrics:
            counts = ['TP', 'TN', 'FP', 'FN']
            for count in counts:
                metric_dict[f'E[{count}]'] = count
        # Precompute products for each metric with NormalizedProb
        for original_metric in metric_dict.values():
            product_column = f"{original_metric}_Prod"
            model_df[product_column] = model_df[original_metric] * model_df['NormalizedProb']

        # Setup aggregation dictionary using precomputed product columns
        agg_dict = {metric: (f"{original_metric}_Prod", 'sum') for metric, original_metric in metric_dict.items()}

        model_df = model_df.groupby(
            ['TrainingSize', 'Iteration', 'Word', 'Sense']
        ).agg(**agg_dict).reset_index()
        model_df = self.finalize_collapsed_df(model_df, calculate_advanced_metrics)

        return model_df

    def finalize_collapsed_df(self, df, calculate_advanced_metrics=False):
        if calculate_advanced_metrics:
            tp = df['E[TP]'].to_numpy()
            tn = df['E[TN]'].to_numpy()
            fp = df['E[FP]'].to_numpy()
            fn = df['E[FN]'].to_numpy()

            # Use np.divide for safe division, filling with NaN where division by zero would occur
            precision = np.divide(tp, tp + fp, where=((tp + fp) != 0))
            recall = np.divide(tp, tp + fn, where=((tp + fn) != 0))
            accuracy = np.divide(tp + tn, tp + tn + fp + fn, where=((tp + tn + fp + fn) != 0))
            f1_numerator = 2 * (precision * recall)
            f1_denominator = precision + recall
            f1 = np.divide(f1_numerator, f1_denominator, where=(f1_denominator != 0))

            df['Precision'] = precision
            df['Recall'] = recall
            df['F1'] = f1
            df['Accuracy'] = accuracy

        # Drop the TP, TN, FP, FN columns
        df.drop(columns=['E[TP]', 'E[TN]', 'E[FP]', 'E[FN]'], inplace=True, errors='ignore')
        return df
