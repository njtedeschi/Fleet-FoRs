#pragma once

#include "Grammar.h"
#include "Singleton.h"
#include "Functional.h"

#include "Ch3MyStructures.h"
#include "Ch3MyDSL.h"

double TERMINAL_WEIGHT = 5.0;

using MyOutput = std::pair<WordSenseJudgments<Felicity>, WordSenseWeights>;

class MyGrammar : public Grammar<Context,MyOutput,   Context,MyOutput,WordSenseJudgments<Truth>,WordSenseConditions<Truth>,SenseConditions<Truth>,Condition<Truth>,WordSenseJudgments<Felicity>,WordSenseConditions<Felicity>,SenseConditions<Felicity>,Condition<Felicity>,Subcondition,Transformation,WordSenseWeights,SenseWeights,Weight>,
				  public Singleton<MyGrammar> {
public:
	MyGrammar() {
            // add("", +[]() -> {});
            add(
                "JW(%s,%s)",
                +[](WordSenseJudgments<Felicity> judgments, WordSenseWeights weights) -> MyOutput {
                    return std::make_pair(judgments, weights);
                }
            );
            add(
                "J-T(%s,%s)",
                +[](WordSenseConditions<Truth> truth_conditions, Context context) -> WordSenseJudgments<Truth> {
                    return DSL::initial_word_sense_judgments<Truth>(truth_conditions, context);
                }
            );
            add(
                "J-F(%s,%s,%s)",
                +[](WordSenseJudgments<Truth> truth_judgments, WordSenseConditions<Felicity> felicity_conditions, Context context) -> WordSenseJudgments<Felicity> {
                    return DSL::updated_word_sense_judgments<Felicity,Truth>(felicity_conditions, context, truth_judgments);
                }
            );
            //////////////////////////////////
            add(
                "T2-xyz(%s)",
                +[](SenseConditions<Truth> sense_conditions_xyz) -> WordSenseConditions<Truth> {
                    return DSL::generate_word_sense_conditions<Truth>(
                        sense_conditions_xyz,
                        sense_conditions_xyz,
                        sense_conditions_xyz
                    );
                }
            );
            add(
                "T2-xyz_part(%s,%s)",
                +[](SenseConditions<Truth> sense_conditions_xyz, Condition<Truth> condition_part) -> WordSenseConditions<Truth> {
                    return DSL::generate_word_sense_conditions_with_parts<Truth>(
                        sense_conditions_xyz,
                        sense_conditions_xyz,
                        sense_conditions_xyz,
                        condition_part
                    );
                }
            );
            // add(
            //     "t-xy_z(%s,%s)",
            //     +[](SenseConditions<Truth> sense_conditions_xy, SenseConditions<Truth> sense_conditions_z) -> WordSenseConditions<Truth> {
            //         return DSL::generate_word_sense_conditions<Truth>(
            //             sense_conditions_xy,
            //             sense_conditions_xy,
            //             sense_conditions_z
            //         );
            //     }
            // );
            // add(
            //     "t-x_y_z(%s,%s,%s)",
            //     +[](SenseConditions<Truth> sense_conditions_x, SenseConditions<Truth> sense_conditions_y, SenseConditions<Truth> sense_conditions_z) -> WordSenseConditions<Truth> {
            //         return DSL::generate_word_sense_conditions<Truth>(
            //             sense_conditions_x,
            //             sense_conditions_y,
            //             sense_conditions_z
            //         );
            //     }
            // );
            // add(
            //     "t-i(%s)",
            //     +[](Condition<Truth> condition_i) -> SenseConditions<Truth> {
            //         return DSL::generate_sense_conditions<Truth>(
            //             condition_i,
            //             DSL::False<Truth>(),
            //             DSL::False<Truth>()
            //         );
            //     }
            // );
            // add(
            //     "t-r(%s)",
            //     +[](Condition<Truth> condition_r) -> SenseConditions<Truth> {
            //         return DSL::generate_sense_conditions<Truth>(
            //             DSL::False<Truth>(),
            //             condition_r,
            //             DSL::False<Truth>()
            //         );
            //     }
            // );
            // add(
            //     "t-a(%s)",
            //     +[](Condition<Truth> condition_a) -> SenseConditions<Truth> {
            //         return DSL::generate_sense_conditions<Truth>(
            //             DSL::False<Truth>(),
            //             DSL::False<Truth>(),
            //             condition_a
            //         );
            //     }
            // );
            add(
                "T1-i_r(%s,%s)",
                +[](Condition<Truth> condition_i, Condition<Truth> condition_r) -> SenseConditions<Truth> {
                    return DSL::generate_sense_conditions<Truth>(
                        condition_i,
                        condition_r,
                        DSL::False<Truth>()
                    );
                }
            );
            // add(
            //     "t-i_a(%s,%s)",
            //     +[](Condition<Truth> condition_i, Condition<Truth> condition_a) -> SenseConditions<Truth> {
            //         return DSL::generate_sense_conditions<Truth>(
            //             condition_i,
            //             DSL::False<Truth>(),
            //             condition_a
            //         );
            //     }
            // );
            // add(
            //     "t-r_a(%s,%s)",
            //     +[](Condition<Truth> condition_r, Condition<Truth> condition_a) -> SenseConditions<Truth> {
            //         return DSL::generate_sense_conditions<Truth>(
            //             DSL::False<Truth>(),
            //             condition_r,
            //             condition_a
            //         );
            //     }
            // );
            // add(
            //     "t-i_r_a(%s,%s,%s)",
            //     +[](Condition<Truth> condition_i, Condition<Truth> condition_r, Condition<Truth> condition_a) -> SenseConditions<Truth> {
            //         return DSL::generate_sense_conditions<Truth>(
            //             condition_i,
            //             condition_r,
            //             condition_a
            //         );
            //     }
            // );
            ///////////////////////////////////////////
            // add(
            //     "f-xyz(%s)",
            //     +[](SenseConditions<Felicity> sense_conditions_xyz) -> WordSenseConditions<Felicity> {
            //         return DSL::generate_word_sense_conditions<Felicity>(
            //             sense_conditions_xyz,
            //             sense_conditions_xyz,
            //             sense_conditions_xyz
            //         );
            //     }
            // );
            // add(
            //     "f-xy_z(%s,%s)",
            //     +[](SenseConditions<Felicity> sense_conditions_xy, SenseConditions<Felicity> sense_conditions_z) -> WordSenseConditions<Felicity> {
            //         return DSL::generate_word_sense_conditions<Felicity>(
            //             sense_conditions_xy,
            //             sense_conditions_xy,
            //             sense_conditions_z
            //         );
            //     }
            // );
            // add(
            //     "F2-x_y_z(%s,%s,%s)",
            //     +[](SenseConditions<Felicity> sense_conditions_x, SenseConditions<Felicity> sense_conditions_y, SenseConditions<Felicity> sense_conditions_z) -> WordSenseConditions<Felicity> {
            //         return DSL::generate_word_sense_conditions<Felicity>(
            //             sense_conditions_x,
            //             sense_conditions_y,
            //             sense_conditions_z
            //         );
            //     }
            // );
            // add(
            //     "F2-x_y_z_part(%s,%s,%s,%s)",
            //     +[](SenseConditions<Felicity> sense_conditions_x, SenseConditions<Felicity> sense_conditions_y, SenseConditions<Felicity> sense_conditions_z, Condition<Felicity> condition_part) -> WordSenseConditions<Felicity> {
            //         return DSL::generate_word_sense_conditions_with_parts<Felicity>(
            //             sense_conditions_x,
            //             sense_conditions_y,
            //             sense_conditions_z,
            //             condition_part
            //         );
            //     }
            // );
            add(
                "F2-xy_z(%s,%s)",
                +[](SenseConditions<Felicity> sense_conditions_xy, SenseConditions<Felicity> sense_conditions_z) -> WordSenseConditions<Felicity> {
                    return DSL::generate_word_sense_conditions<Felicity>(
                        sense_conditions_xy,
                        sense_conditions_xy,
                        sense_conditions_z
                    );
                }
            );
            add(
                "F2-xy_z_part(%s,%s,%s)",
                +[](SenseConditions<Felicity> sense_conditions_xy, SenseConditions<Felicity> sense_conditions_z, Condition<Felicity> condition_part) -> WordSenseConditions<Felicity> {
                    return DSL::generate_word_sense_conditions_with_parts<Felicity>(
                        sense_conditions_xy,
                        sense_conditions_xy,
                        sense_conditions_z,
                        condition_part
                    );
                }
            );
            add(
                "F1-true",
                +[]() -> SenseConditions<Felicity> {
                    return DSL::generate_sense_conditions<Felicity>(
                        DSL::True<Felicity>(),
                        DSL::True<Felicity>(),
                        DSL::True<Felicity>()
                    );
                }
            );
            add(
                "F1-i(%s)",
                +[](Condition<Felicity> condition_i) -> SenseConditions<Felicity> {
                    return DSL::generate_sense_conditions<Felicity>(
                        condition_i,
                        DSL::True<Felicity>(),
                        DSL::True<Felicity>()
                    );
                }
            );
            add(
                "F1-r(%s)",
                +[](Condition<Felicity> condition_r) -> SenseConditions<Felicity> {
                    return DSL::generate_sense_conditions<Felicity>(
                        DSL::True<Felicity>(),
                        condition_r,
                        DSL::True<Felicity>()
                    );
                }
            );
            // add(
            //     "f-a(%s)",
            //     +[](Condition<Felicity> condition_a) -> SenseConditions<Felicity> {
            //         return DSL::generate_sense_conditions<Felicity>(
            //             DSL::True<Felicity>(),
            //             DSL::True<Felicity>(),
            //             condition_a
            //         );
            //     }
            // );
            add(
                "F1-i_r(%s,%s)",
                +[](Condition<Felicity> condition_i, Condition<Felicity> condition_r) -> SenseConditions<Felicity> {
                    return DSL::generate_sense_conditions<Felicity>(
                        condition_i,
                        condition_r,
                        DSL::True<Felicity>()
                    );
                }
            );
            // add(
            //     "f-i_a(%s,%s)",
            //     +[](Condition<Felicity> condition_i, Condition<Felicity> condition_a) -> SenseConditions<Felicity> {
            //         return DSL::generate_sense_conditions<Felicity>(
            //             condition_i,
            //             DSL::True<Felicity>(),
            //             condition_a
            //         );
            //     }
            // );
            // add(
            //     "f-r_a(%s,%s)",
            //     +[](Condition<Felicity> condition_r, Condition<Felicity> condition_a) -> SenseConditions<Felicity> {
            //         return DSL::generate_sense_conditions<Felicity>(
            //             DSL::True<Felicity>(),
            //             condition_r,
            //             condition_a
            //         );
            //     }
            // );
            // add(
            //     "f-i_r_a(%s,%s,%s)",
            //     +[](Condition<Felicity> condition_i, Condition<Felicity> condition_r, Condition<Felicity> condition_a) -> SenseConditions<Felicity> {
            //         return DSL::generate_sense_conditions<Felicity>(
            //             condition_i,
            //             condition_r,
            //             condition_a
            //         );
            //     }
            // );
            ///////////////////////////////////////////
            add(
                "T0(%s)",
                +[](Transformation transformation) -> Condition<Truth> {
                    switch(transformation) {
                        case Transformation::AlignStandard:
                            return DSL::aligned_standard;
                        case Transformation::AlignMirrored:
                            return DSL::aligned_mirrored;
                        case Transformation::AlignVertical: // Note: not used right now
                            return DSL::aligned_vertical;
                        case Transformation::Mimic:
                            return DSL::aligned_mimic;
                    }
                    return DSL::aligned_standard;
                }
            );
            // add(
            //     "truth'(%s,%s)",
            //     +[](Transformation transformation, Subcondition subcondition) -> Condition<Truth> {
            //         switch(transformation) {
            //             case Transformation::AlignStandard:
            //                 return DSL::condition_and_subcondition(DSL::aligned_standard, subcondition);
            //             case Transformation::AlignMirrored:
            //                 return DSL::condition_and_subcondition(DSL::aligned_mirrored, subcondition);
            //             case Transformation::AlignVertical:
            //                 return DSL::condition_and_subcondition(DSL::aligned_vertical, subcondition);
            //         }
            //         return DSL::condition_and_subcondition(DSL::aligned_standard, subcondition);
            //     }
            // );
            add(
                "F0(%s)",
                +[](Subcondition subcondition) -> Condition<Felicity> {
                    return DSL::to_condition<Felicity>(subcondition);
                }
            );
            add(
                "F0-false",
                +[]() -> Condition<Felicity> {
                    return DSL::False<Felicity>();
                }
            );
            // add(
            //     "presup(%s)",
            //     +[](Subcondition subcondition) -> Condition<Presupposition> {
            //         return subcondition.func;
            //     }
            // );
            add(
                "align-standard",
                +[]() -> Transformation {return Transformation::AlignStandard;},
                TERMINAL_WEIGHT
            );
            add(
                "align-mirrored",
                +[]() -> Transformation {return Transformation::AlignMirrored;},
                TERMINAL_WEIGHT
            );
            add(
                "mimic",
                +[]() -> Transformation {return Transformation::Mimic;},
                TERMINAL_WEIGHT
            );
            // add(
            //     "align-vertical",
            //     +[]() -> Transformation {return Transformation::AlignVertical;},
            //     TERMINAL_WEIGHT
            // );
            add(
                "and(%s,%s)",
                +[](Subcondition f, Subcondition g) -> Subcondition {return DSL::And(f, g); },
                TERMINAL_WEIGHT
            );
            add(
                "or(%s,%s)",
                +[](Subcondition f, Subcondition g) -> Subcondition {return DSL::Or(f, g); },
                TERMINAL_WEIGHT
            );
            add(
                "not(%s)",
                +[](Subcondition f) -> Subcondition {return DSL::Not(f); },
                TERMINAL_WEIGHT
            );
            add(
                "g-upward-up",
                +[]() -> Subcondition {
                    return Subcondition([](const Context& context, const Word& word, const Sense& sense) -> bool {
                        return context.ground.upward_is_up();
                    });
                },
                TERMINAL_WEIGHT
            );
            add(
                "g-upward-down",
                +[]() -> Subcondition {
                    return Subcondition([](const Context& context, const Word& word, const Sense& sense) -> bool {
                        return context.ground.upward_is_down();
                    });
                },
                TERMINAL_WEIGHT
            );
            add(
                "g-axis-vertical",
                +[]() -> Subcondition {
                    return Subcondition([](const Context& context, const Word& word, const Sense& sense) -> bool {
                        return context.ground.axis_is_vertical(word.axis);
                    });
                },
                TERMINAL_WEIGHT
            );
            // add(
            //     "g-upward-vertical",
            //     +[]() -> Subcondition {
            //         return Subcondition([](const Context& context, const Word& word, const Sense& sense) -> bool {
            //             return context.ground.axis_is_vertical(Axis::Z);
            //         });
            //     },
            //     TERMINAL_WEIGHT
            // );
            // add(
            //     "g-forward-vertical",
            //     +[]() -> Subcondition {
            //         return Subcondition([](const Context& context, const Word& word, const Sense& sense) -> bool {
            //             return context.ground.axis_is_vertical(Axis::Y);
            //         });
            //     },
            //     TERMINAL_WEIGHT
            // );
            // add(
            //     "g-rightward-vertical",
            //     +[]() -> Subcondition {
            //         return Subcondition([](const Context& context, const Word& word, const Sense& sense) -> bool {
            //             return context.ground.axis_is_vertical(Axis::X);
            //         });
            //     },
            //     TERMINAL_WEIGHT
            // );
            add("x",             Builtins::X<MyGrammar>, TERMINAL_WEIGHT);
            add(
                "W2-xyz(%s)",
                +[](SenseWeights sense_weights_xyz) -> WordSenseWeights {
                    return DSL::generate_word_sense_weights(
                        sense_weights_xyz,
                        sense_weights_xyz,
                        sense_weights_xyz
                    );
                });
            // add(
            //     "w-xy_z(%s,%s)",
            //     +[](SenseWeights sense_weights_xy, SenseWeights sense_weights_z) -> WordSenseWeights {
            //         return DSL::generate_word_sense_weights(
            //             sense_weights_xy,
            //             sense_weights_xy,
            //             sense_weights_z
            //         );
            //     });
            // add(
            //     "w-x_y_z(%s,%s,%s)",
            //     +[](SenseWeights sense_weights_x, SenseWeights sense_weights_y, SenseWeights sense_weights_z) -> WordSenseWeights {
            //         return DSL::generate_word_sense_weights(
            //             sense_weights_x,
            //             sense_weights_y,
            //             sense_weights_z
            //         );
            //     });
            add(
                "W1-i_r(%s,%s)",
                +[](Weight weight_i, Weight weight_r) -> SenseWeights {
                    return DSL::generate_sense_weights(
                        weight_i,
                        weight_r,
                        0.0);
                });
            // add(
            //     "w-i_r_a(%s,%s,%s)",
            //     +[](Weight weight_i, Weight weight_r, Weight weight_a) -> SenseWeights {
            //         return DSL::generate_sense_weights(
            //             weight_i,
            //             weight_r,
            //             weight_a);
            //     });
            // add(
            //     "w-ira(%s)",
            //     +[](Weight weight_ira) -> SenseWeights {
            //         return DSL::generate_sense_weights(
            //             weight_ira,
            //             weight_ira,
            //             weight_ira);
            //     });
            add("001", +[]() -> Weight {return 1.0;}, 128.0);
            add("002", +[]() -> Weight {return 0.5;}, 64.0);
            add("004", +[]() -> Weight {return 0.25;}, 32.0);
            add("008", +[]() -> Weight {return 0.125;}, 16.0);
            add("016", +[]() -> Weight {return 0.0625;}, 8.0);
            add("032", +[]() -> Weight {return 0.03125;}, 4.0);
            add("064", +[]() -> Weight {return 0.015625;}, 2.0);
            add("128", +[]() -> Weight {return 0.0078125;}, 1.0);
    }
} grammar;
