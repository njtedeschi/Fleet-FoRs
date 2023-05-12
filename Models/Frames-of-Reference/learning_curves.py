import csv
from enum import Enum
import re
import argparse
# import logsumexp from scipy.special
import numpy as np
from scipy.special import logsumexp
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

class FoR(Enum):
    INT = 1
    REL = 2
    ABS = 3
    OTHER = 4

TARGET = {
        "above" : {FoR.ABS},
        "below" : {FoR.ABS},
        "front" : {FoR.INT, FoR.REL},
        "behind" : {FoR.INT, FoR.REL},
        "left" : {FoR.INT, FoR.REL},
        "right" : {FoR.INT, FoR.REL},
        "side" : {FoR.INT}
        }

def categorize_hypothesis(formula):
    categories = set()

    # formulas for manual inspection
    # if "and" in formula:
    #     return {FoR.OTHER}
    # elif formula.count("or(") > 1:
    #     return {FoR.OTHER}

    # checking for INT, REL, and ABS
    # regex patterns looking for specific character after first opening paren
    if re.search(r"G_to_F[^()]*\(G", formula):
        categories.add(FoR.INT)
    if re.search(r"G'_to_F[^()]*\(S", formula):
        categories.add(FoR.REL)
    if "F,U" in formula:
        categories.add(FoR.ABS)
    return categories

class LexiconCategories(Enum):
    All_CORRECT = 0
    THREE_CORRECT = 1
    TWO_CORRECT = 2
    ONE_CORRECT = 3
    ZERO_CORRECT = 4
    OTHER = 5

def categorize_lexicon(formulas):
    wrong_count = 0
    for word, formula in formulas.items():
        hypothesis_categories = categorize_hypothesis(formula)
        # print(f"{hypothesis_categories} - {word} - {formula}")
        # handle "other" cases
        if hypothesis_categories == {FoR.OTHER}:
            return LexiconCategories.OTHER.name
        elif word == "above" or word == "below":
            if FoR.ABS not in hypothesis_categories:
                return LexiconCategories.OTHER.name
        elif word == "side":
            if hypothesis_categories != {FoR.INT}:
                return LexiconCategories.OTHER.name

        # count how many of "front", "behind", "left", and "right" are int only
        elif hypothesis_categories != {FoR.INT, FoR.REL}:
            wrong_count += 1
    return LexiconCategories(wrong_count).name

# Code for learning curves

def extract_results(csv_file, uid):
    results = pd.read_csv(csv_file)
    return results.query(f'uid == "{uid}"') # Filter to only include data from one experiment

def format_for_learning_curves(results, categories):
    # initialize data frame with category columns
    category_names = [category.name for category in categories]
    learning_curve_data = pd.DataFrame(columns = ['data_amount'] + category_names)
    # learning_curve_data = pd.DataFrame(columns=category_names)
    # learning_curve_data.set_index(pd.Index([], name='data_amount'))
    learning_curve_data = learning_curve_data.set_index('data_amount')

    data_amount_groups = results.groupby('data_amount')
    for data_amount, data_amount_group in data_amount_groups:
        # initialize posteriors for data amount as 0
        # new_row = pd.DataFrame({'data_amount': [data_amount], **{category: 0 for category in category_names}})
        # learning_curve_data = pd.concat([learning_curve_data,new_row], axis=0)
        learning_curve_data.loc[data_amount] = {category: 0 for category in category_names}
        # posteriors = {category : 0 for category in category_names}
        posterior_numerators = {category : [] for category in category_names}

        # calculate total posterior for each category
        for _, lexicon_results in data_amount_group.iterrows():
            # Categorize lexicon
            formulas = {word:lexicon_results[word] for word in TARGET.keys()}
            lexicon_categorization = categorize_lexicon(formulas)
            # Add lexicon's posterior to corresponding category
            # posteriors[lexicon_categorization] += np.exp(lexicon_results["posterior"])
            posterior_numerators[lexicon_categorization].append(lexicon_results["posterior"])
        # update data frame with shares of posteriors

        posteriors = posterior_estimates(posterior_numerators)
        for category in category_names:
            learning_curve_data.loc[data_amount, category] = posteriors[category]
        print(learning_curve_data)
    return learning_curve_data

def posterior_estimates(numerators):
    # Compute the log of the total probability for each category
    # Conditional handles empty lists
    category_sums = {category: logsumexp(log_probs) if len(log_probs) > 0 else -np.inf for category, log_probs in numerators.items()}

    # Compute the log of the total probability over all categories
    denominator = logsumexp(list(category_sums.values()))

    # Convert the log probabilities to actual probabilities and normalize them
    normalized_probs = {category: np.exp(numerator - denominator) for category, numerator in category_sums.items()}
    return normalized_probs

def plot_learning_curves(learning_curve_data):
    # Convert data to long form for lineplot
    long_form_data = learning_curve_data.reset_index().melt(id_vars='data_amount', var_name='category', value_name='posterior')

    # Plot
    sns.set_theme(style="darkgrid")
    sns.lineplot(data=long_form_data,
                 x='data_amount',
                 y='posterior',
                 hue='category',
                 marker='o')
    plt.xlabel("Data Amount")
    plt.ylabel("Posterior Probability")
    plt.title("Learning Curves")
    plt.legend(title="Category")
    plt.show()

def main():
    # Command-line arguments
    parser = argparse.ArgumentParser(description='Plot learning curves from CSV data.')
    parser.add_argument('csv_file', type=str, help='Path to the .csv file containing the data.')
    parser.add_argument('uid_to_plot', type=str, help='The UID of the experiment to plot.')
    args = parser.parse_args()

    results = extract_results(args.csv_file, args.uid_to_plot)
    categories = list(LexiconCategories)

    learning_curve_data = format_for_learning_curves(results, categories)
    plot_learning_curves(learning_curve_data)


if __name__ == '__main__':
    main()
