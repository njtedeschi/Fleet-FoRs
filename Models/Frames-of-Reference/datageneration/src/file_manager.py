from dataclasses import fields
import itertools
import json
import os

import numpy as np
import yaml

from .structures.experiment import ExperimentalCondition

class FileManager:
    # Processes the input yaml
    # Saves output files

    def __init__(self, root, training=True):
        self.root = root
        self.config = self.load_config()
        self.validate_config()
        self.experimental_conditions = self.determine_experimental_conditions()

        if training:
            self.create_training_output_directories()

    def get_from_config(self, param):
        return self.config["experiment"][param]

    def load_config(self):
        try:
            # Construct the path to the config.yaml file
            config_path = os.path.join(self.root, 'config_data.yaml')
            with open(config_path, 'r') as file:
                return yaml.safe_load(file)
        except FileNotFoundError:
            print(f"Error: The file {config_path} was not found.")
            return None
        except yaml.YAMLError as exc:
            print(f"Error parsing the YAML file: {exc}")
            return None

    def validate_config(self):
            # Extracting the hyperparameters section from the config
            hyperparameters_config = self.config.get('hyperparameters', {})

            # Getting all the fields from ExperimentalCondition that start with 'p_'
            experimental_condition_fields = [f.name for f in fields(ExperimentalCondition) if f.name.startswith('p_')]

            # Check if all required fields are in the config
            missing_in_config = [field for field in experimental_condition_fields if field not in hyperparameters_config]
            if missing_in_config:
                raise ValueError(f"Missing fields in config: {missing_in_config}")

            # Check if there are extra fields in the config that are not in ExperimentalCondition
            extra_in_config = [key for key in hyperparameters_config if key.startswith('p_') and key not in experimental_condition_fields]
            if extra_in_config:
                raise ValueError(f"Extra fields in config not in ExperimentalCondition: {extra_in_config}")

    # Get all combinations of hyperparameters
    def determine_experimental_conditions(self):
        hyperparameters = self.config["hyperparameters"]

        # Separate fixed and variable hyperparameters
        fixed_hyperparams = {}
        variable_hyperparams = {}
        for key, value in hyperparameters.items():
            if isinstance(value, dict):  # Variable hyperparameters
                variable_hyperparams[key] = value
            else:  # Fixed hyperparameters
                fixed_hyperparams[key] = value

        # Generate combinations of variable hyperparameters
        keys, values = zip(*variable_hyperparams.items())
        combinations = [dict(zip(keys, v)) for v in itertools.product(*values)]

        # Create ExperimentalCondition objects for each combination
        experimental_conditions = []
        for combo in combinations:
            # Merge fixed and variable hyperparameters, using values for attributes
            combined_hyperparams = {**fixed_hyperparams}
            labels = []

            for key, variable_key in combo.items():
                combined_hyperparams[key] = variable_hyperparams[key][variable_key]  # Use the value for the attribute
                labels.append(variable_key)  # Use the variable key as label used in naming condition

            # Instantiate ExperimentalCondition
            experimental_condition = ExperimentalCondition(
                **combined_hyperparams, labels=labels
            )
            experimental_conditions.append(experimental_condition)

        return experimental_conditions

    def create_training_output_directories(self):
        for experimental_condition in self.experimental_conditions:
            output_directory = self.set_training_output_directory(experimental_condition)
            # Check if the directory already exists
            if not os.path.exists(output_directory):
                # If it doesn't exist, create it including any necessary parent directories
                os.makedirs(output_directory)
            else:
                print(f"Directory '{output_directory}' already exists.")

    ### Non init
    def serialize_to_json(self, obj):
        if isinstance(obj, np.ndarray):
            return obj.tolist() # np arrays turned to just list of floats
        elif hasattr(obj, "__dict__"):
            # For dataclass instances, seriealize each attribute
            return {attr: self.serialize_to_json(getattr(obj, attr)) for attr in vars(obj)}
        else:
            return obj

    # Training
    def set_training_output_directory(self, experimental_condition):
        output_directory = os.path.join(
            self.root,
            "training_data",
            experimental_condition.name,
        )
        return output_directory

    def set_training_filepath(self, output_directory, train_size, repetition):
        train_str = str(train_size).zfill(4)
        rep_str = str(repetition).zfill(2)
        filename = train_str + "_" + rep_str + ".json"
        filepath = os.path.join(output_directory, filename)
        return filepath

    def save_training_data(self, data, experimental_condition, train_size, repetition):
        json_data = json.dumps(data, default=self.serialize_to_json)
        output_directory = self.set_training_output_directory(experimental_condition)
        filepath = self.set_training_filepath(output_directory, train_size, repetition)

        with open(filepath, 'w') as file:
            file.write(json_data)

    # Testing
    def set_testing_output_directory(self):
        output_directory = os.path.join(
            self.root,
            "testing_data"
        )
        return output_directory

    def set_testing_filepath(self, output_directory, experimental_condition):
        filename = experimental_condition.name + ".json"
        filepath = os.path.join(output_directory, filename)
        return filepath

    def save_testing_data(self, data, experimental_condition):
        json_data = json.dumps(data, default=self.serialize_to_json)
        output_directory = self.set_testing_output_directory()
        filepath = self.set_testing_filepath(output_directory, experimental_condition)

        with open(filepath, 'w') as file:
            file.write(json_data)