#include <cmath>
#include <array>
#include <vector>
#include <set>
#include <unordered_map>
#include <iostream>

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
std::vector<int> data_amounts = {250};
/* std::vector<int> data_amounts = {0, 1, 5, 10, 25, 50, 75, 100, 250, 500, 750, 1000, 2500, 5000}; */
int CLI_num_samps = 0; // By default, number of samples comes from data_amounts vector, not a command line argument

// Hypothesis sampling parameters
double max_temp = 10.0; // maximum temperature for parallel tempering

int main(int argc, char** argv){ 
	
	// default include to process a bunch of global variables: mcts_steps, mcc_steps, etc
	Fleet fleet("Frames of Reference");
        fleet.add_option("--max_temp", max_temp, "Max temperature for parallel tempering");
        fleet.add_option("--p_direct", p_direct, "Probability a generated scene is direct");
        fleet.add_option("--num_samps", CLI_num_samps, "Number of data points generated");
	fleet.initialize(argc, argv);

        //Reset data_amounts if num_samps was given on command line
        if(CLI_num_samps) {
            data_amounts.assign(1, CLI_num_samps);
        }

        generate_scenes(); // Generate scenes from Scenes.h before sampling 

        // Set target concepts before sampling
        std::unordered_map<std::string, std::string> target_formulas = {
            {"above", Concepts::above_abs},
            {"below", Concepts::below_abs},
            {"front", Concepts::front_int_rel},
            /* {"front", Concepts::front_int_rel_not_behind}, */
            {"behind", Concepts::behind_int_rel},
            /* {"behind", Concepts::behind_int_rel_not_front}, */
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

        TopN<MyHypothesis> top;
        for (int num_samples : data_amounts) {

            // Sample data
            MyData mydata(target_formulas);
            mydata.set_intrinsic(intrinsic_formulas);
            mydata.set_relative(relative_formulas);

            SceneProbs scene_probs;
            scene_probs.p_direct = p_direct;
            WordProbs word_probs;
            /* word_probs.p_intrinsic = p_intrinsic; */
            Probabilities probs = {scene_probs, word_probs};

            mydata.sample_data(num_samples, probs);

            // Refer to target hypothesis and sampled data
            auto& target = mydata.target;
            auto& data = mydata.data;

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
            auto h0 = MyHypothesis::sample(mydata.words);
            /* MCMCChain samp(h0, &mydata); */
            //ChainPool samp(h0, &mydata, FleetArgs::nchains);
            ParallelTempering samp(h0, &data, FleetArgs::nchains, max_temp);
            for(auto& h : samp.run(Control()) | printer(FleetArgs::print) | top) {
                    UNUSED(h);
            }

            // Show the best we've found
            top.print(str(num_samples));
        }
}
