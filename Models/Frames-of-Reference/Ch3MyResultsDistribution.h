#pragma once

#include <cmath>
#include <cassert>
#include <iostream>

using HypothesisDistribution = std::vector<std::pair<MyHypothesis,double>>;
using DescriptionDistribution = std::unordered_map<Word, std::unordered_map<Sense, double>>;

class TrainedModel {
public:
    // Constructor preprocesses the top N hypotheses
    // TrainedModel(TopN<MyHypothesis> top_n, const std::vector<MyInput>& training_data) {
    //     std::vector<MyHypothesis> hypotheses;
    //     hypotheses = top_n.sorted();
    //     set_hypothesis_distribution(hypotheses, training_data);
    // }
    TrainedModel(TopN<MyHypothesis> top_n) {
        std::vector<MyHypothesis> hypotheses;
        hypotheses = top_n.sorted();
        set_hypothesis_distribution(hypotheses);
    }

    DescriptionDistribution compute_marginal_description_distribution(Context context) {
        DescriptionDistribution marginal_distribution;

        // Iterate over the hypothesis distribution
        for (const auto& [hypothesis, log_p_hypothesis] : hypothesis_distribution) {
            // Get the log-probabilities for word-sense pairs for this hypothesis
            DescriptionDistribution description_distribution = compute_description_distribution(hypothesis, context);

            // Combine probabilities using the log-sum-exp trick
            for (const auto& [word, senses] : description_distribution) {
                for (const auto& [sense, log_p_word_sense_given_hypothesis] : senses) {
                    double log_p_word_sense = log_p_hypothesis + log_p_word_sense_given_hypothesis;

                    // Insert or combine with the existing value in the marginal distribution
                    if (marginal_distribution[word].count(sense) > 0) {
                        double current_log_p = marginal_distribution[word][sense];
                        double max_log_p = std::max(current_log_p, log_p_word_sense);
                        marginal_distribution[word][sense] = max_log_p + std::log(
                            std::exp(current_log_p - max_log_p) + std::exp(log_p_word_sense - max_log_p)
                        );
                        // marginal_distribution[word][sense] = std::log(
                        //     std::exp(current_log_p) + std::exp(log_p_word_sense)
                        // );
                    } else {
                        marginal_distribution[word][sense] = log_p_word_sense;
                    }
                }
            }
        }

        return marginal_distribution;
    }

private:
    HypothesisDistribution hypothesis_distribution;

    // void correct_posterior(MyHypothesis& hypothesis, const std::vector<MyInput>& training_data) {
    //     hypothesis.likelihood = compute_corrected_likelihood(hypothesis, training_data);
    //     hypothesis.posterior = hypothesis.prior + hypothesis.likelihood;
    // }

    // double compute_corrected_likelihood(MyHypothesis& hypothesis, const std::vector<MyInput>& training_data) {
    //     double likelihood = 0.0;
    //     for (size_t i = 0; i < training_data.size(); i++) {
    //         const MyInput& training_datum = training_data[i];
    //         likelihood += compute_corrected_single_likelihood(hypothesis, training_datum);
    //     }
    //     return likelihood;
    // }

    // double compute_corrected_single_likelihood(MyHypothesis& hypothesis, const MyInput& datum) {
    //     MyOutput output = hypothesis.call(datum.context);
    //     const auto& judgments = output.first;
    //     const auto& weights = output.second;

    //     double numerators[2] = {0};
    //     double denominators[2] = {0};
    //     for (const auto& [word, sense_judgments] : judgments) {
    //         bool is_observed_word = (word.form == datum.utterance);
    //         for (const auto& [sense, judgment] : sense_judgments) {
    //             double weight = weights.at(word).at(sense);
    //             for (int j = 0; j < 2; j++) {
    //                 if (static_cast<int>(judgment) >= j) {
    //                     numerators[j] += is_observed_word ? weight : 0;
    //                     denominators[j] += weight;
    //                 }
    //             }
    //         }
    //     }

    //     double p = (1.0 - alpha_t)*numerators[0]/denominators[0];
    //     p += (numerators[1] != 0) ? alpha_t*numerators[1]/denominators[1] : 0;
    //     return log(p);
    // }

    // Pair hypotheses with normalized log posteriors
    // void set_hypothesis_distribution(std::vector<MyHypothesis>& hypotheses, const std::vector<MyInput>& training_data) {
    void set_hypothesis_distribution(std::vector<MyHypothesis>& hypotheses) {
        double max_log_posterior = hypotheses[0].posterior;

        for (auto& hypothesis : hypotheses) {
            // recompute hypothesis posterior
            // correct_posterior(hypothesis, training_data);
            if (hypothesis.posterior > max_log_posterior) {
                max_log_posterior = hypothesis.posterior;
            }
        }

        double log_sum_exp = 0.0;
        for (const auto& hypothesis : hypotheses) {
            log_sum_exp += std::exp(hypothesis.posterior - max_log_posterior);
        }
        log_sum_exp = std::log(log_sum_exp) + max_log_posterior;

        for (const auto& hypothesis : hypotheses) {
            hypothesis_distribution.emplace_back(
                hypothesis,
                hypothesis.posterior - log_sum_exp
            );
        }
    }

    // Compute log-probabilities for all Word-Sense pairs for a given hypothesis and context
    DescriptionDistribution compute_description_distribution(MyHypothesis hypothesis, const Context& context) const {
        MyOutput output = hypothesis.call(context);
        const auto& judgments = output.first;
        const auto& weights = output.second;

        DescriptionDistribution description_distribution;

        // "Corrected" likelihood
        // double denominators[3] = {0};
        // for (const auto& [word, sense_judgments] : judgments) {
        //     for (const auto& [sense, judgment] : sense_judgments) {
        //         double weight = weights.at(word).at(sense);
        //         for (int j = 0; j < 3; j++) {
        //             if (to_int(judgment) >= j) {
        //                 denominators[j] += weight;
        //             }
        //         }
        //     }
        // }

        // for (const auto& [word, sense_judgments] : judgments) {
        //     for (const auto& [sense, judgment] : sense_judgments) {
        //         double weight = weights.at(word).at(sense);
        //         int judgment_value = to_int(judgment);

        //         double p = (1.0 - alpha_t) * weight / denominators[0];
        //         p += (judgment_value >= 1) ? alpha_t * (1-alpha_f) * weight / denominators[1] : 0;
        //         p += (judgment_value >= 2) ? alpha_t * alpha_f * weight / denominators[2] : 0;

        //         description_distribution[word][sense] = log(p);
        //     }
        // }

        // "Original" likelihood
        double denominators[2] = {0};
        for (const auto& [word, sense_judgments] : judgments) {
            for (const auto& [sense, judgment] : sense_judgments) {
                double weight = weights.at(word).at(sense);
                for (int j = 0; j < 2; j++) {
                    if (static_cast<int>(judgment) >= j) {
                        denominators[j] += weight;
                    }
                }
            }
        }

        for (const auto& [word, sense_judgments] : judgments) {
            for (const auto& [sense, judgment] : sense_judgments) {
                double weight = weights.at(word).at(sense);
                int judgment_value = static_cast<int>(judgment);

                double p = (1.0 - alpha_t) * weight / denominators[0];
                p += (judgment_value >= 1) ? alpha_t * weight / denominators[1] : 0;

                description_distribution[word][sense] = log(p);
            }
        }

        assert_probabilities_sum_to_one(description_distribution);

        return description_distribution;
    }

    void assert_probabilities_sum_to_one(const DescriptionDistribution& description_distribution) const {
        // Define a tolerance for floating-point inaccuracies
        const double epsilon = 1e-6;
        double total_probability = 0.0;
        for (const auto& [word, senses] : description_distribution) {
            for (const auto& [sense, log_p] : senses) {
                // std::cout << word.form << ", " << to_string(sense) << std::exp(log_p) << std::endl;
                total_probability += std::exp(log_p); // Convert log-probabilities to probabilities
            }
        }
        // Check if the total probability is approximately 1
        if (std::abs(total_probability - 1.0) > epsilon) {
            std::cerr << "Probabilities do not sum to 1: "
                    << total_probability << std::endl; // Optional for debugging
            assert(false && "Probabilities do not sum to 1!");
        }
    }
};

class ModelEvaluator {
public:
    ModelEvaluator(TrainedModel m) : model(m) {}

    static void set_testing_data(const std::vector<UniqueTestingDatum>& data) {
        testing_data = data;
    }

    void evaluate_posterior_distributions() {
        for (auto& testing_datum : testing_data) {
            description_distributions.emplace_back(
                model.compute_marginal_description_distribution(testing_datum.context)
            );
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
        for (size_t i = 0; i < description_distributions.size(); i++) {
            const auto& context_properties = testing_data[i].properties;
            for (const auto& [word, sense_probs] : description_distributions[i]) {
                for (const auto& [sense, prob] : sense_probs) {
                    testing_results_file << training_size
                        << "," << iteration
                        << "," << i // test datum index
                        << "," << context_properties.intrinsic_description
                        << "," << context_properties.relative_description
                        << "," << context_properties.ground_x_canonical
                        << "," << context_properties.ground_y_canonical
                        << "," << context_properties.ground_z_canonical
                        // << "," << word.form
                        << "," << ContextProperties::abbreviate_description(word.form)
                        << "," << to_string(sense)
                        << "," << prob
                        << std::endl;
                }
            }
        }
        testing_results_file.close();
    }
private:
    TrainedModel model;
    static std::vector<UniqueTestingDatum> testing_data;
    std::vector<DescriptionDistribution> description_distributions;
};