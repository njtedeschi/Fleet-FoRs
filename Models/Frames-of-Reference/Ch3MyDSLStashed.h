#pragma once

#include <concepts>
#include <type_traits>
#include <unordered_map>
#include <memory>
#include <stdexcept>


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

    // Initialize from true/false
    Bool(bool v = false)
        : value(v ? JudgmentType::max_value : JudgmentType::min_value) {}

    // Initialize from Judgment enum value
    Bool(Judgment v)
        : value(v) {}

    // Initialize from Bool of "previous" JudgmentType
    template <typename PreviousJudgmentType>
    Bool (const Bool<PreviousJudgmentType>& other)
        : value(other.value) {}

    // Conversion operator to bool to allow Bool<JudgmentType> to be used as a boolean
    operator bool() const {
        return value == JudgmentType::max_value;
    }
};

// template <typename JudgmentType>
// struct Condition {
//     using FnType = std::function<Bool<JudgmentType>(const Context&, const Word&, const Sense&)>;
//     FnType func;

//     Condition(FnType f) : func(std::move(f)) {}

//     bool operator()(const Context& context, const Word& word, const Sense& sense) const {
//         return func(context, word, sense);
//     }
// };

template <typename Input, typename Key, typename Output>
struct FunctionWrapper {
    using FnType = std::function<Output(const Input&, const Key&)>;
    std::shared_ptr<FnType> fn_ptr;

    // Constructors
    FunctionWrapper() : fn_ptr(nullptr) {}
    explicit FunctionWrapper(const FnType& f) :
        fn_ptr(std::make_shared<FnType>(f)) {}
    explicit FunctionWrapper(FnType&& f)
    : fn_ptr(std::make_shared<FnType>(std::move(f)))

    // Call operator
    OutputType operator()(const Input& input, const Key& key) const {
        if (!fn_ptr || !(*fn_ptr)) {
            throw std::runtime_error("FunctionWrapper: Attempting to call an uninitialized function.");
        }
        return (*fn_ptr)(input, key);
    }

    // Move semantics
    FunctionWrapper(FunctionWrapper&&) noexcept = default;
    FunctionWrapper& operator=(FunctionWrapper&&) noexcept = default;

    // Copy semantics
    FunctionWrapper(const FunctionWrapper&) = default;
    FunctionWrapper& operator=(const FunctionWrapper&) = default;
};

template <typename Key, typename Value>
struct MapWrapper {
    using MapType = std::unordered_map<Key, Value>;
    std::shared_ptr<MapType> map_ptr;

    // Default constructor for an uninitialized map
    MapWrapper() : map_ptr(nullptr) {}

    // Constructor to accept a map by const reference
    explicit MapWrapper(const MapType& m)
        : map_ptr(std::make_shared<MapType>(m)) {}

    // Constructor to accept a map by rvalue reference
    explicit MapWrapper(MapType&& m)
        : map_ptr(std::make_shared<MapType>(std::move(m))) {}

    // Accessors
    const Value& at(const Key& key) const {
        ensureInitialized();
        return map_ptr->at(key);
    }

    // Modifiers
    void insert(const Key& key, const Value& value) {
        ensureInitialized();
        map_ptr->insert(key, value)
    }

    //

    // Accessors
    Value& operator[](const Key& key) {
        ensureInitialized();
        return (*map_ptr)[key];
    }

    const Value& at(const Key& key) const {
        ensureInitialized();
        return map_ptr->at(key);
    }

    // Modifiers
    void emplace(const Key& key, const Value& value) {
        ensureInitialized();
        map_ptr->emplace(key, value);
    }

    // Queries
    size_t size() const {
        ensureInitialized();
        return map_ptr->size();
    }

    bool empty() const {
        ensureInitialized();
        return map_ptr->empty();
    }

    // Iterators
    typename MapType::iterator begin() {
        ensureInitialized();
        return map_ptr->begin();
    }

    typename MapType::const_iterator begin() const {
        ensureInitialized();
        return map_ptr->begin();
    }

    typename MapType::iterator end() {
        ensureInitialized();
        return map_ptr->end();
    }

    typename MapType::const_iterator end() const {
        ensureInitialized();
        return map_ptr->end();
    }

    // Utility function to ensure the map is initialized
    void ensureInitialized() const {
        if (!map_ptr) {
            throw std::runtime_error("MapWrapper: Attempting to access an uninitialized map.");
        }
    }
};

// struct Subcondition {
//     using FnType = std::function<bool(const Context&, const Word&, const Sense&)>;
//     FnType func;

//     Subcondition(FnType f) : func(std::move(f)) {}

//     bool operator()(const Context& context, const Word& word, const Sense& sense) const {
//         return func(context, word, sense);
//     }
// };

// using Subcondition = std::function<bool(const Context&, const Word& const Sense&)>;

template <typename OuterKey, typename InnerKey, typename Value>
class MapManager {
private:
    // Private constructor for singleton
    MapManager() = default;

public:
    using InnerMap = MapWrapper<InnerKey, Value>;
    using OuterMap = std::unordered_map<OuterKey, InnerMap>;

    using FlatKey = std::pair<OuterKey, InnerKey>;
    using FlatHash = KeyPairHash<OuterKey, InnerKey>;
    using FlatMap = std::unordered_map<KeyPair, Value, FlatHash>;

    // Delete copy constructor and assignment operator
    MapManager(const MapManager&) = delete;
    MapManager& operator=(const MapManager&) = delete;

    // Accessor for Singleton instance
    static MapManager& get_instance() {
        static MapManager instance;
        return instance;
    }

    // Set possible key values
    void set_valid_keys(const OuterKeys& outer_keys, const InnerKeys& inner_keys) {
        valid_outer_keys = outer_keys;
        valid_inner_keys = inner_keys;
        num_outer_keys = valid_outer_keys.size();
        num_inner_keys = valid_inner_keys.size();
        num_keys = num_outer_keys * num_inner_keys;
    }

    InnerMap create_inner_map(const InnerKeys& keys, const InnerAssignmentFn& assignment_fn) {
        const auto& map = create_map<InnerKey, Value>(keys, assignment_fn);
        return InnerMap(map);
    }

    OuterMap create_outer_map(const OuterKeys& keys, const OuterAssignmentFn& assignment_fn) {
        return create_map<OuterKey, InnerMap>(keys, assignment_fn);
    }

    FlatMap create_flattened_outer_map(const OuterKeys& outer_keys, const OuterAssignmentFn& assignment_fn) {
        FlatMap flat_map;
        flat_map.reserve(outer_keys.size() * num_inner_keys);
        for (const auto& outer_key : outer_keys) {
            const InnerMap& inner_map = assignment_fn(outer_key);
            for (const auto& [inner_key, value] : inner_map) {
                flat_map.emplace(std::make_pair(outer_key, inner_key), value);
            }
        }
    }

protected:
    using OuterKeys = std::vector<OuterKey>;
    using InnerKeys = std::vector<InnerKey>;

    using OuterAssignmentFn = std::function<InnerMap(OuterKey)>;
    using InnerAssignmentFn = std::function<Value(InnerKey)>;

    std::vector<OuterKey> valid_outer_keys;
    std::vector<InnerKey> valid_inner_keys;
    int num_outer_keys;
    int num_inner_keys;
    int num_keys;

    template <typename K, typename V>
    std::unordered_map<K, V> create_map(const std::vector<K>& keys, const std::function<V(K)> assignment_fn) {
        std::unordered_map<K, V> map;
        map.reserve(keys.size());
        for (const auto& key : keys) {
            map.emplace(key, assignment_fn(key));
        }
        return map;
    }
};

template <typename InnerKey, typename OuterKey, typename JudgmentType, typename PreviousJudgmentType = void>
class JudgmentManager : public MapManager<InnerKey, OuterKey, Condition<JudgmentType>> {
private:
    using Base = MapManager<InnerKey, OuterKey, Condition<JudgmentType>>; // Alias for Base class
    // Private constructor for singleton
    JudgmentManager() = default;

public:
    using typename Base::OuterMap; // Import types from base class
    using typename Base::InnerMap;
    using Judgments = std::unordered_map<WordSense, Bool<JudgmentType>>;

    concept JudgmentIsInitial = std::is_same_v<PreviousJudgmentType, void>;
    concept JudgmentIsSubsequent = !std::is_same_v<PreviousJudgmentType, void>;
    using PreviousJudgments = std::unordered_map<WordSense, Bool<PreviousJudgmentType>>
        requires JudgmentIsSubsequent;

    // Delete copy constructor and assignment operator
    JudgmentManager(const JudgmentManager&) = delete;
    JudgmentManager& operator=(const JudgmentManager&) = delete;

    // Accessor for Singleton instance
    static JudgmentManager& get_instance() {
        static JudgmentManager instance;
        return instance;
    }

    // Public interface
    Judgments create_initial_judgments(const OuterMap& outer_map, const Context& context) {
        Judgments judgments;
        judgments.reserve(this->num_keys); // Access base member via `this`
        for (const auto& [outer_key, inner_map] : outer_map) {
            for (const auto& [inner_key, condition] : inner_map) {
                WordSense word_sense; // TODO constructor
                judgments.emplace(word_sense, condition(word_sense));
            }
        }
        return judgments;
    }

    Judgments create_updated_judgments(const OuterMap& outer_map, const Context& context, const PreviousJudgments& previous_judgments) {
        Judgments judgments;
        judgments.reserve(this->num_keys); // Access base member via `this`
        for (const auto& [outer_key, inner_map] : outer_map) {
            for (const auto& [inner_key, condition] : inner_map) {
                WordSense word_sense; // TODO constructor
                const auto& previous_judgment = previous_judgments.at(word_sense);
                if (previous_judgment && condition(context, word_sense)) {
                    judgments.emplace(word_sense, JudgmentType::max_value);
                } else {
                    judgments.emplace(word_sense, previous_judgment);
                }
            }
        }
        return judgments;
    }
};

using OuterKey = Word;
using InnerKey = Sense;

using Weight = double;
using WeightManager = MapManager<OuterKey, InnerKey, Weight>;
auto& weight_manager = WeightManager::get_instance();
using JudgmentManagerT = JudgmentManager<OuterKey, InnerKey, Truth>;
auto& judgment_manager_t = JudgmentManagerT::get_instance();
using JudgmentManagerF = JudgmentManager<OuterKey, InnerKey, Felicity, Truth>;
auto& judgment_manager_f = JudgmentManagerF::get_instance();
// using WordSenseConditionsT = JudgmentManagerT::OuterMap;

template <typename Value, bool store_ptr>
class WordSenseMapManager {
    using ValuePtr = std::shared_ptr<const Value>;
    using StoredValue = std::conditional_t<store_ptr, ValuePtr, Value>;
    using SenseMap = std::unordered_map<Sense, StoredValue>;
    using WordSenseMap = std::unordered_map<Word, SenseMap>;

public:
    static SenseMap create_sense_map(
        const std::vector<Sense>& senses,
        const Value& value_i,
        const Value& value_r,
        const Value& value_a
    ) {
        SenseMap sense_map;
        for (const auto& sense : senses) {
            if constexpr (store_ptr) {
                sense_map[sense] = get_value_ptr(sense, value_i, value_r, value_a);
            } else {
                sense_map[sense] = get_value_direct(sense, value_i, value_r, value_a);
            }
        }
        return sense_map;
    }

    static WordSenseMap create_word_sense_map(
        const std::vector<Word>& words,
        const SenseMap& sense_map_x,
        const SenseMap& sense_map_y,
        const SenseMap& sense_map_z
    ) {
        WordSenseMap word_sense_map;
        for (const auto& word : words) {
            word_sense_map[word] = get_sense_map(word, sense_map_x, sense_map_x, sense_map_x);
        }
    }

private:

    static Value get_value_direct(
        Sense sense,
        const Value& value_i,
        const Value& value_r,
        const Value& value_a
    ) {
        switch (sense) {
            case Sense::Intrinsic:
                return value_i;
            case Sense::Relative:
                return value_r;
            case Sense::Absolute:
                return value_a;
            default:
                throw std::invalid_argument("Unsupported sense.");
        }
    }

    static ValuePtr get_value_ptr(
        Sense sense,
        const Value& value_i,
        const Value& value_r,
        const Value& value_a
    ) {
        const Value* selected_value_ptr = nullptr;
        switch (sense) {
            case Sense::Intrinsic:
                selected_value_ptr = &value_i;
                break;
            case Sense::Relative:
                selected_value_ptr = &value_r;
                break;
            case Sense::Absolute:
                selected_value_ptr = &value_a;
                break;
            default:
                throw std::invalid_argument("Unsupported sense.");
        }
        return std::make_shared<const Value>(*selected_value_ptr);
    }

    static SenseMap get_sense_map(
        Word word,
        const SenseMap& sense_map_x,
        const SenseMap& sense_map_y,
        const SenseMap& sense_map_z
    ) {
        switch (word.axis) {
            case Axis::X:
                return sense_map_x;
            case Axis::Y:
                return sense_map_y;
            case Axis::Z:
                return sense_map_z;
            default:
                throw std::invalid_argument("Unsupported axis.");
        }
    }
};

template<typename JudgmentType, typename PreviousJudgmentType = void>
class JudgmentManager {
public:
    using Condition = std::function<Bool<JudgmentType>(const Context&, const Word&, const Sense&)>;
    using ConditionPtr = std::shared_ptr<const Condition<JudgmentType>>;
    using SenseConditions = std::unordered_map<Sense, ConditionPtr<JudgmentType>>;
    // using SenseConditionsPtr = std::shared_ptr<const SenseConditions<JudgmentType>>;
    using WordSenseConditions = std::unordered_map<Word, SenseConditions<JudgmentType>>;
    // using WordSenseConditions = std::unordered_map<Word, SenseConditionsPtr<JudgmentType>>;
    using WordSenseJudgments = std::unordered_map<Word, std::unordered_map<Sense, Bool<JudgmentType>>>;

    using MapManager = WordSenseMapManager<Condition, true>;

    concept JudgmentIsInitial = std::is_same_v<PreviousJudgmentType, void>;
    concept JudgmentIsSubsequent = !std::is_same_v<PreviousJudgmentType, void>;

    requires JudgmentIsSubsequent
    using PreviousJudgments = std::unordered_map<Word, std::unordered_map<Sense, Bool<PreviousJudgmentType>>>;

    static SenseConditions create_sense_map(
        const Condition& condition_i,
        const Condition& condition_r,
        const Condition& condition_a
    ) {
        const auto& senses = LanguageContext::get_senses();
        return MapManager::create_sense_map(
            senses,
            condition_i,
            condition_r,
            condition_a
        );
    }

    static WordSenseConditions create_word_sense_map(
        const SenseConditions& sense_conditions_x,
        const SenseConditions& sense_conditions_y,
        const SenseConditions& sense_conditions_z
    ) {
        const auto& words = LanguageContext::get_words();
        return MapManager::create_word_sense_map(
            words,
            sense_conditions_x,
            sense_conditions_y,
            sense_conditions_z
        );
    }

    // // Mixtec-type languages only
    // template <typename JudgmentType>
    // WordSenseConditions<JudgmentType> generate_word_sense_conditions_with_parts(
    //     const SenseConditions<JudgmentType>& sense_conditions_part_x,
    //     const SenseConditions<JudgmentType>& sense_conditions_part_y,
    //     const SenseConditions<JudgmentType>& sense_conditions_part_z,
    //     const SenseConditions<JudgmentType>& sense_conditions_nonpart_x,
    //     const SenseConditions<JudgmentType>& sense_conditions_nonpart_y,
    //     const SenseConditions<JudgmentType>& sense_conditions_nonpart_z
    // ) {
    //     WordSenseConditions<JudgmentType> word_sense_conditions;
    //     const auto& part_words = LanguageContext::get_part_words();
    //     update_word_sense_values<SenseConditions<JudgmentType>>(
    //         word_sense_conditions,
    //         part_words,
    //         sense_conditions_part_x,
    //         sense_conditions_part_y,
    //         sense_conditions_part_z
    //     );
    //     const auto& nonpart_words = LanguageContext::get_nonpart_words();
    //     update_word_sense_values<SenseConditions<JudgmentType>>(
    //         word_sense_conditions,
    //         nonpart_words,
    //         sense_conditions_nonpart_x,
    //         sense_conditions_nonpart_y,
    //         sense_conditions_nonpart_z
    //     );
    //     return word_sense_conditions;
    // }

    requires JudgmentIsInitial
    static WordSenseJudgments initial_judgments(const WordSenseConditions& word_sense_conditions, const Context& context) {
        WordSenseJudgments judgments;
        judgments.reserve(word_sense_conditions.size());
        for (const auto& [word, sense_conditions] : word_sense_conditions) {
            // for (const auto& [sense, condition] : *sense_conditions) {
            for (const auto& [sense, condition] : sense_conditions) {
                judgments[word][sense] = (*condition)(context, word, sense);
            }
        }
        return judgments;
    }

    requires JudgmentIsSubsequent
    static WordSenseJudgments updated_judgments(const WordSenseConditions& word_sense_conditions, const Context& context, const PreviousJudgments& previous_judgments) {
        WordSenseJudgments judgments;
        judgments.reserve(word_sense_conditions.size());
        for (const auto& [word, sense_conditions] : word_sense_conditions) {
            // for (const auto& [sense, condition] : *sense_conditions) {
            for (const auto& [sense, condition] : sense_conditions) {
                const auto& previous_judgment = previous_judgments.at(word).at(sense);
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

    // Higher order function methods

    // Condition<Truth> condition_and_subcondition(const Condition<Truth>& f, const Subcondition& g) {
    //     return [f, g](const Context& context, const Word& word, const Sense& sense) {
    //         return f(context, word, sense) && g(context, word, sense);
    //     };
    // }

    // TODO: Super hacky; make more efficient
    static Condition to_condition(const Subcondition& f) {
        return [f](const Context& context, const Word& word, const Sense& sense) {
            // return Bool<JudgmentType>(f(context, word, sense));
            return f(context, word, sense);
        };
    }

    static Condition True() {
        return [](const Context& context, const Word&, const Sense&) {return true;};
    }

    static Condition False() {
        return [](const Context& context, const Word&, const Sense&) {return false;};
    }
};

namespace DSL {
    // Higher-order boolean functions
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

    // Spatial functions
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
        if (sense == Sense::Intrinsic && !anchor.part_directions.empty()) {
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
}