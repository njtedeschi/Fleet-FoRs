#pragma once

#include <tuple>

// Numerical columns of CSV
struct ConfusionMatrix {
    int true_positives;
    int true_negatives;
    int false_positives;
    int false_negatives;

    ConfusionMatrix() : true_positives(0), true_negatives(0), false_positives(0), false_negatives(0) {}

    void update_counts(bool predicted_value, bool target_value) {
        if(predicted_value){
            target_value ? true_positives++ : false_positives++;
        }
        else{
            target_value ? false_negatives++ : true_negatives++;
        }
    }
};

// Key representing a row of CSV (first 3 columns)
struct RankWordSense {
    int rank;
    std::string word;
    std::string sense;

    RankWordSense(int r, std::string w, std::string s) : rank(r), word(w), sense(s) {}

    bool operator==(const RankWordSense& other) const {
        return rank == other.rank && word == other.word && sense == other.sense;
    }
};

namespace std {
    template<>
    struct hash<RankWordSense> {
        std::size_t operator()(const RankWordSense& k) const {
            return std::hash<int>()(k.rank) 
                 ^ std::hash<std::string>()(k.word) 
                 ^ std::hash<std::string>()(k.sense);
        }
    };
}

// Calculates CSV entries and writes them
class TestingEvaluator {
public:

    TestingEvaluator(TopN<MyHypothesis> t, const std::unordered_map<std::string,BodyPartMeaning> wm) : top(t), word_meanings(wm) {}

    void evaluate(std::vector<TestingDatum> testing_data) {
        int rank = top.size();
        for (auto& testing_datum : testing_data) {
            for (auto& lexicon : top.sorted()) {
                for (auto& [word, target_values] : testing_datum.label) {
                    BodyPartMeaning* meaning = &(word_meanings[word]);
                    MyInput my_input(testing_datum.scene, word, meaning);
                    bool predicted_value = lexicon.at(word).call(my_input);
                    for (auto& [sense, target_value] : target_values) {
                        RankWordSense key(rank, word, sense);
                        auto& matrix = counts[key]; // Creates or accesses matrix
                        matrix.update_counts(predicted_value, target_value);
                    }
                }
                rank--;
            }
        }
    }

    void write_testing_stats(const std::string& output_filepath) {
        // Open output file in append mode
        std::ofstream testing_results_file(output_filepath, std::ios_base::app);
        if (!testing_results_file.is_open()) {
            std::cerr << "Failed to open or re-open file " << output_filepath << std::endl;
            return;
        }

        // Write header row
        testing_results_file << "Rank,Word,Sense,TP,TN,FP,FN" << std::endl;
        // Iterate over the counts map to write stats for each rank-word-sense combination
        for (auto& [key, matrix] : counts) {
            // Extract rank, word, and sense from the key
            const auto& [rank, word, sense] = key;

            // Write the data to the file
            testing_results_file << rank << "," << word << "," << sense
                    << "," << matrix.true_positives
                    << "," << matrix.true_negatives
                    << "," << matrix.false_positives
                    << "," << matrix.false_negatives
                    << std::endl; // End the line for this word's sense
        }
        testing_results_file.close();
    }

    void write_lookup_info(const std::string& lookup_filepath, int training_size, int iteration) {
        // Open lookup file in append mode
        std::ofstream lookup_file(lookup_filepath, std::ios_base::app);
        if (!lookup_file.is_open()) {
            std::cerr << "Failed to open or re-open file " << lookup_filepath << std::endl;
            return;
        }
        int rank = top.size();
        for (auto& lexicon : top.sorted()) {
            lookup_file << training_size << "," << iteration
                << "," << rank
                << "," << lexicon.prior
                << "," << lexicon.likelihood
                << "," << lexicon.posterior
                << std::endl;
            rank--;
        }
    }

private:
    TopN<MyHypothesis> top;
    std::unordered_map<std::string, BodyPartMeaning> word_meanings;
    std::unordered_map<RankWordSense, ConfusionMatrix> counts;
};