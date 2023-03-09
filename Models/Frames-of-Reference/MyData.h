#pragma once
# include <optional>

struct SceneProbs {
    std::optional<double> p_direct = std::nullopt;
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
    std::vector<MyInput> data;

    MyHypothesis intrinsic;
    MyHypothesis relative;

    // SET UP

    // Default constructor
    MyData() : words(), target(), data(), intrinsic(), relative() {}

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

    Scene sample_scene(const SceneProbs& scene_probs) {
        Scene scene;
        std::vector<Scene> candidate_scenes;

        if (scene_probs.p_direct.has_value()) {
            if(flip(scene_probs.p_direct.value())) {
                candidate_scenes = direct_scenes;
            } else {
                candidate_scenes = nondirect_scenes;
            }
        } else {
            candidate_scenes.insert(candidate_scenes.end(), direct_scenes.begin(), direct_scenes.end());
        candidate_scenes.insert(candidate_scenes.end(), nondirect_scenes.begin(), nondirect_scenes.end());
        }

        scene = candidate_scenes[myrandom(candidate_scenes.size())];
        return scene;
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
    void sample_data(int num_samples, Args... args){
        for (int i = 0; i < num_samples; i++){
            data.push_back(sample_datum(args...));
        }
    }

};
