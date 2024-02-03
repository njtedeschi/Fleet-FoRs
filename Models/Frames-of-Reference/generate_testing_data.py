import argparse

from datageneration.data_generator import DataGenerator
from datageneration.file_manager import FileManager

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('root', help="Path to the output root directory") # e.g. results/DATE/
    parser.add_argument('--seed', type=int, default=None, help="Random seed for reproducibility")
    # TODO: handle --verbose argument
    parser.add_argument('--verbose', action='store_true', help="Adds recording of flip results to data")
    args = parser.parse_args()

    root = args.root
    # Optional arguments
    seed = args.seed
    verbose = args.verbose

    file_manager = FileManager(root, training=False)

    test_size = file_manager.get_from_config("test_size")
    experimental_conditions = file_manager.experimental_conditions

    for experimental_condition in experimental_conditions:
        data_generator = DataGenerator(experimental_condition, seed, verbose)
        test_data = data_generator.sample_testing_data(test_size)
        file_manager.save_testing_data(test_data, experimental_condition)

if __name__ == "__main__":
    main()