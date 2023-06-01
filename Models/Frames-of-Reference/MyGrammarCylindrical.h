#pragma once

#include "Grammar.h"
#include "Singleton.h"
#include "Functional.h"

#include "DSL.h"

double TERMINAL_WEIGHT = 5.0;
double TERMINATING_WEIGHT = 1.0;
double ARGUMENT_WEIGHT = 1.0;
double UPWARD_WEIGHT = 1.0;
double RIGHTWARD_WEIGHT = 1.0;
double NEGATION_WEIGHT = 1.0;

/* Types for FoRs */

enum class Anchor {
    ground = 0,
    speaker = 1,
    environment = 2,
};

enum class Transformation {
    none = 0,
    translated = 1,
    transreflected = 2
};

struct AbstractFrame {
    Anchor anchor;
    Transformation transformation;

    // Constructor
    AbstractFrame(Anchor a, Transformation t)
        : anchor(a), transformation(t) {}
};

struct Frame : AbstractFrame {
    Position origin;
    Direction upward;
    Direction rightward;
    Direction forward;

    // Constructor from vectors directly
    Frame(Position o, Direction u, Direction f, Direction r, Anchor a, Transformation t)
        : AbstractFrame(a, t), origin(o), upward(u), forward(f), rightward(r) {}

    // Constructor from Scene and AbstractFrame
    Frame(const AbstractFrame& af, const Scene& x) 
        : AbstractFrame(af.anchor, af.transformation) {
            Direction u;
            Direction f;
            Direction r;

            switch(anchor) {
                case Anchor::ground:
                    u = x.ground.upward;
                    r = x.ground.rightward;
                    break;
                case Anchor::speaker:
                    u = x.speaker.upward;
                    r = x.speaker.rightward;
                    break;
                case Anchor::environment:
                    u = {0,0,1};
                    r = {0,0,0};
                    break;
            }

            // right-handed by default
            // left-handed if mirrored
            f = cross_product(u, r);
            if(transformation == Transformation::transreflected) {
                f = -f;
            }

            this->origin = x.ground.position;
            this->upward = u;
            this->forward = f;
            this->rightward = r;
    }
};

/* "Copies" of type std::function<bool(Frame)> */

struct fBool {
    std::function<bool(Frame)> func;

    fBool(std::function<bool(Frame)> f) :
        func(f){}

    bool operator()(const Frame& frame) const {
        return func(frame);
    }
};

struct coordinateBool {
    std::function<bool(Frame)> func;

    coordinateBool(std::function<bool(Frame)> f) :
        func(f){}

    bool operator()(const Frame& frame) const {
        return func(frame);
    }
};

struct rBool {
    std::function<bool(Frame)> func;

    rBool(std::function<bool(Frame)> f) :
        func(f){}

    bool operator()(const Frame& frame) const {
        return func(frame);
    }
};

struct thetaBool {
    std::function<bool(Frame)> func;

    thetaBool(std::function<bool(Frame)> f) :
        func(f){}

    bool operator()(const Frame& frame) const {
        return func(frame);
    }
};

struct zBool {
    std::function<bool(Frame)> func;

    zBool(std::function<bool(Frame)> f) :
        func(f){}

    bool operator()(const Frame& frame) const {
        return func(frame);
    }
};

class MyGrammar : public Grammar<MyInput,bool,   MyInput,bool,Frame,ft<bool,Frame>,std::vector<Frame>,fBool,Anchor,Transformation,coordinateBool,rBool,thetaBool,zBool>,
				  public Singleton<MyGrammar> {
public:
	MyGrammar() {
            /* Quantification */
            add("exists(%s,%s)",
                    +[](ft<bool,Frame> func, std::vector<Frame> possible_frames) -> bool {
                        for (auto& frame : possible_frames) {
                            if(func(frame)) return true;
                        }
                        return false;
                    });
            add("pf(%s)", // possible frames
                    +[](MyInput x) -> std::vector<Frame> {
                        std::vector<Frame> possible_frames;
                        AbstractFrame intrinsic(Anchor::ground, Transformation::none);
                        AbstractFrame relative_reflected(Anchor::speaker, Transformation::transreflected);
                        AbstractFrame absolute(Anchor::environment, Transformation::none);
                        if(x.scene.ground.is_participant){
                            possible_frames = {
                                Frame(intrinsic, x.scene),
                                Frame(absolute, x.scene)
                            };
                        }
                        else {
                            possible_frames = {
                                Frame(intrinsic, x.scene),
                                Frame(relative_reflected, x.scene),
                                Frame(absolute, x.scene)
                                };
                        }
                        return possible_frames;
                    });
            /* Angular Specification */
            add("as(%s,%s)", // Angular Specification
                    +[](fBool fb, coordinateBool cb) ->  ft<bool,Frame>{
                        return fBool([=](Frame f){
                            return fb(f) && cb(f);
                        });
                    });
            add("cyl(%s,%s,%s)",
                    +[](rBool rb, thetaBool tb, zBool zb) -> coordinateBool {
                        return coordinateBool([=](Frame f){
                            return rb(f) && tb(f) && zb(f);
                        });
                    });
            /* Frame Conditions */
            add("or(%s,%s)",
                    +[](fBool a, fBool b) -> fBool {
                        return fBool([=](Frame f) {
                            return a(f) || b(f);
                        });
                    });
            add("f=ABS",
                    +[]() -> fBool {
                        return fBool([=](Frame f) {
                            return (f.anchor == Anchor::environment);
                        });
                    });
            add("f=frame(%s)",
                    +[](Anchor a) -> fBool {
                        return fBool([=](Frame f) {
                            return (f.anchor == a) && (f.transformation == Transformation::none);
                        });
                    });
            add("f=frame'(%s,%s)",
                    +[](Anchor a, Transformation t) -> fBool {
                        return fBool([=](Frame f) {
                            return (f.anchor == a) && (f.transformation == t);
                        });
                    });
            add("G",
                    +[]() -> Anchor {
                        return Anchor::ground;
                    }, TERMINAL_WEIGHT);
            add("S",
                    +[]() -> Anchor {
                        return Anchor::speaker;
                    }, TERMINAL_WEIGHT);
            /* add("E", */
            /*         +[]() -> Anchor { */
            /*             return Anchor::environment; */
            /*         }, TERMINAL_WEIGHT); */
            add("T",
                    +[]() -> Transformation {
                        return Transformation::translated;
                    }, TERMINAL_WEIGHT);
            add("TR",
                    +[]() -> Transformation {
                        return Transformation::transreflected;
                    }, TERMINAL_WEIGHT);
            /***/
            /* Radial conditions */
            add("rTRUE",
                    +[]() -> rBool {
                        return rBool([=](Frame f) {
                            return true;
                        });
                    });
            add("r=0(%s)",
                    +[](MyInput x) -> rBool {
                        return rBool([=](Frame f) {
                            if(x.scene.g_to_f[0] == 0 && x.scene.g_to_f[1] == 0) {return true;}
                            return false;
                        });
                    });
            add("r-near(%s)",
                    +[](MyInput x) -> rBool {
                        return rBool([=](Frame f) {
                            Displacement v = x.scene.g_to_f;
                            double r = sqrt(v[0]*v[0] + v[1]*v[1]);
                            return (r < 1);
                        });
                    });
            add("r-far(%s)",
                    +[](MyInput x) -> rBool {
                        return rBool([=](Frame f) {
                            Displacement v = x.scene.g_to_f;
                            double r = sqrt(v[0]*v[0] + v[1]*v[1]);
                            return (r > 1);
                        });
                    });
            add("r>0(%s)",
                    +[](MyInput x) -> rBool {
                        return rBool([=](Frame f) {
                            if(x.scene.g_to_f[0] == 0 && x.scene.g_to_f[1] == 0) {return false;}
                            return true;
                        });
                    });
            /***/
            /* Azimuthal conditions */
            add("tTRUE",
                    +[]() -> thetaBool {
                        return thetaBool([=](Frame f) {
                            return true;
                        });
                    });
            // theta=0; theta=pi; theta_mod_pi=0; theta=+pi/2; theta=-pi/2
            // NOTE: settign theta=0 to right helps with handling front-back mirroring
            add("rightward(%s)", // theta=0
                    +[](MyInput x) -> thetaBool {
                        return thetaBool([=](Frame f) {
                            return cosine_similarity(x.scene.g_to_f, f.rightward) == 1;
                        });
                    });
            add("leftward(%s)", // theta=180
                    +[](MyInput x) -> thetaBool {
                        return thetaBool([=](Frame f) {
                            return cosine_similarity(x.scene.g_to_f, f.rightward) == -1;
                        });
                    });
            add("sideward(%s)", // theta=0 or 180
                    +[](MyInput x) -> thetaBool {
                        return thetaBool([=](Frame f) {
                            double cs = cosine_similarity(x.scene.g_to_f, f.rightward);
                            return cs == 1 or cs == -1;
                        });
                    });
            add("forward(%s)", // theta=90
                    +[](MyInput x) -> thetaBool {
                        return thetaBool([=](Frame f) {
                            return cosine_similarity(x.scene.g_to_f, f.forward) == 1;
                        });
                    });
            add("backward(%s)", // theta=270
                    +[](MyInput x) -> thetaBool {
                        return thetaBool([=](Frame f) {
                            return cosine_similarity(x.scene.g_to_f, f.forward) == -1;
                        });
                    });
            /***/
            /* Longitudinal conditions */
            // TODO: generalize to different angles, using Frame f
            // Right now, this is assuming a canonically-oriented ground at zero height
            add("zTRUE",
                    +[]() -> zBool {
                        return zBool([=](Frame f) {
                            return true;
                        });
                    });
            add("z=0(%s)",
                    +[](MyInput x) -> zBool {
                        return zBool([=](Frame f) {
                            return x.scene.figure.position[2] == 0;
                        });
                    });
            add("downward(%s)",
                    +[](MyInput x) -> zBool {
                        return zBool([=](Frame f) {
                            return x.scene.figure.position[2] < 0;
                        });
                    });
            add("upward(%s)",
                    +[](MyInput x) -> zBool {
                        return zBool([=](Frame f) {
                            return x.scene.figure.position[2] > 0;
                        });
                    });
            /***/
            add("x",             Builtins::X<MyGrammar>, TERMINAL_WEIGHT);

	}

        /* double log_probability(const Node& n) const override { */

        /*     double lp = 0.0; */
        /*     for(auto& x : n) { */
        /*             if(x.rule == NullRule) continue; */
        /*             // 0 prior probability for repeated negation */
        /*             if(x.rule->format == "not(%s)") { */
        /*                 if(x.get_children()[0].rule->format == "not(%s)") return -10000; */
        /*             } */
        /*             lp += log(x.rule->p) - log(rule_normalizer(x.rule->nt)); */
        /*     } */

        /*     return lp; */
        /* } */

} grammar;
