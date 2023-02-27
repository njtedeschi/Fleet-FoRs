#include <gtest/gtest.h>

#include <unordered_map>

#include "MyGrammar.h"
#include "MyHypothesis.h"
#include "MyData.h"

class MyDataTest : public ::testing::Test {
    protected:
        void SetUp() override {
            std::unordered_map<std::string, std::string> formulas = {
            {"above", "parallel(displacement(ground(x),figure(x)),up(x))"},
            {"below", "parallel(displacement(figure(x),ground(x)),up(x))"},
            {"front", "or(parallel(displacement(ground(x),figure(x)),orientation(ground(x))),parallel(displacement(figure(x),ground(x)),orientation(speaker(x))))"},
            {"behind", "or(parallel(displacement(figure(x),ground(x)),orientation(ground(x))),parallel(displacement(ground(x),figure(x)),orientation(speaker(x))))"},
            {"side", "orthogonal(displacement(ground(x),figure(x)),orientation(ground(x)))"}
        };
            mydata = MyData(formulas);
            // other set up
        }
};

TEST_F(MyDataTest, ComputeTrueWords) {
    // test code for compute_true_words
}
