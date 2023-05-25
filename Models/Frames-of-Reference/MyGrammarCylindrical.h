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

struct Frame {
    /* Position origin; // TODO: Remove because ground is always at original origin */
    Direction upward;
    Direction rightward;
    Direction forward;

    // Constructor
    Frame(Direction u, Direction r, Direction f)
        : upward(u), rightward(r), forward(f) {}
};

/* Abstract Frame */
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

    // Methods
    Frame instantiate(Scene x) {
        Direction upward; // longitudinal axis
        Direction rightward; // polar axis
        Direction forward; // theta = pi/2

        switch(anchor) {
            case Anchor::ground:
                upward = x.ground.upward;
                rightward = x.ground.rightward;
                break;
            case Anchor::speaker:
                upward = x.speaker.upward;
                rightward = x.speaker.rightward;
                break;
            /* case Anchor::environment: */
            /*     upward = {0,0,1}; */
            /*     rightward = {0,0,0}; */
            /*     break; */
        }

        // right-handed by default
        // left-handed if mirrored
        forward = cross_product(upward, rightward);
        if(transformation == Transformation::transreflected) {
            forward = -forward;
        }
        return Frame(upward, rightward, forward);
    }
};

AbstractFrame intrinsic(Anchor::ground, Transformation::none);
AbstractFrame relative_reflected(Anchor::speaker, Transformation::transreflected);
/* AbstractFrame absolute(Anchor::environment, Transformation::none); */
/* std::vector<AbstractFrame> possible_frames = {intrinsic, relative_reflected, absolute}; */
std::vector<AbstractFrame> possible_frames = {intrinsic, relative_reflected};

/* Types for Fleet */
/* struct fBool { */
/*     std::function<bool(AbstractFrame)> func; */
/* }; */
/* struct rBool { */
/*     std::function<bool(Frame)> func; */
/* }; */
/* struct thetaBool { */
/*     std::function<bool(Frame)> func; */
/* }; */
/* struct zBool { */
/*     std::function<bool(Frame)> func; */
/* }; */
using fBool = std::function<bool(AbstractFrame)>;
using coordinateBool = std::function<bool(Frame)>;
struct rBool : public coordinateBool {};
struct thetaBool : public coordinateBool {};
struct zBool : public coordinateBool {};

class MyGrammar : public Grammar<MyInput,bool,   MyInput,bool,fBool,Anchor,Transformation,rBool,thetaBool,zBool>,
				  public Singleton<MyGrammar> {
public:
	MyGrammar() {
            /* add("", */
            /*         +[]() -> { */
            /*             // body */
            /*         }); */
            // TODO: remove argument x somehow
            add("cyl(%s,%s,%s,%s,%s)",
                    +[](MyInput x, fBool fb, rBool rb, thetaBool tb, zBool zb) -> bool {
                        for(auto& abstract_frame : possible_frames) {
                            Frame frame = abstract_frame.instantiate(x.scene);
                            if(fb(abstract_frame) && rb(frame) && tb(frame) && zb(frame)) {
                                return true;
                            }
                        }
                        return false;
                    });
            /* add("or(%s,%s)", */
            /*         +[](fBool a, fBool b) -> fBool { */
            /*             return +[](AbstractFrame f) { */
            /*                 return a(f) || b(f); */
            /*             }; */
            /*         }); */
            add("or(%s,%s)",
                    +[](fBool a, fBool b) -> fBool {
                        return fBool{
                            [a,b](AbstractFrame f) {
                            return a(f) || b(f);
                            }
                        };
                    });
            /* add("f=frame(%s)", */
            /*         +[](Anchor a) -> fBool { */
            /*             return +[](AbstractFrame f) { */
            /*                 return (f.anchor == a) && (f.transformation == none); //TODO: replace None */
            /*             }; */
            /*         }); */
            add("f=frame(%s)",
                    +[](Anchor a) -> fBool {
                        return fBool{
                            [a](AbstractFrame f) {
                                return (f.anchor == a) && (f.transformation == Transformation::none);
                            }
                        };
                    });
            /* add("f=frame'(%s,%s)", */
            /*         +[](Anchor a, Transformation t) -> fBool { */
            /*             return +[](AbstractFrame f) { */
            /*                 return (f.anchor == a) && (f.transformation == t); //TODO: replace None */
            /*             }; */
            /*         }); */
            add("f=frame'(%s,%s)",
                    +[](Anchor a, Transformation t) -> fBool {
                        return fBool{
                            [a,t](AbstractFrame f) {
                                return (f.anchor == a) && (f.transformation == t);
                            }
                        };
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
            /* add("rTRUE", */
            /*         +[]() -> rBool { */
            /*             return +[](Frame frame) { */
            /*                 return true; */
            /*             }; */
            /*         }); // TODO: give prior */
            add("rTRUE",
                    +[]() -> rBool {
                        return rBool {
                            [](Frame f) {
                                return true;
                            }
                        };
                    });
            add("r=0(%s)",
                    +[](MyInput x) -> rBool {
                        return rBool {
                            [x](Frame f) {
                                if(x.scene.g_to_f[0] == 0 && x.scene.g_to_f[1] == 0) {return true;}
                                return false;
                            }
                        };
                    });
            add("r>0(%s)",
                    +[](MyInput x) -> rBool {
                        return rBool {
                            [x](Frame f) {
                                if(x.scene.g_to_f[0] == 0 && x.scene.g_to_f[1] == 0) {return false;}
                                return true;
                            }
                        };
                    });
            /***/
            /* Azimuthal conditions */
            /* add("tTRUE", */
            /*         +[]() -> thetaBool { */
            /*             return +[](Frame frame) { */
            /*                 return true; */
            /*             }; */
            /*         }); // TODO: give prior */
            add("tTRUE",
                    +[]() -> thetaBool {
                        return thetaBool {
                            [](Frame f) {
                                return true;
                            }
                        };
                    });
            // theta=0; theta=pi; theta_mod_pi=0; theta=+pi/2; theta=-pi/2
            // NOTE: settign theta=0 to right helps with handling front-back mirroring
            /* add("rightward()", */
            /*         +[](MyInput x) -> thetaBool { */
            /*             return +[](Frame frame) { */
            /*                 Vector theta_0 = frame.rightward; */
            /*                 return cosine_similarity(x.g_to_f, theta_0) == 1; */
            /*             }; */
            /*         }); */
            add("rightward(%s)",
                    +[](MyInput x) -> thetaBool {
                        return thetaBool {
                            [x](Frame f) {
                                Vector theta_0 = f.rightward;
                                return cosine_similarity(x.scene.g_to_f, theta_0) == 1;
                            }
                        };
                    });
            add("leftward(%s)",
                    +[](MyInput x) -> thetaBool {
                        return thetaBool {
                            [x](Frame f) {
                                Vector theta_180 = -f.rightward;
                                return cosine_similarity(x.scene.g_to_f, theta_180) == 1;
                            }
                        };
                    });
            add("sideward(%s)",
                    +[](MyInput x) -> thetaBool {
                        return thetaBool {
                            [x](Frame f) {
                                Vector theta_0 = f.rightward;
                                double cs = cosine_similarity(x.scene.g_to_f, theta_0);
                                return cs == 1 or cs == -1;
                            }
                        };
                    });
            add("forward(%s)",
                    +[](MyInput x) -> thetaBool {
                        return thetaBool {
                            [x](Frame f) {
                                Vector theta_90 = f.forward;
                                return cosine_similarity(x.scene.g_to_f, theta_90) == 1;
                            }
                        };
                    });
            add("backward(%s)",
                    +[](MyInput x) -> thetaBool {
                        return thetaBool {
                            [x](Frame f) {
                                Vector theta_270 = -f.forward;
                                return cosine_similarity(x.scene.g_to_f, theta_270) == 1;
                            }
                        };
                    });
            /***/
            /* Longitudinal conditions */
            // TODO: generalize to different angles, using Frame f
            // Right now, this is assuming a canonically-oriented ground at zero height
            /* add("zTRUE", */
            /*         +[]() -> zBool { */
            /*             return +[](Frame frame) { */
            /*                 return true; */
            /*             }; */
            /*         }); // TODO: give prior */
            add("zTRUE",
                    +[]() -> zBool {
                        return zBool {
                            [](Frame f) {
                                return true;
                            }
                        };
                    });
            /* add("z=0(%s)", */
            /*         +[](MyInput x) -> zBool { */
            /*             double z = x.scene.figure.position[2]; */
            /*             return +[](Frame frame) { */
            /*                 return z == 0; */
            /*             }; */
            /*         }); */
            add("z=0(%s)",
                    +[](MyInput x) -> zBool {
                        return zBool {
                            [x](Frame f) {
                                double z = x.scene.figure.position[2];
                                return z == 0;
                            }
                        };
                    });
            add("downward(%s)",
                    +[](MyInput x) -> zBool {
                        return zBool {
                            [x](Frame f) {
                                double z = x.scene.figure.position[2];
                                return z < 0;
                            }
                        };
                    });
            add("upward(%s)",
                    +[](MyInput x) -> zBool {
                        return zBool {
                            [x](Frame f) {
                                double z = x.scene.figure.position[2];
                                return z > 0;
                            }
                        };
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
