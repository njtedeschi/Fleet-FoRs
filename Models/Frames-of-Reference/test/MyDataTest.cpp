#include <gtest/gtest.h>

#include "MyGrammar.h"
#include "MyHypothesis.h"
#include "MyData.h"

class MyDataTest : public ::testing::Test {
    protected:
        void SetUp() override {
            // formulas = ...
            // other set up
        }
        // mydata = MyData(formulas);
};

TEST_F(MyDataTest, ComputeTrueWords) {
    // test code for compute_true_words
}
