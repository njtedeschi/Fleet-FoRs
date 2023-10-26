#pragma once

struct TrainingStats {
    MyHypothesis target;
    int training_round;
    int training_size = 0;
    std::unordered_map<std::string, int> description_counts;

    TrainingStats(MyHypothesis t, int r) : target(t), training_round(r) {
        for(auto& [word, formula] : target.factors){
            description_counts[word] = 0;
        }
    }

    void set_counts(std::vector<MyInput> training_data){
        for(auto& datum : training_data){
            description_counts[datum.word]++;
            training_size++;
        }
    } 

    int get_count(std::string word){
        return description_counts[word];
    }
};

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

    // void set_counts(InnerHypothesis target, std::vector<MyInput> test_data){
    //     for (const auto& datum : test_data) {
    //         bool predicted_value = formula.call(datum);
    //         bool target_value = target.call(datum);

    //         if(predicted_value){
    //             target_value ? true_positives++ : false_positives++;
    //         }
    //         else{
    //             target_value ? false_negatives++ : true_negatives++;
    //         }
    //     }
    //     compute_metrics();
    // }

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
    MyData data_sampler;

    LexiconStats(MyHypothesis l, MyData d) : lexicon(l), data_sampler(d) {
        posterior = lexicon.posterior;
        for(auto& [word, formula] : lexicon.factors) {
            // WordStats word_stats(formula);
            WordStats word_stats;
            lexicon_stats[word] = word_stats;
        }
    }

    void set_counts(MyHypothesis target, std::vector<MyInput> test_data){
        for(auto& datum : test_data){
            Scene scene = datum.scene;

            std::set<std::string> target_true_words;
            std::set<std::string> lexicon_true_words;
            target_true_words = data_sampler.compute_true_words(target, scene);
            lexicon_true_words = data_sampler.compute_true_words(lexicon, scene);

            for(auto& [word, word_stats] : lexicon_stats){
                bool predicted_value = lexicon_true_words.count(word);
                bool target_value = target_true_words.count(word);

                if(predicted_value){
                    target_value ? word_stats.true_positives++ : word_stats.false_positives++;
                }
                else{
                    target_value ? word_stats.false_negatives++ : word_stats.true_negatives++;
                }
            }
        }
        compute_metrics();
    }

    void compute_metrics(){
        for(auto& [word, word_stats] : lexicon_stats){
            word_stats.compute_metrics();
        }
    }
    // void set_counts(MyHypothesis target, std::vector<MyInput> test_data){
    //     for(auto& [word, word_stats] : lexicon_stats){
    //         InnerHypothesis target_formula = target.at(word);
    //         word_stats.set_counts(target_formula, test_data);
    //     }
    // }

    double get_statistic(std::string word, std::string statistic){
        WordStats word_stats = lexicon_stats[word];
        double statistic_value = word_stats.get_statistic(statistic);
        return statistic_value;
    }
};

struct TrialStats {
    TopN<MyHypothesis> top;
    std::vector<LexiconStats> top_stats;
    MyData data_sampler;

    TrialStats(TopN<MyHypothesis> t, MyData d) : top(t), data_sampler(d) {}

    void set_counts(MyHypothesis target, std::vector<MyInput> test_data){
        for(auto lexicon : top.sorted()){
            LexiconStats lexicon_stats(lexicon, data_sampler);
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

    std::string current_datetime() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream datetime_ss;
        datetime_ss << std::put_time(std::localtime(&time), "%Y-%m-%d_%H-%M-%S");
        std::string datetime = datetime_ss.str();

        return datetime;
    }

    void write_lexicon_stats(std::ofstream& csvFile, TrainingStats& training_stats) {
        // std::string datetime = current_datetime();
        int training_size = training_stats.training_size;
        int round = training_stats.training_round;
        int n = top.size();

        for(auto& lexicon : top_stats) {
            double posterior = lexicon.posterior;

            for(auto& [word, word_stats] : lexicon.lexicon_stats) {
                int word_training_count = training_stats.get_count(word);
                // Trial
                csvFile << training_size << "," << round;
                // Lexicon
                csvFile << "," << n << "," << posterior;
                // Word
                csvFile << "," << word << "," << word_training_count;
                csvFile << "," << word_stats.true_positives;
                csvFile << "," << word_stats.true_negatives;
                csvFile << "," << word_stats.false_positives;
                csvFile << "," << word_stats.false_negatives;
                csvFile << std::endl; // end the line for this lexicon's word
            }
            n--;
        }
    }
};