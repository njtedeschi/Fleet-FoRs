#pragma once

#include <memory>

#include "Grammar.h"
#include "Singleton.h"
#include "Functional.h"

#include "Ch3MyStructures.h"

double TERMINAL_WEIGHT = 5.0;

using Weight = double;
using SenseWeights = std::unordered_map<Sense, Weight>;
using WordSenseWeights = std::unordered_map<Word, SenseWeights>;

struct Subcondition {
    std::function<bool(const Context&, const Word&, const Sense&)> func;

    Subcondition(std::function<bool(const Context&, const Word&, const Sense&)> f) :
        func(f){}

    bool operator()(const Context& context, const Word& word, const Sense& sense) const {
        return func(context, word, sense);
    }
};


enum class Judgment {
    False = 0,
    True = 1,
    Felicitous = 2
};

struct Truth {
    static constexpr Judgment max_value = Judgment::True;
    static constexpr Judgment min_value = Judgment::False;
};

struct Felicity {
    static constexpr Judgment max_value = Judgment::Felicitous;
    static constexpr Judgment min_value = Judgment::False;
};

template <typename JudgmentType>
struct Bool {
    Judgment value;

    Bool(bool v = false)
        : value(v ? JudgmentType::max_value : JudgmentType::min_value) {}

    Bool(Judgment v)
        : value(v) {}

    template <typename PreviousJudgmentType>
    Bool (const Bool<PreviousJudgmentType>& other)
        : value(other.value) {}

    // Conversion operator to bool to allow Bool<JudgmentType> to be used as a boolean
    operator bool() const {
        return value == JudgmentType::max_value;
    }
};

template <typename JudgmentType>
using Condition = std::function<Bool<JudgmentType>(const Context&, const Word&, const Sense&)>;
template <typename JudgmentType>
using SenseConditions = std::unordered_map<Sense, std::shared_ptr<const Condition<JudgmentType>>>;
template <typename JudgmentType>
using WordSenseConditions = std::unordered_map<Word, std::shared_ptr<const SenseConditions<JudgmentType>>>;
template <typename JudgmentType>
using WordSenseJudgments = std::unordered_map<Word, std::unordered_map<Sense, Bool<JudgmentType>>>;

namespace DSL {

    template <typename JudgmentType>
    WordSenseJudgments<JudgmentType> initial_word_sense_judgments(const WordSenseConditions<JudgmentType>& word_sense_conditions, const Context& context) {
        WordSenseJudgments<JudgmentType> judgments;
        judgments.reserve(word_sense_conditions.size());
        for (const auto& [word, sense_conditions] : word_sense_conditions) {
            for (const auto& [sense, condition] : *sense_conditions) {
                judgments[word][sense] = (*condition)(context, word, sense);
            }
        }
        return judgments;
    }

    template <typename JudgmentType, typename PreviousJudgmentType>
    WordSenseJudgments<JudgmentType> updated_word_sense_judgments(const WordSenseConditions<JudgmentType>& word_sense_conditions, const Context& context, const WordSenseJudgments<PreviousJudgmentType>& previous_judgments) {
        WordSenseJudgments<JudgmentType> judgments;
        judgments.reserve(word_sense_conditions.size());
        for (const auto& [word, sense_conditions] : word_sense_conditions) {
            for (const auto& [sense, condition] : *sense_conditions) {
                Bool<PreviousJudgmentType> previous_judgment = previous_judgments.at(word).at(sense);
                if (previous_judgment && (*condition)(context, word, sense)) {
                    judgments[word][sense] = JudgmentType::max_value;
                }
                else {
                    judgments[word][sense] = previous_judgment;
                }
            }
        }
        return judgments;
    }

    template <typename JudgmentType>
    WordSenseConditions<JudgmentType> generate_word_sense_conditions(
        const SenseConditions<JudgmentType>& sense_conditions_x,
        const SenseConditions<JudgmentType>& sense_conditions_y,
        const SenseConditions<JudgmentType>& sense_conditions_z
    ) {
        WordSenseConditions<JudgmentType> word_sense_conditions;
        for (const auto& word : LanguageContext::get_words()) {
            switch(word.axis) {
                case Axis::X:
                    word_sense_conditions.emplace(word, std::make_shared<const SenseConditions<JudgmentType>>(sense_conditions_x));
                    break;
                case Axis::Y:
                    word_sense_conditions.emplace(word, std::make_shared<const SenseConditions<JudgmentType>>(sense_conditions_y));
                    break;
                case Axis::Z:
                    word_sense_conditions.emplace(word, std::make_shared<const SenseConditions<JudgmentType>>(sense_conditions_z));
                    break;
            }
        }
        return word_sense_conditions;
    }

    template <typename JudgmentType>
    WordSenseConditions<JudgmentType> generate_word_sense_conditions_with_parts(
        const SenseConditions<JudgmentType>& sense_conditions_x,
        const SenseConditions<JudgmentType>& sense_conditions_y,
        const SenseConditions<JudgmentType>& sense_conditions_z,
        const Condition<JudgmentType>& condition_part
    ) {
        WordSenseConditions<JudgmentType> word_sense_conditions;
        for (const auto& word : LanguageContext::get_words()) {
            const SenseConditions<JudgmentType>* sense_conditions_axis = nullptr;
            switch(word.axis) {
                case Axis::X:
                    sense_conditions_axis = &sense_conditions_x;
                    break;
                case Axis::Y:
                    sense_conditions_axis = &sense_conditions_y;
                    break;
                case Axis::Z:
                    sense_conditions_axis = &sense_conditions_z;
                    break;
            }
            if (!sense_conditions_axis) {
                throw std::invalid_argument("Some axis lacked sense conditions");
            }

            if (word.part) {
                auto sense_conditions = *sense_conditions_axis;
                sense_conditions[Sense::Intrinsic] = std::make_shared<const Condition<JudgmentType>>(condition_part);
                word_sense_conditions.emplace(word, std::make_shared<const SenseConditions<JudgmentType>>(sense_conditions));
            } else {
                word_sense_conditions.emplace(word, std::make_shared<const SenseConditions<JudgmentType>>(*sense_conditions_axis));
            }
        }
        return word_sense_conditions;
    }

    template <typename JudgmentType>
    SenseConditions<JudgmentType> generate_sense_conditions(
        const Condition<JudgmentType>& condition_i,
        const Condition<JudgmentType>& condition_r,
        const Condition<JudgmentType>& condition_a
    ) {
        SenseConditions<JudgmentType> sense_conditions;
        for (const auto& sense : LanguageContext::get_senses()) {
            switch(sense) {
                case Sense::Intrinsic:
                    sense_conditions[sense] = std::make_shared<const Condition<JudgmentType>>(condition_i);
                    break;
                case Sense::Relative:
                    sense_conditions[sense] = std::make_shared<const Condition<JudgmentType>>(condition_r);
                    break;
                case Sense::Absolute:
                    sense_conditions[sense] = std::make_shared<const Condition<JudgmentType>>(condition_a);
                    break;
            }
        }
        return sense_conditions;
    }

    WordSenseWeights generate_word_sense_weights(
        const SenseWeights& sense_weights_x,
        const SenseWeights& sense_weights_y,
        const SenseWeights& sense_weights_z
    ) {
        WordSenseWeights word_sense_weights;
        for (const auto& word : LanguageContext::get_words()) {
            switch(word.axis) {
                case Axis::X:
                    word_sense_weights[word] = sense_weights_x;
                    break;
                case Axis::Y:
                    word_sense_weights[word] = sense_weights_y;
                    break;
                case Axis::Z:
                    word_sense_weights[word] = sense_weights_z;
                    break;
            }
        }
        return word_sense_weights;
    }

    SenseWeights generate_sense_weights(
        const Weight& weight_i,
        const Weight& weight_r,
        const Weight& weight_a
    ) {
        SenseWeights sense_weights;
        for (const auto& sense : LanguageContext::get_senses()) {
            switch(sense) {
                case Sense::Intrinsic:
                    sense_weights[sense] = weight_i;
                    break;
                case Sense::Relative:
                    sense_weights[sense] = weight_r;
                    break;
                case Sense::Absolute:
                    sense_weights[sense] = weight_a;
                    break;
            }
        }
        return sense_weights;
    }

    const Object& get_anchor(const Context& context, const Sense& sense) {
        switch (sense) {
            case Sense::Intrinsic:
                return context.ground;
            case Sense::Relative:
                return context.speaker;
            case Sense::Absolute:
                return context.environment;
        }
        return context.ground;
    }

    std::optional<Direction> get_axial_direction(const Object& anchor, const Word& word) {
        const std::optional<Direction>& direction = anchor.axial_directions.at(word.axis);
        if (direction) {
            return word.sign == Sign::Plus ? *direction : -(*direction);
        }
        return std::nullopt;
    }

    bool vectors_are_aligned(const Displacement& g_to_f, const Direction& reference_direction, Alignment alignment) {
        // NOTE: assumes unit vectors
        double cosine_similarity = dot_product(g_to_f, reference_direction);
        switch (alignment) {
            case Alignment::PlusParallel:
                return cosine_similarity == 1;
            case Alignment::MinusParallel:
                return cosine_similarity == -1;
            case Alignment::PlusMinusParallel:
                return (cosine_similarity == 1 || cosine_similarity == -1);
            default:
                return false;
        }
    }

    std::optional<Direction> get_part_direction(const Object& anchor, const Word& word) {
        if (word.part && !anchor.part_directions.empty()) {
            return anchor.part_directions.at(*word.part);
        }
        return std::nullopt;
    }

    // NOTE: Assuming distances are all 1
    bool aligned_standard(const Context& context, const Word& word, const Sense& sense) {
        const Object& anchor = get_anchor(context, sense);
        // if (!anchor.has_all_axes()) {
        //     return false;
        // }
        const Displacement& g_to_f = context.g_to_f;
        const std::optional<Direction>& direction = get_axial_direction(anchor, word);
        if (direction) {
            return vectors_are_aligned(g_to_f, *direction, Alignment::PlusParallel);
        }
        return false;
    }

    bool aligned_mirrored(const Context& context, const Word& word, const Sense& sense) {
        const Object& anchor = get_anchor(context, sense);
        // if (!anchor.has_all_axes()) {
        //     return false;
        // }
        const Displacement& g_to_f = context.g_to_f;
        const std::optional<Direction>& direction = get_axial_direction(anchor, word);
        if (direction) {
            if (word.axis == Axis::Y) {
                return vectors_are_aligned(g_to_f, *direction, Alignment::MinusParallel);
            }
            return vectors_are_aligned(g_to_f, *direction, Alignment::PlusParallel);
        }
        return false;
    }

    bool aligned_mimic(const Context& context, const Word& word, const Sense& sense) {
        const Object& anchor = context.ground; // Only intrinsic FoRs, at least right now
        if (sense == Sense::Intrinsic && word.part && !anchor.part_directions.empty()) {
            const Displacement& g_to_f = context.g_to_f;
            const std::optional<Direction>& direction = get_part_direction(anchor, word);
            if (direction) {
                return vectors_are_aligned(g_to_f, *direction, Alignment::PlusParallel);
            }
        }
        return false;
    }

    bool aligned_vertical(const Context& context, const Word& word, const Sense& sense) {
        if (word.axis != Axis::Z) {
            return false;
        }
        const Object& anchor = get_anchor(context, sense);
        const Displacement& g_to_f = context.g_to_f;
        const std::optional<Direction>& direction = get_axial_direction(anchor, word);
        if (direction) {
            return vectors_are_aligned(g_to_f, *direction, Alignment::PlusParallel);
        }
        return false;
    }

    bool anchor_has_axis(const Context& context, const Word& word, const Sense& sense) {
        const Object& anchor = get_anchor(context, sense);
        return anchor.has_axis(word.axis);
    }

    bool anchor_has_part(const Context& context, const Word& word, const Sense& sense) {
        if (word.part) {
            const Object& anchor = get_anchor(context, sense);
            return anchor.has_part(*word.part);
        }
        return false;
    }

    // Condition<Truth> condition_and_subcondition(const Condition<Truth>& f, const Subcondition& g) {
    //     return [f, g](const Context& context, const Word& word, const Sense& sense) {
    //         return f(context, word, sense) && g(context, word, sense);
    //     };
    // }

    // TODO: Super hacky; make more efficient
    template <typename JudgmentType>
    Condition<JudgmentType> to_condition(const Subcondition& f) {
        return [f](const Context& context, const Word& word, const Sense& sense) {
            // return Bool<JudgmentType>(f(context, word, sense));
            return f(context, word, sense);
        };
    }

    Subcondition And(const Subcondition& f, const Subcondition& g) {
        return Subcondition([f, g](const Context& context, const Word& word, const Sense& sense) {
            return f(context, word, sense) && g(context, word, sense);
        });
    }

    Subcondition Or(const Subcondition& f, const Subcondition& g) {
        return Subcondition([f, g](const Context& context, const Word& word, const Sense& sense) {
            return f(context, word, sense) && g(context, word, sense);
        });
    }

    Subcondition Not(const Subcondition& f) {
        return Subcondition([f](const Context& context, const Word& word, const Sense& sense) {
            return !f(context, word, sense);
        });
    }

    template <typename JudgmentType>
    Condition<JudgmentType> True() {
        return [](const Context& context, const Word&, const Sense&) {return true;};
    }

    template <typename JudgmentType>
    Condition<JudgmentType> False() {
        return [](const Context& context, const Word&, const Sense&) {return false;};
    }
}

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
