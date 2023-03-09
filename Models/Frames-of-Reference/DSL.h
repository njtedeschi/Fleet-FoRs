#pragma once

namespace DSL {

    // Vector calculations
    const auto nonzero = +[](Vector x, Vector y) -> bool {
        return magnitude(x) == 0 || magnitude(y) == 0;
    };
    /* const auto displacement = +[](Object x, Object y) -> Vector { */
    /*     return y.position - x.position; */
    /* }; */
    const auto displacement = +[](Object x, Object y) -> Displacement {
        return y.position - x.position;
    };
    /* const auto parallel = +[](Vector x, Vector y) -> bool { */
    /*     if (nonzero(x,y)) {return false;} */
    /*     return cosine_similarity(x,y) == 1; */
    /* }; */
    /* const auto antiparallel = +[](Vector x, Vector y) -> bool { */
    /*     if (nonzero(x,y)) {return false;} */
    /*     return cosine_similarity(x,y) == -1; */
    /* }; */
    /* const auto orthogonal = +[](Vector x, Vector y) -> bool { */
    /*     if (nonzero(x,y)) {return false;} */
    /*     return cosine_similarity(x,y) == 0; */
    /* }; */
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

    // Functions only used for data generation
    const auto parallel_orientation = +[](Direction x, Direction y) -> bool {
        if (nonzero(x,y)) {return false;}
        return cosine_similarity(x,y) == 1;
    };
    const auto antiparallel_orientation = +[](Direction x, Direction y) -> bool {
        if (nonzero(x,y)) {return false;}
        return cosine_similarity(x,y) == -1;
    };

    // Feature extraction
    const auto forward = +[](Object x) -> Direction {return x.forward;};
    const auto upward = +[](Object x) -> Direction {return x.upward;};
    const auto rightward = +[](Object x) -> Direction {return x.rightward;};
}
