#pragma once

const std::vector<std::string> words = {"above", "below", "front", "behind", "side"};

/* struct MyInput { */
/*     Scene scene; */
/*     std::string word; */
/* }; */

struct MyData {
    MyHypothesis target;

    MyData() {
         // Set target concepts for words.
        target["above"] = InnerHypothesis(grammar.simple_parse("parallel(displacement(ground(x),figure(x)),up(x))"));
        target["below"] = InnerHypothesis(grammar.simple_parse("parallel(displacement(figure(x),ground(x)),up(x))"));
        target["front"] = InnerHypothesis(grammar.simple_parse("or(parallel(displacement(ground(x),figure(x)),orientation(ground(x))),parallel(displacement(figure(x),ground(x)),orientation(speaker(x))))"));
        /* target["front"] = InnerHypothesis(grammar.simple_parse("parallel(displacement(ground(x),figure(x)),orientation(ground(x)))")); // Intrinsic only */
        /* target["front"] = InnerHypothesis(grammar.simple_parse("or(parallel(displacement(ground(x),figure(x)),orientation(ground(x))),and(parallel(displacement(figure(x),ground(x)),orientation(speaker(x))),not(parallel(orientation(ground(x)),orientation(speaker(x))))))")); // No relative when speaker and ground are aligned */
        target["behind"] = InnerHypothesis(grammar.simple_parse("or(parallel(displacement(figure(x),ground(x)),orientation(ground(x))),parallel(displacement(ground(x),figure(x)),orientation(speaker(x))))"));
        /* target["behind"] = InnerHypothesis(grammar.simple_parse("parallel(displacement(figure(x),ground(x)),orientation(ground(x)))")); // Intrinsic only */
        /* target["behind"] = InnerHypothesis(grammar.simple_parse("or(parallel(displacement(figure(x),ground(x)),orientation(ground(x))),and(parallel(displacement(ground(x),figure(x)),orientation(speaker(x))),not(parallel(orientation(ground(x)),orientation(speaker(x))))))")); // No relative when speaker and ground are aligned */
        target["side"] = InnerHypothesis(grammar.simple_parse("or(orthogonal(displacement(ground(x),figure(x)),orientation(ground(x))),orthogonal(displacement(ground(x),figure(x)),orientation(speaker(x))))"));
        /* target["side"] = InnerHypothesis(grammar.simple_parse("orthogonal(displacement(ground(x),figure(x)),orientation(ground(x)))")); // Intrinsic only */
    }

    MyHypothesis::datum_t sample_datum(double p_direct) {
        // Sample scene
        Scene scene;
        // Handle direct and nondirect scenes separately
        bool is_direct = flip(p_direct);
        if(is_direct) {
            scene = direct_scenes[myrandom(direct_scenes.size())];
        } else {
            scene = nondirect_scenes[myrandom(nondirect_scenes.size())];
        }

        // Sample word
        std::string word;
        // First evaluate truth values for all words
        std::set<std::string> true_words;

        for(auto& w : words) {
            MyInput input{.scene=scene, .word=EMPTY_STRING};
            bool output = target.at(w).call(input);
            
            if (output == true){
                true_words.insert(w);
            }
        }
        // Then sample accordingly
        // TODO: figure out what's going on with normalizer argument of sample
        if(flip(alpha_t)) {
            // Sample from true descriptions
            word = *sample<std::string, decltype(true_words)>(true_words).first;
        } else {
            // Sample randomly
            word = *sample<std::string, decltype(words)>(words).first;
        }

        return MyInput{.scene=scene, .word=word};
    }
};
