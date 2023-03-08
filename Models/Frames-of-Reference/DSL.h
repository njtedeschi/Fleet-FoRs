#pragma once

namespace DSL {

    // Vector calculations
    const auto nonzero = +[](Vector x, Vector y) -> bool {
        return magnitude(x) == 0 || magnitude(y) == 0;
    };
    const auto displacement = +[](Object x, Object y) -> Vector {
        return y.location - x.location;
    };
    const auto parallel = +[](Vector x, Vector y) -> bool {
        if (nonzero(x,y)) {return false;}
        return cosine_similarity(x,y) == 1;
    };
    const auto antiparallel = +[](Vector x, Vector y) -> bool {
        if (nonzero(x,y)) {return false;}
        return cosine_similarity(x,y) == -1;
    };
    const auto orthogonal = +[](Vector x, Vector y) -> bool {
        if (nonzero(x,y)) {return false;}
        return cosine_similarity(x,y) == 0;
    };

    // Feature extraction
    const auto forward = +[](Object x) -> Vector {return x.forward;};
    const auto upward = +[](Object x) -> Vector {return x.upward;};
    const auto rightward = +[](Object x) -> Vector {return x.rightward;};
}
