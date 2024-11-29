#pragma once

#include <memory>

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

struct Subcondition {
    std::function<bool(const Context&, const Word&, const Sense&)> func;

    Subcondition(std::function<bool(const Context&, const Word&, const Sense&)> f) :
        func(f){}

    bool operator()(const Context& context, const Word& word, const Sense& sense) const {
        return func(context, word, sense);
    }
};

using Weight = double;
using SenseWeights = std::unordered_map<Sense, Weight>;
using WordSenseWeights = std::unordered_map<Word, SenseWeights>;

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

    ReferenceDirection get_axial_direction(const Object& anchor, const Word& word) {
        const ReferenceDirection& direction = anchor.axial_directions.at(word.axis);
        if (direction) {
            return word.sign == Sign::Plus ? *direction : -(*direction);
        }
        return std::nullopt;
    }

    bool vectors_are_aligned(const UnitVector& g_to_f, const UnitVector& v, Alignment alignment) {
        if (g_to_f.axis == v.axis) {
            if (alignment == Alignment::PlusMinusParallel || v.sign == Sign::PlusMinus) {
                return true;
            }
            else if (alignment == Alignment::PlusParallel) {
                return g_to_f.sign == v.sign;
            } else {
                return g_to_f.sign != v.sign;
            }
        } else {
            return false;
        }
    }

    ReferenceDirection get_part_direction(const Object& anchor, const Word& word) {
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
        const UnitVector& g_to_f = context.g_to_f;
        const ReferenceDirection& direction = get_axial_direction(anchor, word);
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
        const UnitVector& g_to_f = context.g_to_f;
        const ReferenceDirection& direction = get_axial_direction(anchor, word);
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
            const UnitVector& g_to_f = context.g_to_f;
            const ReferenceDirection& direction = get_part_direction(anchor, word);
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
        const UnitVector& g_to_f = context.g_to_f;
        const ReferenceDirection& direction = get_axial_direction(anchor, word);
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