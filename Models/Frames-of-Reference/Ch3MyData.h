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

struct ContextProperties {
    std::string intrinsic_description;
    std::string relative_description;
    bool ground_x_canonical;
    bool ground_y_canonical;
    bool ground_z_canonical;

    // Mapping function for description conversion
    static std::string abbreviate_description(const std::string& input) {
        static const std::unordered_map<std::string, std::string> abbreviation_map = {
            {"above", "z+"}, {"below", "z-"},
            {"front", "y+"}, {"behind", "y-"},
            {"right", "x+"}, {"left", "x-"}
        };

        auto it = abbreviation_map.find(input);
        return (it != abbreviation_map.end()) ? it->second : input; // Default to input if not found
    }

    // Constructor with mapping logic
    ContextProperties(const std::string& i,
                      const std::string& r,
                      bool x,
                      bool y,
                      bool z)
        :
          intrinsic_description(abbreviate_description(i)),
          relative_description(abbreviate_description(r)),
          ground_x_canonical(x),
          ground_y_canonical(y),
          ground_z_canonical(z)
    {}
};

struct UniqueTestingDatum {
    Context context;
    int uid;
    ContextProperties properties;

    UniqueTestingDatum(Context c, int u, ContextProperties p) : context(c), uid(u), properties(p) {}
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

    std::vector<UniqueTestingDatum> json_file_to_unique_testing_data(const std::string& filename) {
        json j_object = json_file_to_item(filename);
        return json_item_to_unique_testing_data(j_object);
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

    std::vector<UniqueTestingDatum> json_item_to_unique_testing_data(json& j_object){
        std::vector<UniqueTestingDatum> testing_data;
        for (auto& [key, val] : j_object.items()) {
            // std::cout << "key: " << key << ", value:" << val << '\n';
            testing_data.emplace_back(json_item_to_unique_testing_datum(val));
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

    UniqueTestingDatum json_item_to_unique_testing_datum(const json& j) {
        const Context& context = json_item_to_context(j["scene"]);
        const int& uid = j["uid"];
        const ContextProperties& properties = json_item_to_context_properties(j["properties"]);
        return UniqueTestingDatum(context, uid, properties);
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
            BodyType body_type = j["body_type"] == "biped" ? BodyType::biped : BodyType::quadruped;
            return Object(position, upward, forward, rightward, body_type);
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

    ContextProperties json_item_to_context_properties(const json& j) {
        return ContextProperties(
            j["intrinsic_description"],
            j["relative_description"],
            j["ground_x_canonical"],
            j["ground_y_canonical"],
            j["ground_z_canonical"]
        );
    }
};