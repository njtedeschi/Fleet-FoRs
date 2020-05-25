/* Globally shared variables and declarations */


/*! \mainpage Fleet - Fast inference in the Language of Thought
 *
 * \section intro_sec Introduction
 *
 * Fleet is a C++ library for programming language of thought models. In these models, you will typically
 * specify a grammar of primitive operations which can be composed to form complex hypotheses. These hypotheses
 * are best thought of as programs in a mental programming language, and the job of learners is to observe
 * data (typically inputs and outputs of programs) and infer the most likely program to have generated the outputs
 * from the inputs. This is accomplished in Fleet by using a fully-Bayesian setup, with a prior over programs typically
 * defined thought a Probabilistic Context-Free Grammar (PCFG) and a likelihood model that typically says that the 
 * output of a program is observed with some noise. 
 * 
 * Fleet is most similar to LOTlib (https://github.com/piantado/LOTlib3) but is considerably faster. LOTlib converts
 * grammar productions into python expressions which are then evaled in python; this process is flexible and powerful, 
 * but very slow. Fleet avoids this by implementing a lightweight stack-based virtual machine in which programs can be 
 * directly evaluated. This is especially advantageous when evaluating stochastic hypotheses (e.g. those using flip() or 
 * sample()) in which multiple execution paths must be evaluated. Fleet stores these multiple execution traces of a single
 * program in a priority queue (sorted by probability) and allows you to rapidly explore the space of execution traces.
 *
 * To accomplish this, Fleet makes heavy use of C++ template metaprogramming. It requires strongly-typed functions
 * and requires you to specify the macro FLEET_GRAMMAR_TYPES in order to tell its virtual machine what kinds of variables
 * must be stored. In addition, Fleet uses a std::tuple named PRIMITIVES in order to help define the grammar. This tuple consists of
 * a collection of Primitive objects, essentially just lambda functions and weights). The input/output types of these primitives
 * are automatically deduced from the lambdas (using templates) and the corresponding functions are added to the grammar. Note
 * that the details of this mechanism may change in future versions in order to make it easier to add grammar types in 
 * other ways. In addition, Fleet has a number of built-in operations, which do special things to the virtual machine 
 * (including Builtin::Flip, which stores multiple execution traces; Builtin::If which uses short-circuit evaluation; 
 * Builtin::Recurse, which handles recursives hypotheses; and Builtin::X which provides the argument to the expression). 
 * These are not currently well documented but should be soon.  * 
 * 
 * \section install_sec Installation
 * 
 * Fleet is based on header-files, and requires no additional dependencies (command line arguments are processed in CL11.hpp,
 * which is included in src/dependencies/). 
 * 
 * The easiest way to begin using Fleet is to modify one of the examples. For simple rational-rules style inference, try
 * Models/RationalRules; for an example using stochastic operations, try Models/FormalLanguageTheory-Simple. 
 * 
 * Fleet is developed using GCC.
 *
 * \section install_sec Inference
 * 
 * Fleet provides a number of simple inference routines to use. 
 * 
 * \subsection step1 Markov-Chain Monte-Carlo
 * \subsection step2 Search (Monte-Carlo Tree Search)
 * \subsection step3 Enumeration 
 * etc...
 * 
 * \section install_sec Typical approach
 * 	- Sample things, store in TopN, then evaluate...
 */

#pragma once 

#include <sys/resource.h> // just for setting priority defaulty 

const std::string FLEET_VERSION = "0.0.94";

///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// We defaultly define a fleet object which stores all our info, prints our options
// and our runtime on construction and destruction respectively
///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// We defaultly include all of the major requirements for Fleet
///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "Statistics/FleetStatistics.h"

#include "Timing.h"
#include "Program.h"
#include "Control.h"
#include "Numerics.h"
#include "Random.h"
#include "Strings.h"
#include "Hash.h"
#include "Miscellaneous.h"
#include "IntegerizedStack.h"

#include "Rule.h"
#include "VirtualMachine/VirtualMachinePool.h"
#include "VirtualMachine/VirtualMachineState.h"
#include "Node.h"
#include "Grammar.h"
#include "CaseMacros.h"
#include "DiscreteDistribution.h"

#include "IO.h"
#include "Hypotheses/LOTHypothesis.h"
#include "Hypotheses/Lexicon.h"

#include "Inference/MCMCChain.h"
#include "Inference/MCTS.h"
#include "Inference/ParallelTempering.h"
#include "Inference/ChainPool.h"

#include "Top.h"
#include "Primitives.h"

// Thie one really should be included last because it depends on Primitves
#include "VirtualMachine/applyPrimitives.h"

///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// Actual initialization
///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <unistd.h>
#include <stdlib.h>


///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// A fleet class provides an interface to command line and prints a nice header
/// and footer
///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


unsigned long random_seed  = 0;
unsigned long mcts_steps   = 0;
unsigned long mcmc_steps   = 0; 
unsigned long thin         = 0;
unsigned long ntop         = 100;
unsigned long mcmc_restart = 0;
unsigned long checkpoint   = 0; 
double        explore      = 1.0; // we want to exploit the string prefixes we find
size_t        nthreads     = 1;
unsigned long runtime      = 0;
unsigned long nchains      = 1;
bool          quiet      = false; // this is used to indicate that we want to not print much out (typically only posteriors and counts)
std::string   input_path   = "input.txt";
std::string   tree_path    = "tree.txt";
std::string   output_path  = "output";
std::string   timestring   = "0s";


class Fleet {
public:
	CLI::App app;
	timept start_time;
	
	Fleet(std::string brief) : app{brief} {

		app.add_option("-R,--seed",    random_seed, "Seed the rng (0 is no seed)");
		app.add_option("-s,--mcts",    mcts_steps, "Number of MCTS search steps to run");
		app.add_option("-m,--mcmc",     mcmc_steps, "Number of mcmc steps to run");
		app.add_option("-t,--thin",     thin, "Thinning on the number printed");
		app.add_option("-o,--output",   output_path, "Where we write output");
		app.add_option("-O,--top",      ntop, "The number to store");
		app.add_option("-n,--threads",  nthreads, "Number of threads for parallel search");
		app.add_option("-e,--explore",  explore, "Exploration parameter for MCTS");
		app.add_option("-r,--restart",  mcmc_restart, "If we don't improve after this many, restart");
		app.add_option("-i,--input",    input_path, "Read standard input from here");
		app.add_option("-T,--time",     timestring, "Stop (via CTRL-C) after this much time (takes smhd as seconds/minutes/hour/day units)");
		app.add_option("-E,--tree",     tree_path, "Write the tree here");
		app.add_option("-c,--chains",   nchains, "How many chains to run");
		
		app.add_flag(  "-q,--quiet",  quiet, "Don't print very much and do so on one line");
//		app.add_flag(  "-C,--checkpoint",   checkpoint, "Checkpoint every this many steps");

		start_time = now();
	}
	
	virtual ~Fleet() {
		completed();
	}
	
	template<typename T> 
	void add_option(std::string c, T& var, std::string description ) {
		app.add_option(c, var, description);
	}
	template<typename T> 
	void add_flag(std::string c, T& var, std::string description ) {
		app.add_flag(c, var, description);
	}
	
	
	int initialize(int argc, char** argv) {
		CLI11_PARSE(app, argc, argv);
		
		// set our own handlers -- defaulty HUP won't stop
		signal(SIGINT, fleet_interrupt_handler);
		signal(SIGHUP, fleet_interrupt_handler);

		// give us a defaultly kinda nice niceness
		setpriority(PRIO_PROCESS, 0, 5);

		// set up the random seed:
		if(random_seed != 0) {
			rng.seed(random_seed);
		}

		// Print standard fleet header
		
		// apparently some OSes don't define this
		#ifndef HOST_NAME_MAX
			size_t HOST_NAME_MAX = 256;
		#endif
		char hostname[HOST_NAME_MAX]; 	gethostname(hostname, HOST_NAME_MAX);

		//	#ifndef LOGIN_NAME_MAX
		//		size_t LOGIN_NAME_MAX = 256;
		//	#endif
		//	char username[LOGIN_NAME_MAX];	getlogin_r(username, LOGIN_NAME_MAX);

		// and build the command to get the md5 checksum of myself
		char tmp[64]; sprintf(tmp, "md5sum /proc/%d/exe", getpid());
		
		// convert everything to ms
		runtime = convert_time(timestring);	
		
		COUT "# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" ENDL;
		COUT "# Running Fleet on " << hostname << " with PID=" << getpid() << " by user " << getenv("USER") << " at " <<  datestring() ENDL;
		COUT "# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" ENDL;
		COUT "# Fleet version: " << FLEET_VERSION ENDL;
		COUT "# Executable checksum: " << system_exec(tmp);
		COUT "# Run options: " ENDL;
		COUT "# \t --input=" << input_path ENDL;
		COUT "# \t --threads=" << nthreads ENDL;
		COUT "# \t --chains=" << nchains ENDL;
		COUT "# \t --mcmc=" << mcmc_steps ENDL;
		COUT "# \t --mcts=" << mcts_steps ENDL;
		COUT "# \t --time=" << timestring << " (" << runtime << " ms)" ENDL;
		COUT "# \t --restart=" << mcmc_restart ENDL;
		COUT "# \t --seed=" << random_seed ENDL;
		COUT "# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" ENDL;	
		
		return 0;
	}
	
	
	void completed() {
		
		auto elapsed_seconds = time_since(start_time) / 1000.0;
		
		COUT "# Elapsed time:"        TAB elapsed_seconds << " seconds " ENDL;
		COUT "# VM ops per second:" TAB FleetStatistics::vm_ops/elapsed_seconds ENDL;

		if(FleetStatistics::global_sample_count > 0) {
			COUT "# Global sample count:" TAB FleetStatistics::global_sample_count ENDL;
			COUT "# Samples per second:"  TAB FleetStatistics::global_sample_count/elapsed_seconds ENDL;
		}
		
		// HMM CANT DO THIS BC THERE MAY BE MORE THAN ONE TREE....
		//COUT "# MCTS tree size:" TAB m.size() ENDL;	
		//COUT "# Max score: " TAB maxscore ENDL;
		//COUT "# MCTS steps per second:" TAB m.statistics.N ENDL;	
	}
	
};

