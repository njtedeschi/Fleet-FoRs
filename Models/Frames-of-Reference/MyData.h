#pragma once
# include <optional>

struct SceneProbs {
    double p_direct;
    /* double p_listener_ground; */
    double p_near;
    double p_axis;
};

struct WordProbs {
    std::optional<double> p_intrinsic = std::nullopt;
    std::optional<double> p_canonical = std::nullopt;
};

struct Probabilities {
    SceneProbs scene_probs;
    WordProbs word_probs;
};

struct MyData {
    std::vector<std::string> words;
    MyHypothesis target;

    MyHypothesis intrinsic;
    MyHypothesis relative;

    // SET UP

    // Default constructor
    MyData() : words(), target(), intrinsic(), relative() {}

    // Construct from dict of formulas
    MyData(const std::unordered_map<std::string, std::string>& formulas) {
        for (const auto& [word, formula] : formulas) {
            words.push_back(word);
            target[word] = InnerHypothesis(grammar.simple_parse(formula));
        }
    }

    void set_intrinsic(const std::unordered_map<std::string, std::string>& formulas) {
        for (const auto& [word, formula] : formulas) {
            intrinsic[word] = InnerHypothesis(grammar.simple_parse(formula));
        }
    }

    void set_relative(const std::unordered_map<std::string, std::string>& formulas) {
        for (const auto& [word, formula] : formulas) {
            relative[word] = InnerHypothesis(grammar.simple_parse(formula));
        }
    }

    // SAMPLING METHODS

    MyHypothesis::datum_t sample_datum(const Probabilities& probs) {
        // Sample scene
        Scene scene = sample_scene(probs.scene_probs);

        //Sample word
        std::string word;
        bool true_description = flip(alpha_t);
        if (!true_description) {
            word = sample_random_word();
        }
        else {
            word = sample_true_word(probs.word_probs, scene);
        }
        return MyInput{.scene=scene, .word=word, .true_description=true_description};
    }

    // Wrapper for `myrandom` to implement uniform distribution
    template<typename T>
    T myrandom_uniform(const std::vector<T>& entries) {
        if (entries.empty()) {
        throw std::invalid_argument("Cannot get random entry from empty list");
    }
        return entries[myrandom(entries.size())];
    }

    Scene sample_scene(const SceneProbs& scene_probs) {
            // Ground
            Direction ground_direction;
            ground_direction = myrandom_uniform(Space::ground_directions);

            // Figure
            double distance;
            distance = (flip(scene_probs.p_near)) ? 0.5 : 1.5;

            bool on_axis;
            on_axis = flip(scene_probs.p_axis);

            Position figure_position;
            if (on_axis) {
                figure_position = {distance * myrandom_uniform(Space::figure_positions_axis)};
            }
            else {
                figure_position = {distance * myrandom_uniform(Space::figure_positions_off_axis)};
            }

            // Scene
            bool is_direct;
            is_direct = flip(scene_probs.p_direct);
            if (is_direct) {
                OrientedObject direct_speaker = {Space::origin, ground_direction, true};
                Object figure = {figure_position};

                Scene direct = {direct_speaker, direct_speaker, figure};
                return direct;
            }
            else {
                OrientedObject nondirect_speaker = {Space::nondirect_speaker_spot, Space::east, true};
                OrientedObject ground = {Space::origin, ground_direction, false};
                Object figure = {figure_position};

                Scene nondirect = {nondirect_speaker, ground, figure};
                return nondirect;
            }
            }

    std::string sample_true_word(const WordProbs& word_probs, Scene scene) {
        std::string word;
        std::set<std::string> candidate_words;

        if(word_probs.p_intrinsic.has_value()) {
            if (flip(word_probs.p_intrinsic.value())) {
                candidate_words = compute_true_words(intrinsic, scene);
            } else {
                candidate_words = compute_true_words(relative, scene);
            }
        } else {
            candidate_words = compute_true_words(target, scene);
        }

        word = *sample<std::string, decltype(candidate_words)>(candidate_words).first;
        return word;
    }

    // Compute words that correctly describe a scene given a set of concepts for each word
    std::set<std::string> compute_true_words(MyHypothesis concepts, Scene scene){
        std::set<std::string> true_words;

        for(auto& w : words) {
            MyInput input{.scene=scene, .word=EMPTY_STRING, .true_description=true};
            bool output = concepts.at(w).call(input); 
            if (output == true){
                true_words.insert(w);
            }
        }
        return true_words;
    }

    std::string sample_random_word() {
        return *sample<std::string, decltype(words)>(words).first;
    }

    // Takes variable number of probability args to call appropriate sample_datum method
    // TODO: maybe update, since it looks like I'm not going to use variable number of arguments
    template<typename... Args>
    std::vector<MyInput> sample_data(int num_samples, Args... args){
        std::vector<MyInput> data;
        for (int i = 0; i < num_samples; i++){
            data.push_back(sample_datum(args...));
        }
        return data;
    }

};
