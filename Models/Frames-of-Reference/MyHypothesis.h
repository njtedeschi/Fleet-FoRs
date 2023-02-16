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

        virtual double compute_prior() override {
            return prior = ( get_value().count() < MAX_NODES ? Super::compute_prior() : -infinity);
        }

        /**
	 * @brief This computes the weight of this factor from its cached values
	 * @return 
	 */	
	double get_weight_fromcache() const {
		if(cache.size() == 0) { return 1.0; } // for empty cache
		
		int numtrue = 0;
		for(const auto& v : cache) {
			numtrue += (v == true);
		}
		
		// use the actual weight formula
		return 1.0 / (0.1 + double(numtrue) / cache.size());
	}
};

#include "Lexicon.h"

struct ignore_t  {}; /// we don't need inputs/outputs for out MyHypothesis
class MyHypothesis : public Lexicon<MyHypothesis, std::string, InnerHypothesis, ignore_t,ignore_t, MyInput> {

    using Super = Lexicon<MyHypothesis, std::string, InnerHypothesis, ignore_t, ignore_t, MyInput>;
    using Super::Super; // inherit the constructors

public:

    void clear_cache(){
        for(auto& [k,f] : factors) {
                f.clear_cache();
                }
    }

    virtual double compute_likelihood(const data_t& data, double breakout=-infinity) override {
        // need to set this; probably not necessary?
		for(auto& [k,f] : factors) 
			f.program.loader = this; 		

		// get the cached version
		for(auto& [k,f] : factors) {
			f.cached_call(data);
			
			if(f.got_error) 
				return likelihood = -infinity;
		}
		
		// Compute weights
		std::map<key_t,double> weights; 
		double weight_total = 0.0;
		for(auto& [k, f] : factors) {
			weights[k] = f.get_weight_fromcache();
			weight_total += weights[k];
		}
		

		likelihood = 0.0;
                for(size_t i=0;i<data.size();i++) {
			const MyInput& input = data[i];
			
			// Check all the words that are true and select 
			bool word_is_true   = false;
			double weight_true = 0.0; // total weight of those that are true
			
			// must loop over each word and see when it is true
			for(auto& [k,f] : factors) {
				// we just call the right factor on utterance -- note that the "word" in u is
				// ignored within the function call
				auto tv = f.cache.at(i);

                                if (tv == true) {
                                    weight_true += weights[k];
                                }
				
				if(input.word == k) {
					word_is_true   = (tv == true);
				}
			}
			
			// Size principle calculation with weights
			double weight_word = weights.at(input.word);
			double p = (word_is_true ? alpha_t*weight_word/weight_true : 0)  + 
					   (1.0-alpha_t)*weight_word/weight_total;				
			likelihood += log(p);
		}
		
		return likelihood;
            }

    // Printing functions: string and show
    virtual void show(std::string prefix="") override {
        extern MyHypothesis target;

        print(":", prefix, this->posterior, this->prior, this->likelihood, target.likelihood);
        print(this->string());
    }
};

