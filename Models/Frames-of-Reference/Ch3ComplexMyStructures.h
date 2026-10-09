#pragma once

#include <array>
#include <cmath>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include<optional>

using Vector = std::array<double,3>;
struct Displacement : public Vector {};
struct Direction : public Vector {};
struct Position : public Vector {};

std::string to_string(const Vector& v) {
    std::ostringstream oss;
    oss << "[" << v[0] << "," << v[1] << "," << v[2] << "]";
    return oss.str();
}

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

enum class Axis {
    X = 0,
    Y = 1,
    Z = 2
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
    std::string word_form;
    Axis axis;
    bool is_positive;
    std::optional<Part> part;

    Word(
        std::string wf,
        Axis a,
        bool is_p,
        std::optional<Part> p = std::nullopt
    )
        : word_form(wf), axis(a), is_positive(is_p), part(p) {}

};

Word above_english  = {"above", Axis::Z, true};
Word below_english  = {"below", Axis::Z, false};
Word front_english  = {"above", Axis::Y, true};
Word behind_english  = {"below", Axis::Y, false};
Word right_english  = {"above", Axis::X, true};
Word left_english  = {"below", Axis::X, false};

enum class Sense {
    Intrinsic = 0,
    Relative = 1,
    Absolute = 2
};

struct Object {
    using ReferenceDirection = std::optional<Direction>;
    using AxialDirections = std::unordered_map<Axis, ReferenceDirection>;
    using PartDirections = std::unordered_map<Part, ReferenceDirection>;

    // Basic properties
    Position position;
    bool is_participant;

    AxialDirections axial_directions;
    std::optional<PartDirections> part_directions;

    Object(
        const Position& p,
        bool is_p = false,
        const std::optional<Direction>& upward = std::nullopt,
        const std::optional<Direction>& forward = std::nullopt,
        const std::optional<Direction>& rightward = std::nullopt,
        std::optional<BodyType> body_type = std::nullopt
    )
        : position(p), is_participant(is_p)
    {
        axial_directions[Axis::Z] = upward;
        axial_directions[Axis::Y] = forward;
        axial_directions[Axis::X] = rightward;
        if (body_type.has_value() && has_all_axes()) {
            switch (body_type.value()) {
                case BodyType::biped:
                    part_directions = biped_part_directions();
                    break;
                case BodyType::quadruped:
                    part_directions = quadruped_part_directions();
                    break;
            }
        }
    }

    bool has_axis(Axis axis) const {
        return axial_directions.at(axis).has_value();
    }

    bool has_all_axes() const {
        return has_axis(Axis::Z) && has_axis(Axis::Y) & has_axis(Axis::X);
    }

    const ReferenceDirection& get_axial_direction(Axis axis, bool is_positive) {
        const ReferenceDirection& direction = axial_directions.at(axis);
        if (direction.has_value()) {
            return is_positive ? direction.value() : -direction.value();
        }
        return std::nullopt;
    }

    const ReferenceDirection& get_part_direction(Part part) {
        if (part_directions.has_value()) {
            const ReferenceDirection& direction = part_directions.value().at(part);
            if (direction.has_value()) {
                return direction.value();
            }
        }
        return std::nullopt;
    }

private:
    const PartDirections& biped_part_directions() {
        PartDirections directions;
        directions[Part::Head] = axial_directions.at(Axis::Z).value();
        directions[Part::Belly] = -axial_directions.at(Axis::Z).value();
        directions[Part::Face] = axial_directions.at(Axis::Y).value();
        directions[Part::Back] = -axial_directions.at(Axis::Y).value();
    }

    const PartDirections& quadruped_part_directions() {
        PartDirections directions;
        directions[Part::Head] = std::nullopt;
        directions[Part::Belly] = -axial_directions.at(Axis::Z).value();
        directions[Part::Face] = axial_directions.at(Axis::Y).value();
        directions[Part::Back] = axial_directions.at(Axis::Z).value();
    }
};

struct Context {
	const Object& ground;
	const Object& figure;
	const Object& speaker;
    Object environment;
    Displacement g_to_f;

        Context(const Object& g, const Object& f, const Object& s)
            : ground(g), figure(f), speaker(s), environment(Position({0, 0, 0}), false, Direction({0, 0, 1}))
        {
            g_to_f = figure.position - ground.position;
        }
};

struct Datum {
    std::string utterance;
    const Context& context;

    Datum(const std::string u, const Context& c)
        : utterance(u), context(c)
    {}
};

enum class Transformation {
    AlignStandard,
    AlignMirrored,
    AlignVertical,
    AlignCanonical,
    Mimic
};

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
