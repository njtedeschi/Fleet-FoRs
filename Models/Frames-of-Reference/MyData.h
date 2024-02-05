#pragma once
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>

#include "json.hpp"
#include "Scene.h"

using json = nlohmann::json;
using TruthValue = std::unordered_map<std::string,bool>;
using Label = std::unordered_map<std::string,TruthValue>;

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

struct TestingDatum {
    Scene scene;
    Label label;

    TestingDatum(Scene s, Label l) : scene(s), label(l) {}
};

class MyData {
public:
    std::unordered_map<std::string,BodyPartMeaning> word_meanings;
    std::vector<std::string> words;

    MyData(const std::unordered_map<std::string,BodyPartMeaning> wm) : word_meanings(wm) {
        for (const auto& [word, meaning] : word_meanings) {
            words.push_back(word);
        }
    }

    std::vector<MyInput> json_file_to_training_data(const std::string& filename) {
        json j_object = json_file_to_item(filename);
        return json_item_to_training_data(j_object);
    }

    std::vector<TestingDatum> json_file_to_testing_data(const std::string& filename) {
        json j_object = json_file_to_item(filename);
        return json_item_to_testing_data(j_object);
    }

private:
    // Parse file
    json json_file_to_item(const std::string& filename) {
        std::ifstream file(filename);
        json j_object;
        j_object = json::parse(file);
        return j_object;
    }

    // Parse data list
    std::vector<MyInput> json_item_to_training_data(json& j_object){
        std::vector<MyInput> training_data;
        for (auto& [key, val] : j_object.items()) {
            // std::cout << "key: " << key << ", value:" << val << '\n';
            training_data.emplace_back(json_item_to_my_input(val));
        }
        return training_data;
    }

    std::vector<TestingDatum> json_item_to_testing_data(json& j_object){
        std::vector<TestingDatum> testing_data;
        for (auto& [key, val] : j_object.items()) {
            // std::cout << "key: " << key << ", value:" << val << '\n';
            testing_data.emplace_back(json_item_to_testing_datum(val));
        }
        return testing_data;
    }

    // Parse datum
    MyInput json_item_to_my_input(json j){
        Scene scene = json_item_to_scene(j["scene"]);
        std::string description = j["description"];
        BodyPartMeaning* meaning = &(word_meanings[description]);
        return MyInput(scene, description, meaning);
    }

    TestingDatum json_item_to_testing_datum(const json& j) {
        Scene scene = json_item_to_scene(j["scene"]);
        std::unordered_map<std::string, TruthValue> label = json_item_to_label(j["label"]);
        return TestingDatum(scene, label);
    }

    // Parse Scene
    Scene json_item_to_scene(json j){
        OrientedObject speaker = json_item_to_oriented_object(j["speaker"]);
        OrientedObject ground = json_item_to_oriented_object(j["ground"]);
        BaseObject figure = json_item_to_base_object(j["figure"]);
        return Scene(speaker, ground, figure);
    }

    BaseObject json_item_to_base_object(json j) {
        Position position = json_item_to_vector<Position>(j["position"]);
        return BaseObject(position);
    }

    OrientedObject json_item_to_oriented_object(json j){
        Position position = json_item_to_vector<Position>(j["position"]);
        Direction forward = json_item_to_vector<Direction>(j["forward"]);
        Direction upward = json_item_to_vector<Direction>(j["upward"]);
        Direction rightward = json_item_to_vector<Direction>(j["rightward"]);
        bool is_participant = j["is_participant"];
        BodyType body_type = j["body_type"] == "biped" ? BodyType::biped : BodyType::quadruped;
        return OrientedObject(position, forward, upward, rightward, is_participant, body_type);
    }

    template <typename T>
    T json_item_to_vector(json j){
        Vector vector = {j.at(0), j.at(1), j.at(2)};
        return T(vector);
    }

    // Parse testing label
    Label json_item_to_label(const json& j) {
        Label label;
        for (auto& item : j.items()) {
            label[item.key()] = json_to_truth_value(item.value());
        }
        return label;
    }

    TruthValue json_to_truth_value(const json& j) {
        TruthValue truth_value;
        for (auto& item : j.items()) {
            truth_value[item.key()] = item.value();
        }
        return truth_value;
    }
};