#include <cmath>
#include <array>
#include <vector>
#include <set>
#include <iostream>

static const double alpha_t = 0.95; // probability of true description
/* static const size_t MAX_NODES = 10; */

const std::vector<size_t> data_amounts = {50, 100, 250, 750};

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
	fleet.initialize(argc, argv);

        generate_scenes(); // Generate scenes from Scenes.h before sampling 

        TopN<MyHypothesis> top;
        for (size_t num_samples : data_amounts) {

            // Sample data
            MyData mydata;
            mydata.sample_data(num_samples, p_direct);

            // Refer to target hypothesis and sampled data
            auto& target = mydata.target;
            auto& data = mydata.data;

            TopN<MyHypothesis> newtop;
            for(auto h : top.values()) {
                h.clear_cache();
                h.compute_posterior(data);
                /* h.compute_posterior(mydata.data); */
                newtop << h;
            }
            top = newtop;

            target.clear_cache();
            target.compute_posterior(data);
            /* mydata.target.clear_cache(); */
            /* mydata.target.compute_posterior(mydata.data); */

            // Inference steps
            auto h0 = MyHypothesis::sample(words);
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
