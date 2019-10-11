#pragma once

#include "Node.h"


Node::Node(const Rule* r=nullptr, double _lp=0.0, bool cr=true) : 
	parent(nullptr), pi(0), child(r==nullptr ? 0 : r->N), rule(r==nullptr ? NullRule : r), lp(_lp), can_resample(cr) {	
	// NOTE: We don't allow parent to be set here bcause that maeks pi hard to set. We should only be placed
	// in trees with set_child
}

/* We must define our own copy and move since parent can't just be simply copied */

Node::Node(const Node& n) :
	parent(nullptr), rule(n.rule), lp(n.lp), can_resample(n.can_resample) {
	child.resize(n.child.size());
	for(size_t i=0;i<n.child.size();i++) {
		set_child(i, Node(n.child[i]));
	}
}
Node::Node(Node&& n) :
	parent(nullptr), rule(n.rule), lp(n.lp), can_resample(n.can_resample) {
	child.resize(n.child.size());
	for(size_t i=0;i<n.child.size();i++) {
		set_child(i, n.child[i]);
	}
}
void Node::operator=(const Node& n) {
	parent = nullptr; 
	rule = n.rule;
	lp = n.lp;
	can_resample = n.can_resample;
	child.resize(n.child.size());
	for(size_t i=0;i<n.child.size();i++) {
		set_child(i, Node(n.child[i]));
	}
}
void Node::operator=(Node&& n) {
	parent = nullptr; 
	rule = n.rule;
	lp = n.lp;
	can_resample = n.can_resample;
	child.resize(n.child.size());
	for(size_t i=0;i<n.child.size();i++) {
		set_child(i, n.child[i]);
	}
}

Node::NodeIterator Node::begin() const { return NodeIterator(this); }
Node::NodeIterator Node::end()   const { return EndNodeIterator; }

Node* Node::left_descend() const {
	Node* k = (Node*) this;
	while(k != nullptr && k->child.size() > 0) 
		k = &(k->child[0]);
	return k;
}



void Node::set_child(size_t i, Node& n) {
	child[i] = n;
	child[i].pi = i;
	child[i].parent = this;
}
void Node::set_child(size_t i, Node&& n) {
	child[i] = n;
	child[i].pi = i;
	child[i].parent = this;
}

bool Node::is_null() const { // am I the null rule?
	return rule == NullRule;
}

template<typename T>
T Node::sum(std::function<T(const Node&)>& f ) const {
	T s = f(*this);
	for(auto& c: child) {
		s += c.sum<T>(f);
	}
	return s;
}

//	double logsumexp(std::function<double(const Node&)>& f ) const {
//		double s = f(*this);
//		for(auto& c: child) {
//			s = logplusexp(s, c.logsumexp(f));
//		}
//		return s;
//	}
//	
void Node::map( const std::function<void(Node&)>& f) {
	f(*this);
	for(auto& c: child) {
		c.map(f); 
	}
}

void Node::mapconst( const std::function<void(const Node&)>& f) const { // mapping that is constant
	f(*this);
	for(auto& c: child) {
		c.mapconst(f); 
	}
}

//	void map_conditionalrecurse( std::function<bool(Node&)>& f ) {
//		// f here returns a function and we only recurse if the function returns true
//		// This is often useful if we are modifying the tree and don't want to keep going once we've found something
//		bool b = f(*this);
//		if(b){
//			for(auto& c: child) {
//				c.map_conditionalrecurse(f);
//			}
//		}
//	}
	
//	size_t count_equal(const Node& n) const {
//		// how many of my descendants are equal to n?
//		if(*this == n) return 1; // nothing below can be equal
//		size_t cnt = 1;
//		for(auto& c: child) {
//			cnt += c.count_equal(n);
//		}
//		return cnt;
//	}
	
size_t Node::count() const {
	std::function<size_t(const Node& n)> one = [](const Node& n){return (size_t)1;};
	return sum<size_t>( one );
}

bool Node::is_root() const {
	return parent == nullptr;
}

bool Node::is_evaluable() const {
	
	// does this have any subnodes below that are null?
	for(auto& c: child) {
		if(c.is_null()) return false; 
		if(not c.is_evaluable()) return false;
	}
	return child.size() == rule->N; // must have all my kids
}
	
Node* Node::get_nth(int n, std::function<int(const Node&)>& f) {
	// return a pointer to the nth  child satisfying f (f's output cast to bool)

	for(auto& x : *this) {
		if(f(x)) {
			if(n == 0) return &x;
			else       --n;
		}
	}	

	return nullptr; // not here, losers
}
Node* Node::get_nth(int& n) { // default true on every node
	std::function<int(const Node&)> f = [](const Node& n) { return 1;};
	return get_nth(n, f); 
}


template<typename T>
std::pair<Node*, double> Node::sample(std::function<T(const Node&)>& f) {
	// sample a subnode satisfying f and return its probability
	// where f maps each node to a probability (possibly zero)
	// we allow T here to be a double, int, whatever 
	
	T z = sum<T>(f);
	double r = z * uniform();

	for(auto& x : *this) {
		double fx = f(x);
		r -= fx;
		if(r <= 0.0) {
			return std::make_pair(&x, log(fx) - log(z));
		}
	}

	assert(false && "*** Should not get here in sampling!");
}


std::string Node::string() const { 
	// To convert to a string, we need to essentially simulate the evaluator
	// and be careful to process the arguments in the right orders

	if(rule->N == 0) {
		return rule->format;
	}
	else {
		
		std::string childStrings[child.size()];
		
		// The order should be maintained (because that's how CaseMacros works)
		for(size_t i=0;i<rule->N;i++) {
			childStrings[i] = child[i].string();
		}
		
		// now substitute the children into the format
		std::string s = rule->format;
		for(size_t i=0;i<rule->N;i++) {
			auto pos = s.find(ChildStr);
			assert(pos != std::string::npos && "Node format must contain one ChildStr (typically='%s') for each argument"); // must contain the ChildStr for all children all children
			s.replace(pos, ChildStr.length(), childStrings[i] );
		}
		return s;
	}
}


std::string Node::parseable(std::string delim=":") const {
	// get a string like one we could parse
	std::string out = rule->format;
	for(auto& c: child) {
		out += delim + c.parseable(delim);
	}
	return out;
}


/********************************************************
 * Operaitons for running programs
 ********************************************************/

size_t Node::program_size() const {
	// compute the size of the program -- just number of nodes plus special stuff for IF
	size_t n = 1; // I am one node
	if( rule->instr.is_a(BuiltinOp::op_IF) ) { n += 1; } // I have to add this many more instructions for an if
	for(auto& c: child) {
		n += c.program_size();
	}
	return n;
}

void Node::linearize(Program &ops) const { 
	// convert tree to a linear sequence of operations 
	// to do this, we first linearize the kids, leaving their values as the top on the stack
	// then we compute our value, remove our kids' values to clean up the stack, and push on our return
	// the only fanciness is for if: here we will use the following layout 
	// <TOP OF STACK> <bool> op_IF(xsize) X-branch JUMP(ysize) Y-branch
	
	// NOTE: If you change the order here ever, you have to change how string() works so that 
	//       string order matches evaluation order
	// TODO: We should restructure this to use "map" so that the order is always the same as for printing
	
	
	// and just a little checking here
	for(size_t i=0;i<rule->N;i++) {
		assert(not child[i].is_null() && "Cannot linearize a Node with null children");
		assert(child[i].rule->nt == rule->child_types[i] && "Somehow the child has incorrect types"); // make sure my kids types are what they should be
	}
	
	
	// Main code
	if( rule->instr.is_a(BuiltinOp::op_IF) ) {
		assert(rule->N == 3 && "BuiltinOp::op_IF require three arguments"); // must have 3 parts
		
		int xsize = child[1].program_size()+1; // must be +1 in order to skip over the JMP too
		int ysize = child[2].program_size();
		assert(xsize < (1<<12) && "If statement jump size too large to be encoded in Instruction arg"); // these sizes come from the arg bitfield 
		assert(ysize < (1<<12) && "If statement jump size too large to be encoded in Instruction arg");
		
		child[2].linearize(ops);
		
		// make the right instruction 
		// TODO: Assert that ysize fits 
		ops.push(Instruction(BuiltinOp::op_JMP,ysize));
		
		child[1].linearize(ops);
		
		// encode jump
		ops.push(Instruction(BuiltinOp::op_IF, xsize)); 
		
		// evaluate the bool first so its on the stack when we get to if
		child[0].linearize(ops);
		
	}
	else {
		/* Here we push the children in order, first first. So that means that when each evalutes, it will push its value to the *bottom* 
		 * of the stack. so in caseMacros, we need to reverse the order of the arguments, so that the first popped is the last argument */
		Instruction i = rule->instr; // use this as a template
		ops.push(i);
		for(size_t i=0;i<rule->N;i++) {
			child[i].linearize(ops);
		}
	}
	
}

bool Node::operator==(const Node& n) const{
	// Check equality between notes. Note that this compares the rule *pointers* so we need to be careful with 
	// serialization and storing/recovering full node trees with equality comparison
	
	if(not (*rule == *n.rule))
		return false;
		
	for(size_t i=0;i<rule->N;i++){
		if(not (child[i] == n.child[i])) return false;
	}
	return true;
}

//	size_t count_equal_child(const Node& n) const {
//		// how many of my IMMEDIATE children are equal to n?
//		size_t cnt = 0;
//		for(size_t i=0;i<rule->N;i++) {
//			cnt += (child[i] == n);
//		}
//		return cnt;
//	}


size_t Node::hash(size_t depth=0) const {
	// hash and include
	size_t output = rule->get_hash(); // tunrs out, this is actually important to prevent hash collisions when rule_id and i are small
	for(size_t i=0;i<child.size();i++) {
		hash_combine(output, depth, child[i].hash(depth+1), i); // modifies output
	}
	return output;
}


	
