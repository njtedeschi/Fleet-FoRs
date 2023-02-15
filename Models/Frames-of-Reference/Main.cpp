#include <cmath>
#include <array>
#include <vector>
#include <random>

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

///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// Define the grammar
/// Thid requires the types of the thing we will add to the grammar (bool,MyObject)
///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "Grammar.h"
#include "Singleton.h"

class MyGrammar : public Grammar<Scene,bool,   Scene,bool,Object,Vector, double>,
				  public Singleton<MyGrammar> {
public:
	MyGrammar() {
                add("displacement(%s,%s)", +[](Object x, Object y) -> Vector {
                        return y.location - x.location;
                        });
		add("orientation(%s)", +[](Object x) -> Vector {return x.orientation;});
                add("parallel(%s,%s)", +[](Vector x, Vector y) -> bool {
                        if (magnitude(x) == 0 || magnitude(y) == 0) {return false;}
                        return cosine_similarity(x,y) == 1;
                        });
                add("antiparallel(%s,%s)", +[](Vector x, Vector y) -> bool {
                        if (magnitude(x) == 0 || magnitude(y) == 0) {return false;}
                        return cosine_similarity(x,y) == -1;
                        });
                add("orthogonal(%s,%s)", +[](Vector x, Vector y) -> bool {
                        if (magnitude(x) == 0 || magnitude(y) == 0) {return false;}
                        return cosine_similarity(x,y) == 0;
                        });
		
                add("and(%s,%s)",    Builtins::And<MyGrammar>);
		add("or(%s,%s)",     Builtins::Or<MyGrammar>);
		add("not(%s)",       Builtins::Not<MyGrammar>);

		add("speaker(%s)",       +[](Scene x) -> Object { return x.speaker; });
		add("figure(%s)",        +[](Scene x) -> Object { return x.figure; });
		add("ground(%s)",        +[](Scene x) -> Object { return x.ground; });
		add("x",             Builtins::X<MyGrammar>);
	}
} grammar;

///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// Define a class for handling my specific hypotheses and data. Everything is defaultly 
/// a PCFG prior and regeneration proposals, but I have to define a likelihood
///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "DeterministicLOTHypothesis.h"

class MyHypothesis final : public DeterministicLOTHypothesis<MyHypothesis,Scene,bool,MyGrammar,&grammar> {
public:
	using Super = DeterministicLOTHypothesis<MyHypothesis,Scene,bool,MyGrammar,&grammar>;
	using Super::Super; // inherit the constructors
	

	double compute_single_likelihood(const datum_t& di) override {
		const Scene& s = di.input;
		bool   true_out = di.output; 
		bool my_output = this->call(s);
		
		double likelihood = (1.0-di.reliability)*0.5 + di.reliability*(my_output == true_out);
                return log(likelihood);
		
	}
};

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
        int num_samples = 100
        std::mt19937 engine(0); // RNG with set seed
        p_direct = 0.2; // probability a scene is direct
        std::bernoulli_distribution direct_dist(p_direct);
        p_wrong = 0.05; // probability a scene is labeled incorrectly
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
                bool behind_nondirect(scene);
            }
            // Add noise
            if (wrong_dist(p_wrong)) {
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
