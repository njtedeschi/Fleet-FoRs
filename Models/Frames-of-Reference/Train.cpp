#include <cmath>
#include <algorithm>
#include <array>
#include <vector>
#include <set>
#include <unordered_map>
#include <functional>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <chrono>

static const double alpha_t = 0.9; // model's reliability parameter
/* static const size_t MAX_NODES = 10; */

#include "Scene.h"
#include "MyData.h"

#include "MyGrammar.h"
#include "MyHypothesis.h"

///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// Main code
///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "TopN.h"
#include "ParallelTempering.h"
#include "Fleet.h"
#include "Random.h"
#include "Builtins.h"

// Hypothesis sampling parameters
double max_temp = 10.0; // maximum temperature for parallel tempering

std::string input_path = "";
std::string output_path = "";
void save_top_n(const std::string& filepath, const std::string& top_n) {
    std::ofstream outFile(filepath);
    if (outFile.is_open()) {
        outFile << top_n;
        outFile.close();
        std::cout << "File saved successfully to " << filepath << std::endl;
    }
    else {
        std::cerr << "Failed to open file at " << filepath << std::endl;
    }
}

// Language to sample training data from
std::string language_name = "";

int main(int argc, char** argv){

	// default include to process a bunch of global variables: mcts_steps, mcc_steps, etc
	Fleet fleet("Frames of Reference");

    // Input and output
    fleet.add_option("--input_path", input_path, "Location of JSON with trianing data");
    fleet.add_option("--output_path", output_path, "Where to save training results");

    // Inference settings
    fleet.add_option("--max_temp", max_temp, "Max temperature for parallel tempering");

    // Model hyperparameters
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

    // Training data
    MyData my_data(language);
    std::vector<MyInput> training_data = my_data.json_file_to_training_data(input_path);

    // Inference
    TopN<MyHypothesis> top;

    // Refer to target hypothesis and sampled data
    TopN<MyHypothesis> newtop;
    for(auto h : top.values()) {
        h.clear_cache();
        h.compute_posterior(training_data);
        newtop << h;
    }
    top = newtop;

    // Inference steps
    auto h0 = MyHypothesis::sample(my_data.words);
    /* MCMCChain samp(h0, &mydata); */
    //ChainPool samp(h0, &mydata, FleetArgs::nchains);
    ParallelTempering samp(h0, &training_data, FleetArgs::nchains, max_temp);
    for(auto& h : samp.run(Control()) | printer(FleetArgs::print) | top) {
            UNUSED(h);
    }

    top.print();
    std::string serialized_top = top.serialize();
    if (!output_path.empty()) {
        save_top_n(output_path, serialized_top);
    }
}