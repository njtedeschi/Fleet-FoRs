#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>
#include <utility> // for std::pair

#include "Ch3MyStructures.h"
#include "Ch3MyData.h"
#include "Ch3GrammarSelection.h"
#include "Ch3MyHypothesis.h"

#include "TopN.h"
#include "Fleet.h"
// #include "Builtins.h"

#include "Ch3MyResultsDistribution.h"
// Declare here to avoid runtime symbol not found error
std::vector<UniqueTestingDatum> ModelEvaluator::testing_data;

// Use the std::filesystem namespace for directory operations
namespace fs = std::filesystem;

// Function to read the serialized string from a file
std::string read_top_n(const std::string& filepath);
std::string create_file_with_header(const std::string& directory,
                                 const std::string& filename_stem,
                                 const std::string& header,
                                 const std::string& suffix = "");
std::pair<int, int> extract_model_label(const std::string& filename_stem);

// std::string training_data_directory = "";
std::string testing_data_path = "";
std::string model_directory = "";
std::string output_directory = "";
std::string output_filename_stem = "";

std::string language_name = "";
std::shared_ptr<Language> LanguageContext::current_language = nullptr;

int main(int argc, char** argv) {
	Fleet fleet("Frames of Reference");

    // Input and output
    // fleet.add_option("--training_data_directory", training_data_directory, "Location of training data for model");
    fleet.add_option("--testing_data_path", testing_data_path, "Location of JSON with testing data");
    fleet.add_option("--model_directory", model_directory, "Location of trained models");
    fleet.add_option("--output_directory", output_directory, "Where to save testing results");
    fleet.add_option("--output_filename_stem", output_filename_stem, "Stem to use for results and lookup table files");

    fleet.add_option("--language", language_name, "Name of language used by model");

    fleet.initialize(argc, argv);

    // Set language
    if (language_name == "english") {
        LanguageContext::set_language(std::make_shared<English>());
    } else if (language_name == "mixtec") {
        LanguageContext::set_language(std::make_shared<Mixtec>());
    } else {
        std::cerr << "Invalid language name provided." << std::endl;
        return 1;
    }

    // Testing data
    MyData my_data(LanguageContext::get_words());
    std::vector<UniqueTestingDatum> testing_data = my_data.json_file_to_unique_testing_data(testing_data_path);

    // Create test results table and save path
    // std::string results_header = "TrainingSize,Iteration,TestDatum,Word,Sense,LogProbability";
    std::string results_header = "TrainingSize,Iteration,TestID,IntDescription,RelDescription,CanonicalX,CanonicalY,CanonicalZ,Word,Sense,LogProbability";
    std::string results_filepath = create_file_with_header(output_directory, output_filename_stem, results_header);

    // Iterate over trained models and test each
    for (const auto& entry : fs::directory_iterator(model_directory)) {
        if (entry.path().extension() == ".txt") {
            // // Read and deserialize model's top n hypotheses
            // std::string serialized_top = read_top_n(entry.path());
            // TopN<MyHypothesis> top = TopN<MyHypothesis>::deserialize(serialized_top);

            // Obtain training size and iteration from model filename
            std::string input_filename_stem = entry.path().stem().string();
            std::pair<int, int> model_label = extract_model_label(input_filename_stem);
            int training_size = model_label.first;
            int iteration = model_label.second;

            // Read and deserialize model's top n hypotheses
            std::string serialized_top = read_top_n(entry.path());
            TopN<MyHypothesis> top = TopN<MyHypothesis>::deserialize(serialized_top);

            // Extract training data for "fixing" likelihood calculation
            // std::string training_data_path = training_data_directory + "/" + input_filename_stem + ".json";
            // std::vector<MyInput> training_data = my_data.json_file_to_training_data(training_data_path);
            // TrainedModel model(top, training_data);

            TrainedModel model(top);

            // Write testing results
            ModelEvaluator model_evaluator(model);
            model_evaluator.set_testing_data(testing_data);
            // Write confusion matrix entries
            model_evaluator.evaluate_posterior_distributions();
            model_evaluator.write_testing_stats(results_filepath, training_size, iteration);
        }
    }
}

std::string read_top_n(const std::string& filepath) {
    std::ifstream inFile(filepath);
    if (!inFile.is_open()) {
        std::cerr << "Failed to open file at " << filepath << std::endl;
        return ""; // Return an empty string to indicate failure
    }

    std::stringstream buffer;
    buffer << inFile.rdbuf();
    inFile.close();

    return buffer.str();
}

// Function to create a file, write a header, and return the file path
std::string create_file_with_header(const std::string& directory,
                                 const std::string& filename_stem,
                                 const std::string& header,
                                 const std::string& suffix) {
    // Construct the full file path
    std::string filepath = (fs::path(directory) / (filename_stem + suffix + ".csv")).string();

    // Create and open the file
    std::ofstream file(filepath, std::ios_base::app);

    // Check if the file stream is open
    if (file.is_open()) {
        // Write the header row to the file
        file << header << std::endl;

        // Close the file after writing
        file.close();
        std::cout << "File created successfully at: " << filepath << std::endl;
    } else {
        // If the file couldn't be opened, print an error message
        std::cout << "Error opening file for writing: " << filepath << std::endl;
    }

    // Return the file path for later use
    return filepath;
}

std::pair<int, int> extract_model_label(const std::string& filename_stem) {
    size_t underscorePos = filename_stem.find('_');

    if (underscorePos != std::string::npos) {
        std::string trainingSizeStr = filename_stem.substr(0, underscorePos);
        std::string iterationStr = filename_stem.substr(underscorePos + 1);

        try {
            int trainingSize = std::stoi(trainingSizeStr);
            int iteration = std::stoi(iterationStr);

            return std::make_pair(trainingSize, iteration);
        } catch (const std::invalid_argument& ia) {
            // Handle invalid argument exception if conversion fails
            std::cerr << "Invalid argument: " << ia.what() << '\n';
        } catch (const std::out_of_range& oor) {
            // Handle out of range exception if conversion results are out of int range
            std::cerr << "Out of Range error: " << oor.what() << '\n';
        }
    }

    // Return a default pair if parsing fails or the format is incorrect
    return std::make_pair(-1, -1);
}