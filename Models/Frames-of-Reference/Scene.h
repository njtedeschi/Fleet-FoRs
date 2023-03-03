#pragma once

#include <array>
#include <cmath>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>

using Vector = std::array<double,3>;

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

Vector operator-(const Vector& v1, const Vector& v2) {
    Vector result;
    for (int i = 0; i < 3; ++i) {
        result[i] = v1[i] - v2[i];
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

Vector cross_product(const Vector &a, const Vector &b){
    Vector result;
    result[0] = a[1] * b[2] - a[2] * b[1];
    result[1] = a[2] * b[0] - a[0] * b[2];
    result[2] = a[0] * b[1] - a[1] * b[0];
    return result;
}

struct Object {
	Vector location;
	Vector forward;
        Vector upward;
        Vector rightward;
	/* std::string name; */

        // Default constructor
        Object() : location({0,0,0}), forward({0,0,0}), upward({0,0,0}), rightward({0,0,0}) {}
        // Calculate rightward from forward and upward
        Object(const Vector& location, const Vector& forward, const Vector& upward = {0, 0, 1})
        : location(location), forward(forward), upward(upward) {
            rightward = cross_product(forward, upward);
        }
};

struct Scene {
	Object speaker;
	Object ground;
	Object figure; 

        // Default constructor using default objects
        Scene() : speaker(), ground(), figure() {}
        Scene(const Object& speaker, const Object& ground, const Object& figure)
            : speaker(speaker), ground(ground), figure(figure) {}

        void print() const {
            int label_width = 10;
            std::cout << std::left << std::setw(label_width) << "speaker: " << std::left << std::setw(label_width) << to_string(speaker.location) << to_string(speaker.forward) << std::endl;

            std::cout << std::left << std::setw(label_width) << "ground: " << std::left << std::setw(label_width) << to_string(ground.location) << to_string(ground.forward) << std::endl;

            std::cout << std::left << std::setw(label_width) << "figure: " << std::left << std::setw(label_width) << to_string(figure.location) << std::endl;
        }
};

namespace Space {
    Vector origin = {0,0,0};
    Vector east = {1,0,0};
    Vector west = {-1,0.0};
    Vector north = {0,1,0};
    Vector south = {0,-1,0};
    Vector up = {0,0,1};
    Vector down = {0,0,-1};

    // Possible speakers
    Object direct_speaker = {origin, east};
    Object nondirect_speaker = {2 * west, east};

    // Possible figures
    Object east_figure = {east, origin};
    Object west_figure = {west, origin};
    Object north_figure = {north, origin};
    Object south_figure = {south, origin};
    Object up_figure = {up, origin};
    Object down_figure = {down, origin};
    std::vector<Object> figures = {east_figure, west_figure, north_figure, south_figure, up_figure, down_figure};

    // Possible grounds
    Object east_facing_ground = {origin, east};
    Object west_facing_ground = {origin, west};
    Object north_facing_ground = {origin, north};
    Object south_facing_ground = {origin, south};
    std::vector<Object> grounds = {east_facing_ground, west_facing_ground, north_facing_ground, south_facing_ground};
}

    // Possible direct and nondirect scenes
std::vector<Scene> direct_scenes;
std::vector<Scene> nondirect_scenes;

void generate_scenes(){
    for (const auto& figure : Space::figures) {
        Scene direct = {Space::direct_speaker, Space::direct_speaker, figure};
        direct_scenes.push_back(direct);
        for (const auto& ground : Space::grounds) {
            Scene nondirect = {Space::nondirect_speaker, ground, figure};
            nondirect_scenes.push_back(nondirect);
        }
    }
}
