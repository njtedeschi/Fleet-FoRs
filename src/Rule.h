#pragma once

#include <functional>
#include <string>
#include <vector>

class Rule {
	// A rule stores nonterminal types and possible children
	// Here we "emulate" a type system using t_nonterminal to store an integer for the types
	// this means that the type only needs to be finally discovered in VirtualMachineState, since
	// everything else can operate on a uniforntrules. 
	
public:
	nonterminal_t         nt;
	Instruction           instr; // a template for my instruction, which here mainly stores my optype
	std::string           format; // how am I printed?
	size_t                N; // how many children?
	// this next one should be a vector, but gcc doesn't like copying it for some reason
	nonterminal_t         child_types[Fleet::MAX_CHILD_SIZE]; // An array of what I expand to; note that this should be const but isn't to allow list initialization (https://stackoverflow.com/questions/5549524/how-do-i-initialize-a-member-array-with-an-initializer-list)
	double                p;
protected:
	std::size_t          my_hash; // a hash value for this rule
	
public:
	// Rule's constructors convert CustomOp and BuiltinOp to the appropriate instruction types
	Rule(const nonterminal_t rt, const CustomOp o, const std::string fmt, std::initializer_list<nonterminal_t> c, double _p, const int arg=0) :
		nt(rt), instr(o,arg), format(fmt), N(c.size()), p(_p) {
		// mainly we just convert c to an array
		assert(c.size() < Fleet::MAX_CHILD_SIZE);
		std::copy(c.begin(), c.end(), child_types);
		
		// Set up hashing for rules (cached so we only do it once)
		std::hash<std::string> h; 
		my_hash = h(fmt);
		hash_combine(my_hash, (size_t) o, (size_t) arg, (size_t)nt);
		for(size_t i=0;i<N;i++) 
			hash_combine(my_hash, i, (size_t)child_types[i]);
		
		
		// check that the format string has the right number of %s
		assert( N == count(fmt, ChildStr) && "*** Wrong number of format string arguments");
	}
		
	Rule(const nonterminal_t rt, const BuiltinOp o, const std::string fmt, std::initializer_list<nonterminal_t> c, double _p, const int arg=0) :
		nt(rt), instr(o,arg), format(fmt), N(c.size()), p(_p) {
		
		// mainly we just convert c to an array
		assert(c.size() < Fleet::MAX_CHILD_SIZE);
		std::copy(c.begin(), c.end(), child_types);
		
		// Set up hashing for rules (cached so we only do it once)
		std::hash<std::string> h; 
		my_hash = h(fmt);
		hash_combine(my_hash, (size_t) o);
		hash_combine(my_hash, (size_t) arg);
		hash_combine(my_hash, (size_t) nt);
		for(size_t i=0;i<N;i++) hash_combine(my_hash, (size_t)child_types[i]);
		
		// check that the format string has the right number of %s
		assert( N == count(fmt, ChildStr) && "*** Wrong number of format string arguments");
	}
	
	bool operator<(const Rule& r) const {
		// This is structured so that we always put terminals first and then we put the HIGHER probability things first. 
		// this helps in enumeration
		if(N < r.N) return true;
		else		return p > r.p; // weird, but helpful, that we sort in decreasing order of probability
	}
	bool operator==(const Rule& r) const {
		if(not (nt==r.nt and instr==r.instr and format==r.format and N==r.N and p==r.p)) return false;
		for(size_t i=0;i<N;i++) {
			if(child_types[i] != r.child_types[i]) return false;
		}
		return true;
	}
	
	size_t get_hash() const {
		return my_hash;
	}
	
	bool is_terminal() const {
		// I am a terminal rule if I have no children
		return N==0;
	}
	
};


// A single constant NullRule for gaps in trees. Always has type 0
const Rule* NullRule = new Rule((nonterminal_t)0, BuiltinOp::op_NOP, "\u2b1c", {}, 0.0);
 