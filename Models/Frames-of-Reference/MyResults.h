#pragma once

struct WordStats {
    InnerHypothesis formula;

    int true_positives = 0;
    int true_negatives = 0;
    int false_positives = 0;
    int false_negatives = 0;

    double precision = 0;
    double recall = 0;
    double accuracy = 0;
    double f_1 = 0;

    void set_counts(InnerHypothesis target, std::vector<MyInput> test_data){
        for (const auto& datum : test_data) {
            bool predicted_value = formula.call(datum);
            bool target_value = target.call(datum);

            if(predicted_value){
                target_value ? true_positives++ : false_positives++;
            }
            else{
                target_value ? false_negatives++ : true_negatives++;
            }
        }
        compute_metrics();
    }

    void compute_metrics() {
        int predicted_positives = true_positives + false_positives;
        precision = predicted_positives > 0 ? 
            static_cast<double>(true_positives) / predicted_positives : 0;

        int positives = true_positives + false_negatives;
        recall = positives > 0 ?
            static_cast<double>(true_positives) / positives : 0;

        int total = true_positives + true_negatives + false_positives + false_negatives;
        accuracy = total > 0 ?
            static_cast<double>(true_positives + true_negatives) / total : 0;

        f_1 = (precision + recall) > 0 ?
            (2 * precision * recall) / (precision + recall) : 0;
    }

    double get_statistic(std::string statistic){
        if (statistic == "precision") {
            return precision;
        } else if (statistic == "recall") {
            return recall;
        } else if (statistic == "accuracy") {
            return accuracy;
        } else if (statistic == "f_1") {
            return f_1;
        } else {
            // TODO: error handling
            return 0;
        }
    }
};

struct LexiconStats {
    MyHypothesis lexicon;
    double posterior;
    std::unordered_map<std::string, WordStats> lexicon_stats;

    LexiconStats(MyHypothesis l) : lexicon(l) {
        posterior = lexicon.posterior;
        for(auto& [word, formula] : lexicon.factors) {
            WordStats word_stats(formula);
            lexicon_stats[word] = word_stats;
        }
    }

    void set_counts(MyHypothesis target, std::vector<MyInput> test_data){
        for(auto& [word, word_stats] : lexicon_stats){
            InnerHypothesis target_formula = target.at(word);
            word_stats.set_counts(target_formula, test_data);
        }
    }

    double get_statistic(std::string word, std::string statistic){
        WordStats word_stats = lexicon_stats[word];
        double statistic_value = word_stats.get_statistic(statistic);
        return statistic_value;
    }
};

struct TrialStats {
    TopN<MyHypothesis> top;
    std::vector<LexiconStats> top_stats;

    TrialStats(TopN<MyHypothesis> t) : top(t) {}

    void set_counts(MyHypothesis target, std::vector<MyInput> test_data){
        for(auto lexicon : top.values()){
            LexiconStats lexicon_stats(lexicon);
            lexicon_stats.set_counts(target, test_data);
            top_stats.push_back(lexicon_stats);
        }
    }

    // TODO: update to work with log probabilities
    double posterior_weighted_statistic(std::string word, std::string statistic){
        double weighted_sum = 0;
        double total_weight = 0;
        for(auto lexicon : top_stats){
            double posterior = std::exp(lexicon.posterior);
            weighted_sum += posterior * lexicon.get_statistic(word, statistic);
            total_weight += posterior;
        }
        return weighted_sum / total_weight;
    }
};