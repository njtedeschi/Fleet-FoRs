#pragma once
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>

#include "json.hpp"
#include "Ch3MyStructures.h"

using json = nlohmann::json;

class MyData {
public:
    const std::vector<Word> words;

    MyData(const std::vector<Word>& w) : words(w) {}

    std::vector<MyInput> json_file_to_training_data(const std::string& filename) {
        json j_object = json_file_to_item(filename);
        return json_item_to_training_data(j_object);
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
            training_data.emplace_back(json_item_to_datum(val));
        }
        return training_data;
    }

    // Parse datum
    MyInput json_item_to_datum(json j){
        const Context& context = json_item_to_context(j["scene"]);
        std::string utterance = j["description"];
        return MyInput(context, utterance);
    }

    // Parse Scene
    Context json_item_to_context(json j){
        const Object& ground = json_item_to_object(j["ground"], true);
        const Object& figure = json_item_to_object(j["figure"], false);
        const Object& speaker = json_item_to_object(j["speaker"], true);
        return Context(ground, figure, speaker);
    }

    // Parse Object
    Object json_item_to_object(json j, bool include_orientation) {
        const Position& position = json_item_to_vector<Position>(j["position"]);
        if (include_orientation) {
            const Direction& upward = json_item_to_vector<Direction>(j["upward"]);
            const Direction& forward = json_item_to_vector<Direction>(j["forward"]);
            const Direction& rightward = json_item_to_vector<Direction>(j["rightward"]);
            return Object(position, upward, forward, rightward);
        }
        return Object(position);
    }

    template <typename T>
    T json_item_to_vector(json j){
        Vector vector = {j.at(0), j.at(1), j.at(2)};
        return T(vector);
    }
};