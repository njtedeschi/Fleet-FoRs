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
import sympy
from sympy.parsing.sympy_parser import parse_expr

### SymPy definitions

UP = sympy.Symbol("UP")
G_to_F = sympy.Symbol("G_to_F")
S = sympy.Symbol("S")
G = sympy.Symbol("G")
GG = sympy.Symbol("GG")
upward = sympy.Function("upward")
forward = sympy.Function("forward")
rightward = sympy.Function("rightward")
# G_not_P = sympy.Symbol("G_not_P")
cosine_similarity = sympy.Function("cosine_similiarity")

class BooleanFunction(sympy.Function, sympy.logic.boolalg.Boolean):
    is_Boolean = True

class is_participant(BooleanFunction):
    pass

class parallel(BooleanFunction):
    def simplify(self, *args, **kwargs):
        arg1, arg2 = self.args
        # return sympy.Eq(cosine_similarity(arg1, arg2), 1)
        return cosine_similarity(arg1, arg2) > 0

class antiparallel(BooleanFunction):
    def simplify(self, *args, **kwargs):
        arg1, arg2 = self.args
        # return sympy.Eq(cosine_similarity(arg1, arg2), -1)
        return cosine_similarity(arg1, arg2) < 0

class pm_parallel(BooleanFunction):
    def simplify(self, *args, **kwargs):
        arg1, arg2 = self.args
        # return sympy.Eq(cosine_similarity(arg1, arg2), -1)
        return sympy.Or(cosine_similarity(arg1, arg2) > 0, cosine_similarity(arg1, arg2) < 0)

### Classification

class Lexicon:

    def __init__(self, concepts):
        self.concepts = concepts # word : Concept dict

    def classify(self, targets):
            pass

class DNF:

    def __init__(self, disjuncts):
        self.disjuncts = disjuncts

    @classmethod
    def from_sympy(cls, sympy_dnf):
        disjuncts = sympy_dnf.args
        dnf = cls(disjuncts)
        return dnf

    def classify(self, target):
        pass

class ConjunctiveClause:

    def __init__(self, conjuncts):
        self.conjuncts = conjuncts

    @classmethod
    def from_sympy(cls, sympy_conjunctive_clause):
        conjuncts = sympy_conjunctive_clause.args
        conjunctive_clause = cls(conjuncts)
        return conjunctive_clause

    def classify(self):
        pass

class Literal:

    def __init__(self, atom, is_negated=False):
        self.atom = atom
        self.is_negated = is_negated

    @classmethod
    def from_sympy(cls, sympy_literal):
        is_negated = False
        if isinstance(sympy_literal, sympy.Not):
            is_negated = True
            atom = sympy_literal.args[0]
        else:
            atom = sympy_literal
        literal = cls(atom, is_negated)
        return literal

    def classify(self):
        pass

class Axis(Enum):
    ABOVE = 1
    BELOW = -1
    FRONT = 2
    BEHIND = -2
    RIGHT = 3
    LEFT = -3

class FoR(Enum):
    INT = 1
    REL = 2
    ABS = 3
    OTHER = 4

class AngularSpecification:

    # parameters:
    # frame : FoR
    # direction : Axis
    def __init__(self,frame,direction):
        self.frame = frame
        self.direction = direction

def angular_specifications(frames, directions):
    angular_specifications = set()
    for frame in frames:
        for direction in directions:
            angular_specification = AngularSpecification(frame=frame,direction=direction)
            angular_specifications.add(angular_specification)
    return angular_specifications

TARGETS = {
        "above" : angular_specifications(frames={FoR.ABS},
                                         directions={Axis.ABOVE}),
        "below" : angular_specifications(frames={FoR.ABS},
                                         directions={Axis.BELOW}),
        "front" : angular_specifications(frames={FoR.INT, FoR.REL},
                                         directions={Axis.FRONT}),
        "behind" : angular_specifications(frames={FoR.INT, FoR.REL},
                                          directions={Axis.BEHIND}),
        "left" : angular_specifications(frames={FoR.INT, FoR.REL},
                                        directions={Axis.LEFT}),
        "right" : angular_specifications(frames={FoR.INT, FoR.REL},
                                         directions={Axis.RIGHT}),
        "side" : angular_specifications(frames={FoR.INT},
                                        directions={Axis.LEFT, Axis.RIGHT})
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
            formulas = {word:lexicon_results[word] for word in TARGETS.keys()}
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

### Functions to simplify formula text for SymPy

def reformat_formula(formula):
    # NOTE: regex in replace_G_prime specifically handles up to two levels of nested parens
    # More specifically: \(([^()]*|\([^()]*\))*\)
    formula = replace_G_prime(formula)
    formula = capitalize_boolean_operators(formula)
    formula = rename_parallel_functions(formula)
    formula = remove_scene(formula)
    formula = rename_absolute_vertical(formula)
    return formula

def replace_G_prime(formula):
    return re.sub(r"([a-z+-]+)\((G\'_to_F,)(\w+\(([^()]*|\([^()]*\))*\))\)",r"And(\1(G_to_F,\3),Not(is_participant(GG)))",formula)

def capitalize_boolean_operators(formula):
    formula = formula.replace('and(', 'And(')
    formula = formula.replace('or(', 'Or(')
    formula = formula.replace('not(', 'Not(')
    return formula

def rename_parallel_functions(formula):
    formula = formula.replace('+-parallel', 'pm_parallel')
    formula = formula.replace('+parallel', 'parallel')
    formula = formula.replace('-parallel', 'antiparallel')
    return formula

def remove_scene(formula):
    return formula.replace('(scene)', '')

def rename_absolute_vertical(formula):
    return formula.replace('UP', 'upward(E)')


local_dict = {
        'G_to_F' : G_to_F,
        'S' : S,
        'G' : G,
        'GG' : GG,
        'parallel' : parallel,
        'antiparallel' : antiparallel,
        'pm_parallel' : pm_parallel,
        'is_participant' : is_participant,
        'upward' : upward,
        'forward' : forward,
        'righward' : rightward,
        # 'G_not_P' : G_not_P
        }

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
