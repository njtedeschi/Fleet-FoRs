#pragma once

#include <array>
#include <cmath>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <optional>
#include <unordered_map>

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

struct Word {
    std::string form;
    Axis axis;
    bool is_positive;

    Word(
        std::string f,
        Axis a,
        bool is_p
    )
        : form(f), axis(a), is_positive(is_p) {}

    bool operator==(const Word& other) const {
        return form == other.form &&
               axis == other.axis &&
               is_positive == other.is_positive;
    }

};

enum class Sense {
    Intrinsic = 0,
    Relative = 1,
    Absolute = 2
};

// Custom hash functions for Word, Axis, and Sense
namespace std {
    template <>
    struct hash<Word> {
        std::size_t operator()(const Word& word) const {
            // Hash the components of Word (word_form, axis, is_positive) and combine them
            std::size_t h1 = std::hash<std::string>{}(word.form);
            std::size_t h2 = std::hash<int>{}(static_cast<int>(word.axis));
            std::size_t h3 = std::hash<bool>{}(word.is_positive);

            // Combine the hashes in a way to avoid collisions
            return h1 ^ (h2 * 31) ^ (h3 * 131);
        }
    };

    template<>
    struct hash<Axis> {
        std::size_t operator()(const Axis& axis) const noexcept {
            return static_cast<std::size_t>(axis);
        }
    };

    template<>
    struct hash<Sense> {
        std::size_t operator()(Sense sense) const noexcept {
            return static_cast<std::size_t>(sense);
        }
    };
}


struct Object {
    using ReferenceDirection = std::optional<Direction>;
    using AxialDirections = std::unordered_map<Axis, ReferenceDirection>;

    // Basic properties
    Position position;

    AxialDirections axial_directions;

    Object(
        Position p,
        ReferenceDirection upward = std::nullopt,
        ReferenceDirection forward = std::nullopt,
        ReferenceDirection rightward = std::nullopt
    )
        : position(p)
    {
        axial_directions[Axis::Z] = upward;
        axial_directions[Axis::Y] = forward;
        axial_directions[Axis::X] = rightward;
    }

    bool has_axis(Axis axis) const {
        return axial_directions.at(axis).has_value();
    }

    bool has_all_axes() const {
        return has_axis(Axis::Z) && has_axis(Axis::Y) & has_axis(Axis::X);
    }

    bool upward_is_up() const {
        ReferenceDirection upward = axial_directions.at(Axis::Z);
        if (upward) {
            return *upward == Space::up;
        }
        return false;
    }

    bool upward_is_down() const {
        ReferenceDirection upward = axial_directions.at(Axis::Z);
        if (upward) {
            return *upward == Space::down;
        }
        return false;
    }

    bool upward_is_vertical() const {
        ReferenceDirection upward = axial_directions.at(Axis::Z);
        if (upward) {
            return *upward == Space::up || *upward == Space::down;
        }
        return false;
    }

    bool upward_is_horizontal() const {
        ReferenceDirection upward = axial_directions.at(Axis::Z);
        if (upward) {
            return dot_product(*upward, Space::up) == 0;
        }
        return false;
    }

    bool forward_is_vertical() const {
        ReferenceDirection forward = axial_directions.at(Axis::Y);
        if (forward) {
            return *forward == Space::up || *forward == Space::down;
        }
        return false;
    }

    bool rightward_is_vertical() const {
        ReferenceDirection rightward = axial_directions.at(Axis::X);
        if (rightward) {
            return *rightward == Space::up || *rightward == Space::down;
        }
        return false;
    }

};

struct Context {
	Object ground;
	Object figure;
	Object speaker;
    Object environment;
    Displacement g_to_f;

        Context(const Object& g, const Object& f, const Object& s)
            : ground(g), figure(f), speaker(s), environment(Space::origin, Space::up)
        {
            g_to_f = figure.position - ground.position;
        }
};

enum class Transformation {
    AlignStandard,
    AlignMirrored,
    AlignVertical
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

std::string to_string(Axis axis) {
    switch (axis) {
        case Axis::X: return "X";
        case Axis::Y: return "Y";
        case Axis::Z: return "Z";
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
