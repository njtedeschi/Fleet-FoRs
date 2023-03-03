#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <set>
#include <unordered_map>

static const double alpha_t = 0.95;

#include "../Scene.h"

struct MyInput {
    Scene scene;
    std::string word;
};

#include "../MyGrammar.h"
#include "../MyHypothesis.h"
#include "../MyData.h"

#include "TopN.h"
#include "ParallelTempering.h"
#include "Fleet.h" 
#include "Random.h"
#include "Builtins.h"

class MyDataFixture {
    public:
        MyDataFixture() {
            std::unordered_map<std::string, std::string> formulas = {
            {"above", "parallel(displacement(ground(x),figure(x)),up(x))"},
            {"below", "parallel(displacement(figure(x),ground(x)),up(x))"},
            {"front", "or(parallel(displacement(ground(x),figure(x)),orientation(ground(x))),parallel(displacement(figure(x),ground(x)),orientation(speaker(x))))"},
            {"behind", "or(parallel(displacement(figure(x),ground(x)),orientation(ground(x))),parallel(displacement(ground(x),figure(x)),orientation(speaker(x))))"},
            {"side", "orthogonal(displacement(ground(x),figure(x)),orientation(ground(x)))"}
        };
            mydata = MyData(formulas);
        }
        MyData mydata;
};

/* TEST_CASE_METHOD(MyDataFixture, "MyData::words initializes from formulas map correctly" ) { */
/*     std::vector<std::string> words = {"above", "below", "front", "behind", "side"}; */
/*     REQUIRE(mydata.words == words); */
/* } */

TEST_CASE_METHOD(MyDataFixture, "MyData::compute_true_words returns expected words" ) { // test code for compute_true_words
    /* Scene scene = {{{-2,0,0},{1,0,0}},{{0,0,0},{}},{{},{0,0,0}}}; */
    /* Scene scene = {{{-2,0,0},{1,0,0}},{{0,0,0},{0,1,0}},{{1,0,0},{0,0,0}}}; */
    Scene scene = {Space::direct_speaker, Space::north_facing_ground, Space::east_figure};
    std::set<std::string> computed_words = mydata.compute_true_words(scene);
    std::set<std::string> expected_words = {"behind", "side"};
    REQUIRE(expected_words == computed_words);
}

/* int main( int argc, char* argv[] ) { */
/*     // Use Catch's default reporter */
/*     Catch::Session().run(); */
/* } */
