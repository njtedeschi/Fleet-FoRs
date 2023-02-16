#pragma once 

#include "DeterministicLOTHypothesis.h"
#include "CachedCallHypothesis.h"

class InnerHypothesis : public DeterministicLOTHypothesis<InnerHypothesis,MyInput,bool,MyGrammar,&grammar>,
                        public CachedCallHypothesis<InnerHypothesis,MyInput,bool>  {
public:
	using Super = DeterministicLOTHypothesis<InnerHypothesis,MyInput,bool,MyGrammar,&grammar>;
	using Super::Super; // inherit the constructors
	using CCH = CachedCallHypothesis<InnerHypothesis,MyInput,bool>;
	
	InnerHypothesis(const InnerHypothesis& c) : Super(c), CCH(c) {}	
	InnerHypothesis(const InnerHypothesis&& c) :  Super(c), CCH(c) { }	
	
	InnerHypothesis& operator=(const InnerHypothesis& c) {
		Super::operator=(c);
		CachedCallHypothesis::operator=(c);
		return *this;
	}
	InnerHypothesis& operator=(const InnerHypothesis&& c) {
		Super::operator=(c);
		CachedCallHypothesis::operator=(c);
		return *this;
	}
	
	void set_value(Node&  v) { 
		Super::set_value(v);
		CachedCallHypothesis::clear_cache();
	}
	void set_value(Node&& v) { 
		Super::set_value(v);
		CachedCallHypothesis::clear_cache();
	}

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
