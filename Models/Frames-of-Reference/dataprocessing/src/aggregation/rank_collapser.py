import numpy as np
import pandas as pd
from tqdm import tqdm

class RankCollapser:

    def __init__(self, word_groupings, senses, chunksize=10000):
        self.word_groupings = word_groupings
        self.senses = senses
        self.chunksize = chunksize

    def csv_to_filtered_counts_df(self, results_path):
        words_to_include = self.word_groupings.keys()
        filtered_rows = []
        for chunk in tqdm(pd.read_csv(results_path, chunksize=self.chunksize),
                          desc="Input chunks read"):
            filtered_chunk = chunk[chunk['Word'].isin(words_to_include)]
            filtered_rows.append(filtered_chunk)
        df = pd.concat(filtered_rows, ignore_index=True)
        return df

    ## Posterior share methods
    def process_experimental_condition(self, counts_path, posterior_lookup_path):
        counts_df = self.csv_to_filtered_counts_df(counts_path)
        classification_df = self.classify_rows(counts_df)
        consolidated_df = self.consolidate_word_pairs(classification_df)

        posterior_lookup_df = pd.read_csv(posterior_lookup_path)
        collapsed_df = self.collapse_df_ranks_to_posterior_shares(consolidated_df, posterior_lookup_df)
        return collapsed_df

    def classify_rows(self, counts_df):
        # Calculate whether accuracy is perfect for sense
        counts_df['PerfectAccuracy'] = ((counts_df['FP'] == 0) & (counts_df['FN'] == 0)).astype(int)
        # Pivote the table
        classification_df = counts_df.pivot_table(index=["TrainingSize", "Iteration", "Rank", "Word"],
                                                  columns="Sense",
                                                  values="PerfectAccuracy",
                                                  fill_value=0).reset_index()
        # Get rid of "Sense" column index from pivoting
        classification_df.columns.name = None
        return classification_df

    def consolidate_word_pairs(self, classification_df):
        # Stage 1: Replace words with their combined names
        classification_df['Word'] = classification_df['Word'].apply(
            lambda word: self.word_groupings.get(word, word))

        # Stage 2: Average sense column values for rows that are the same except for the sense classification values
        consolidated_df = classification_df.groupby(['TrainingSize', 'Iteration', 'Rank', 'Word'], as_index=False).mean()
        return consolidated_df

    def collapse_df_ranks_to_posterior_shares(self, consolidated_df, posterior_lookup_df):
        # Get unique combinations of TrainingSize and Iteration
        combinations = consolidated_df[['TrainingSize', 'Iteration']].drop_duplicates()

        # TODO update overall use of tqdm
        collapsed_groups = []
        for _, row in tqdm(combinations.iterrows(), total=combinations.shape[0], desc="Models processed"):
            training_size, iteration = row['TrainingSize'], row['Iteration']
            # Filter consolidated_df and self.posterior_df for the current combination of TrainingSize and Iteration
            classification_group = consolidated_df[(consolidated_df['TrainingSize'] == training_size) &
                                      (consolidated_df['Iteration'] == iteration)]
            posterior_group = posterior_lookup_df[(posterior_lookup_df['TrainingSize'] == training_size) &
                                                (posterior_lookup_df['Iteration'] == iteration)]

            # Merge Posterior into the group based on Rank
            merged_group = pd.merge(classification_group, posterior_group[['Rank', 'Posterior']], on='Rank', how='left')

            # Collapse ranks within this group
            collapsed_group = self.collapse_group_ranks_to_posterior_shares(merged_group)
            collapsed_groups.append(collapsed_group)
        collapsed_df = pd.concat(collapsed_groups, ignore_index=True)
        return collapsed_df

    # Takes group with fixed "TrainingSize" and "Iteration", and "Posterior" values merged in
    def collapse_group_ranks_to_posterior_shares(self, group):
        # Apply the log-sum-exp trick
        # Subtract the max log posterior from each log posterior to make numbers smaller
        max_log_posterior = group['Posterior'].max()
        group['StableLogProb'] = group['Posterior'] - max_log_posterior

        # Convert these stabilized log probabilities to unnormalized probabilities
        group['UnnormalizedProb'] = np.exp(group['StableLogProb'])

        # Normalize the probabilities
        # sum_prob = group.groupby(['Word'])['UnnormalizedProb'].sum()
        # print(f"Sum of probabilities: {sum_prob}")
        # group['NormalizedProb'] = group['UnnormalizedProb'] / sum_prob
        group['NormalizedProb'] = group.groupby(['Word'])['UnnormalizedProb'].transform(lambda x: x / x.sum())

        # Calculate the posterior weighted average classifications for each sense column
        for sense in self.senses:
            group[f'{sense}-Share'] = group[sense] * group['NormalizedProb']

        # Collapse the results across ranks by summing the SenseShares across ranks
        collapsed_group = group.groupby(['TrainingSize', 'Iteration', 'Word']).agg({
            f'{sense}-Share': 'sum' for sense in self.senses
        }).reset_index()

        return collapsed_group

    ## Precision/recall/accuracy methods