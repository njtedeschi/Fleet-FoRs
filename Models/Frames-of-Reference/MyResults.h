#pragma once

struct TrainingStats {
    std::vector<MyInput> train_data;
    int train_size;

    TrainingStats(std::vector<MyInput> td) : train_data(td) {
        train_size = train_data.size();
    }

    std::string vector_to_string(const Vector& v) {
        std::ostringstream oss;
        oss << "[" << v[0] << ";" << v[1] << ";" << v[2] << "]";
        return oss.str();
    }

    void write_training_data(std::string train_file_name) {
        // Open output file in append mode
        std::ofstream train_file;
        train_file.open(train_file_name, std::ios_base::app);
        if (!train_file.is_open()) {
            std::cerr << "Failed to open or re-open file " << train_file_name << std::endl;
            return;
        }

        for(auto& input : train_data) {
            // Convert each Vector to string
            std::string figure_position = vector_to_string(input.scene.figure.position);
            std::string ground_position = vector_to_string(input.scene.ground.position);
            std::string ground_forward = vector_to_string(input.scene.ground.forward);
            std::string speaker_position = vector_to_string(input.scene.speaker.position);
            std::string speaker_forward = vector_to_string(input.scene.speaker.forward);

            // Write the data
            train_file << train_size << ","
                << input.word << ","
                << (input.true_description ? 1 : 0) << ","
                << figure_position << ","
                << ground_position << ","
                << ground_forward << ","
                << speaker_position << ","
                << speaker_forward << std::endl;
        }
    }

};

struct WordStats {
    InnerHypothesis formula;

    int true_positives = 0;
    int true_negatives = 0;
    int false_positives = 0;
    int false_negatives = 0;

    int int_true_positives = 0;
    int int_true_negatives = 0;
    int int_false_positives = 0;
    int int_false_negatives = 0;

    int rel_true_positives = 0;
    int rel_true_negatives = 0;
    int rel_false_positives = 0;
    int rel_false_negatives = 0;
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

    void set_counts(std::vector<MyInput> test_data, MyHypothesis target, MyHypothesis intrinsic, MyHypothesis relative){
        for(auto& datum : test_data){
            Scene scene = datum.scene;

            std::set<std::string> lexicon_true_words;
            std::set<std::string> target_true_words;
            std::set<std::string> int_true_words;
            std::set<std::string> rel_true_words;
            lexicon_true_words = data_sampler.compute_true_words(lexicon, scene);
            target_true_words = data_sampler.compute_true_words(target, scene);
            int_true_words = data_sampler.compute_true_words(intrinsic, scene);
            rel_true_words = data_sampler.compute_true_words(relative, scene);

            for(auto& [word, word_stats] : lexicon_stats){
                bool predicted_value = lexicon_true_words.count(word);
                bool target_value = target_true_words.count(word);
                bool int_value = int_true_words.count(word);
                bool rel_value = rel_true_words.count(word);

                // Update true/false positives/negatives
                if(predicted_value){
                    target_value ? word_stats.true_positives++ : word_stats.false_positives++;
                    int_value ? word_stats.int_true_positives++ : word_stats.int_false_positives++;
                    rel_value ? word_stats.rel_true_positives++ : word_stats.rel_false_positives++;
                }
                else{
                    target_value ? word_stats.false_negatives++ : word_stats.true_negatives++;
                    int_value ? word_stats.int_false_negatives++ : word_stats.int_true_negatives++;
                    rel_value ? word_stats.rel_false_negatives++ : word_stats.rel_true_negatives++;
                }
            }
        }
    }
};

struct TrialStats {
    TopN<MyHypothesis> top;
    MyData data_sampler; // Necessary for computing true words
    int train_size;

    std::vector<LexiconStats> top_stats;

    TrialStats(TopN<MyHypothesis> t, MyData d, int ts) : top(t), data_sampler(d), train_size(ts) {}

    void set_counts(std::vector<MyInput> test_data, MyHypothesis target, MyHypothesis intrinsic, MyHypothesis relative){
        for(auto lexicon : top.sorted()){
            LexiconStats lexicon_stats(lexicon, data_sampler);
            lexicon_stats.set_counts(test_data, target, intrinsic, relative);
            top_stats.push_back(lexicon_stats);
        }
    }

    // std::string current_datetime() {
    //     auto now = std::chrono::system_clock::now();
    //     auto time = std::chrono::system_clock::to_time_t(now);
    //     std::stringstream datetime_ss;
    //     datetime_ss << std::put_time(std::localtime(&time), "%Y-%m-%d_%H-%M-%S");
    //     std::string datetime = datetime_ss.str();

    //     return datetime;
    // }

    void write_lexicon_stats(std::string pr_file_name) {
        // std::string datetime = current_datetime();

        // Open output file in append mode
        std::ofstream pr_file;
        pr_file.open(pr_file_name, std::ios_base::app);
        if (!pr_file.is_open()) {
            std::cerr << "Failed to open or re-open file " << pr_file_name << std::endl;
            return;
        }

        int n = top.size();

        for(auto& lexicon : top_stats) {
            double posterior = lexicon.posterior;

            for(auto& [word, word_stats] : lexicon.lexicon_stats) {
                // Trial
                pr_file << train_size;
                // Lexicon
                pr_file << "," << n << "," << posterior;
                // Word
                pr_file << "," << word;
                pr_file << "," << word_stats.true_positives;
                pr_file << "," << word_stats.true_negatives;
                pr_file << "," << word_stats.false_positives;
                pr_file << "," << word_stats.false_negatives;
                pr_file << "," << word_stats.int_true_positives;
                pr_file << "," << word_stats.int_true_negatives;
                pr_file << "," << word_stats.int_false_positives;
                pr_file << "," << word_stats.int_false_negatives;
                pr_file << "," << word_stats.rel_true_positives;
                pr_file << "," << word_stats.rel_true_negatives;
                pr_file << "," << word_stats.rel_false_positives;
                pr_file << "," << word_stats.rel_false_negatives;
                pr_file << std::endl; // end the line for this lexicon's word
            }
            n--;
        }
    }
};