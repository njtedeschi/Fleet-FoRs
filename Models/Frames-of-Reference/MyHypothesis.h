#pragma once 

#include "DeterministicLOTHypothesis.h"
#include "CachedCallHypothesis.h"

class InnerHypothesis : public DeterministicLOTHypothesis<InnerHypothesis,MyInput,bool,MyGrammar,&grammar>,
                        public CachedCallHypothesis<InnerHypothesis,MyInput,bool>  {

};

#include "Lexicon.h"

struct ignore_t  {}; /// we don't need inputs/outputs for out MyHypothesis
class MyHypothesis : public Lexicon<MyHypothesis, std::string, InnerHypothesis, ignore_t,ignore_t, MyInput> {

    using Super = Lexicon<MyHypothesis, std::string, InnerHypothesis, ignore_t, ignore_t, MyInput>;
    using Super::Super; // inherit the constructors

public:

    void clear_cache(){
        // Is this necessary?
    }

    virtual double compute_likelihood(const data_t& data, double breakout=-infinity) override {

    }

    // Printing functions: string and show

};
