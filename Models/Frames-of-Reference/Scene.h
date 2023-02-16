#pragma once

#include <array>
#include <cmath>
#include <vector>

using Vector = std::array<double,3>;

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

struct Object {
	Vector location;
	Vector orientation;
	/* std::string name; */ 
};

struct Scene {
	Object speaker;
	Object ground;
	Object figure; 
};

namespace {
    static Vector origin = {0,0,0};
    static Vector east = {1,0,0};
    static Vector west = {-1,0.0};
    static Vector north = {0,1,0};
    static Vector south = {0,-1,0};
    static Vector up = {0,0,1};
    static Vector down = {0,0,-1};

    // Possible speakers
    static Object direct_speaker = {origin, east};
    static Object nondirect_speaker = {2 * west, east};

    // Possible figures
    static Object east_figure = {east, origin};
    static Object west_figure = {west, origin};
    static Object north_figure = {north, origin};
    static Object south_figure = {south, origin};
    static Object up_figure = {up, origin};
    static Object down_figure = {down, origin};
    static std::vector<Object> figures = {east_figure, west_figure, north_figure, south_figure, up_figure, down_figure};

    // Possible grounds
    static Object east_facing_ground = {origin, east};
    static Object west_facing_ground = {origin, west};
    static Object north_facing_ground = {origin, north};
    static Object south_facing_ground = {origin, south};
    static std::vector<Object> grounds = {east_facing_ground, west_facing_ground, north_facing_ground, south_facing_ground};
}

    // Possible direct and nondirect scenes
std::vector<Scene> direct_scenes;
std::vector<Scene> nondirect_scenes;

void generate_scenes(){
    for (const auto& figure : figures) {
        Scene direct = {direct_speaker, direct_speaker, figure};
        direct_scenes.push_back(direct);
        for (const auto& ground : grounds) {
            Scene nondirect = {nondirect_speaker, ground, figure};
            nondirect_scenes.push_back(nondirect);
        }
    }
}
