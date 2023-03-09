#pragma once

namespace Concepts {

    // Function for "string formatting"
    std::string bin_op(std::string function, std::string a, std::string b) {
        return function + "(" + a + "," + b + ")";
    }

    // Above
    std::string above_abs = "parallel(displacement(G(x),F(x)),UP(x))";

    // Below
    std::string below_abs = "parallel(displacement(F(x),G(x)),UP(x))";

    // Front
    std::string front_int = "parallel(displacement(G(x),F(x)),forward(G(x)))";
    std::string front_rel = "parallel(displacement(F(x),G(x)),forward(S(x)))";
    std::string front_int_rel = bin_op("or", front_int, front_rel);

    // Behind
    std::string behind_int = "parallel(displacement(F(x),G(x)),forward(G(x)))";
    std::string behind_rel = "parallel(displacement(G(x),F(x)),forward(S(x)))";
    std::string behind_int_rel = bin_op("or", behind_int, behind_rel);

    // Left
    std::string left_int = "parallel(displacement(F(x),G(x)),rightward(G(x)))";
    std::string left_rel = "parallel(displacement(F(x),G(x)),rightward(S(x)))";
    std::string left_int_rel = bin_op("or", left_int, left_rel);

    // Right
    std::string right_int = "parallel(displacement(G(x),F(x)),rightward(G(x)))";
    std::string right_rel = "parallel(displacement(G(x),F(x)),rightward(S(x)))";
    std::string right_int_rel = bin_op("or", right_int, right_rel);

    // Side
    /* std::string side_int = "orthogonal(displacement(G(x),F(x)),forward(G(x)))"; */
    std::string side_int = bin_op("or", left_int, right_int);
    std::string side_rel = bin_op("or", left_rel, right_rel);
    std::string side_int_rel = bin_op("or", side_int, side_rel);
    /* std::string side_int_rel = "or(orthogonal(displacement(G(x),F(x)),forward(G(x))),orthogonal(displacement(G(x),F(x)),forward(S(x))))"; */

    // Conditional definitions: no relative when opposite intrinsic holds
    std::string fb_rel_condition = "not(Parallel(forward(G(x)),forward(S(x))))";
    std::string lr_rel_condition = "not(Antiparallel(forward(G(x)),forward(S(x))))";

    std::string front_int_rel_not_behind = bin_op("or", front_int, bin_op("and", front_rel, fb_rel_condition));
    std::string behind_int_rel_not_front = bin_op("or", behind_int, bin_op("and", behind_rel, fb_rel_condition));
    std::string left_int_rel_not_right = bin_op("or", left_int, bin_op("and", left_rel, lr_rel_condition));
    std::string right_int_rel_not_left = bin_op("or", right_int, bin_op("and", right_rel, lr_rel_condition));
}
