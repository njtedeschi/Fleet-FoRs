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

MyHypothesis::datum_t sample_datum() {

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
        std::mt19937 engine(0); // RNG with set seed
        double p_direct = 0.2; // probability a scene is direct
        std::bernoulli_distribution direct_dist(p_direct);
        double p_wrong = 0.05; // probability a scene is labeled incorrectly
        std::bernoulli_distribution wrong_dist(p_wrong);

        // Uniform dists over scenes (direct_scenes and nondirect_scenes defined in "Scene.h")
        std::uniform_int_distribution<int> direct_scene_dist(0, direct_scenes.size() - 1);
        std::uniform_int_distribution<int> nondirect_scene_dist(0, nondirect_scenes.size() - 1);

        for (int i = 0; i < num_samples; i++){
            // Sample scene
            Scene scene; bool label;
            if (direct_dist(engine)) {
                scene = direct_scenes[direct_scene_dist(engine)];
                label = behind_direct(scene);
            } else {
                scene = nondirect_scenes[nondirect_scene_dist(engine)];
                label = behind_nondirect(scene);
            }
            // Add noise
            if (wrong_dist(engine)) {
                    label = !label;
                    }
            mydata.emplace_back(scene, label, 1-p_wrong);
        }


        /* Scene direct_behind = {direct_speaker, direct_speaker, west_figure}; */
        /* Scene direct_front = {direct_speaker, direct_speaker, east_figure}; */
        /* Scene direct_left = {direct_speaker, direct_speaker, north_figure}; */
        /* Scene direct_right = {direct_speaker, direct_speaker, south_figure}; */
        /* Scene direct_above = {direct_speaker, direct_speaker, up_figure}; */
        /* Scene direct_below = {direct_speaker, direct_speaker, down_figure}; */

        /* Scene front_0_int = {nondirect_speaker, east_facing_ground, east_figure}; */
        /* Scene behind_0_int = {nondirect_speaker, east_facing_ground, west_figure}; */
        /* Scene front_90_int = {nondirect_speaker, north_facing_ground, north_figure}; */
        /* Scene front_90_rel ={nondirect_speaker, north_facing_ground, west_figure}; */
        /* Scene behind_90_int = {nondirect_speaker, north_facing_ground, south_figure}; */
        /* Scene behind_90_rel = {nondirect_speaker, north_facing_ground, east_figure}; */
        /* Scene front_180_int = {nondirect_speaker, west_facing_ground, west_figure}; */
        /* Scene behind_180_int = {nondirect_speaker, west_facing_ground, east_figure}; */

        /* // Learning 'behind' */
        /* mydata.emplace_back(direct_behind, true, 0.95); */
        /* mydata.emplace_back(direct_front, false, 0.95); */
        /* mydata.emplace_back(direct_left, false, 0.95); */
        /* mydata.emplace_back(direct_above, false, 0.95); */
        /* mydata.emplace_back(direct_below, false, 0.95); */
        /* mydata.emplace_back(front_0_int, true, 0.95); */
        /* mydata.emplace_back(behind_0_int, true, 0.95); */
        /* mydata.emplace_back(front_90_int, false, 0.95); */
        /* mydata.emplace_back(front_90_rel, false, 0.95); */
        /* mydata.emplace_back(behind_90_int, true, 0.95); */
        /* mydata.emplace_back(behind_90_rel, true, 0.95); */
        /* mydata.emplace_back(front_180_int, false, 0.95); */
        /* mydata.emplace_back(behind_180_int, true, 0.95); */


	/* for(int i=0;i<10;i++) { */
		
	/* 	if(flip(0.8)) { */
	/* 		mydata.emplace_back(....) */
	/* 	} */
	/* 	else { */
			
			
	/* 	} */
		
	/* } */
	//------------------
	// Run
	//------------------
	
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
