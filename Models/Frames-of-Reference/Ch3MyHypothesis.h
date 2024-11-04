#pragma once

#include "DeterministicLOTHypothesis.h"
// #include "CachedCallHypothesis.h"

#include "Ch3MyStructures.h"
#include "Ch3MyGrammar.h"

// model reliability parameters
// static const double alpha_p = 0.9;
static const double alpha_t = 0.9;
static const double alpha_f = 0.9;
/* static const size_t MAX_NODES = 10; */

class MyHypothesis : public DeterministicLOTHypothesis<MyHypothesis,Context,MyOutput,MyGrammar,&grammar,MyInput> {

public:
	using Super = DeterministicLOTHypothesis<MyHypothesis,Context,MyOutput,MyGrammar,&grammar,MyInput>;
	using Super::Super;

    // static const size_t MAX_NODES = 20;

	// MyHypothesis(const MyHypothesis& c) : Super(c) {}
	// MyHypothesis(const MyHypothesis&& c) :  Super(c) { }

	// MyHypothesis& operator=(const MyHypothesis& c) {
	// 	Super::operator=(c);
	// 	return *this;
	// }
	// MyHypothesis& operator=(const MyHypothesis&& c) {
	// 	Super::operator=(c);
	// 	return *this;
	// }

	// void set_value(Node&  v) {
	// 	Super::set_value(v);
	// }
	// void set_value(Node&& v) {
	// 	Super::set_value(v);
	// }

    virtual double compute_prior() override {
        return prior = Super::compute_prior();
    }

    // [[nodiscard]] virtual ProposalType propose() const override {

	// 	ProposalType p;

	// 	if(flip(0.85))      p = Proposals::regenerate(&grammar, value);
    //             else if(flip(0.5))  p = Proposals::sample_function_leaving_args(&grammar, value);
	// 	else if(flip(0.5))  p = Proposals::swap_args(&grammar, value);
	// 	else if(flip(0.5))     p = Proposals::insert_tree(&grammar, value);
	// 	else                p = Proposals::delete_tree(&grammar, value);

	// 	return p;
	// }

    virtual double compute_likelihood(const data_t& data, double breakout=-infinity) override {
        likelihood = 0.0;
        for (size_t i = 0; i < data.size(); i++) {
            const datum_t& datum = data[i];
            likelihood += compute_single_likelihood(datum);
        }
        return likelihood;
    }

    // Truth only
    // virtual double compute_single_likelihood(const datum_t& datum) override {
    //     MyOutput output = call(datum.context);
    //     const auto& judgments = output.first;
    //     const auto& weights = output.second;

    //     double numerators[2] = {0};
    //     double denominators[2] = {0};
    //     for (const auto& [word, sense_judgments] : judgments) {
    //         bool is_observed_word = (word.form == datum.utterance);
    //         for (const auto& [sense, judgment] : sense_judgments) {
    //             double weight = weights.at(word).at(sense);
    //             // for (int j = 0; j < 2; j++) {
    //             //     if (static_cast<int>(judgment) >= j) {
    //             //         numerators[j] += is_observed_word ? weight : 0;
    //             //         denominators[j] += weight;
    //             //     }
    //             // }
    //             numerators[0] += is_observed_word ? weight : 0;
    //             denominators[0] += weight;
    //             if (judgment.value == Judgment::True) {
    //                 numerators[1] += is_observed_word ? weight : 0;
    //                 denominators[1] += weight;
    //             }
    //         }
    //     }

    //     // double p = (numerators[1] ? alpha_t*numerators[1]/denominators[1] : 0) + (1-alpha_t)*numerators[0]/denominators[0];
    //     double p = (1.0 - alpha_t)*numerators[0]/denominators[0];
    //     p += (numerators[1] != 0) ? alpha_t*numerators[1]/denominators[1] : 0;
    //     return log(p);
    // }

    // // Truth + Felicity
    virtual double compute_single_likelihood(const datum_t& datum) override {
        MyOutput output = call(datum.context);
        const auto& judgments = output.first;
        const auto& weights = output.second;

        double numerators[3] = {0};
        double denominators[3] = {0};
        for (const auto& [word, sense_judgments] : judgments) {
            bool is_observed_word = (word.form == datum.utterance);
            for (const auto& [sense, judgment] : sense_judgments) {
                double weight = weights.at(word).at(sense);
                for (int j = 0; j < 3; j++) {
                    if (static_cast<int>(judgment) >= j) {
                        numerators[j] += is_observed_word ? weight : 0;
                        denominators[j] += weight;
                    }
                }
            }
        }

        double p = (1.0 - alpha_t)*numerators[0]/denominators[0];
        p += (numerators[1] != 0) ? (1-alpha_f)*alpha_t*numerators[1]/denominators[1] : 0;
        p += (numerators[2] != 0) ? alpha_f*alpha_t*numerators[2]/denominators[2] : 0;
        return log(p);
    }

    virtual void show(std::string prefix="") override {
        print(":", prefix, this->posterior, this->prior, this->likelihood);
        print(this->string());
    }
};
