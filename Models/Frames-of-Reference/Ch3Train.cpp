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

#include "Ch3MyStructures.h"
#include "Ch3MyData.h"

#include "Ch3MyGrammar.h"
#include "Ch3MyHypothesis.h"

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
std::shared_ptr<Language> LanguageContext::current_language = nullptr;

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

    // Set language
    if (language_name == "english") {
        LanguageContext::set_language(std::make_shared<English>());
    } else if (language_name == "mixtec") {
        LanguageContext::set_language(std::make_shared<Mixtec>());
    } else {
        std::cerr << "Invalid language name provided." << std::endl;
        return 1;
    }

    // Training data
    MyData my_data(LanguageContext::get_words());
    std::vector<MyInput> training_data = my_data.json_file_to_training_data(input_path);

    // Inference
    TopN<MyHypothesis> top;

    // Refer to target hypothesis and sampled data
    TopN<MyHypothesis> newtop;
    for(auto h : top.values()) {
        // h.clear_cache();
        h.compute_posterior(training_data);
        newtop << h;
    }
    top = newtop;

    // Inference steps
    auto h0 = MyHypothesis::sample();
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