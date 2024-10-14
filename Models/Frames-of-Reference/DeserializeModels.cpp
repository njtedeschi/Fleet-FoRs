#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>
#include <utility> // for std::pair

static const double alpha_t = 0.9; // model's reliability parameter

#include "Scene.h"
#include "MyData.h"
#include "MyGrammar.h"
#include "MyHypothesis.h"

#include "TopN.h"
#include "Fleet.h"
// #include "Bultins.h"

// Use the std::filesystem namespace for directory operations
namespace fs = std::filesystem;

// Function to read the serialized string from a file
std::string read_top_n(const std::string& filepath);
std::pair<int, int> extract_model_label(const std::string& filename_stem);

std::string model_directory = "";
int n_top = 1;
int n_train = 1000;
// std::string output_directory = "";
// std::string output_filename_stem = "";

std::string language_name = "";

int main(int argc, char** argv) {
	Fleet fleet("Frames of Reference");

    // Input and output
    fleet.add_option("--model_directory", model_directory, "Location of trained models");
    fleet.add_option("--n_top", model_directory, "Number of top hypotheses to show per model");
    fleet.add_option("--n_train", model_directory, "Minimum training size to start displaying results");
    // fleet.add_option("--output_directory", output_directory, "Where to save testing results");
    // fleet.add_option("--output_filename_stem", output_filename_stem, "Stem to use for results and lookup table files");

    fleet.initialize(argc, argv);

    // Iterate over trained models and test each
    for (const auto& entry : fs::directory_iterator(model_directory)) {
        if (entry.path().extension() == ".txt") {
            std::string input_filename_stem = entry.path().stem().string();
            std::pair<int, int> model_label = extract_model_label(input_filename_stem);
            int training_size = model_label.first;
            int iteration = model_label.second;
            if (training_size >= n_train) {
                std::cout << training_size << "-" << iteration << std::endl;
                // Read and deserialize model's top n hypotheses
                std::string serialized_top = read_top_n(entry.path());
                TopN<MyHypothesis> top = TopN<MyHypothesis>::deserialize(serialized_top);
                int i = 0;
                for(auto& h : top.sorted(false)) {
                    h.show();
                    i++;
                    if (i == n_top){
                        break;
                    }
                }
            }
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