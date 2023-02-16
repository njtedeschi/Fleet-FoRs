#include <cmath>
#include <array>
#include <vector>
#include <random>

const std::vector<std::string> words = {"above", "below", "front", "behind", "side"};

static const double alpha_t = 0.95; // probability of true description

#include "Scene.h"

struct MyInput {
    Scene scene;
    std::string word;
};

// Calculate truth value
bool behind_direct(const Scene& scene){
    Object speaker = scene.speaker;
    Object figure = scene.figure;

    Vector s_to_f = figure.location - speaker.location;
    return cosine_similarity(s_to_f, speaker.orientation) == -1;
}

bool behind_nondirect(const Scene& scene){
    Object speaker = scene.speaker;
    Object figure = scene.figure;
    Object ground = scene.ground;

    Vector g_to_f = figure.location - ground.location;
    bool intrinsic = (cosine_similarity(g_to_f, ground.orientation) == -1);
    bool relative = (cosine_similarity(g_to_f, speaker.orientation) == 1);
    return (intrinsic || relative);
}

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
std::bernoulli_distribution wrong_dist(1-alpha_t);

// Uniform dists over scenes (direct_scenes and nondirect_scenes defined in "Scene.h")
std::uniform_int_distribution<int> direct_scene_dist(0, direct_scenes.size() - 1);
std::uniform_int_distribution<int> nondirect_scene_dist(0, nondirect_scenes.size() - 1);

MyHypothesis::datum_t sample_datum() {
    // Sample scene
    Scene scene;
    if(direct_dist(engine)) {

    } else {

    }
    std::string word;
    // Sample word
    return MyInput{.scene=scene, .word=word}
}

int main(int argc, char** argv){ 
	
	// default include to process a bunch of global variables: mcts_steps, mcc_steps, etc
	Fleet fleet("Frames of Reference");
	fleet.initialize(argc, argv);

        // Set target concepts for words. TODO: add formulas
        target["above"] = InnerHypothesis(grammar.simple_parse(""));
        target["below"] = InnerHypothesis(grammar.simple_parse(""));
        target["front"] = InnerHypothesis(grammar.simple_parse(""));
        target["behind"] = InnerHypothesis(grammar.simple_parse(""));
        target["side"] = InnerHypothesis(grammar.simple_parse(""));
	
	//------------------
	// set up the data
	//------------------
	// mydata stores the data for the inference model
	MyHypothesis::data_t mydata;

        // Sample
        int num_samples = 100;

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
