#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <set>
#include <unordered_map>

static const double alpha_t = 0.95;

#include "../Scene.h"

struct MyInput {
    Scene scene;
    std::string word;
    bool true_description;
};

#include "../MyGrammar.h"
#include "../MyHypothesis.h"
#include "../MyData.h"

#include "../Concepts.h"

#include "TopN.h"
#include "ParallelTempering.h"
#include "Fleet.h" 
#include "Random.h"
#include "Builtins.h"

// Necessary for "MyHypothesis.h" to compile because it references an extern MyHypothesis object called target
MyHypothesis target;

class MyDataFixture {
    public:
        MyDataFixture() {
            std::unordered_map<std::string, std::string> formulas = {
            {"above", Concepts::above_abs},
            {"below", Concepts::below_abs},
            {"front", Concepts::front_int_rel},
            {"behind", Concepts::behind_int_rel},
            {"side", Concepts::side_int}
        };
            mydata = MyData(formulas);
        }
        MyData mydata;
};

/* TEST_CASE_METHOD(MyDataFixture, "MyData::words initializes from formulas map correctly" ) { */
/*     std::vector<std::string> words = {"above", "below", "front", "behind", "side"}; */
/*     REQUIRE(mydata.words == words); */
/* } */

TEST_CASE_METHOD(MyDataFixture, "MyData::compute_true_words returns expected words for nondirect scenes" ) {
    // TODO: better names for scenes later
    Scene scene1 = {Space::nondirect_speaker, Space::north_facing_ground, Space::east_figure};
    REQUIRE(mydata.compute_true_words(mydata.target, scene1) == std::set<std::string>{"behind", "side"});

    Scene scene2 = {Space::nondirect_speaker, Space::north_facing_ground, Space::west_figure};
    REQUIRE(mydata.compute_true_words(mydata.target, scene2) == std::set<std::string>{"front", "side"});

    Scene scene3 = {Space::nondirect_speaker, Space::east_facing_ground, Space::east_figure};
    REQUIRE(mydata.compute_true_words(mydata.target, scene3) == std::set<std::string>{"behind", "front"});

    Scene scene4 = {Space::nondirect_speaker, Space::east_facing_ground, Space::west_figure};
    REQUIRE(mydata.compute_true_words(mydata.target, scene4) == std::set<std::string>{"behind", "front"});

    Scene scene5 = {Space::nondirect_speaker, Space::west_facing_ground, Space::east_figure};
    REQUIRE(mydata.compute_true_words(mydata.target, scene5) == std::set<std::string>{"behind"});

    Scene scene6 = {Space::nondirect_speaker, Space::west_facing_ground, Space::west_figure};
    REQUIRE(mydata.compute_true_words(mydata.target, scene6) == std::set<std::string>{"front"});

    Scene scene7 = {Space::nondirect_speaker, Space::east_facing_ground, Space::north_figure};
    REQUIRE(mydata.compute_true_words(mydata.target, scene7) == std::set<std::string>{"side"});

    Scene scene8 = {Space::nondirect_speaker, Space::north_facing_ground, Space::north_figure};
    REQUIRE(mydata.compute_true_words(mydata.target, scene8) == std::set<std::string>{"front"});

    Scene scene9 = {Space::nondirect_speaker, Space::north_facing_ground, Space::south_figure};
    REQUIRE(mydata.compute_true_words(mydata.target, scene9) == std::set<std::string>{"behind"});
}

TEST_CASE_METHOD(MyDataFixture, "MyData::compute_true_words returns expected words for direct scenes" ) {
    // TODO: better names for scenes later
    Scene scene_east = {Space::direct_speaker, Space::direct_speaker, Space::east_figure, true};
    REQUIRE(mydata.compute_true_words(mydata.target, scene_east) == std::set<std::string>{"front"});

    Scene scene_west = {Space::direct_speaker, Space::direct_speaker, Space::west_figure, true};
    REQUIRE(mydata.compute_true_words(mydata.target, scene_west) == std::set<std::string>{"behind"});

    Scene scene_north = {Space::direct_speaker, Space::direct_speaker, Space::north_figure, true};
    REQUIRE(mydata.compute_true_words(mydata.target, scene_north) == std::set<std::string>{"side"});

    Scene scene_south = {Space::direct_speaker, Space::direct_speaker, Space::south_figure, true};
    REQUIRE(mydata.compute_true_words(mydata.target, scene_south) == std::set<std::string>{"side"});
}
/* int main( int argc, char* argv[] ) { */
/*     // Use Catch's default reporter */
/*     Catch::Session().run(); */
/* } */
