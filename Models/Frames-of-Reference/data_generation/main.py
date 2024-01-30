import argparse

from tqdm import tqdm

from my_data import DataGenerator
from file_management import FileManager

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('config_path', help="Path to the configuration YAML file")
    parser.add_argument('output_root', help="Path to the output root directory")
    parser.add_argument('--seed', type=int, default=None, help="Random seed for reproducibility")
    parser.add_argument('--verbose', action='store_true', help="Adds recording of flip results to data")
    args = parser.parse_args()

    config_path = args.config_path
    output_root = args.output_root
    # Optional arguments
    seed = args.seed
    verbose = args.verbose

    file_manager = FileManager(config_path, output_root)

    # Get loop parameters
    train_min = file_manager.train_min
    train_max = file_manager.train_max
    train_step = file_manager.train_step
    train_sizes = range(train_min, train_max+1, train_step)
    repetitions = file_manager.repetitions
    experimental_conditions = file_manager.experimental_conditions

    # Calculate total number of items created for estimating progress
    total_objects = sum(train_sizes) * repetitions * len(experimental_conditions)
    # Produce and save data
    with tqdm(total=total_objects, desc="Overall Progress", unit="object") as pbar:
        for experimental_condition in experimental_conditions:
            data_generator = DataGenerator(experimental_condition, seed, verbose)
            for train_size in train_sizes:
                for repetition in range(repetitions):
                    train_data = data_generator.sample_data(train_size)
                    file_manager.save_data(train_data,
                                        experimental_condition, # Subdirectory
                                        train_size, # File name part 1
                                        repetition) # File name part 2
                    pbar.update(train_size) # Update progress bar by 1 per iteration

if __name__ == "__main__":
    main()