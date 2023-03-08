#pragma once

namespace DSL {

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
    const auto parallel = +[](Vector x, Vector y) -> bool {
        if (nonzero(x,y)) {return false;}
        return cosine_similarity(x,y) == 0;
    };
}
