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

static const double data_reliability = 0.9; // probability of generating true description in training data
static const double alpha_t = 0.9; // model's reliability parameter
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
bool save_results = true;
std::string output_dir = "results/";

std::string get_current_datetime() {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::stringstream date_time_ss;
    date_time_ss << std::put_time(std::localtime(&time), "%Y-%m-%d_%H-%M-%S");
    return date_time_ss.str();
}

std::string create_file_name(const std::string& dir, const std::string& datetime, const std::string& suffix){
    std::stringstream file_name_ss;
    file_name_ss << dir << "/" << datetime << "_" << suffix << ".csv";
    return file_name_ss.str();
}

void create_csv(const std::string& file_name, const std::string& header){
    std::ofstream file(file_name);
    if(file.is_open()) {
        // Write the header line to the csv file
        file << header << std::endl;
    } else {
        // Handle the error case where the file couldn't be opened
        std::cerr << "Unable to open file: " << file_name << std::endl;
    }
}

// Data sampling parameters
double p_direct = 0.2; // probability a scene is direct
double p_listener_ground = 0.2; // probability that the ground of a nondirect scene is the listener

double p_frame = 0.0; // probability that a description uses an FoR if one applies
double p_intrinsic = 0.0; // probability that a description is intrinsic

// int train_size = 10;
// int repetitions = 1;
int train_min = 10;
int train_max = 100;
int train_spacing = 10;

std::vector<int> set_train_sizes(int min, int max, int spacing){
    std::vector<int> train_sizes;
    for (int size = min; size <= max; size+= spacing){
        train_sizes.push_back(size);
    }
    return train_sizes;
}

int test_size = 256;

// Hypothesis sampling parameters
double max_temp = 10.0; // maximum temperature for parallel tempering


// Language to sample training data from
std::string language_name = "english";

// Declare here so MyHypothesis class can reference the specific target instantiated in main
MyHypothesis target;
// Declare intrinsic and relative only hypotheses for testing
MyHypothesis intrinsic;
MyHypothesis relative;

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

    fleet.add_option("--train_min", train_min, "Minimum number of training data points");
    fleet.add_option("--train_max", train_max, "Maximum number of training data points");
    fleet.add_option("--train_spacing", train_spacing, "Interval between training data sizes");
    // fleet.add_option("--train_size", train_size, "Numbers of training data points");
    // fleet.add_option("--repetitions", repetitions, "Numbers of times to repeat training");
    
    fleet.add_option("--test_size", test_size, "Numbers of stimuli to use for precision/recall calculations");

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
        {"front", WordMeaning("if(g_sap(x),parallel(x,forward(speaker(x))),parallel(x,backward(speaker(x))))")},
        {"behind", WordMeaning("if(g_sap(x),parallel(x,backward(speaker(x))),parallel(x,forward(speaker(x))))")},
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
    /*                 case BodyType::biped: */
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
    /*                 case BodyType::biped: */
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

    std::string pr_file_name;
    std::string train_file_name;

    if(save_results){
        // Set up intrinsic and relative only hypotheses
        for (const auto& [word, meaning] : english_int_only) {
            intrinsic[word] = InnerHypothesis(grammar.simple_parse(meaning.target_formula));
        }
        for (const auto& [word, meaning] : english_rel_only) {
            relative[word] = InnerHypothesis(grammar.simple_parse(meaning.target_formula));
        }

        // Get datetime for use in file names
        std::string datetime = get_current_datetime();

        // Set up precision recall results
        pr_file_name = create_file_name(output_dir, datetime, "pr");
        std::string pr_header = "TrainingSize,Rank,Posterior,Word,TP,TN,FP,FN,I-TP,I-TN,I-FP,I-FN,R-TP,R-TN,R-FP,R-FN";
        create_csv(pr_file_name, pr_header);

        // Set up training data records
        train_file_name = create_file_name(output_dir, datetime, "train");
        std::string train_header = "TrainingSize,Word,TrueDescription,FigurePosition,GroundPosition,GroundOrientation,SpeakerPosition,SpeakerOrientation";
        create_csv(train_file_name, train_header);
    }

    // Inference
    TopN<MyHypothesis> top;
    std::vector<int> train_sizes = set_train_sizes(train_min, train_max, train_spacing);
    // for (int i = 0; i < repetitions; i++) {
    for (int train_size : train_sizes) {
        // Sample training data
        SceneProbs scene_probs;
        scene_probs.p_direct = p_direct;
        /* scene_probs.p_listener_ground = p_listener_ground; */
        scene_probs.p_near = 0.5;
        scene_probs.p_axis = 0.9;
        // scene_probs.p_axis = 1.0;

        WordProbs word_probs;
        word_probs.p_intrinsic = p_intrinsic;
        word_probs.p_frame = p_frame;

        Probabilities probs = {data_reliability, scene_probs, word_probs};

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

        if(save_results){
            // Write training data
            TrainingStats training_stats(train_data);
            training_stats.write_training_data(train_file_name);

            // Sample test data
            Probabilities test_probs = {1.0, scene_probs, word_probs};
            std::vector<MyInput> test_data = data_sampler.sample_data(test_size, test_probs);

            // Write test results
            TrialStats trial_stats(top, data_sampler, train_size);
            trial_stats.set_counts(test_data, target, intrinsic, relative);
            trial_stats.write_lexicon_stats(pr_file_name);
        }
    }
}