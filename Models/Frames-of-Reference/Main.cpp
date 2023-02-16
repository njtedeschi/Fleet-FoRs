#include <cmath>
#include <array>
#include <vector>
#include <random>

const std::vector<std::string> words = {"above", "below", "front", "behind", "side"};

static const double alpha_t = 0.95; // probability of true description

using Vector = std::array<double,3>;

Vector operator*(double s, const Vector& v) {
    Vector result;
    for (int i = 0; i < 3; ++i) {
        result[i] = v[i] * s;
    }
    return result;
}

Vector operator-(const Vector& v1, const Vector& v2) {
    Vector result;
    for (int i = 0; i < 3; ++i) {
        result[i] = v1[i] - v2[i];
    }
    return result;
}

double dot_product(const Vector &a, const Vector &b){
    double result = 0.0;
    for(int i = 0; i < a.size(); i++){
        result += a[i] * b[i];
    }
    return result;
}

double magnitude(const Vector &a){
    double result = 0.0;
    for (int i = 0; i < a.size(); i++){
        result += a[i] * a[i];
    }
    return sqrt(result);
}

double cosine_similarity(const Vector &a, const Vector &b){
    return dot_product(a, b)/(magnitude(a) * magnitude(b));
}

double angle_between(){
    // TODO: implement
}

struct Object {
	Vector location;
	Vector orientation;
	/* std::string name; */ 
};

struct Scene {
	Object speaker;
	Object ground;
	Object figure; 
};

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

///~~~~~ TODO: uncomment when refactoring
/* #include "MyGrammar.h" */
/* #include "MyHypothesis.h" */

/* // target stores a mapping from strings in w to functions that compute them correctly */
/* MyHypothesis target; */
///~~~~~

///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// Main code
///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "TopN.h"
#include "ParallelTempering.h"
#include "Fleet.h" 
#include "Builtins.h"

int main(int argc, char** argv){ 
	
	// default include to process a bunch of global variables: mcts_steps, mcc_steps, etc
	Fleet fleet("Rational rules");
	fleet.initialize(argc, argv);
	
	//------------------
	// set up the data
	//------------------
	// mydata stores the data for the inference model
	MyHypothesis::data_t mydata;

	Vector origin = {0,0,0};
        Vector east = {1,0,0};
        Vector west = {-1,0.0};
        Vector north = {0,1,0};
        Vector south = {0,-1,0};
        Vector up = {0,0,1};
        Vector down = {0,0,-1};

        // Possible speakers
        Object direct_speaker = {origin, east};
        Object nondirect_speaker = {2 * west, east};

        // Possible figures
        Object east_figure = {east, origin};
        Object west_figure = {west, origin};
        Object north_figure = {north, origin};
        Object south_figure = {south, origin};
        Object up_figure = {up, origin};
        Object down_figure = {down, origin};
        std::vector<Object> figures = {east_figure, west_figure, north_figure, south_figure, up_figure, down_figure};

        // Possible grounds
        Object east_facing_ground = {origin, east};
        Object west_facing_ground = {origin, west};
        Object north_facing_ground = {origin, north};
        Object south_facing_ground = {origin, south};
        std::vector<Object> grounds = {east_facing_ground, west_facing_ground, north_facing_ground, south_facing_ground};

        // Possible direct and nondirect scenes
        std::vector<Scene> direct_scenes;
        std::vector<Scene> nondirect_scenes;
        for (const auto& figure : figures) {
            Scene direct = {direct_speaker, direct_speaker, figure};
            direct_scenes.push_back(direct);
            for (const auto& ground : grounds) {
                Scene nondirect = {nondirect_speaker, ground, figure};
                nondirect_scenes.push_back(nondirect);
            }
        }

        // Sample
        int num_samples = 100;
        std::mt19937 engine(0); // RNG with set seed
        double p_direct = 0.2; // probability a scene is direct
        std::bernoulli_distribution direct_dist(p_direct);
        double p_wrong = 0.05; // probability a scene is labeled incorrectly
        std::bernoulli_distribution wrong_dist(p_wrong);

        // Uniform dists over scenes
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
