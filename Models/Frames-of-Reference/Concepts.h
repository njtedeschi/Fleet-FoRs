#pragma once

namespace Concepts {

    // Functions for "string formatting"
    std::string un_op(std::string function, std::string a) {
        return function + "(" + a + ")";
    }

    std::string bin_op(std::string function, std::string a, std::string b) {
        return function + "(" + a + "," + b + ")";
    }

    // Above
    std::string above_abs = "parallel(displacement(G(x),F(x)),UP(scene(x)))";

    // Below
    std::string below_abs = "parallel(displacement(F(x),G(x)),UP(scene(x)))";

    // Front
    std::string front_int = "parallel(displacement(G(x),F(x)),forward(GG(x)))";
    std::string front_rel = "parallel(displacement(F(x),G'(x)),forward(SS(scene(x))))";
    std::string front_int_rel = bin_op("or", front_int, front_rel);

    // Behind
    std::string behind_int = "parallel(displacement(F(x),G(x)),forward(GG(x)))";
    std::string behind_rel = "parallel(displacement(G'(x),F(x)),forward(SS(scene(x))))";
    std::string behind_int_rel = bin_op("or", behind_int, behind_rel);

    // Front and behind without reflection
    std::string front_int_rel_no_reflect = bin_op("or", front_int, behind_rel);
    std::string behind_int_rel_no_reflect = bin_op("or", behind_int, front_rel);

    // Left
    std::string left_int = "parallel(displacement(F(x),G(x)),rightward(GG(x)))";
    std::string left_rel = "parallel(displacement(F(x),G'(x)),rightward(SS(scene(x))))";
    std::string left_int_rel = bin_op("or", left_int, left_rel);

    // Right
    std::string right_int = "parallel(displacement(G(x),F(x)),rightward(GG(x)))";
    std::string right_rel = "parallel(displacement(G'(x),F(x)),rightward(SS(scene(x))))";
    std::string right_int_rel = bin_op("or", right_int, right_rel);

    // Side
    /* std::string side_int = "orthogonal(displacement(G(x),F(x)),forward(G(x)))"; */
    std::string side_int = bin_op("or", left_int, right_int);
    // Version of intrinsic side that uses +-parallel instead of disjunction
    std::string side_int_pm = "+-parallel(displacement(G(x),F(x)),rightward(GG(x)))";
    std::string side_rel = bin_op("or", left_rel, right_rel);
    std::string side_int_rel = bin_op("or", side_int, side_rel);
    /* std::string side_int_rel = "or(orthogonal(displacement(G(x),F(x)),forward(G(x))),orthogonal(displacement(G(x),F(x)),forward(S(scene(x)))))"; */

    // Conditional definitions: no relative when opposite intrinsic holds
    std::string fb_rel_condition = "not(Parallel(forward(G(x)),forward(S(scene(x)))))";
    std::string lr_rel_condition = "not(Antiparallel(forward(G(x)),forward(S(scene(x)))))";

    std::string front_int_rel_not_behind = bin_op("or", front_int, bin_op("and", front_rel, fb_rel_condition));
    std::string behind_int_rel_not_front = bin_op("or", behind_int, bin_op("and", behind_rel, fb_rel_condition));
    std::string left_int_rel_not_right = bin_op("or", left_int, bin_op("and", left_rel, lr_rel_condition));
    std::string right_int_rel_not_left = bin_op("or", right_int, bin_op("and", right_rel, lr_rel_condition));

    // Exclusive definitions (distinct intrinsic, relative, or both)
    std::string front_int_only = bin_op("and", front_int, un_op("not", front_rel));
    std::string front_rel_only = bin_op("and", front_rel, un_op("not", front_int));
    std::string front_int_and_rel = bin_op("and", front_int, front_rel);
    std::string behind_int_only = bin_op("and", behind_int, un_op("not", behind_rel));
    std::string behind_rel_only = bin_op("and", behind_rel, un_op("not", behind_int));
    std::string behind_int_and_rel = bin_op("and", behind_int, behind_rel);
    std::string left_int_only = bin_op("and", left_int, un_op("not", left_rel));
    std::string left_rel_only = bin_op("and", left_rel, un_op("not", left_int));
    std::string left_int_and_rel = bin_op("and", left_int, left_rel);
    std::string right_int_only = bin_op("and", right_int, un_op("not", right_rel));
    std::string right_rel_only = bin_op("and", right_rel, un_op("not", right_int));
    std::string right_int_and_rel = bin_op("and", right_int, right_rel);
}
