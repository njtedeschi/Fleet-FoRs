#include <cmath>
#include <array>
#include <vector>
#include <set>
#include <unordered_map>
#include <iostream>

static const double alpha_t = 0.95; // probability of true description
/* static const size_t MAX_NODES = 10; */

std::vector<int> data_amounts = {250};
int CLI_num_samps = 0; // By default, number of samples comes from data_amounts vector, not a command line argument

#include "Scene.h"

struct MyInput {
    Scene scene;
    std::string word;
};

#include "MyGrammar.h"
#include "MyHypothesis.h"
#include "MyData.h"

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

double p_direct = 0.2; // probability a scene is direct
double p_intrinsic = 0.5; // probability that a description is intrinsic

int main(int argc, char** argv){ 
	
	// default include to process a bunch of global variables: mcts_steps, mcc_steps, etc
	Fleet fleet("Frames of Reference");
        fleet.add_option("--p_direct", p_direct, "Probability a generated scene is direct");
        fleet.add_option("--num_samps", CLI_num_samps, "Number of data points generated");
	fleet.initialize(argc, argv);

        //Reset data_amounts if num_samps was given on command line
        if(CLI_num_samps) {
            data_amounts.assign(1, CLI_num_samps);
        }

        generate_scenes(); // Generate scenes from Scenes.h before sampling 

        // Above
        std::string above_abs = "parallel(displacement(ground(x),figure(x)),up(x))";
        // Below
        std::string below_abs = "parallel(displacement(figure(x),ground(x)),up(x))";
        // Front
        std::string front_int = "parallel(displacement(ground(x),figure(x)),orientation(ground(x)))";
        std::string front_int_rel = "or(parallel(displacement(ground(x),figure(x)),orientation(ground(x))),parallel(displacement(figure(x),ground(x)),orientation(speaker(x))))";
            std::string front_int_rel_not_behind = "or(parallel(displacement(ground(x),figure(x)),orientation(ground(x))),and(parallel(displacement(figure(x),ground(x)),orientation(speaker(x))),not(parallel(orientation(ground(x)),orientation(speaker(x))))))"; // No relative front when intrinsic behind
        // Behind
        std::string behind_int = "parallel(displacement(figure(x),ground(x)),orientation(ground(x)))";
        std::string behind_int_rel = "or(parallel(displacement(figure(x),ground(x)),orientation(ground(x))),parallel(displacement(ground(x),figure(x)),orientation(speaker(x))))";
            std::string behind_int_rel_not_front = "or(parallel(displacement(figure(x),ground(x)),orientation(ground(x))),and(parallel(displacement(ground(x),figure(x)),orientation(speaker(x))),not(parallel(orientation(ground(x)),orientation(speaker(x))))))"; // No relative behind when intrinsic front
        // Side
        std::string side_int = "orthogonal(displacement(ground(x),figure(x)),orientation(ground(x)))";
        std::string side_int_rel = "or(orthogonal(displacement(ground(x),figure(x)),orientation(ground(x))),orthogonal(displacement(ground(x),figure(x)),orientation(speaker(x))))";

        // Set target concepts before sampling
        std::unordered_map<std::string, std::string> formulas = {
            {"above", above_abs},
            {"below", below_abs},
            {"front", front_int_rel},
            {"behind", behind_int_rel},
            {"side", side_int}
        };

        TopN<MyHypothesis> top;
        for (int num_samples : data_amounts) {

            // Sample data
            MyData mydata(formulas);
            mydata.sample_data(num_samples, p_direct);

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
            ParallelTempering samp(h0, &data, FleetArgs::nchains, 10.0); 
            for(auto& h : samp.run(Control()) | printer(FleetArgs::print) | top) {
                    UNUSED(h);
            }

            // Show the best we've found
            top.print(str(num_samples));
        }
}
