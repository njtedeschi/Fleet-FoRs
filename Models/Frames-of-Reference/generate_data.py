import argparse
from tqdm import tqdm

from datageneration.src.data_generator import TrainingDataGenerator, TestingDataGenerator
from datageneration.src.file_manager import FileManager

def generate_train_data(root, chapter, seed, verbose):
    file_manager = FileManager(root, training=True)

    # Get loop parameters
    train_min = file_manager.get_from_config("train_min")
    train_max = file_manager.get_from_config("train_max")
    train_step = file_manager.get_from_config("train_step")

    train_sizes = list(range(train_min, train_max + 1, train_step))
    repetitions = file_manager.get_from_config("repetitions")
    experimental_conditions = file_manager.experimental_conditions

    # Calculate total number of items created for estimating progress
    total_objects = sum(train_sizes) * repetitions * len(experimental_conditions)

    additional_train_sizes = file_manager.get_from_config("additional_train_sizes")
    if additional_train_sizes:
        train_sizes += additional_train_sizes
        total_objects += sum(additional_train_sizes) * repetitions * len(experimental_conditions)

    # Produce and save data
    with tqdm(total=total_objects, desc="Overall Progress", unit="object") as pbar:
        for experimental_condition in experimental_conditions:
            data_generator = TrainingDataGenerator(experimental_condition, chapter, seed, verbose)
            for train_size in train_sizes:
                for repetition in range(repetitions):
                    train_data = data_generator.generate_data(train_size)
                    file_manager.save_training_data(train_data,
                                                    experimental_condition,  # Subdirectory
                                                    train_size,  # File name part 1
                                                    repetition)  # File name part 2
                    pbar.update(train_size)  # Update progress bar by 1 per iteration

def generate_test_data(root, chapter, seed, verbose):
    file_manager = FileManager(root, training=False)

    test_size = file_manager.get_from_config("test_size")
    test_condition = file_manager.determine_test_condition()

    data_generator = TestingDataGenerator(test_condition, chapter, seed, verbose)
    test_data = data_generator.generate_data(test_size)
    file_manager.save_testing_data(test_data, test_condition)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('root', help="Path to the output root directory")  # e.g. results/DATE/
    parser.add_argument('--type', choices=['train', 'test'], required=True,
                        help="Specify whether to generate training or testing data")
    parser.add_argument('--chapter', choices=[2, 3], required=True, type=int,
                        help="Chapter number of model data is being generated for")
    parser.add_argument('--seed', type=int, default=None, help="Random seed for reproducibility")
    parser.add_argument('--verbose', action='store_true', help="Adds recording of flip results to data")
    args = parser.parse_args()

    root = args.root
    data_type = args.type
    chapter = args.chapter
    seed = args.seed
    verbose = args.verbose

    if data_type == 'train':
        generate_train_data(root, chapter, seed, verbose)
    elif data_type == 'test':
        generate_test_data(root, chapter, seed, verbose)

if __name__ == "__main__":
    main()
