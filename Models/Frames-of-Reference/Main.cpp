#include <cmath>

using Vector = std::array<double,3>;

double dot_product(const Vector &a, const Vector &b){
    double result = 0.0;
    for(int i = 0; i < a.size(); i++){
        result += a[i] * b[i];
    }
    return result;
}

double magnitude(const Vector &a){
    double result = 0.0;
    for (int i = 0; i < a.size(); i++){
        result += a[i] * a[i];
    }
    return sqrt(result);
}

double cosine_similarity(const Vector &a, const Vector &b){
    return dot_product(a, b)/(magnitude(a) * magnitude(b));
}

double angle_between(){
    // TODO: implement
}

struct Object {
	Vector location;
	Vector orientation;
	std::string name; 
};

struct Scene {
	Object speaker;
	Object ground;
	Object figure; 
};

///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// Define the grammar
/// Thid requires the types of the thing we will add to the grammar (bool,MyObject)
///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "Grammar.h"
#include "Singleton.h"

class MyGrammar : public Grammar<Scene,bool,   Scene,bool,Object,Vector, double>,
				  public Singleton<MyGrammar> {
public:
	MyGrammar() {
                add("displacement(%s,%s)", +[](Object x, Object y) -> Vector {
                        return y.location - x.location
                        });
		add("orientation(%s)", +[](Object x) -> Vector {return x.orientation;});
                add("parallel(%s,%s)", +[](Vector x, Vector y) -> bool {
                        return cosine_similarity(x,y) == 1;
                        });
                add("antiparallel(%s,%s)", +[](Vector x, Vector y) -> bool {
                        return cosine_similarity(x,y) == -1;
                        });
                add("orthogonal(%s,%s)", +[](Vector x, Vector y) -> bool {
                        return cosine_similarity(x,y) == 0;
                        });
		
                add("and(%s,%s)",    Builtins::And<MyGrammar>);
		add("or(%s,%s)",     Builtins::Or<MyGrammar>);
		add("not(%s)",       Builtins::Not<MyGrammar>);

		add("speaker(%s)",       +[](Scene x) -> Object { return x.speaker; });
		add("figure(%s)",        +[](Scene x) -> Object { return x.figure; });
		add("ground(%s)",        +[](Scene x) -> Object { return x.ground; });
		add("x",             Builtins::X<MyGrammar>);
	}
} grammar;

///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// Define a class for handling my specific hypotheses and data. Everything is defaultly 
/// a PCFG prior and regeneration proposals, but I have to define a likelihood
///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "DeterministicLOTHypothesis.h"

class MyHypothesis final : public DeterministicLOTHypothesis<MyHypothesis,Scene,bool,MyGrammar,&grammar> {
public:
	using Super = DeterministicLOTHypothesis<MyHypothesis,Scene,bool,MyGrammar,&grammar>;
	using Super::Super; // inherit the constructors
	

	double compute_single_likelihood(const datum_t& di) override {
		Scene& s = di.input;
		bool   true_out = di.output; 
		bool my_output = this->call(s);
		
		// TODO: whatever the likelihood should be
		
	}
};

///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// Main code
///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "TopN.h"
#include "ParallelTempering.h"
#include "Fleet.h" 
#include "Builtins.h"

int main(int argc, char** argv){ 
	
	// default include to process a bunch of global variables: mcts_steps, mcc_steps, etc
	Fleet fleet("Rational rules");
	fleet.initialize(argc, argv);
	
	//------------------
	// set up the data
	//------------------
	// mydata stores the data for the inference model
	MyHypothesis::data_t mydata;

	Vector origin = {0,0,0};
        Vector east = {1,0,0};
        Vector west = {-1,0.0};
        Vector north = {0,1,0};
        Vector south = {0,-1,0};
        Vector up = {0,0,1};
        Vector down = {0,0,-1};

	for(int i=0;i<10;i++) {
		
		if(flip(0.8)) {
			mydata.emplace_back(....)
		}
		else {
			
			
		}
		
	}
	
	//------------------
	// Run
	//------------------
	
	TopN<MyHypothesis> top;

	auto h0 = MyHypothesis::sample();
	
	MCMCChain samp(h0, &mydata);
	//ChainPool samp(h0, &mydata, FleetArgs::nchains);
	//	ParallelTempering samp(h0, &mydata, FleetArgs::nchains, 10.0); 
	for(auto& h : samp.run(Control()) | printer(FleetArgs::print) | top) {
		UNUSED(h);
	}

	// Show the best we've found
	top.print();
}
