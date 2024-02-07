#pragma once

#include <functional>
#include <tuple>
using RankWordSense = std::tuple<int, std::string, std::string>;

namespace hash_tuple {

template <typename T>
struct hash {
    size_t operator()(T const& tt) const {
        return std::hash<T>()(tt);
    }
};

namespace {
    template <class T>
    inline void hash_combine(std::size_t& seed, T const& v) {
        seed ^= hash<T>()(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    }

    template <class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
    struct HashValueImpl {
        static void apply(size_t& seed, Tuple const& tuple) {
            HashValueImpl<Tuple, Index-1>::apply(seed, tuple);
            hash_combine(seed, std::get<Index>(tuple));
        }
    };

    template <class Tuple>
    struct HashValueImpl<Tuple,0> {
        static void apply(size_t& seed, Tuple const& tuple) {
            hash_combine(seed, std::get<0>(tuple));
        }
    };
}

template <typename ... TT>
struct hash<std::tuple<TT...>> {
    size_t operator()(std::tuple<TT...> const& tt) const {                                              
        size_t seed = 0;                             
        HashValueImpl<std::tuple<TT...>>::apply(seed, tt);    
        return seed;                                 
    }                                              
};

} // namespace hash_tuple

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

// Calculates CSV entries and writes them
class TestingEvaluator {
public:

    TestingEvaluator(TopN<MyHypothesis> t, const std::unordered_map<std::string,BodyPartMeaning> wm) : top(t), word_meanings(wm) {
        num_hypotheses = top.size();
    }

    void evaluate(std::vector<TestingDatum> testing_data) {
        for (auto& testing_datum : testing_data) {
            int rank = num_hypotheses;
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

    void write_testing_stats(const std::string& results_filepath, int training_size, int iteration) {
        // Open output file in append mode
        std::ofstream testing_results_file(results_filepath, std::ios_base::app);
        if (!testing_results_file.is_open()) {
            std::cerr << "Failed to open or re-open file " << results_filepath << std::endl;
            return;
        }

        // Iterate over the counts map to write stats for each rank-word-sense combination
        for (auto& [key, matrix] : counts) {
            // Extract rank, word, and sense from the key
            const auto& [rank, word, sense] = key;

            // Write the data to the file
            testing_results_file << training_size << "," << iteration
                    << "," << rank
                    << "," << word 
                    << "," << sense
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
    int num_hypotheses;
    std::unordered_map<std::string, BodyPartMeaning> word_meanings;
    std::unordered_map<RankWordSense, ConfusionMatrix, hash_tuple::hash<RankWordSense>> counts;
};