#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>
#include <utility> // for std::pair

#include "Scene.h"
#include "MyData.h"
#include "MyGrammarCylindrical.h"
#include "MyHypothesis.h"
#include "MyResults.h"

#include "TopN.h"
#include "Fleet.h"
// #include "Bultins.h"

// Use the std::filesystem namespace for directory operations
namespace fs = std::filesystem;

// Function to read the serialized string from a file
std::string read_top_n(const std::string& filepath);
std::pair<int, int> extract_model_label(const std::string& filename_stem);

std::string testing_data_path = "";
std::string model_directory = "";
std::string output_directory = "";

std::string language_name = "";

int main(int argc, char** argv) {
	Fleet fleet("Frames of Reference");

    // Input and output
    fleet.add_option("--testing_data_path", testing_data_path, "Location of JSON with testing data");
    fleet.add_option("--model_directory", model_directory, "Location of trained models");
    fleet.add_option("--output_directory", output_directory, "Where to save testing results");

    fleet.add_option("--language", language_name, "Name of language used by model");

    fleet.initialize(argc, argv);

    std::unordered_map<std::string, BodyPartMeaning> english = {
        {"above", BodyPartMeaning()},
        {"below", BodyPartMeaning()},
        {"front", BodyPartMeaning()},
        {"behind", BodyPartMeaning()},
        {"left", BodyPartMeaning()},
        {"right", BodyPartMeaning()},
        {"side", BodyPartMeaning()},
        {"near", BodyPartMeaning()},
        {"far", BodyPartMeaning()},
    };

    std::unordered_map<std::string, BodyPartMeaning> english_w_body_parts = {
        {"above", BodyPartMeaning(
                BodyPartNoun::head,
                [](const OrientedObject& a) -> Direction {return a.upward;})},
        {"below", BodyPartMeaning(
                BodyPartNoun::belly,
                [](const OrientedObject& a) -> Direction {return -a.upward;})},
        {"front", BodyPartMeaning(
                BodyPartNoun::face,
                [](const OrientedObject& a) -> Direction {return a.forward;})},
        {"behind", BodyPartMeaning(
                BodyPartNoun::back,
                [](const OrientedObject& a) -> Direction {return -a.forward;})},
        {"right", BodyPartMeaning()},
        {"left", BodyPartMeaning()},
        {"side", BodyPartMeaning(
                BodyPartNoun::flank,
                [](const OrientedObject& a) -> Direction {Direction sideward(a.rightward, true); return sideward;})},
        {"near", BodyPartMeaning()},
        {"far", BodyPartMeaning()},
    };

    std::unordered_map<std::string, BodyPartMeaning> mixtec = {
         {"head", BodyPartMeaning( 
                 BodyPartNoun::head, 
                 [](const OrientedObject& a) -> Direction { 
                 switch(a.body_type) { 
                     case BodyType::biped: 
                         return a.upward; 
                     case BodyType::quadruped: 
                         return {0,0,0}; 
                 } 
                 return a.upward; // Default to biped
                 })}, 
         {"belly", BodyPartMeaning( 
                 BodyPartNoun::belly, 
                 [](const OrientedObject& a) -> Direction {return -a.upward;})}, 
         {"face", BodyPartMeaning( 
                 BodyPartNoun::face, 
                 [](const OrientedObject& a) -> Direction {return a.forward;})}, 
         {"back", BodyPartMeaning( 
                 BodyPartNoun::back, 
                 [](const OrientedObject& a) -> Direction { 
                 switch(a.body_type) { 
                     case BodyType::biped: 
                         return -a.forward; 
                     case BodyType::quadruped: 
                         return a.upward; 
                 } 
                 return -a.forward; // Default to biped
                 })}, 
        {"flank", BodyPartMeaning(
                BodyPartNoun::flank,
                [](const OrientedObject& a) -> Direction {Direction sideward(a.rightward, true); return sideward;})},
        {"right", BodyPartMeaning()},
        {"left", BodyPartMeaning()},
         {"near", BodyPartMeaning()}, 
         {"far", BodyPartMeaning()}
    };

    // Set language
    std::unordered_map<std::string, BodyPartMeaning> language;
    if (language_name == "english"){
        language = english;
    } else if (language_name == "english_w_body_parts"){
        language = english_w_body_parts;
    } else if (language_name == "mixtec") {
        language = mixtec;
    }
    else {
        std::cerr << "Invalid language name provided." << std::endl;
        return 1;
    }

    // Testing data
    MyData my_data(language);
    std::vector<TestingDatum> testing_data = my_data.json_file_to_testing_data(testing_data_path);

    // Create lookup table
    std::string lookup_filename = "lookup_table.csv";
    std::string lookup_filepath = fs::path(output_directory) / lookup_filename;
    std::ofstream lookup_file(lookup_filepath, std::ios_base::app);
    // Check if the file stream is open
    if (lookup_file.is_open()) {
        // Write the header row to the file
        lookup_file << "TrainingSize,Iteration,Prior,Likelihood,Posterior\n";

        // Close the file after writing
        lookup_file.close();
        std::cout << "Lookup table created successfully at: " << lookup_filepath << std::endl;
    } else {
        // If the file couldn't be opened, print an error message
        std::cout << "Error opening file for writing: " << lookup_filepath << std::endl;
    }

    // Iterate over trained models and test each
    for (const auto& entry : fs::directory_iterator(model_directory)) {
        if (entry.path().extension() == ".txt") {
            // Read and deserialize model's top n hypotheses
            std::string serialized_top = read_top_n(entry.path());
            TopN<MyHypothesis> top = TopN<MyHypothesis>::deserialize(serialized_top);

            // TODO: Write model's posterior, prior, and likelihood to JSON

            // Construct csv output file path
            std::string input_filename_stem = entry.path().stem().string();
            std::string output_filename = input_filename_stem + ".csv";
            std::string output_filepath = fs::path(output_directory) / output_filename;

            // Write testing results
            TestingEvaluator testing_evaluator(top, language);
            // Write lookup table entries
            std::pair<int, int> model_label = extract_model_label(input_filename_stem);
            int training_size = model_label.first;
            int iteration = model_label.second;
            testing_evaluator.write_lookup_info(lookup_filepath, training_size, iteration);
            // Write confusion matrix entries
            testing_evaluator.evaluate(testing_data);
            testing_evaluator.write_testing_stats(output_filepath);
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