#pragma once
# include <optional>

// INT or REL
struct AbstractDirection {
    std::string name;
    std::function<bool(const Displacement&, Frame&)> truth_condition;

    // constructor
    AbstractDirection(std::string n, std::function<bool(const Displacement&, Frame&)> t)
        : name(n), truth_condition(t) {}
};

std::vector<AbstractDirection> abstract_directions = {
    AbstractDirection("above", [](const Displacement& v, Frame& f){return cosine_similarity(v, f.upward) == 1;}),
                AbstractDirection("below", [](const Displacement& v, Frame& f){return cosine_similarity(v, f.upward) == -1;}),
                AbstractDirection("front", [](const Displacement& v, Frame& f){return cosine_similarity(v, f.forward) == 1;}),
                AbstractDirection("behind", [](const Displacement& v, Frame& f){return cosine_similarity(v, f.forward) == -1;}),
                AbstractDirection("right", [](const Displacement& v, Frame& f){return cosine_similarity(v, f.rightward) == 1;}),
                AbstractDirection("left", [](const Displacement& v, Frame& f){return cosine_similarity(v, f.rightward) == -1;})
};

// ABS
struct AbsoluteDirection {
    std::string name;
    std::function<bool(const Displacement&)> truth_condition;

    // constructor
    AbsoluteDirection(std::string n, std::function<bool(const Displacement&)> t)
        : name(n), truth_condition(t) {}
};

std::vector<AbsoluteDirection> absolute_directions = {
    AbsoluteDirection("above", [](const Displacement& v){return cosine_similarity(v, Space::up) == 1;}),
                AbsoluteDirection("below", [](const Displacement& v){return cosine_similarity(v, Space::up) == -1;}),
                AbsoluteDirection("north", [](const Displacement& v){return cosine_similarity(v, Space::north) == 1;}),
                AbsoluteDirection("south", [](const Displacement& v){return cosine_similarity(v, Space::north) == -1;}),
                AbsoluteDirection("east", [](const Displacement& v){return cosine_similarity(v, Space::east) == 1;}),
                AbsoluteDirection("west", [](const Displacement& v){return cosine_similarity(v, Space::east) == -1;})
};

// 1-1 map with scenes (given speaker faces ground)
struct SpatialDescription {
    std::string intrinsic;
    std::string relative;
    std::string absolute;

    double dist_ground_figure;
    double dist_speaker_ground;
    std::string proximity;

    // Default constructor
    SpatialDescription(std::string i, std::string r, std::string a, double gf, double sg) :
        intrinsic(i), relative(r), absolute(a), dist_ground_figure(gf), dist_speaker_ground(sg) {proximity = (gf < 1) ? "near" : "far";} // proximity word from distance between ground and speaker

    // Convert from scene
    explicit SpatialDescription(const Scene& scene) {
            Displacement g_to_f = scene.g_to_f;
            this-> dist_ground_figure = magnitude(g_to_f);
            Displacement s_to_g = scene.ground.position - scene.speaker.position;
            this-> dist_speaker_ground = magnitude(s_to_g);
            this->proximity = (dist_ground_figure < 1) ? "near" : "far";

            this->intrinsic = intrinsic_description(scene);
            this->relative = relative_description(scene);
            this->absolute = absolute_description(scene);
            }

    std::string intrinsic_description(const Scene& scene) {
        Frame frame(AbstractFrame(Anchor::ground,Transformation::none),scene);
        for (auto& abstract_direction : abstract_directions) {
            if (abstract_direction.truth_condition(scene.g_to_f, frame)){
                return abstract_direction.name;
            }
        }
        // Description is empty string if no word applies in frame
        return "";
    }
    std::string relative_description(const Scene& scene) {
        Frame frame(AbstractFrame(Anchor::speaker,Transformation::transreflected),scene);
        for (auto& abstract_direction : abstract_directions) {
            if (abstract_direction.truth_condition(scene.g_to_f, frame)){
                return abstract_direction.name;
            }
        }
        // Description is empty string if no word applies in frame
        return "";
    }
    std::string absolute_description(const Scene& scene) {
        for (auto& absolute_direction : absolute_directions) {
            if (absolute_direction.truth_condition(scene.g_to_f)) {
                return absolute_direction.name;
            }
        }
        // Description is empty string if no word applies in frame
        return "";
    }
};


/***/

struct SceneProbs {
    double p_direct;
    /* double p_listener_ground; */
    double p_near;
    double p_axis;
};

struct WordProbs {
    double p_frame = 0;
    double p_intrinsic = 0;
    /* std::optional<double> p_canonical = std::nullopt; */
};

struct Probabilities {
    SceneProbs scene_probs;
    WordProbs word_probs;
};

struct MyData {
    std::unordered_map<std::string,WordMeaning> word_meanings;
    std::vector<std::string> words;
    MyHypothesis target;

    /* MyHypothesis intrinsic; */
    /* MyHypothesis relative; */

    // SET UP

    // Default constructor
    /* MyData() : words(), target(), intrinsic(), relative() {} */
    MyData() : word_meanings(), words(), target() {}

    // Construct from dict of WordMeaning objects tbat contain
    // target formula and lamba function for potential body part direction
    MyData(const std::unordered_map<std::string, WordMeaning> wm) : word_meanings(wm) {
        for (const auto& [word, meaning] : word_meanings) {
            words.push_back(word);
            target[word] = InnerHypothesis(grammar.simple_parse(meaning.target_formula));
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
        return MyInput{.scene=scene, .word=word, .body_part_meaning=word_meanings[word].body_part, .true_description=true_description};
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
                OrientedObject direct_speaker = {Space::origin, ground_direction, Space::up, true};
                BaseObject figure = {figure_position};

                Scene direct = {direct_speaker, direct_speaker, figure};
                return direct;
            }
            else {
                OrientedObject nondirect_speaker = {Space::nondirect_speaker_spot, Space::east, Space::up, true};

                BodyType body_type = (flip(0.5)) ? BodyType::human : BodyType::quadruped;
                OrientedObject ground = {Space::origin, ground_direction, Space::up, false, body_type};
                BaseObject figure = {figure_position};

                Scene nondirect = {nondirect_speaker, ground, figure};
                return nondirect;
            }
            }

    std::string sample_true_word(const WordProbs& word_probs, Scene scene) {
        std::set<std::string> candidate_words;
        // Sample from all true words
        if(word_probs.p_frame == 0 && word_probs.p_intrinsic == 0) {
            candidate_words = compute_true_words(target, scene);
        }
        // Sample from true words according to WordProbs
        else {
            std::string word;
            SpatialDescription description(scene);
            bool uses_frame = flip(word_probs.p_frame);
            bool is_intrinsic = flip(word_probs.p_intrinsic);
            if(uses_frame) {
                word = (is_intrinsic) ? description.intrinsic : description.relative;
            }
            else {
                word = description.proximity;
            }
            candidate_words.insert(word);
            // SpatialDescription doesn't encode side, so it mus be added manually
            if(is_intrinsic && (word == "left" || word == "right")) {
                candidate_words.insert("side");
            }
        }
        std::string sampled_word = *sample<std::string, decltype(candidate_words)>(candidate_words).first;
        return sampled_word;
        }

    // Compute all words that correctly describe a scene given a set of concepts for each word
    std::set<std::string> compute_true_words(MyHypothesis concepts, Scene scene){
        std::set<std::string> true_words;

        for(auto& w : words) {
            MyInput input{.scene=scene, .word=w, .body_part_meaning=word_meanings[w].body_part, .true_description=true};
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
