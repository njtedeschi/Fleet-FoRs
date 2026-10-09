#pragma once

namespace DSL {

    // Vector calculations
    const auto nonzero = +[](Vector x, Vector y) -> bool {
        return magnitude(x) == 0 || magnitude(y) == 0;
    };

    const auto both_valid = +[](Object x, Object y) -> bool {
        return x.is_valid && y.is_valid;
    };
    /* const auto displacement = +[](Object x, Object y) -> Vector { */
    /*     return y.position - x.position; */
    /* }; */
    const auto displacement = +[](Object x, Object y) -> Displacement {
        if(!both_valid(x,y)) {
            return Displacement {0,0,0};
        }
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
    const auto forward = +[](OrientedObject x) -> Direction {return x.forward;};
    const auto upward = +[](OrientedObject x) -> Direction {return x.upward;};
    const auto rightward = +[](OrientedObject x) -> Direction {return x.rightward;};
}
