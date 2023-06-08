#pragma once

#include <array>
#include <cmath>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>

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
    for(int i = 0; i < a.size(); i++){
        result += a[i] * b[i];
    }
    return result;
}

double magnitude(const Vector &a){
    double result = 0.0;
    for (int i = 0; i < a.size(); i++){
        result += a[i] * a[i];
    }
    return sqrt(result);
}

double cosine_similarity(const Vector &a, const Vector &b){
    return dot_product(a, b)/(magnitude(a) * magnitude(b));
}

double angle_between(){
    // TODO: implement
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
    BaseObject() : position({0, 0, 0}), is_participant(false) {}

    // Construct from position
    BaseObject(const Position& position, bool is_participant=false)
    : position(position), is_participant(is_participant) {}
};

struct OrientedObject : public BaseObject {
    Direction forward;
    Direction upward;
    Direction rightward;

    // Default constructor
    OrientedObject() : BaseObject(), forward({0, 0, 0}), upward({0, 0, 0}), rightward({0, 0, 0}) {}

    // Calculate rightward from forward and upward
    OrientedObject(const Position& position, const Direction& forward, const Direction& upward = {0, 0, 1}, bool is_participant = false)
    : BaseObject(position, is_participant, is_valid), forward(forward), upward(upward) {
        rightward = cross_product(forward, upward);
    }

    // Constructor with is_participant parameter
    OrientedObject(const Position& position, const Direction& forward, bool is_participant, const Direction& upward = {0, 0, 1})
    : BaseObject(position, is_participant), forward(forward), upward(upward) {
        rightward = cross_product(forward, upward);
    }
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
        Scene(const OrientedObject& speaker, const OrientedObject& ground, const BaseObject& figure)
            : speaker(speaker), ground(ground), figure(figure) {
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
    Position nondirect_speaker_spot = {-2,0,0};
    std::vector<Position> figure_positions_axis = {east_spot, west_spot, north_spot, south_spot, up_spot, down_spot};

    // Corner positions
    std::vector<Position> figure_positions_off_axis;
    // TODO: figure out where to keep generating loop (currently in generate_scenes)

    Direction null_vector = {0,0,0};
    Direction east = {1,0,0};
    Direction west = {-1,0.0};
    Direction north = {0,1,0};
    Direction south = {0,-1,0};
    Direction up = {0,0,1};
    Direction down = {0,0,-1};
    /* std::vector<Direction> ground_directions = {east, west, north, south, up, down}; */
    std::vector<Direction> ground_directions = {east, west, north, south};

    // Possible speakers
    OrientedObject direct_speaker = {origin, east, true};
    OrientedObject nondirect_speaker = {nondirect_speaker_spot, east, true};

    // Possible figures
    BaseObject east_figure = {east_spot};
    BaseObject west_figure = {west_spot};
    BaseObject north_figure = {north_spot};
    BaseObject south_figure = {south_spot};
    BaseObject up_figure = {up_spot};
    BaseObject down_figure = {down_spot};
    std::vector<BaseObject> figures = {east_figure, west_figure, north_figure, south_figure, up_figure, down_figure};
    /* std::vector<BaseObject> figures = {east_figure, west_figure, north_figure, south_figure}; */

    // Possible grounds
    OrientedObject east_facing_ground = {origin, east};
    OrientedObject west_facing_ground = {origin, west};
    OrientedObject north_facing_ground = {origin, north};
    OrientedObject south_facing_ground = {origin, south};
    std::vector<OrientedObject> grounds = {east_facing_ground, west_facing_ground, north_facing_ground, south_facing_ground};

    // Possible grounds (listener)
    OrientedObject listener_east_facing_ground = {origin, east, true};
    OrientedObject listener_west_facing_ground = {origin, west, true};
    OrientedObject listener_north_facing_ground = {origin, north, true};
    OrientedObject listener_south_facing_ground = {origin, south, true};
    std::vector<OrientedObject> listener_grounds = {listener_east_facing_ground, listener_west_facing_ground, listener_north_facing_ground, listener_south_facing_ground};
}

    // Possible direct and nondirect scenes
std::vector<Scene> direct_scenes;
std::vector<Scene> nondirect_scenes;
std::vector<Scene> listener_nondirect_scenes;

void generate_scenes(){
    for (const auto& figure : Space::figures) {
        Scene direct = {Space::direct_speaker, Space::direct_speaker, figure};
        direct_scenes.push_back(direct);
        for (const auto& ground : Space::grounds) {
            Scene nondirect = {Space::nondirect_speaker, ground, figure};
            nondirect_scenes.push_back(nondirect);
        }
        for (const auto& ground : Space::listener_grounds) {
            Scene listener_nondirect = {Space::nondirect_speaker, ground, figure};
            listener_nondirect_scenes.push_back(listener_nondirect);
        }
    }

    // Create off-axis positions
    float sqrt_val = sqrt(2)/2;
    for(int i = 0; i < 3; i++) {
        for (int sign1 = -1; sign1 <= 1; sign1 += 2) {
            for (int sign2 = -1; sign2 <= 1; sign2 += 2) {
                Position p = {0,0,0};
                p[(i+1) % 3] = sign1 * sqrt_val;
                p[(i+2) % 3] = sign2 * sqrt_val;
                Space::figure_positions_off_axis.push_back(p);
            }
        }
    }
}
