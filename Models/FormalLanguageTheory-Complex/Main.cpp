/* 
 * In this version, input specifies the prdata minus the txt 
 * 	--input=data/NewportAslin
 * and then we'll add in the data amounts, so that now each call will loop over amounts of data
 * and preserve the top hypotheses
 * 
 * 
 * Some notes -- 
				What if we add something that lets you access the previous character generated? cons(x,y) where y gets acess to x? cons(x,F(x))?
  * 					Not so easy to see exactly what it is, ithas to be a Fcons function where Fcons(x,i) = cons(x,Fi(x))
  * 					It's a lot like a lambda -- apply(lambda x: cons(x, Y[x]), Z)
  * 
  * */
  
#include <set>
#include <string>
#include <vector>
#include <numeric> // for gcd

#include "Data.h"

using S = std::string;
using StrSet = std::set<S>;

const std::string my_default_input = "data/SimpleEnglish"; 
S alphabet="nvadt";
size_t max_length = 256; // max string length, else throw an error (128+ needed for count)
size_t max_setsize = 64; // throw error if we have more than this
size_t nfactors = 2; // how may factors do we run on?

// Run completed on Dec 12th static const double alpha = 0.95; // reliability of the data
static const double alpha = 0.99; // reliability of the data

const size_t PREC_REC_N   = 25;  // if we make this too high, then the data is finite so we won't see some stuff
const size_t MAX_LINES    = 1000000; // how many lines of data do we load? The more data, the slower...
const size_t MAX_PR_LINES = 1000000; 

const size_t NTEMPS = 10;
const size_t MAXTEMP = 1000.0;

std::vector<S> data_amounts={"1", "2", "5", "10", "50", "100", "500", "1000", "5000", "10000", "50000", "100000"}; // how many data points do we run on?
//std::vector<S> data_amounts={"50000"}; // how many data points do we run on?

// Parameters for running a virtual machine
const double MIN_LP = -25.0; // -10 corresponds to 1/10000 approximately, but we go to -25 to catch some less frequent things that happen by chance


/// NOTE: IF YOU CHANGE, CHANGE BELOW TOO ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
unsigned long MAX_STEPS_PER_FACTOR   = 2048; //4096;  
unsigned long MAX_OUTPUTS_PER_FACTOR = 512; //512; - make it bigger than
/// NOTE: IF YOU CHANGE, CHANGE BELOW TOO ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
const unsigned long PRINT_STRINGS = 128; // print at most this many strings for each hypothesis


///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// These define all of the types that are used in the grammar.
/// This macro must be defined before we import Fleet.
///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define FLEET_GRAMMAR_TYPES S,bool,double,StrSet

#define CUSTOM_OPS op_UniformSample,op_P

///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// This is a global variable that provides a convenient way to wrap our primitives
/// where we can pair up a function with a name, and pass that as a constructor
/// to the grammar. We need a tuple here because Primitive has a bunch of template
/// types to handle thee function it has, so each is actually a different type.
///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "Primitives.h"
#include "Builtins.h"

std::tuple PRIMITIVES = {
	Primitive("tail(%s)",      +[](S& s)     -> void       { if(s.length()>0) s.erase(0); }), //sreturn (s.empty() ? S("") : s.substr(1,S::npos)); }), // REPLACE: if(s.length() >0) s.erase(0)
	Primitive("head(%s)",      +[](S s)      -> S          { return (s.empty() ? S("") : S(1,s.at(0))); }), // MAYBE REPLACE:  S(1,vms.stack<S>().topref().at(0));
	Primitive("pair(%s,%s)",   +[](S& a, S b) -> void      { 
			if(a.length() + b.length() > max_length) 
				throw VMSRuntimeError;
			a.append(b); // modify on stack
	}), // also add a function to check length to throw an error if its getting too long

	Primitive("\u00D8",        +[]()         -> S          { return S(""); }),
	
	Primitive("(%s==%s)",      +[](S x, S y) -> bool       { return x==y; }),
	Primitive("empty(%s)",     +[](S x) -> bool            { return x.length()==0; }),
	

	Primitive("and(%s,%s)",    +[](bool a, bool b) -> bool { return (a and b); }), // optional specification of prior weight (default=1.0)
	Primitive("or(%s,%s)",     +[](bool a, bool b) -> bool { return (a or b); }),
	Primitive("not(%s)",       +[](bool a)         -> bool { return (not a); }),
	
	
	Primitive("insert(%s,%s)", +[](S x, S y) -> S { 
				
				size_t l = x.length();
				if(l == 0) 
					return y;
				else if(l + y.length() > max_length) 
					throw VMSRuntimeError;
				else {
					// put y into the middle of x
					size_t pos = l/2;
					S out = x.substr(0, pos); 
					out.append(y);
					out.append(x.substr(pos));
					return out;
				}				
			}),
	
	
		// add an alphabet symbol
	Primitive("\u03A3", +[]() -> StrSet {
		StrSet out; 
		for(const auto& a: alphabet) 
			out.insert(S(a,1));
		return out;
	}, 5.0),
	
	// set operations:
	Primitive("{%s}",         +[](S x) -> StrSet          { StrSet s; s.insert(x); return s; }, 10.0),
	Primitive("(%s\u222A%s)", +[](StrSet& s, StrSet x) -> void { 
		if(s.size() + x.size() > max_setsize) 
			throw VMSRuntimeError; 
		
		for(auto& a: x) {
			s.insert(a);
		}
	}),
	
	Primitive("(%s\u2216%s)", +[](StrSet& s, StrSet x) -> void {
		for(auto& a: x) {
			s.erase(a);
		}
	}),
	
	// And add built-ins:
	Builtin::If<S>("if(%s,%s,%s)", 1.0),		
	Builtin::If<StrSet>("if(%s,%s,%s)", 1.0),		
	Builtin::If<double>("if(%s,%s,%s)", 1.0),		
	Builtin::X<S>("x"),
	Builtin::FlipP("flip(%s)", 10.0)
};

#include "Fleet.h" 

class InnerHypothesis;
class InnerHypothesis : public  LOTHypothesis<InnerHypothesis,Node,S,S> {
public:
	using Super = LOTHypothesis<InnerHypothesis,Node,S,S>;
	using Super::Super; // inherit constructors
	
	virtual vmstatus_t dispatch_custom(Instruction i, VirtualMachinePool<S,S>* pool, VirtualMachineState<S,S>* vms, Dispatchable<S,S>* loader) {
		assert(i.is<CustomOp>());
		switch(i.as<CustomOp>()) {
			case CustomOp::op_UniformSample: {
					// implement sampling from the set.
					// to do this, we read the set and then push all the alternatives onto the stack
					StrSet s = vms->template getpop<StrSet>();
					
					// now just push on each, along with their probability
					// which is here decided to be uniform.
					const double lp = (s.empty()?-infinity:-log(s.size()));
					for(const auto& x : s) {
						pool->copy_increment_push(vms,x,lp);
					}

					// TODO: we can make this faster, like in flip, by following one of the paths?
					return vmstatus_t::RANDOM_CHOICE; // if we don't continue with this context 
			}
			default: {assert(0 && "Should not get here!");}
		}
		return vmstatus_t::GOOD;
	}
	
	
};


class MyHypothesis : public Lexicon<MyHypothesis, InnerHypothesis, S, S> {
public:	
	
	using Super = Lexicon<MyHypothesis, InnerHypothesis, S, S>;
	
	MyHypothesis()                       : Super()   {}
	MyHypothesis(const MyHypothesis& h)  : Super(h)  {}


	bool check_reachable() const {
		// checks if the last factor calls all the earlier (else we're "wasting" factors)
		// We do this by making the graph of what factors calls which other, and then
		// computing the transitive closure
		
		const size_t N = factors.size();
		assert(N > 0); 
		
		// is calls[i][j] stores whether factor i calls factor j
		bool calls[N][N]; 
		
		// everyone calls themselves, zero the rest
		for(size_t i=0;i<N;i++) {
			for(size_t j=0;j<N;j++){
				calls[i][j] = (i==j);
			}
		}
		
		for(size_t i=0;i<N;i++){
			
			for(const auto& n : factors[i].value) {
				if(n.rule->instr.is_a(BuiltinOp::op_RECURSE,
									  BuiltinOp::op_MEM_RECURSE,
									  BuiltinOp::op_SAFE_RECURSE,
									  BuiltinOp::op_SAFE_MEM_RECURSE)) {
					calls[i][n.rule->instr.arg] = true;
				}
			}
		}
		
		// now we take the transitive closure to see if calls[N-1] calls everything (eventually)
		// otherwise it has probability of zero
		// TOOD: This could probably be lazier because we really only need to check reachability
		for(size_t a=0;a<N;a++) {	
		for(size_t b=0;b<N;b++) {
		for(size_t c=0;c<N;c++) {
			calls[b][c] = calls[b][c] or (calls[b][a] and calls[a][c]);		
		}
		}
		}

		// don't do anything if we have uncalled functions from the root
		for(size_t i=0;i<N;i++) {
			if(not calls[N-1][i]) {
				return false;
			}
		}		
		return true;		
	}

	virtual double compute_prior() {
		// since we aren't searching over nodes, we are going to enforce a prior that requires
		// each function to be called -- this should make the search a bit more efficient by 
		// allowing us to prune out the functions which could be found with a smaller number of factors
		
		if(not check_reachable()) {
			return prior = -infinity;
		}
		else {
			return prior = Lexicon<MyHypothesis,InnerHypothesis,S,S>::compute_prior();
		}
	}
	


	/********************************************************
	 * Calling
	 ********************************************************/
	 
	virtual DiscreteDistribution<S> call(const S x, const S err) {
		// this calls by calling only the last factor, which, according to our prior,
		// must call everything else
		
		if(!has_valid_indices()) return DiscreteDistribution<S>();
		
		size_t i = factors.size()-1; 
		return factors[i].call(x, err, this, MAX_STEPS_PER_FACTOR, MAX_OUTPUTS_PER_FACTOR,MIN_LP); 
	}
	
	 
	 
	 // We assume input,output with reliability as the number of counts that input was seen going to that output
	 virtual double compute_single_likelihood(const t_datum& datum) { assert(0); }
	 

	 double compute_likelihood(const t_data& data, const double breakout=-infinity) {
		// this version goes through and computes the predictive probability of each prefix
		 
		// call -- treats all input as emptystrings
		const auto M = call(S(""), S("<err>")); 
		
		likelihood = 0.0;
		const double lpnoise = log((1.0-alpha)/alphabet.size());
		const double lpend   = log(alpha); // end a string with this probability
		for(const auto& a : data) {
			S astr = a.output;
			double alp = -infinity; // the model's probability of this
			for(const auto& m : M.values()) {
				const S mstr = m.first;
				if(is_prefix(mstr, astr)) {
					alp = logplusexp(alp, m.second + lpnoise * (astr.size() - mstr.size()) + lpend);
				}
			}
			likelihood += alp * a.reliability; 
			
			if(likelihood == -infinity) {
				return likelihood;
			}
			if(likelihood < breakout) {	
				likelihood = -infinity;
				break;				
			}
		}
		return likelihood; 
	
	 }
	 
	 void print(std::string prefix="") {
		std::lock_guard guard(Fleet::output_lock); // better not call Super wtih this here
		extern MyHypothesis::t_data prdata;
		extern std::string current_data;
		auto o = this->call(S(""), S("<err>"));
		auto [prec, rec] = get_precision_and_recall(std::cout, o, prdata, PREC_REC_N);
		COUT "#\n";
		COUT "# "; o.print(PRINT_STRINGS);	COUT "\n";
		COUT prefix << current_data TAB this->born TAB this->posterior TAB this->prior TAB this->likelihood TAB QQ(this->parseable()) TAB prec TAB rec;
		COUT "" TAB QQ(this->string()) ENDL
	}
	 
};


std::string prdata_path = ""; 
MyHypothesis::t_data prdata; // used for computing precision and recall -- in case we want to use more strings?
S current_data = "";


////////////////////////////////////////////////////////////////////////////////////////////
// Main
////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv){ 
	
	input_path = my_default_input; // set this so it's not fleet's normal input default
	
	// default include to process a bunch of global variables: mcts_steps, mcc_steps, etc
	auto app = Fleet::DefaultArguments("Formal language learner");
	app.add_option("-N,--nfactors",      nfactors, "How many factors do we run on?");
	app.add_option("-L,--maxlength",     max_length, "Max allowed string length");
	app.add_option("-A,--alphabet",  alphabet, "The alphabet of characters to use");
	app.add_option("-P,--prdata",  prdata_path, "What data do we use to compute precion/recall?");
	CLI11_PARSE(app, argc, argv);

	Fleet_initialize();
	COUT "# Using alphabet=" << alphabet ENDL;
	
	
	// Input here is going to specify the PRdata path, minus the txt
	
	if(prdata_path == "") {	prdata_path = input_path+".txt"; }
	
	load_data_file(prdata, prdata_path.c_str()); // put all the data in prdata
	for(auto d : prdata) {	// Add a check for any data not in the alphabet
		for(size_t i=0;i<d.output.length();i++){
			if(alphabet.find(d.output.at(i)) == std::string::npos) {
				CERR "*** Character '" << d.output.at(i) << "' in " << d.output << " is not in the alphabet '" << alphabet << "'" ENDL;
				assert(0);
			}
		}
	}
	
	Grammar grammar(PRIMITIVES);
	
	
	for(int a=1;a<=Fleet::Pdenom/2;a++) { // pack probability into arg, out of 20, since it never needs to be greater than 1/2	
		std::string s = str(a/std::gcd(a,Fleet::Pdenom)) + "/" + str(Fleet::Pdenom/std::gcd(a,Fleet::Pdenom)); // std::to_string(double(a)/24.0).substr(1,4); // substr just truncates lesser digits
		grammar.add<double>(BuiltinOp::op_P, s, (a==Fleet::Pdenom/2?5.0:1.0), a);
	}
	
	grammar.add<S,StrSet>(CustomOp::op_UniformSample, "sample(%s)");

	for(size_t a=0;a<nfactors;a++) {	
		auto s = std::to_string(a);
		grammar.add<S,S>(BuiltinOp::op_SAFE_RECURSE, S("F")+s+"(%s)", 1.0/nfactors, a);
		grammar.add<S,S>(BuiltinOp::op_SAFE_MEM_RECURSE, S("memF")+s+"(%s)", 0.5/nfactors, a); // disprefer mem when we don't need it
	}

	// push for each
	for(size_t ai=0;ai<alphabet.length();ai++) {
		grammar.add<S>(BuiltinOp::op_ALPHABET,   Q(alphabet.substr(ai,1)),  20.0/alphabet.length(), (int)alphabet.at(ai) );
	}
	


	
	
	
	
	
	
	
	
	
	// TODO: ADD the alphabet as a set?
	
	
	
	
	
	
	MyHypothesis h0; 
	for(size_t fi=0;fi<nfactors;fi++) {// start with the right number of factors
		InnerHypothesis f(&grammar);
		h0.factors.push_back(f.restart());
	}
		
	// we are building up data and TopNs to give t parallel tempering
	std::vector<MyHypothesis::t_data> datas; // load all the data	
	std::vector<TopN<MyHypothesis>> tops;
	for(size_t i=0;i<data_amounts.size();i++){ 
		MyHypothesis::t_data d;
		
		S data_path = input_path + "-" + data_amounts[i] + ".txt";	
		load_data_file(d, data_path.c_str());
		
		datas.push_back(d);
		tops.push_back(TopN<MyHypothesis>(ntop));
	}


//	TopN<MyHypothesis> top(100);
//
//	MyHypothesis h;
//	for(size_t fi=0;fi<nfactors;fi++) {// start with the right number of factors
//		InnerHypothesis f(&grammar);
//		h.factors.push_back(f.restart());
//	}
//
//	tic();
////	ChainPool t(h,&datas[10], top, nthreads);
//	ParallelTempering t(h0, &datas[10], top, nthreads, 1.0);
//	t.run(10000,0, 100000, 100000);
//	tic();
	
	// TODO: Let's check out heap allocators again -- seems like that msut be where most of our time is going...
//	
//	auto f = [&]() {
//		MyHypothesis h;
//		for(size_t fi=0;fi<nfactors;fi++) {// start with the right number of factors
//			InnerHypothesis f(&grammar);
//			h.factors.push_back(f.restart());
//		}
//	
////		MCMCChain t(h, &datas[10], top);
////		t.run(100000,0);
//
////		for(size_t i=0;i<1000000;i++) {
////			h.compute_posterior(datas[10]);
////			top << h;
////			h = h.restart();
////			FleetStatistics::global_sample_count++;
////		}
//	};
//
//
//	tic();
//	std::thread threads[nthreads]; 
//	for(unsigned long t=0;t<nthreads;t++) {
//		threads[t] = std::thread(f);
//	}
//	
//	// wait for all to complete
//	for(unsigned long t=0;t<nthreads;t++) {
//		threads[t].join();
//	}
//	tic();


//	for(auto& tn : tops) { 
//		tn.set_print_best(true);
//	}
//	tops[tops.size()-1].set_print_best(true);
	
	// Run Parallel Tempering on data -- doesn't work great because it rarely swaps
//	ParallelTempering samp(h0, datas, tops);
//	tic();	
//	samp.run(mcmc_steps, runtime, 200, 6000);	
//	tic();

	TopN<MyHypothesis> all(ntop); 
//	all.set_print_best(true);
	
	tic();	
	for(size_t di=0;di<datas.size() and !CTRL_C;di++) {
		
		ParallelTempering samp(h0, &datas[di], all, NTEMPS, MAXTEMP);
		samp.run(Control(mcmc_steps, runtime/datas.size()), 1000, 60*1000);	

		// set up to print using a larger set
		MAX_STEPS_PER_FACTOR   = 32000; //4096; 
		MAX_OUTPUTS_PER_FACTOR = 8000; //512; - make it bigger than
		all.print(data_amounts[di]);
		MAX_STEPS_PER_FACTOR   = 2048; 
		MAX_OUTPUTS_PER_FACTOR = 512; 


		h0 = all.best();
		
		if(di+1 < datas.size()) {
			all = all.compute_posterior(datas[di+1]); // update for next time
		}
	}
	tic();
	
	
	// And finally print
//	for(auto& tn : tops) { 
//		all << tn; // will occur in some weird order since they're not in all the data
//	} 


//	// Vanilla MCMC, serial
//	for(size_t i=0;i<datas.size();i++){
//		tic();
//		MCMCChain chain(h0, &datas[i], all);
//		chain.run(mcmc_steps, runtime);
//		tic();	
//	}
//	
//	for(size_t i=0;i<data_amounts.size();i++) {
//		all.compute_posterior(datas[i]).print(data_amounts[i]);
//	}
//	
	COUT "# Global sample count:" TAB FleetStatistics::global_sample_count ENDL;
	COUT "# Elapsed time:"        TAB elapsed_seconds() << " seconds " ENDL;
	COUT "# Samples per second:"  TAB FleetStatistics::global_sample_count/elapsed_seconds() ENDL;
}
