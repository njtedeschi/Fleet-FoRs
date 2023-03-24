#include <cmath>
#include <array>
#include <vector>
#include <set>
#include <unordered_map>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <chrono>

static const double alpha_t = 0.95; // probability of true description
/* static const size_t MAX_NODES = 10; */

#include "Scene.h"

struct MyInput {
    Scene scene;
    std::string word;
    bool true_description;
};

#include "MyGrammar.h"
#include "MyHypothesis.h"
#include "MyData.h"

#include "Concepts.h"

// target stores mapping from the words to functions that compute them correctly
/* MyHypothesis target; */
/* MyHypothesis intrinsic; */
/* MyHypothesis relative; */

///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// Main code
///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "TopN.h"
#include "ParallelTempering.h"
#include "Fleet.h" 
#include "Random.h"
#include "Builtins.h"

// Data sampling parameters
double p_direct = 0.2; // probability a scene is direct
double p_intrinsic = 0.5; // probability that a description is intrinsic

// Data amounts
int data_min = 0;
int data_max = 250;
int data_step = 10;
std::vector<int> generate_range(int start, int stop, int step) {
    std::vector<int> result;
    for(int i = start; i <= stop; i+= step) {
        result.push_back(i);
    }
    return result;
}
int data_amount = 0; // If non-zero, data_min and data_max both get set equal to it

// Hypothesis sampling parameters
double max_temp = 10.0; // maximum temperature for parallel tempering

// Declare here so MyHypothesis class can reference the specific target instantiated in main
MyHypothesis target;

int main(int argc, char** argv){ 
	
	// default include to process a bunch of global variables: mcts_steps, mcc_steps, etc
	Fleet fleet("Frames of Reference");
        fleet.add_option("--max_temp", max_temp, "Max temperature for parallel tempering");
        fleet.add_option("--p_direct", p_direct, "Probability a generated scene is direct");
        fleet.add_option("--data_min", data_min, "Initial number of data points generated");
        fleet.add_option("--data_max", data_max, "Final number of data points generated");
        fleet.add_option("--data_step", data_step, "Number of data points added in each iteration");
        // Shouldn't be used with data_min, data_max, and data_step flags
        fleet.add_option("--data_amount", data_amount, "Number of data points, specifically for a single iteration");
	fleet.initialize(argc, argv);

        if(data_amount) {
            data_min = data_amount;
            data_max = data_amount;
        }

        generate_scenes(); // Generate scenes from Scenes.h before sampling 

        // Set target concepts before sampling
        std::unordered_map<std::string, std::string> target_formulas = {
            {"above", Concepts::above_abs},
            {"below", Concepts::below_abs},
            {"front", Concepts::front_int_rel},
            /* {"front", Concepts::front_int_rel_not_behind}, */
            /* {"front", Concepts::front_int_rel_no_reflect}, */
            {"behind", Concepts::behind_int_rel},
            /* {"behind", Concepts::behind_int_rel_not_front}, */
            /* {"behind", Concepts::behind_int_rel_no_reflect}, */
            {"side", Concepts::side_int},
            {"left", Concepts::left_int_rel},
            /* {"left", Concepts::left_int_rel_not_right}, */
            {"right", Concepts::right_int_rel}
            /* {"right", Concepts::right_int_rel_not_left} */
        };

        std::unordered_map<std::string, std::string> intrinsic_formulas = {
            {"above", Concepts::above_abs}, // TODO: replace with above_int when non-canonical orientations are introduced
            {"below", Concepts::below_abs},
            {"front", Concepts::front_int},
            {"behind", Concepts::behind_int},
            {"side", Concepts::side_int},
            {"left", Concepts::left_int},
            {"right", Concepts::right_int}
        };

        std::unordered_map<std::string, std::string> relative_formulas = {
            {"above", Concepts::above_abs}, // TODO: replace with above_rel when non-canonical orientations are introduced
            {"below", Concepts::below_abs},
            {"front", Concepts::front_rel},
            {"behind", Concepts::behind_rel},
            {"side", Concepts::side_rel},
            {"left", Concepts::left_rel},
            {"right", Concepts::right_rel}
        };

        // Initialize sampler
        MyData data_sampler(target_formulas);
        data_sampler.set_intrinsic(intrinsic_formulas);
        data_sampler.set_relative(relative_formulas);

        target = data_sampler.target;

        // Initialize amount of data to sample for each iteration
        std::vector<int> data_amounts = generate_range(data_min, data_max, data_step);

        // Output file set up
        // Save original buffer of std::cout
        std::streambuf *originalCoutBuffer = std::cout.rdbuf();

        // Create output file stream using date and time
        auto now = std::chrono::system_clock::now();
        std::time_t time = std::chrono::system_clock::to_time_t(now);
        std::stringstream date_time_ss;
        date_time_ss << "results/" << std::put_time(std::localtime(&time), "%Y-%m-%d_%H-%M-%S") << ".txt";
        std::string file_name = date_time_ss.str();
        std::ofstream outFile(file_name);

        // Redirect std::cout to file
        std::cout.rdbuf(outFile.rdbuf());

        // Inference
        TopN<MyHypothesis> top;
        for (int num_samples : data_amounts) {
            // Sample data
            SceneProbs scene_probs;
            scene_probs.p_direct = p_direct;
            WordProbs word_probs;
            /* word_probs.p_intrinsic = p_intrinsic; */
            Probabilities probs = {scene_probs, word_probs};

            std::vector<MyInput> data = data_sampler.sample_data(num_samples, probs);

            // Refer to target hypothesis and sampled data
            TopN<MyHypothesis> newtop;
            for(auto h : top.values()) {
                h.clear_cache();
                h.compute_posterior(data);
                newtop << h;
            }
            top = newtop;

            target.clear_cache();
            target.compute_posterior(data);

            // Inference steps
            auto h0 = MyHypothesis::sample(data_sampler.words);
            /* MCMCChain samp(h0, &mydata); */
            //ChainPool samp(h0, &mydata, FleetArgs::nchains);
            ParallelTempering samp(h0, &data, FleetArgs::nchains, max_temp);
            for(auto& h : samp.run(Control()) | printer(FleetArgs::print) | top) {
                    UNUSED(h);
            }

            // Show the best we've found
            top.print(str(num_samples));
        }

        // Restore original buffer of std::cout
        std::cout.rdbuf(originalCoutBuffer);
        // Close output file stream
        outFile.close();
}
