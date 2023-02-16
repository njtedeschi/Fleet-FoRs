#include <cmath>
#include <array>
#include <vector>
#include <random>
#include <set>

const std::vector<std::string> words = {"above", "below", "front", "behind", "side"};

static const double alpha_t = 0.95; // probability of true description

#include "Scene.h"

struct MyInput {
    Scene scene;
    std::string word;
};

#include "MyGrammar.h"
#include "MyHypothesis.h"

// target stores mapping from the words to functions that compute them correctly
MyHypothesis target;

///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// Main code
///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "TopN.h"
#include "ParallelTempering.h"
#include "Fleet.h" 
#include "Random.h"
#include "Builtins.h"

std::mt19937 engine(0); // RNG with set seed
double p_direct = 0.2; // probability a scene is direct
std::bernoulli_distribution direct_dist(p_direct);
std::bernoulli_distribution correct_dist(alpha_t);

// Uniform dists over scenes (direct_scenes and nondirect_scenes defined in "Scene.h")
std::uniform_int_distribution<int> direct_scene_dist(0, direct_scenes.size() - 1);
std::uniform_int_distribution<int> nondirect_scene_dist(0, nondirect_scenes.size() - 1);

MyHypothesis::datum_t sample_datum() {
    // Sample scene
    Scene scene;
    // Handle direct and nondirect scenes separately
    if(direct_dist(engine)) {
        scene = direct_scenes[direct_scene_dist(engine)];
    } else {
        scene = nondirect_scenes[nondirect_scene_dist(engine)];
    }

    // Sample word
    std::string word;
    // First evaluate truth values for all words
    std::set<std::string> true_words;
    for(auto& w : words) {
        MyInput input{.scene=scene, .word=EMPTY_STRING};
        auto output = target.at(w).call(input);
        if (output == true){
            true_words.insert(w);
        }
    }
    // Then sample accordingly
    // TODO: figure out what's going on with normalizer argument of sample
    if(correct_dist(engine)) {
        // Sample from true descriptions
        word = *sample<std::string, decltype(true_words)>(true_words).first;
    } else {
        // Sample randomly
        word = *sample<std::string, decltype(true_words)>(words).first;
    }

    return MyInput{.scene=scene, .word=word};
}

int main(int argc, char** argv){ 
	
	// default include to process a bunch of global variables: mcts_steps, mcc_steps, etc
	Fleet fleet("Frames of Reference");
	fleet.initialize(argc, argv);

        // Set target concepts for words. TODO: add formulas
        target["above"] = InnerHypothesis(grammar.simple_parse("parallel(displacement(ground(x),figure(x)),up(x))"));
        target["below"] = InnerHypothesis(grammar.simple_parse("parallel(displacement(figure(x),ground(x)),up(x))"));
        target["front"] = InnerHypothesis(grammar.simple_parse("or(parallel(displacement(ground(x),figure(x)),orientation(ground(x))),parallel(displacement(figure(x),ground(x)),orientation(speaker(x))))"));
        target["behind"] = InnerHypothesis(grammar.simple_parse("or(parallel(displacement(figure(x),ground(x)),orientation(ground(x))),parallel(displacement(ground(x),figure(x)),orientation(speaker(x))))"));
        target["side"] = InnerHypothesis(grammar.simple_parse("or(orthogonal(displacement(ground(x),figure(x)),orientation(ground(x))),orthogonal(displacement(ground(x),figure(x)),orientation(speaker(x))))"));
	
	//------------------
	// set up the data
	//------------------
	// mydata stores the data for the inference model
	MyHypothesis::data_t mydata;

        // Sample
        int num_samples = 100;

        generate_scenes(); // Generate scenes from Scenes.h before sampling
        for (int i = 0; i < num_samples; i++){
            MyInput input = sample_datum();
            mydata.emplace_back(input, true, 1-alpha_t);
        }

	TopN<MyHypothesis> top;

	auto h0 = MyHypothesis::sample();
	
	MCMCChain samp(h0, &mydata);
	//ChainPool samp(h0, &mydata, FleetArgs::nchains);
	//	ParallelTempering samp(h0, &mydata, FleetArgs::nchains, 10.0); 
	for(auto& h : samp.run(Control()) | printer(FleetArgs::print) | top) {
		UNUSED(h);
	}

	// Show the best we've found
	top.print();
}
