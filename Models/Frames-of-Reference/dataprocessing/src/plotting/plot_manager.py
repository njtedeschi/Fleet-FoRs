import itertools
import os

import pandas as pd
import yaml

class PlotManager:

    def __init__(self, root, languages, plotter,
                 simple=False, save_results=False):
        self.root = root # TODO: root to save paths
        self.config = self._load_config()
        self.output_directory = None
        self.languages = self._filter_languages(languages)
        self.plotter = plotter
        # Optional flags
        self.simple = simple
        self.save_results = save_results

    def _load_config(self):
        try:
            # Construct the path to the config.yaml file
            config_path = os.path.join(
                self.root,
                'config_analysis.yaml'
            )
            with open(config_path, 'r') as file:
                return yaml.safe_load(file)
        except FileNotFoundError:
            print(f"Error: The file {config_path} was not found.")
            return None
        except yaml.YAMLError as exc:
            print(f"Error parsing the YAML file: {exc}")
            return None

    def load_df(self, results_type):
        results_file = self.config["results_files"][results_type]
        csv_path = os.path.join(
            self.root,
            "aggregate_results",
            results_file
        )
        return pd.read_csv(csv_path)

    def _filter_languages(self, languages):
        words_to_include = self.config["words_to_include"]
        filtered_languages = {
            language: {
                word: senses
                for word, senses in word_senses.items()
                if word in words_to_include
            }
            for language, word_senses in languages.items()
        }
        return filtered_languages

    def _determine_experimental_conditions(self):
        options = self.config["experimental_conditions"]
        combinations = itertools.product(*options.values())
        experimental_conditions = [
            {
                option: value
                for option, value in zip(
                    options.keys(), combination
                )
            }
            for combination in combinations
        ]
        return experimental_conditions

    # Senses plots

    def create_senses_plots(self, df, metric):
        experimental_conditions = self._determine_experimental_conditions()
        for experimental_condition in experimental_conditions:
            for language, words in self.languages.items():
                for word, senses in words.items():
                    self._create_senses_plot( df,
                        metric,
                        experimental_condition,
                        language,
                        word,
                        senses
                    )

    def _create_senses_plot(self,
                            df,
                            metric,
                            experimental_condition,
                            language,
                            word,
                            senses
                            ):
        filter_conditions = self._create_senses_plot_filter(experimental_condition, language, word)
        title = self._create_senses_plot_title(experimental_condition, language, word)
        if self.save_results:
            save_path = self._create_senses_plot_save_path(experimental_condition, language, word, metric)
        else:
            save_path = None
        self.plotter.plot_mean_values(
            df,
            metric,
            "Sense",
            senses,
            filter_conditions,
            title,
            save_path
        )

    def _create_senses_plot_filter(self, experimental_condition, language, word):
        filter_conditions = {
            "Language": language,
            "Word": word,
        }
        for subcondition, subcondition_value in experimental_condition.items():
            filter_conditions[subcondition] = subcondition_value
        return filter_conditions

    def _create_senses_plot_title(self, experimental_condition, language, word):
        return self._senses_plot_formatting(
            'title_format',
            experimental_condition, language, word
        )

    def _create_senses_plot_save_path(self, experimental_condition, language, word, metric):
        filestem = self._senses_plot_formatting(
            'file_format',
            experimental_condition, language, word
        )
        if self.simple:
            filename = filestem + '_simple.png'
        else:
            filename = filestem + f'_{metric}.png'
        filename = filename.replace("/", "_").lower()
        save_path = os.path.join(
            self.root,
            "plots",
            filename
        )
        return save_path

    def _senses_plot_formatting(self, format_type, experimental_condition, language, word):
        format_string = self.config["plot_type"]["senses"][format_type]
        # Prepare the format arguments including the experimental_condition dictionary
        format_args = {
            'experimental_condition': experimental_condition,
            'language': language,
            'word': word
        }
        # Use the format string from the naming dictionary and fill it using **format_args
        # This allows for dictionary access within the format string
        return format_string.format(**format_args)
