#pragma once

namespace DSL {

    // Vector calculations
    const auto nonzero = +[](Vector x, Vector y) -> bool {
        return magnitude(x) == 0 || magnitude(y) == 0;
    };
    const auto displacement = +[](Object x, Object y) -> Displacement {
        return y.position - x.position;
    };
    const auto parallel = +[](Displacement x, Direction y) -> bool {
        if (nonzero(x,y)) {return false;}
        return cosine_similarity(x,y) == 1;
    };
    const auto antiparallel = +[](Displacement x, Direction y) -> bool {
        if (nonzero(x,y)) {return false;}
        return cosine_similarity(x,y) == -1;
    };
    const auto orthogonal = +[](Displacement x, Direction y) -> bool {
        if (nonzero(x,y)) {return false;}
        return cosine_similarity(x,y) == 0;
    };

    // Feature extraction
    const auto forward = +[](Object x) -> Direction {return x.forward;};
    const auto upward = +[](Object x) -> Direction {return x.upward;};
    const auto rightward = +[](Object x) -> Direction {return x.rightward;};
}
