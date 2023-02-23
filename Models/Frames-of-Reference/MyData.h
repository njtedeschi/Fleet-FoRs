#pragma once

struct MyData {
    std::vector<std::string> words;
    MyHypothesis target;
    std::vector<MyInput> data;

    MyData(const std::unordered_map<std::string, std::string>& formulas) {
        for (const auto& [word, formula] : formulas) {
            words.push_back(word);
            target[word] = InnerHypothesis(grammar.simple_parse(formula));
        }
    }

    MyHypothesis::datum_t sample_datum(double p_direct) {
        // Sample scene
        Scene scene = sample_scene(p_direct);

        //Sample word
        // First evaluate truth values for all words
        std::set<std::string> true_words = compute_true_words(scene);
        // Then sample accordingly
        std::string word = sample_word(true_words);

        return MyInput{.scene=scene, .word=word};
    }

    Scene sample_scene(double p_direct){
        Scene scene;
        // Handle direct and nondirect scenes separately
        bool is_direct = flip(p_direct);
        if(is_direct) {
            scene = direct_scenes[myrandom(direct_scenes.size())];
        } else {
            scene = nondirect_scenes[myrandom(nondirect_scenes.size())];
        }
        return scene;
    }

    std::set<std::string> compute_true_words(Scene scene){
        std::set<std::string> true_words;

        for(auto& w : words) {
            MyInput input{.scene=scene, .word=EMPTY_STRING};
            bool output = target.at(w).call(input);
            
            if (output == true){
                true_words.insert(w);
            }
        }
        return true_words;
    }

    std::string sample_word(std::set<std::string> true_words){
        std::string word;
        if(flip(alpha_t)) {
            // Sample from true descriptions
            word = *sample<std::string, decltype(true_words)>(true_words).first;
        } else {
            // Sample randomly
            word = *sample<std::string, decltype(words)>(words).first;
        }
        return word;
    }
    /* MyHypothesis::datum_t sample_datum(double p_direct, double p_intrinsic) { */
    /*     // Sample scene */
    /*     Scene scene; */
    /*     // Handle direct and nondirect scenes separately */
    /*     bool is_direct = flip(p_direct); */
    /*     if(is_direct) { */
    /*         scene = direct_scenes[myrandom(direct_scenes.size())]; */
    /*     } else { */
    /*         scene = nondirect_scenes[myrandom(nondirect_scenes.size())]; */
    /*     } */

    /*     // Sample word */
    /*     std::string word; */
    /*     // First evaluate truth values for all words */
    /*     std::set<std::string> true_words; */
    /*     bool is_intrinsic = flip(p_intrinsic); */

    /*     for(auto& w : words) { */
    /*         MyInput input{.scene=scene, .word=EMPTY_STRING}; */
    /*         bool output; */
    /*         if (is_direct || is_intrinsic) { */
    /*             output = intrinsic.at(w).call(input); */
    /*         } else { */
    /*             output = relative.at(w).call(input); */
    /*         } */
    /*         if (output == true){ */
    /*             true_words.insert(w); */
    /*         } */
    /*     } */
    /*     // Then sample accordingly */
    /*     // TODO: figure out what's going on with normalizer argument of sample */
    /*     if(flip(alpha_t)) { */
    /*         // Sample from true descriptions */
    /*         word = *sample<std::string, decltype(true_words)>(true_words).first; */
    /*     } else { */
    /*         // Sample randomly */
    /*         word = *sample<std::string, decltype(words)>(words).first; */
    /*     } */

    /*     return MyInput{.scene=scene, .word=word}; */
    /* } */

    // Takes variable number of probability args to call appropriate sample_datum method
    template<typename... Args>
    void sample_data(int num_samples, Args... args){
        for (int i = 0; i < num_samples; i++){
            data.push_back(sample_datum(args...));
        }
    }

};
