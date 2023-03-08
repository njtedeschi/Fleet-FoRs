#pragma once

namespace Concepts {
    // Above
    std::string above_abs = "parallel(displacement(ground(x),figure(x)),up(x))";

    // Below
    std::string below_abs = "parallel(displacement(figure(x),ground(x)),up(x))";

    // Front
    std::string front_int = "parallel(displacement(ground(x),figure(x)),forward(ground(x)))";
    std::string front_int_rel = "or(parallel(displacement(ground(x),figure(x)),forward(ground(x))),parallel(displacement(figure(x),ground(x)),forward(speaker(x))))";
        std::string front_int_rel_not_behind = "or(parallel(displacement(ground(x),figure(x)),forward(ground(x))),and(parallel(displacement(figure(x),ground(x)),forward(speaker(x))),not(parallel(forward(ground(x)),forward(speaker(x))))))"; // No relative front when intrinsic behind

    // Behind
    std::string behind_int = "parallel(displacement(figure(x),ground(x)),forward(ground(x)))";
    std::string behind_int_rel = "or(parallel(displacement(figure(x),ground(x)),forward(ground(x))),parallel(displacement(ground(x),figure(x)),forward(speaker(x))))";
        std::string behind_int_rel_not_front = "or(parallel(displacement(figure(x),ground(x)),forward(ground(x))),and(parallel(displacement(ground(x),figure(x)),forward(speaker(x))),not(parallel(forward(ground(x)),forward(speaker(x))))))"; // No relative behind when intrinsic front

    // Side
    std::string side_int = "orthogonal(displacement(ground(x),figure(x)),forward(ground(x)))";
    std::string side_int_rel = "or(orthogonal(displacement(ground(x),figure(x)),forward(ground(x))),orthogonal(displacement(ground(x),figure(x)),forward(speaker(x))))";
}
