#include <cmath>
#include <algorithm>
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

enum class BodyPartNoun {
    none = 0,
    head = 1,
    belly = 2,
    face = 3,
    back = 4,
    right_side = 5,
    left_side = 6,
    side = 7,
    tail = 8
};

struct WordMeaning {
    std::string target_formula;
    BodyPartNoun body_part_noun;
    std::function<Direction(OrientedObject&)> body_part_direction;

    // No argument constructor
    WordMeaning() : target_formula(""), body_part_noun(BodyPartNoun::none), body_part_direction([](const OrientedObject& a) -> Direction {return {0,0,0};}) {}

    // Default body part direction is the zero vector
    WordMeaning(std::string tf) : target_formula(tf), body_part_noun(BodyPartNoun::none), body_part_direction([](const OrientedObject& a) -> Direction {return {0,0,0};}) {}

    WordMeaning(std::string tf, BodyPartNoun bpn, std::function<Direction(const OrientedObject&)> bpd) :
        target_formula(tf), body_part_noun(bpn), body_part_direction(bpd) {}
};

struct MyInput {
    Scene scene;
    std::string word;
    WordMeaning* meaning;
    bool true_description;
};

/* #include "MyGrammar.h" */
#include "MyGrammarCylindrical.h"
#include "MyHypothesis.h"
#include "MyData.h"

#include "Concepts.h"

///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// Main code
///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "TopN.h"
#include "ParallelTempering.h"
#include "Fleet.h" 
#include "Random.h"
#include "Builtins.h"


#include "MyResults.h"

// Printing parameters
bool precision_recall = true;
std::string output_dir = "results/";

// Data sampling parameters
double p_direct = 0.2; // probability a scene is direct
double p_listener_ground = 0.2; // probability that the ground of a nondirect scene is the listener

double p_frame = 0.0; // probability that a description uses an FoR if one applies
double p_intrinsic = 0.0; // probability that a description is intrinsic

int train_size = 10;
int repetitions = 1;
int test_size = 256;

// Hypothesis sampling parameters
double max_temp = 10.0; // maximum temperature for parallel tempering


// Language to sample training data from
std::string language_name = "english";

// Declare here so MyHypothesis class can reference the specific target instantiated in main
MyHypothesis target;

int main(int argc, char** argv){ 
	
	// default include to process a bunch of global variables: mcts_steps, mcc_steps, etc
	Fleet fleet("Frames of Reference");

    fleet.add_option("--output_dir", output_dir, "Directory to save output files in");

    fleet.add_option("--max_temp", max_temp, "Max temperature for parallel tempering");

    // Choose language
    fleet.add_option("--language", language_name, "Target language");

    // p_axis
    // p_near
    fleet.add_option("--p_direct", p_direct, "Probability a generated scene is direct");

    fleet.add_option("--p_intrinsic", p_intrinsic, "Probability an angular description uses an intrinsic FoR");
    fleet.add_option("--p_frame", p_frame, "Probability a description uses an FoR at all");

    fleet.add_option("--train_size", train_size, "Numbers of training data points");
    fleet.add_option("--repetitions", repetitions, "Numbers of times to repeat training");

fleet.initialize(argc, argv);

    generate_scenes(); // Generate scenes from Scenes.h before sampling 

    std::unordered_map<std::string, WordMeaning> english_int_only = {
        {"above", WordMeaning("parallel(x,UP)")},
        {"below", WordMeaning("parallel(x,DOWN)")},
        {"front", WordMeaning("parallel(x,forward(ground(x)))")},
        {"behind", WordMeaning("parallel(x,backward(ground(x)))")},
        {"left", WordMeaning("parallel(x,leftward(ground(x)))")},
        {"right", WordMeaning("parallel(x,rightward(ground(x)))")},
        {"side", WordMeaning("parallel(x,sideward(ground(x)))")},
        {"near", WordMeaning("near(x)")},
        {"far", WordMeaning("far(x)")},
    };

    std::unordered_map<std::string, WordMeaning> english_rel_only = {
        {"above", WordMeaning("parallel(x,UP)")},
        {"below", WordMeaning("parallel(x,DOWN)")},
        {"front", WordMeaning("or(and(parallel(x,forward(speaker(x))),not(g_not_sap(x))),and(parallel(x,backward(speaker(x))),g_not_sap(x)))")},
        {"behind", WordMeaning("or(and(parallel(x,backward(speaker(x))),not(g_not_sap(x))),and(parallel(x,forward(speaker(x))),g_not_sap(x)))")},
        {"left", WordMeaning("parallel(x,leftward(speaker(x)))")},
        {"right", WordMeaning("parallel(x,rightward(speaker(x)))")},
        {"side", WordMeaning("parallel(x,sideward(speaker(x)))")},
        {"near", WordMeaning("near(x)")},
        {"far", WordMeaning("far(x)")},
    };

    std::unordered_map<std::string, WordMeaning> english = {
        {"above", WordMeaning("parallel(x,UP)")},
        {"below", WordMeaning("parallel(x,DOWN)")},
        {"front", WordMeaning("exists(cs(Y+(x),or'(frame(G),frame'(S,TR))),pf(x))")},
        {"behind", WordMeaning("exists(cs(Y-(x),or'(frame(G),frame'(S,TR))),pf(x))")},
        {"left", WordMeaning("exists(cs(X-(x),or'(frame(G),frame'(S,TR))),pf(x))")},
        {"right", WordMeaning("exists(cs(X+(x),or'(frame(G),frame'(S,TR))),pf(x))")},
        {"side", WordMeaning("exists(cs(X+-(x),or'(frame(G),frame'(S,TR))),pf(x))")},
        {"near", WordMeaning("near(x)")},
        {"far", WordMeaning("far(x)")},
    };

    std::unordered_map<std::string, WordMeaning> english_w_body_parts = {
        {"above", WordMeaning(
                "parallel(x,UP)",
                BodyPartNoun::head,
                [](const OrientedObject& a) -> Direction {return a.upward;})},
        {"below", WordMeaning(
                "parallel(x,DOWN)",
                BodyPartNoun::belly,
                [](const OrientedObject& a) -> Direction {return -a.upward;})},
        {"front", WordMeaning(
                "exists(cs(Y+(x),or'(frame(G),frame'(S,TR))),pf(x))",
                BodyPartNoun::face,
                [](const OrientedObject& a) -> Direction {return a.forward;})},
        {"behind", WordMeaning(
                /* "parallel(x,back(x))", */
                "exists(cs(Y-(x),or'(frame(G),frame'(S,TR))),pf(x))",
                BodyPartNoun::back,
                [](const OrientedObject& a) -> Direction {return -a.forward;})},
        {"right", WordMeaning(
                "exists(cs(X+(x),or'(frame(G),frame'(S,TR))),pf(x))")},
        {"left", WordMeaning(
                "exists(cs(X-(x),or'(frame(G),frame'(S,TR))),pf(x))")},
        {"side", WordMeaning(
                "parallel(x,sideward(ground(x)))",
                BodyPartNoun::side,
                [](const OrientedObject& a) -> Direction {Direction sideward(a.rightward, true); return sideward;})},
        {"near", WordMeaning("near(x)")},
        {"far", WordMeaning("far(x)")},
    };

    /* std::unordered_map<std::string, WordMeaning> mixtec_words = { */
    /*     {"head", WordMeaning( */
    /*             "parallel(x,UP)", */
    /*             BodyPartNoun::head, */
    /*             [](const OrientedObject& a) -> Direction { */
    /*             switch(a.body_type) { */
    /*                 case BodyType::human: */
    /*                     return a.upward; */
    /*                 case BodyType::quadruped: */
    /*                     return {0,0,0}; */
    /*             } */
    /*             })}, */
    /*     {"belly", WordMeaning( */
    /*             "parallel(x,DOWN)", */
    /*             BodyPartNoun::belly, */
    /*             [](const OrientedObject& a) -> Direction {return -a.upward;})}, */
    /*     {"face", WordMeaning( */
    /*             "parallel(x,forward(ground(x)))", */
    /*             BodyPartNoun::face, */
    /*             [](const OrientedObject& a) -> Direction {return a.forward;})}, */
    /*     {"back", WordMeaning( */
    /*             /1* "parallel(x,back(x))", *1/ */
    /*             "parallel(x,backward(ground(x)))", */
    /*             BodyPartNoun::back, */
    /*             [](const OrientedObject& a) -> Direction { */
    /*             switch(a.body_type) { */
    /*                 case BodyType::human: */
    /*                     return -a.forward; */
    /*                 case BodyType::quadruped: */
    /*                     return a.upward; */
    /*             } */
    /*             })}, */
    /*     {"right", WordMeaning("parallel(x,rightward(ground(x)))")}, */
    /*     {"left", WordMeaning("parallel(x,leftward(ground(x)))")}, */
    /*     {"near", WordMeaning("near(x)")}, */
    /*     {"far", WordMeaning("far(x)")}, */
    /* }; */

    // Set language
    std::unordered_map<std::string, WordMeaning> language;
    if (language_name == "english"){
        language = english;
    } else if (language_name == "english_int_only") {
        language = english_int_only;
    } else if (language_name == "english_rel_only") {
        language = english_rel_only;
    } else if (language_name == "english_w_body_parts"){
        language = english_w_body_parts;
    } else {
        std::cerr << "Invalid language name provided." << std::endl;
        return 1;
    }

    // Initialize sampler
    MyData data_sampler(language);
    target = data_sampler.target;

    // Initialize amount of data to sample for each iteration
    /* std::vector<int> data_amounts = generate_range(data_min, data_max, data_step); */

    std::ofstream csvFile;
    std::string pr_file_name;
    if(precision_recall){
        // Open a file stream to write to a .csv file
            auto now = std::chrono::system_clock::now();
            std::time_t time = std::chrono::system_clock::to_time_t(now);
            std::stringstream date_time_ss;
            // date_time_ss << "results/" << std::put_time(std::localtime(&time), "%Y-%m-%d_%H-%M-%S") << ".csv";
            date_time_ss << output_dir << "/" << std::put_time(std::localtime(&time), "%Y-%m-%d_%H-%M-%S") << ".csv";
            pr_file_name = date_time_ss.str();
            std::ofstream csvFile(pr_file_name);
            
            // Write the header line to the csv file
            csvFile << "TrainingSize,Round,Rank,Posterior,Word,TrainingCount,TP,TN,FP,FN" << std::endl;
    }

    // Inference
    TopN<MyHypothesis> top;
    for (int i = 0; i < repetitions; i++) {
        // Sample data
        SceneProbs scene_probs;
        scene_probs.p_direct = p_direct;
        /* scene_probs.p_listener_ground = p_listener_ground; */
        scene_probs.p_near = 0.5;
        scene_probs.p_axis = 0.9;
        // scene_probs.p_axis = 1.0;

        WordProbs word_probs;
        word_probs.p_intrinsic = p_intrinsic;
        word_probs.p_frame = p_frame;

        Probabilities probs = {alpha_t, scene_probs, word_probs};

        std::vector<MyInput> train_data = data_sampler.sample_data(train_size, probs);

        // Refer to target hypothesis and sampled data
        TopN<MyHypothesis> newtop;
        for(auto h : top.values()) {
            h.clear_cache();
            h.compute_posterior(train_data);
            newtop << h;
        }
        top = newtop;

        target.clear_cache();
        target.compute_posterior(train_data);

        // Inference steps
        auto h0 = MyHypothesis::sample(data_sampler.words);
        /* MCMCChain samp(h0, &mydata); */
        //ChainPool samp(h0, &mydata, FleetArgs::nchains);
        ParallelTempering samp(h0, &train_data, FleetArgs::nchains, max_temp);
        for(auto& h : samp.run(Control()) | printer(FleetArgs::print) | top) {
                UNUSED(h);
        }

        // Show the best we've found
        top.print(str(train_size));

        if(precision_recall){
            // Reopen file if necessary
            if (!csvFile.is_open()) {
                csvFile.open(pr_file_name, std::ios::app);  // Open in append mode
                if (!csvFile.is_open()) {
                    std::cerr << "Failed to open or re-open file " << pr_file_name << std::endl;
                    return 0;
                }
            }
            // Training data
            TrainingStats training_stats(target, i);
            training_stats.set_counts(train_data);

            // Testing data
            Probabilities test_probs = {1.0, scene_probs, word_probs};
            std::vector<MyInput> test_data = data_sampler.sample_data(test_size, test_probs);

            TrialStats trial_stats(top, data_sampler);
            trial_stats.set_counts(target, test_data);

            trial_stats.write_lexicon_stats(csvFile, training_stats);
        }

        if(precision_recall){
            csvFile.close();
        }
    }
}