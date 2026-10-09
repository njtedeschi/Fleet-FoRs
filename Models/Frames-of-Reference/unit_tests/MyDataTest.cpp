#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <set>
#include <unordered_map>
#include <functional>

static const double alpha_t = 0.95;

#include "../Scene.h"

enum class BodyPartNoun {
    none = 0,
    head = 1,
    belly = 2,
    face = 3,
    back = 4,
    right_side = 5,
    left_side = 6,
    side = 7,
    tail = 8
};

struct WordMeaning {
    std::string target_formula;
    BodyPartNoun body_part_noun;
    std::function<Direction(OrientedObject&)> body_part_direction;

    // No argument constructor
    WordMeaning() : target_formula(""), body_part_noun(BodyPartNoun::none), body_part_direction([](const OrientedObject& a) -> Direction {return {0,0,0};}) {}

    // Default body part direction is the zero vector
    WordMeaning(std::string tf) : target_formula(tf), body_part_noun(BodyPartNoun::none), body_part_direction([](const OrientedObject& a) -> Direction {return {0,0,0};}) {}

    WordMeaning(std::string tf, BodyPartNoun bpn, std::function<Direction(const OrientedObject&)> bpd) :
        target_formula(tf), body_part_noun(bpn), body_part_direction(bpd) {}
};

struct MyInput {
    Scene scene;
    std::string word;
    WordMeaning* meaning;
    bool true_description;
};

/* #include "../MyGrammar.h" */
#include "../MyGrammarCylindrical.h"
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
            /* std::unordered_map<std::string, std::string> formulas = { */
            /* {"above", Concepts::above_abs}, */
            /* {"below", Concepts::below_abs}, */
            /* {"front", Concepts::front_int_rel}, */
            /* {"behind", Concepts::behind_int_rel}, */
            /* {"side", Concepts::side_int} */
        /* }; */
        /* std::unordered_map<std::string, std::string> formulas = { */
        /*     {"above", "exists(as(f=frame(G),cyl(r=0(x),tTRUE,upward(x))),pf(x))"}, */
        /*     {"below", "exists(as(f=frame(G),cyl(r=0(x),tTRUE,downward(x))),pf(x))"}, */
        /*     {"front", "exists(as(or(f=frame(G),f=frame'(S,TR)),cyl(r>0(x),forward(x),z=0(x))),pf(x))"}, */
        /*     {"behind", "exists(as(or(f=frame(G),f=frame'(S,TR)),cyl(r>0(x),backward(x),z=0(x))),pf(x))"}, */
        /*     {"side", "exists(as(f=frame(G),cyl(r>0(x),sideward(x),z=0(x))),pf(x))"}, */
        /*     {"left", "exists(as(or(f=frame(G),f=frame'(S,TR)),cyl(r>0(x),leftward(x),z=0(x))),pf(x))"}, */
        /*     {"right", "exists(as(or(f=frame(G),f=frame'(S,TR)),cyl(r>0(x),rightward(x),z=0(x))),pf(x))"}, */
        /* }; */
        std::unordered_map<std::string, WordMeaning> formulas = {
            {"above", WordMeaning(
                    "parallel(x,normal(x,head))",
                    BodyPartNoun::head,
                    [](const OrientedObject& a) -> Direction {return a.upward;})},
            {"below", WordMeaning(
                    "parallel(x,normal(x,belly))",
                    BodyPartNoun::belly,
                    [](const OrientedObject& a) -> Direction {return -a.upward;})},
            {"front", WordMeaning(
                    "parallel(x,normal(x,face))",
                    BodyPartNoun::face,
                    [](const OrientedObject& a) -> Direction {return a.forward;})},
            {"behind", WordMeaning(
                    /* "parallel(x,back(x))", */
                    "parallel(x,normal(x,Back))",
                    BodyPartNoun::back,
                    [](const OrientedObject& a) -> Direction {return -a.forward;})},
            {"right", WordMeaning(
                    "parallel(x,normal(x,right_side))",
                    BodyPartNoun::right_side,
                    [](const OrientedObject& a) -> Direction {return a.rightward;})},
            {"left", WordMeaning(
                    "parallel(x,normal(x,left_side))",
                    BodyPartNoun::left_side,
                    [](const OrientedObject& a) -> Direction {return -a.rightward;})},
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
    Scene scene_east = {Space::direct_speaker, Space::direct_speaker, Space::east_figure};
    REQUIRE(mydata.compute_true_words(mydata.target, scene_east) == std::set<std::string>{"front"});

    Scene scene_west = {Space::direct_speaker, Space::direct_speaker, Space::west_figure};
    REQUIRE(mydata.compute_true_words(mydata.target, scene_west) == std::set<std::string>{"behind"});

    Scene scene_north = {Space::direct_speaker, Space::direct_speaker, Space::north_figure};
    REQUIRE(mydata.compute_true_words(mydata.target, scene_north) == std::set<std::string>{"side"});

    Scene scene_south = {Space::direct_speaker, Space::direct_speaker, Space::south_figure};
    REQUIRE(mydata.compute_true_words(mydata.target, scene_south) == std::set<std::string>{"side"});
}

/* TEST_CASE_METHOD(MyDataFixture, "MyData::compute_true_words returns expected words for nondirect scenes with a listener ground" ) { */
/*     // TODO: better names for scenes later */
/*     Scene scene1 = {Space::nondirect_speaker, Space::listener_north_facing_ground, Space::east_figure}; */
/*     REQUIRE(mydata.compute_true_words(mydata.target, scene1) == std::set<std::string>{"side"}); */

/*     Scene scene2 = {Space::nondirect_speaker, Space::listener_north_facing_ground, Space::west_figure}; */
/*     REQUIRE(mydata.compute_true_words(mydata.target, scene2) == std::set<std::string>{"side"}); */

/*     Scene scene3 = {Space::nondirect_speaker, Space::listener_east_facing_ground, Space::east_figure}; */
/*     REQUIRE(mydata.compute_true_words(mydata.target, scene3) == std::set<std::string>{"front"}); */

/*     Scene scene4 = {Space::nondirect_speaker, Space::listener_east_facing_ground, Space::west_figure}; */
/*     REQUIRE(mydata.compute_true_words(mydata.target, scene4) == std::set<std::string>{"behind"}); */

/*     Scene scene5 = {Space::nondirect_speaker, Space::listener_west_facing_ground, Space::east_figure}; */
/*     REQUIRE(mydata.compute_true_words(mydata.target, scene5) == std::set<std::string>{"behind"}); */

/*     Scene scene6 = {Space::nondirect_speaker, Space::listener_west_facing_ground, Space::west_figure}; */
/*     REQUIRE(mydata.compute_true_words(mydata.target, scene6) == std::set<std::string>{"front"}); */

/*     Scene scene7 = {Space::nondirect_speaker, Space::listener_east_facing_ground, Space::north_figure}; */
/*     REQUIRE(mydata.compute_true_words(mydata.target, scene7) == std::set<std::string>{"side"}); */

/*     Scene scene8 = {Space::nondirect_speaker, Space::listener_north_facing_ground, Space::north_figure}; */
/*     REQUIRE(mydata.compute_true_words(mydata.target, scene8) == std::set<std::string>{"front"}); */

/*     Scene scene9 = {Space::nondirect_speaker, Space::listener_north_facing_ground, Space::south_figure}; */
/*     REQUIRE(mydata.compute_true_words(mydata.target, scene9) == std::set<std::string>{"behind"}); */
/* } */
/* int main( int argc, char* argv[] ) { */
/*     // Use Catch's default reporter */
/*     Catch::Session().run(); */
/* } */
