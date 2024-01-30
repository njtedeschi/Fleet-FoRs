#pragma once
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>

#include "json.hpp"
#include "Scene.h"

enum class BodyPartNoun {
    none = 0,
    head = 1,
    belly = 2,
    face = 3,
    back = 4,
    flank = 5
};

struct BodyPartMeaning {
    BodyPartNoun body_part_noun;
    std::function<Direction(OrientedObject&)> body_part_direction;

    // No argument constructor
    BodyPartMeaning() : body_part_noun(BodyPartNoun::none), body_part_direction([](const OrientedObject& a) -> Direction {return {0,0,0};}) {}

    BodyPartMeaning(BodyPartNoun bpn, std::function<Direction(const OrientedObject&)> bpd) :
        body_part_noun(bpn), body_part_direction(bpd) {}
};

struct MyInput {
    Scene scene;
    std::string description;
    BodyPartMeaning* meaning;

    MyInput(Scene s, std::string d, BodyPartMeaning* m) : scene(s), description(d), meaning(m) {}
};

struct MyData {
    std::unordered_map<std::string,BodyPartMeaning> word_meanings;
    std::vector<std::string> words;

    MyData(const std::unordered_map<std::string,BodyPartMeaning> wm) : word_meanings(wm) {
        for (const auto& [word, meaning] : word_meanings) {
            words.push_back(word);
        }
    }

    template <typename T>
    T json_item_to_vector(nlohmann::json j){
        Vector vector = {j.at(0), j.at(1), j.at(2)};
        return T(vector);
    }

    BaseObject json_item_to_base_object(nlohmann::json j) {
        // for (auto& [key, val] : j.items())
        // {
        //     std::cout << "key: " << key << ", value:" << val << '\n';
        // }
        Position position = json_item_to_vector<Position>(j["position"]);
        return BaseObject(position);
    }

    OrientedObject json_item_to_oriented_object(nlohmann::json j){
        // for (auto& [key, val] : j.items())
        // {
        //     std::cout << "key: " << key << ", value:" << val << '\n';
        // }
        Position position = json_item_to_vector<Position>(j["position"]);
        Direction forward = json_item_to_vector<Direction>(j["forward"]);
        Direction upward = json_item_to_vector<Direction>(j["upward"]);
        Direction rightward = json_item_to_vector<Direction>(j["rightward"]);
        bool is_participant = j["is_participant"];
        BodyType body_type = j["body_type"] == "biped" ? BodyType::biped : BodyType::quadruped;
        return OrientedObject(position, forward, upward, rightward, is_participant, body_type);
    }

    Scene json_item_to_scene(nlohmann::json j){
        // for (auto& [key, val] : j.items())
        // {
        //     std::cout << "key: " << key << ", value:" << val << '\n';
        // }
        OrientedObject speaker = json_item_to_oriented_object(j["speaker"]);
        OrientedObject ground = json_item_to_oriented_object(j["ground"]);
        BaseObject figure = json_item_to_base_object(j["figure"]);
        return Scene(speaker, ground, figure);
    }

    MyInput json_item_to_my_input(nlohmann::json j){
        // for (auto& [key, val] : j.items())
        // {
        //     std::cout << "key: " << key << ", value:" << val << '\n';
        // }
        Scene scene = json_item_to_scene(j["scene"]);
        std::string description = j["description"];
        BodyPartMeaning* meaning = &(word_meanings[description]);
        return MyInput(scene, description, meaning);
    }

    std::vector<MyInput> json_file_to_data(const std::string& filename){
        std::ifstream file(filename);
        nlohmann::json j_object;
        j_object = nlohmann::json::parse(file);

        std::vector<MyInput> training_data;

        for (auto& [key, val] : j_object.items())
        {
            // std::cout << "key: " << key << ", value:" << val << '\n';
            MyInput datum = json_item_to_my_input(val);
            training_data.push_back(datum);
        }
        return training_data;
    }
};