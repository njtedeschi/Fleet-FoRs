import numpy as np
import pandas as pd
from tqdm import tqdm

class RankCollapser:

    def __init__(self, word_groupings, chunksize=10000):
        self.word_groupings = word_groupings
        self.chunksize = chunksize


    def process_experimental_condition(self, counts_path, posterior_lookup_path):
        counts_df = self.prepare_counts_df(counts_path)
        posterior_lookup_df = pd.read_csv(posterior_lookup_path)

        counts_groups = counts_df.groupby(['TrainingSize', 'Iteration'])
        posterior_groups = posterior_lookup_df.groupby(['TrainingSize', 'Iteration'])

        models = []
        for ((counts_ts, counts_it), counts_group), ((posterior_ts, posterior_it), posterior_group) in zip(counts_groups, posterior_groups):
            # Ensure matching groups are processed together
            assert counts_ts == posterior_ts and counts_it == posterior_it, "Mismatched groups"
            model_df = pd.merge(counts_group, posterior_group[['Rank', 'Posterior']], on='Rank', how='left')
            model_df.set_index(['TrainingSize', 'Iteration', 'Rank', 'Word', 'Sense'], inplace=True)
            model_df = self.collapse_model_ranks(model_df)
            models.append(model_df)
        collapsed_df = pd.concat(models, ignore_index=True)
        # collapsed_df = self.pivot_and_flatten(collapsed_df)
        # collapsed_df.reset_index()
        return collapsed_df

    #####

    def prepare_counts_df(self, counts_path):
        words_to_include = self.word_groupings.keys()
        filtered_rows = []
        for chunk in tqdm(pd.read_csv(counts_path, chunksize=self.chunksize),
                          desc="Input chunks read"):
            filtered_chunk = chunk[chunk['Word'].isin(words_to_include)]
            filtered_rows.append(filtered_chunk)
        counts_df = pd.concat(filtered_rows, ignore_index=True)

        # Calculate whether accuracy is perfect for sense
        counts_df['PerfectAccuracy'] = ((counts_df['FP'] == 0) & (counts_df['FN'] == 0)).astype(int)
        # Combine words by group and average their metrics
        counts_df['Word'] = counts_df['Word'].apply(
            lambda word: self.word_groupings.get(word, word))
        counts_df = counts_df.groupby(['TrainingSize', 'Iteration', 'Rank', 'Word', 'Sense'], as_index=False).mean()
        return counts_df

    # Takes group with fixed "TrainingSize" and "Iteration", and "Posterior" values merged in
    def collapse_model_ranks(self, model_df):
        # Apply the log-sum-exp trick
        # Subtract the max log posterior from each log posterior to make numbers smaller
        max_log_posterior = model_df['Posterior'].max()
        model_df['StableLogProb'] = model_df['Posterior'] - max_log_posterior

        # Convert these stabilized log probabilities to unnormalized probabilities
        model_df['UnnormalizedProb'] = np.exp(model_df['StableLogProb'])

        # Normalize the probabilities
        model_df['NormalizedProb'] = model_df.groupby(['Word', 'Sense'])['UnnormalizedProb'].transform(lambda x: x / x.sum())

        # Sum out ranks to get expected values
        model_df = model_df.groupby(['TrainingSize', 'Iteration', 'Word', 'Sense']).apply(lambda group: pd.Series({
            'E[TP]': np.sum(group['TP'] * group['NormalizedProb']),
            'E[TN]': np.sum(group['TN'] * group['NormalizedProb']),
            'E[FP]': np.sum(group['FP'] * group['NormalizedProb']),
            'E[FN]': np.sum(group['FN'] * group['NormalizedProb']),
            'Share': np.sum(group['PerfectAccuracy'] * group['NormalizedProb']),
        })).reset_index()

        model_df = self.calculate_metrics(model_df)

        return model_df

    def calculate_metrics(self, df):
        tp = df['E[TP]']
        tn = df['E[TN]']
        fp = df['E[FP]']
        fn = df['E[FN]']

        precision = tp / (tp + fp).replace(0, np.nan)
        recall = tp / (tp + fn).replace(0, np.nan)
        accuracy = (tp + tn) / (tp + tn + fp + fn).replace(0, np.nan)
        f1 = 2 * (precision * recall) / (precision + recall).replace(0, np.nan)

        df['Precision'] = precision
        df['Recall'] = recall
        df['F1'] = f1
        df['Accuracy'] = accuracy

        # Drop the TP, TN, FP, FN columns
        for metric in ['E[TP]', 'E[TN]', 'E[FP]', 'E[FN]']:
            df.drop((metric), axis=1, inplace=True, errors='ignore')
        return df
