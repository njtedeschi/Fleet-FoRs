#pragma once

namespace Concepts {
    // Above
    std::string above_abs = "parallel(displacement(G(x),F(x)),UP(x))";

    // Below
    std::string below_abs = "parallel(displacement(F(x),G(x)),UP(x))";

    // Front
    std::string front_int = "parallel(displacement(G(x),F(x)),forward(G(x)))";
    std::string front_int_rel = "or(parallel(displacement(G(x),F(x)),forward(G(x))),parallel(displacement(F(x),G(x)),forward(S(x))))";
        std::string front_int_rel_not_behind = "or(parallel(displacement(G(x),F(x)),forward(G(x))),and(parallel(displacement(F(x),G(x)),forward(S(x))),not(parallel(forward(G(x)),forward(S(x))))))"; // No relative front when intrinsic behind

    // Behind
    std::string behind_int = "parallel(displacement(F(x),G(x)),forward(G(x)))";
    std::string behind_int_rel = "or(parallel(displacement(F(x),G(x)),forward(G(x))),parallel(displacement(G(x),F(x)),forward(S(x))))";
        std::string behind_int_rel_not_front = "or(parallel(displacement(F(x),G(x)),forward(G(x))),and(parallel(displacement(G(x),F(x)),forward(S(x))),not(parallel(forward(G(x)),forward(S(x))))))"; // No relative behind when intrinsic front

    // Side
    std::string side_int = "orthogonal(displacement(G(x),F(x)),forward(G(x)))";
    std::string side_int_disjunction = "or(parallel(displacement(G(x),F(x)),rightward(G(x))),parallel(displacement(F(x),G(x)),rightward(G(x))))";
    std::string side_int_rel = "or(orthogonal(displacement(G(x),F(x)),forward(G(x))),orthogonal(displacement(G(x),F(x)),forward(S(x))))";

    // Left
    std::string left_int = "parallel(displacement(F(x),G(x)),rightward(G(x)))";
    std::string left_int_rel = "or(parallel(displacement(F(x),G(x)),rightward(G(x))),parallel(displacement(F(x),G(x)),rightward(S(x))))";

    // Right
    std::string right_int = "parallel(displacement(G(x),F(x)),rightward(G(x)))";
    std::string right_int_rel = "or(parallel(displacement(G(x),F(x)),rightward(G(x))),parallel(displacement(G(x),F(x)),rightward(S(x))))";
}
