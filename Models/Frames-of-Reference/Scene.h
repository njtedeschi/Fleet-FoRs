#pragma once

#include <array>
#include <cmath>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>

using Vector = std::array<double,3>;
struct Displacement : public Vector {};
struct Direction : public Vector {
    bool bidirectional = false;

    Direction() = default;
    Direction(const Vector& v) : Vector(v) {}
    Direction(const Vector& v, bool bd) : Vector(v), bidirectional(bd) {}
    Direction(std::initializer_list<double> init, bool bd = false) {
        std::copy(init.begin(), init.end(), this->begin());
        this->bidirectional = bd;
    }

    Direction& operator=(const Vector& v) {
        static_cast<Vector&>(*this) = v;
        return *this;
    }
    Direction& operator=(std::initializer_list<double> init) {
        std::copy(init.begin(), init.end(), this->begin());
        return *this;
    }
};
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

// Really should be something like Position(Position, Displacement)
// Hack to create "corner" positions
Position operator+(const Position& v1, const Position& v2) {
    Position result;
    for (int i = 0; i < 3; ++i) {
        result[i] = v1[i] + v2[i];
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

Direction cross_product(const Direction &a, const Direction &b){
    Direction result;
    result[0] = a[1] * b[2] - a[2] * b[1];
    result[1] = a[2] * b[0] - a[0] * b[2];
    result[2] = a[0] * b[1] - a[1] * b[0];
    return result;
}

struct BaseObject {
    Position position;
    bool is_participant;

    // Default constructor
    BaseObject() : position({0, 0, 0}) {}

    // Construct from position
    BaseObject(const Position& p)
    : position(p) {}
};

enum class BodyType {
    biped = 0,
    quadruped = 1
};

struct OrientedObject : public BaseObject {
    Direction forward;
    Direction upward;
    Direction rightward;

    bool is_participant;
    BodyType body_type;

    // Default constructor
    OrientedObject() : BaseObject(), forward({0, 0, 0}), upward({0, 0, 0}), rightward({0, 0, 0}), is_participant(false), body_type(BodyType::biped) {}

    // Calculate rightward from forward and upward
    OrientedObject(const Position& p, const Direction& f, const Direction& u, const Direction& r, bool is_p = false, BodyType bt = BodyType::biped)
    : BaseObject(p), forward(f), upward(u), rightward(r), is_participant(is_p), body_type(bt) {}
};

struct Scene {
	OrientedObject speaker;
	OrientedObject ground;
	BaseObject figure;
        Displacement g_to_f; // calculated once during construction

        // Default constructor using default objects
        Scene() : speaker(), ground(), figure() {
            g_to_f = figure.position - ground.position;
        }
        Scene(const OrientedObject& s, const OrientedObject& g, const BaseObject& f)
            : speaker(s), ground(g), figure(f) {
                g_to_f = figure.position - ground.position;
            }

        void print() const {
            int label_width = 10;
            std::cout << std::left << std::setw(label_width) << "speaker: " << std::left << std::setw(label_width) << to_string(speaker.position) << to_string(speaker.forward) << std::endl;

            std::cout << std::left << std::setw(label_width) << "ground: " << std::left << std::setw(label_width) << to_string(ground.position) << to_string(ground.forward) << std::endl;

            std::cout << std::left << std::setw(label_width) << "figure: " << std::left << std::setw(label_width) << to_string(figure.position) << std::endl;
        }
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
