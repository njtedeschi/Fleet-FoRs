#pragma once
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>

#include "json.hpp"
#include "Ch3MyStructures.h"

using json = nlohmann::json;

// TODO: replace Ch 2 testing equivalents
using TruthValue = std::unordered_map<std::string, bool>;
using Label = std::unordered_map<std::string, TruthValue>;

struct TestingDatum {
    Context context;
    Label label;

    TestingDatum(Context c, Label l) : context(c), label(l) {}
};

class MyData {
public:
    // Note: this isn't currently used. It's a copy-paste relic
    const std::vector<Word> words;

    MyData(const std::vector<Word>& w) : words(w) {}

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
            training_data.emplace_back(json_item_to_datum(val));
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
    MyInput json_item_to_datum(json j){
        const Context& context = json_item_to_context(j["scene"]);
        std::string utterance = j["description"];
        return MyInput(context, utterance);
    }

    TestingDatum json_item_to_testing_datum(const json& j) {
        const Context& context = json_item_to_context(j["scene"]);
        const Label& label = json_item_to_label(j["label"]);
        return TestingDatum(context, label);
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