#pragma once 

#include "EigenLib.h"
#include "Interfaces/MCMCable.h"

/**
 * @class VectorHypothesis
 * @author Steven Piantadosi
 * @date 09/08/20
 * @file VectorHypothesis.h
 * @brief This has all of the MCMCable interfaces but jsut represents n unit Gaussians. This
 * 		  is primarily used in GrammarInference to represent parameters, but it could be used
 *  	  in any other too
 */
class VectorHypothesis : public MCMCable<VectorHypothesis, void*> {
	// This represents a vector of reals, defaultly here just unit normals. 
	// This gets used in GrammarHypothesis to store both the grammar values and
	// parameters for the model. 
public:

	typedef VectorHypothesis self_t; 
	
	double MEAN = 0.0;
	double SD   = 1.0;
	double PROPOSAL_SCALE = 0.20; 
	
	Vector value;
	
	// whether each element of value is constant or not? 
	// This is useful because sometimes we don't want to do MCMC on some parts of the grammar
	std::vector<bool> can_propose; 
	
	VectorHypothesis() {
	}

	VectorHypothesis(int n) {
		set_size(n);
	}
	
	double operator()(int i) const {
		// So we can treat this hypothesis like a vector
		return value(i); 
	}
	
	void set(int i, double v) {
		value(i) = v;
	}
	
	/**
	 * @brief Set whether we can propose to each element of b or not
	 * @param i
	 */	
	void set_can_propose(size_t i, bool b) {
		can_propose[i] = b;
	}
	
	void set_size(size_t n) {
		value = Vector::Zero(n);
		can_propose.resize(n,true);
	}
	
	virtual double compute_prior() override {
		// Defaultly a unit normal 
		this->prior = 0.0;
		
		for(auto i=0;i<value.size();i++) {
			this->prior += normal_lpdf(value(i), MEAN, SD);
		}
		
		return this->prior;
	}
	
	
	virtual double compute_likelihood(const data_t& data, const double breakout=-infinity) override {
		throw YouShouldNotBeHereError("*** Should not call likelihood here");
	}
	
	
	virtual std::pair<self_t,double> propose() const override {
		self_t out = *this;
		
		// choose an index
		// (NOTE -- if can_propose is all false, this might loop infinitely...)
		size_t i;
		do {
			i = myrandom(value.size()); 
		} while(!can_propose[i]);
		
		// propose to one coefficient w/ SD of 0.1
		out.value(i) = value(i) + PROPOSAL_SCALE*random_normal();
		
		// everything is symmetrical so fb=0
		return std::make_pair(out, 0.0);	
	}
	virtual self_t restart() const override {
		self_t out = *this;
		for(auto i=0;i<value.size();i++) {
			if(out.can_propose[i]) {
				// we don't want to change parameters unless we can propose to them
				out.value(i) = MEAN + SD*random_normal();
			}
		}
		return out;
	}
	
	virtual size_t hash() const override {
		if(value.size() == 0) return 0; // hmm what to do here?
		
		size_t output = std::hash<double>{}(value(0)); 
		for(auto i=1;i<value.size();i++) {
			hash_combine(output, std::hash<double>{}(value(i)));
		}
		return output;
	}
	
	virtual bool operator==(const self_t& h) const override {
		return value.size() == h.value.size() and value == h.value;
	}
	
	virtual std::string string(std::string prefix="") const override {
		std::string out = prefix+"NV<";
		for(auto i=0;i<value.size();i++) {
			out += str(value(i));
		}
		out += ">";
		return out; 
	}
};
