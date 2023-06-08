#include <cmath>
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

static const double alpha_t = 0.95; // probability of true description
/* static const size_t MAX_NODES = 10; */

#include "Scene.h"

struct WordMeaning {
    std::string target_formula;
    std::function<Direction(OrientedObject&)> body_part;

    // No argument constructor
    WordMeaning() : target_formula(""), body_part([](const OrientedObject& a) -> Direction {return {0,0,0};}) {}

    // Default body part direction is the zero vector
    WordMeaning(std::string tf) : target_formula(tf), body_part([](const OrientedObject& a) -> Direction {return {0,0,0};}) {}

    WordMeaning(std::string tf, std::function<Direction(const OrientedObject&)> bp) :
        target_formula(tf), body_part(bp) {}
};

// TODO: Possibly switch body_part_meaning to using a pointer
struct MyInput {
    Scene scene;
    std::string word;
    std::function<Direction(OrientedObject&)> body_part_meaning;
    bool true_description;
};

/* #include "MyGrammar.h" */
#include "MyGrammarCylindrical.h"
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
double p_listener_ground = 0.2; // probability that the ground of a nondirect scene is the listener

double p_frame = 0.9; // probability that a description uses an FoR if one applies
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
std::vector<int> data_amounts;

// Hypothesis sampling parameters
double max_temp = 10.0; // maximum temperature for parallel tempering

// Declare here so MyHypothesis class can reference the specific target instantiated in main
MyHypothesis target;

int main(int argc, char** argv){ 
	
	// default include to process a bunch of global variables: mcts_steps, mcc_steps, etc
	Fleet fleet("Frames of Reference");
        fleet.add_option("--max_temp", max_temp, "Max temperature for parallel tempering");

        // p_axis
        // p_near
        fleet.add_option("--p_direct", p_direct, "Probability a generated scene is direct");

        // p_frame
        fleet.add_option("--p_intrinsic", p_intrinsic, "Probability a description uses an intrinsic FoR");

        fleet.add_option("--data_min", data_min, "Initial number of data points generated");
        fleet.add_option("--data_max", data_max, "Final number of data points generated");
        fleet.add_option("--data_step", data_step, "Number of data points added in each iteration");
        // Shouldn't be used with data_min, data_max, and data_step flags
        fleet.add_option("--data_amounts", data_amounts, "Space separated list of numbers of data points to iterate over");
	fleet.initialize(argc, argv);

        generate_scenes(); // Generate scenes from Scenes.h before sampling 

        // Set target concepts before sampling
        /* std::unordered_map<std::string, std::string> target_formulas = { */
        /*     {"above", "exists(as(f=frame(G),cyl(r=0(x),tTRUE,upward(x))),pf(x))"}, */
        /*     {"below", "exists(as(f=frame(G),cyl(r=0(x),tTRUE,downward(x))),pf(x))"}, */
        /*     {"front", "exists(as(or(f=frame(G),f=frame'(S,TR)),cyl(r>0(x),forward(x),z=0(x))),pf(x))"}, */
        /*     {"behind", "exists(as(or(f=frame(G),f=frame'(S,TR)),cyl(r>0(x),backward(x),z=0(x))),pf(x))"}, */
        /*     {"side", "exists(as(f=frame(G),cyl(r>0(x),sideward(x),z=0(x))),pf(x))"}, */
        /*     {"left", "exists(as(or(f=frame(G),f=frame'(S,TR)),cyl(r>0(x),leftward(x),z=0(x))),pf(x))"}, */
        /*     {"right", "exists(as(or(f=frame(G),f=frame'(S,TR)),cyl(r>0(x),rightward(x),z=0(x))),pf(x))"}, */
        /*     {"near", "exists(as(f=frame(G),cyl(r-near(x),tTRUE,zTRUE)),pf(x))"}, */
        /*     {"far", "exists(as(f=frame(G),cyl(r-far(x),tTRUE,zTRUE)),pf(x))"}, */
        /*     /1* {"in", "exists(as(f=frame(G),cyl(r=0(x),tTRUE,z=0(x))),pf(x))"}, *1/ */
        /* }; */
        std::unordered_map<std::string, WordMeaning> target_formulas = {
            {"above", WordMeaning("parallel(x,UP)")},
            {"below", WordMeaning("parallel(x,DOWN)")},
            {"front", WordMeaning("exists(cs(or(f=frame(G),f=frame'(S,TR)),forward-f(x)),pf(x))")},
            {"behind", WordMeaning("exists(cs(or(f=frame(G),f=frame'(S,TR)),backward-f(x)),pf(x))")},
            {"left", WordMeaning("exists(cs(or(f=frame(G),f=frame'(S,TR)),leftward-f(x)),pf(x))")},
            {"right", WordMeaning("exists(cs(or(f=frame(G),f=frame'(S,TR)),rightward-f(x)),pf(x))")},
            {"near", WordMeaning("near(x)")},
            {"far", WordMeaning("far(x)")},
        };

        // Initialize sampler
        MyData data_sampler(target_formulas);
        /* data_sampler.set_intrinsic(intrinsic_formulas); */
        /* data_sampler.set_relative(relative_formulas); */

        target = data_sampler.target;

        // Initialize amount of data to sample for each iteration
        /* std::vector<int> data_amounts = generate_range(data_min, data_max, data_step); */

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
            /* scene_probs.p_listener_ground = p_listener_ground; */
            scene_probs.p_near = 0.5;
            scene_probs.p_axis = 0.8;
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
