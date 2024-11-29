#pragma once

#include <array>
#include <cmath>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <optional>
#include <unordered_map>
#include <memory>

#include <type_traits>

using Vector = std::array<double,3>;
struct Displacement : public Vector {};
struct Direction : public Vector {};
struct Position : public Vector {};

Vector operator*(double s, const Vector& v) {
    Vector result;
    for (int i = 0; i < 3; ++i) {
        result[i] = v[i] * s;
    }
    return result;
}

Direction operator-(const Direction& v) {
    Direction result;
    for (int i = 0; i < 3; ++i) {
        result[i] = v[i] * -1;
    }
    return result;
}

Displacement operator-(const Position& v1, const Position& v2) {
    Displacement result;
    for (int i = 0; i < 3; ++i) {
        result[i] = v1[i] - v2[i];
    }
    return result;
}

double dot_product(const Vector &a, const Vector &b){
    double result = 0.0;
    for(int i = 0; i < 3; i++){
        result += a[i] * b[i];
    }
    return result;
}

double magnitude(const Vector &a){
    double result = 0.0;
    for (int i = 0; i < 3; i++){
        result += a[i] * a[i];
    }
    return sqrt(result);
}

double cosine_similarity(const Vector &a, const Vector &b){
    return dot_product(a, b)/(magnitude(a) * magnitude(b));
}

namespace Space {
    Position origin = {0,0,0};
    Position east_spot = {1,0,0};
    Position west_spot = {-1,0.0};
    Position north_spot = {0,1,0};
    Position south_spot = {0,-1,0};
    Position up_spot = {0,0,1};
    Position down_spot = {0,0,-1};

    Direction null_vector = {0,0,0};
    Direction east = {1,0,0};
    Direction west = {-1,0.0};
    Direction north = {0,1,0};
    Direction south = {0,-1,0};
    Direction up = {0,0,1};
    Direction down = {0,0,-1};
}

enum class Axis {
    X = 0,
    Y = 1,
    Z = 2
};

enum class CardinalAxis {
    EastWest = 0,
    NorthSouth = 1,
    UpDown = 2
};

enum class Sign {
    Plus = 1,
    Minus = -1,
    PlusMinus = 0
};

enum class Alignment {
    PlusParallel = 1,
    MinusParallel = -1,
    PlusMinusParallel = 0
};

struct UnitVector {
    CardinalAxis axis;
    Sign sign;

    UnitVector() = default;
    UnitVector(CardinalAxis a, Sign s) : axis(a), sign(s) {}
    // Constructor to convert from Vector
    UnitVector(const Vector& v, bool is_bidirectional = false) {
        int nonzero_index = -1;

        for (int i = 0; i < 3; i++) {
            if (v[i] != 0) {
                if (nonzero_index != -1) {
                    throw std::invalid_argument("Vector can only have one non-zero component");
                }
                nonzero_index = i;
            }
        }

        if (nonzero_index == -1) {
            throw std::invalid_argument("Vector cannot be the zero vector");
        }

        axis = static_cast<CardinalAxis>(nonzero_index);
        if (is_bidirectional) {
            sign = Sign::PlusMinus;
        } else {
            sign = (v[nonzero_index] > 0) ? Sign::Plus : Sign::Minus;
        }
    }

    UnitVector operator-() const {
        return UnitVector(
            axis,
            static_cast<Sign>(-static_cast<int>(sign))
        );
    }

    Vector to_vector() const {
        Vector v;

        int nonzero_index = static_cast<int>(axis);
        double nonzero_value = (sign == Sign::Minus) ? -1.0 : 1.0;
        v[nonzero_index] = nonzero_value;
        return v;
    }

    bool is_vertical() const {
        return axis == CardinalAxis::UpDown;
    }

    bool is_up() const {
        return is_vertical() && sign == Sign::Plus;
    }

    bool is_down() const {
        return is_vertical() && sign == Sign::Minus;
    }
};

enum class Part {
    Side = 0,
    Face = 1,
    Back = -1,
    Head = 2,
    Belly = -2
};

enum class BodyType {
    biped = 0,
    quadruped = 1
};

struct Word {
    std::string form;
    Axis axis;
    Sign sign;
    std::optional<Part> part;

    Word(
        std::string f,
        Axis a,
        Sign s,
        std::optional<Part> p = std::nullopt
    )
        : form(f), axis(a), sign(s), part(p) {}

    bool operator==(const Word& other) const {
        return form == other.form &&
               axis == other.axis &&
               sign == other.sign &&
               part == other.part;
    }

    bool has_part_meaning() const {
        return part.has_value();
    }

    bool is_vertical() const {
        return axis == Axis::Z;
    }
};

enum class Sense {
    Intrinsic = 0,
    Relative = 1,
    Absolute = 2
};

namespace std {
    template<>
    struct hash<Axis> {
        std::size_t operator()(const Axis& axis) const noexcept {
            return static_cast<std::size_t>(axis);
        }
    };

    template<>
    struct hash<Sign> {
        std::size_t operator()(const Sign& sign) const noexcept {
            return static_cast<std::size_t>(sign);
        }
    };

    template<>
    struct hash<Part> {
        std::size_t operator()(const Part& part) const noexcept {
            return static_cast<std::size_t>(part);
        }
    };

    template<>
    struct hash<Sense> {
        std::size_t operator()(Sense sense) const noexcept {
            return static_cast<std::size_t>(sense);
        }
    };

    template <>
    struct hash<Word> {
        std::size_t operator()(const Word& word) const {
            // Hash the components of Word (word_form, axis, sign, optional part) and combine them
            std::size_t h1 = std::hash<std::string>{}(word.form);
            std::size_t h2 = std::hash<Axis>{}(word.axis);
            std::size_t h3 = std::hash<Sign>{}(word.sign);

            std::size_t h4 = 0;
            if (word.part) {
                h4 = std::hash<Part>{}(*word.part);
            }

            // Combine the hashes in a way to avoid collisions
            return h1 ^ (h2 * 31) ^ (h3 * 131) ^ (h4 * 17);
        }
    };
}

using ReferenceDirection = std::optional<UnitVector>;
struct Object {
    using AxialDirections = std::unordered_map<Axis, ReferenceDirection>;
    using PartDirections = std::unordered_map<Part, ReferenceDirection>;

    // Basic properties
    Position position;

    AxialDirections axial_directions;
    PartDirections part_directions;

    Object(
        Position p,
        std::optional<Direction> upward = std::nullopt,
        std::optional<Direction> forward = std::nullopt,
        std::optional<Direction> rightward = std::nullopt,
        std::optional<BodyType> body_type = std::nullopt
    )
        : position(p)
    {
        // Set axial directions
        set_axial_direction(Axis::Z, upward);
        set_axial_direction(Axis::Y, forward);
        set_axial_direction(Axis::X, rightward);
        // Set body part directions
        if (body_type && has_all_axes()) {
            switch(*body_type) {
                case BodyType::biped:
                    set_part_directions_biped();
                    break;
                case BodyType::quadruped:
                    set_part_directions_quadruped();
                    break;
            }
        }
    }

    bool has_axis(Axis axis) const {
        return axial_directions.at(axis).has_value();
    }

    bool has_part(Part part) const {
        auto it = part_directions.find(part);
        if (it != part_directions.end()) {
            return it->second.has_value();
        }
        return false;
    }

    ReferenceDirection get_part_direction(Part part) const {
        auto it = part_directions.find(part);
        if (it != part_directions.end() && it->second.has_value()) {
            return it->second.value();
        }
        return std::nullopt;
    }

    bool has_all_axes() const {
        return has_axis(Axis::Z) && has_axis(Axis::Y) & has_axis(Axis::X);
    }

    bool upward_is_up() const {
        const ReferenceDirection& upward = axial_directions.at(Axis::Z);
        return upward ? (*upward).is_up() : false;
    }

    bool upward_is_down() const {
        const ReferenceDirection& upward = axial_directions.at(Axis::Z);
        return upward ? (*upward).is_down() : false;
    }

    bool axis_is_vertical(Axis axis) const {
        const ReferenceDirection& direction = axial_directions.at(axis);
        return direction ? (*direction).is_vertical() : false;
    }

    bool axis_is_horizontal(Axis axis) const {
        const ReferenceDirection& direction = axial_directions.at(axis);
        return direction ? !(*direction).is_vertical() : false;
    }

private:
    void set_axial_direction(Axis axis, std::optional<Direction> direction) {
        if (direction) {
            axial_directions.emplace(axis, UnitVector(*direction));
        } else {
            axial_directions.emplace(axis, std::nullopt);
        }
    }

    void set_part_directions_biped() {
        part_directions[Part::Head] = *(axial_directions.at(Axis::Z));
        part_directions[Part::Belly] = -(*(axial_directions.at(Axis::Z)));
        part_directions[Part::Face] = *(axial_directions.at(Axis::Y));
        part_directions[Part::Back] = -(*(axial_directions.at(Axis::Y)));
    }

    void set_part_directions_quadruped() {
        part_directions[Part::Head] = std::nullopt;
        part_directions[Part::Belly] = -(*(axial_directions.at(Axis::Z)));
        part_directions[Part::Face] = *(axial_directions.at(Axis::Y));
        part_directions[Part::Back] = *(axial_directions.at(Axis::Z));
    }
};

struct Context {
	Object ground;
	Object figure;
	Object speaker;
    Object environment;

    UnitVector g_to_f;
    double gf_distance;

        Context(const Object& g, const Object& f, const Object& s)
            : ground(g),
              figure(f),
              speaker(s),
              environment(Space::origin, Space::up)
        {
            const Displacement& gf_displacement = figure.position - ground.position;
            g_to_f = UnitVector(gf_displacement);
            gf_distance = magnitude(gf_displacement);
        }
};

enum class Transformation {
    AlignStandard,
    AlignMirrored,
    AlignVertical,
    Mimic
};

struct MyInput {
    Context context;
    std::string utterance;

    MyInput(const Context& c, std::string u)
        : context(c), utterance(u)
    {}
};

std::string to_string(const Vector& v) {
    std::ostringstream oss;
    oss << "[" << v[0] << ", " << v[1] << ", " << v[2] << "]";
    return oss.str();
}

std::string to_string(const UnitVector& v) {
    return to_string(v.to_vector());
}

std::string to_string(Axis axis) {
    switch (axis) {
        case Axis::X: return "X";
        case Axis::Y: return "Y";
        case Axis::Z: return "Z";
        default: return "Unknown";
    }
}

std::string to_string(Part part) {
    switch (part) {
        case Part::Head: return "head";
        case Part::Belly: return "belly";
        case Part::Face: return "face";
        case Part::Back: return "back";
        case Part::Side: return "side";
        default: return "Unknown";
    }
}

std::string to_string(const Object& obj) {
    std::ostringstream oss;
    oss << "Object(position=" << to_string(obj.position) << ", axial_directions={";

    for (const auto& [axis, direction] : obj.axial_directions) {
        oss << to_string(axis) << ": ";
        if (direction.has_value()) {
            oss << to_string(*direction);
        } else {
            oss << "None";
        }
        oss << ", ";
    }

    oss << "}, part_directions={";

    for (const auto& [part, direction] : obj.part_directions) {
        oss << to_string(part) << ": ";
        if (direction.has_value()) {
            oss << to_string(*direction);
        } else {
            oss << "None";
        }
        oss << ", ";
    }

    // Remove trailing comma and space if present
    auto str = oss.str();
    if (str.size() > 2) str.erase(str.size() - 2);

    oss << "})";
    return oss.str();
}

std::string to_string(const Context& ctx) {
    std::ostringstream oss;
    oss << "Context(\n  ground=" << to_string(ctx.ground)
        << ",\n  figure=" << to_string(ctx.figure)
        << ",\n  speaker=" << to_string(ctx.speaker)
        << ",\n  environment=" << to_string(ctx.environment)
        << ",\n  g_to_f=" << to_string(ctx.g_to_f)
        << "\n)";
    return oss.str();
}

struct Language {
    virtual const std::vector<Word>& get_words() const = 0;
    virtual const std::vector<Word>& get_part_words() const = 0;
    virtual const std::vector<Word>& get_nonpart_words() const = 0;
    virtual const std::vector<Sense>& get_senses() const = 0;
    virtual ~Language() = default;
};

class LanguageContext {
private:
    static std::shared_ptr<Language> current_language;

public:
    static void set_language(std::shared_ptr<Language> language) {
        current_language = language;
    }

    static const Language& get_language() {
        if (!current_language) {
            throw std::runtime_error("Language not set.");
        }
        return *current_language;
    }

    // Convenience getters
    static const std::vector<Word>& get_words() {
        return get_language().get_words();
    }

    static const std::vector<Word>& get_part_words() {
        return get_language().get_part_words();
    }

    static const std::vector<Word>& get_nonpart_words() {
        return get_language().get_nonpart_words();
    }

    static const std::vector<Sense>& get_senses() {
        return get_language().get_senses();
    }
};

struct English : Language {
    // NOTE: static ensures vectors only initialized once
    const std::vector<Word>& get_words() const override {
        static const std::vector<Word> words = {
            Word("above", Axis::Z, Sign::Plus),
            Word("below", Axis::Z, Sign::Minus),
            Word("front", Axis::Y, Sign::Plus),
            Word("behind", Axis::Y, Sign::Minus),
            Word("right", Axis::X, Sign::Plus),
            Word("left", Axis::X, Sign::Minus)
        };
        return words;
    }

    const std::vector<Word>& get_part_words() const override {
        // Return an empty vector since no words are considered "part words"
        static const std::vector<Word> part_words = {};
        return part_words;
    }

    const std::vector<Word>& get_nonpart_words() const override {
        // Return all words since English doesn't have "part words"
        static const std::vector<Word>& nonpart_words = get_words();
        return nonpart_words;
    }

    const std::vector<Sense>& get_senses() const override {
        static const std::vector<Sense> senses = {
            Sense::Intrinsic,
            Sense::Relative
        };
        return senses;
    }
};

struct Mixtec : Language {
    const std::vector<Word>& get_words() const override {
        static const std::vector<Word> words = {
            Word("head", Axis::Z, Sign::Plus, Part::Head),
            Word("belly", Axis::Z, Sign::Minus, Part::Belly),
            Word("face", Axis::Y, Sign::Plus, Part::Face),
            Word("back", Axis::Y, Sign::Minus, Part::Back),
            Word("right", Axis::X, Sign::Plus),
            Word("left", Axis::X, Sign::Minus)
        };
        return words;
    }

    const std::vector<Word>& get_part_words() const override {
        static const std::vector<Word> part_words = [] {
            const auto& words = Mixtec().get_words();
            std::vector<Word> result;
            std::copy_if(words.begin(), words.end(), std::back_inserter(result),
                         [](const Word& word) { return word.part.has_value(); });
            return result;
        }();
        return part_words;
    }

    const std::vector<Word>& get_nonpart_words() const override {
        static const std::vector<Word> nonpart_words = [] {
            const auto& words = Mixtec().get_words();
            std::vector<Word> result;
            std::copy_if(words.begin(), words.end(), std::back_inserter(result),
                         [](const Word& word) { return word.part.has_value(); });
            return result;
        }();
        return nonpart_words;
    }

    const std::vector<Sense>& get_senses() const override {
        static const std::vector<Sense> senses = {
            Sense::Intrinsic,
            Sense::Relative
        };
        return senses;
    }
};

struct WordSense {
    Word word;
    Sense sense;

    WordSense(Word w, Sense s) : word(std::move(w)), sense(s) {}

    bool operator==(const WordSense& other) const {
        return word == other.word && sense == other.sense;
    }
};

template <typename OuterKey, typename InnerKey>
struct KeyPairHash {
    std::size_t operator()(const std::pair<OuterKey, InnerKey>& p) const {
        std::size_t h1 = std::hash<OuterKey>{}(p.first);
        std::size_t h2 = std::hash<InnerKey>{}(p.second);
        // Combine the two hash values
        return h1 ^ (h2 << 1);
    }
};

