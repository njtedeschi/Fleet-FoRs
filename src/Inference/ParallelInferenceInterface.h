#pragma once 

#include <atomic>
#include <thread>
#include <condition_variable>
#include "Control.h"
#include "OrderedLock.h"

/**
 * @class InfereceInterface
 * @author piantado
 * @date 07/06/20
 * @file InferenceInterface.h
 * @brief This manages multiple threads for running inference. This requires a subclass to define run_thread, which 
 * 			is what each individual thread should do. All threads can then be called with run(Control, Args... args), which
 * 			copies the control for each thread (setting threads=1) and then passes the args arguments onward
 */
 template<typename... Args>
class ParallelInferenceInterface {
public:

	// Subclasses must implement run_thread, which is what each individual thread 
	// gets called on (and each thread manages its own locks etc)
	virtual void run_thread(Control ctl, Args... args) = 0;
	
	// index here is used to index into larger parallel collections. Each thread
	// is expected to get its next item to work on through index, though how will vary
	std::atomic<size_t> index; 
	
	// How many threads? Used by some subclasses as asserts
	size_t __nthreads; 
	
	
	// this lock controls the output of the run generator
	// It's kinda important that its FIFO so that we don't hang on one thread for a while
	OrderedLock generator_lock; 
	std::atomic<size_t> who; // who is currently hodling the lock
	std::vector<std::condition_variable> cvars(__nthreads);
		
	
	
	ParallelInferenceInterface() : index(0), __nthreads(0) {
		
	}
	
	/**
	 * @brief Return the next index to operate on (in a thread-safe way).
	 * @return 
	 */	
	unsigned long next_index() {
		return index++;
	}
	
	/**
	 * @brief How many threads are currently run in this interface? 
	 * @return 
	 */
	size_t nthreads() {
		return __nthreads;
	}
	
	/**
	 * @brief Run is the main control interface. Copies of ctl get made and passed to each thread in run_thread. 
	 * @param ctl
	 */	
//	void run(Control ctl, Args... args) {
//		
//		std::vector<std::thread> threads(ctl.nthreads); 
//
//		// save this for children
//		__nthreads = ctl.nthreads;
//
//		for(unsigned long t=0;t<ctl.nthreads;t++) {
//			Control ctl2 = ctl; ctl2.nthreads=1; // we'll make each thread just one
//			threads[t] = std::thread(&ParallelInferenceInterface<Args...>::run_thread, this, ctl2, args...);
//		}
//		
//		// wait for all to complete
//		for(unsigned long t=0;t<ctl.nthreads;t++) {
//			threads[t].join();
//		}
//	}
	
	
	generator<HYP> run(Control ctl, Args... args) {
		
		std::vector<std::thread> threads(ctl.nthreads); 
		__nthreads = ctl.nthreads; // save this for children
		
		// everyone updates this when they are done
		std::atomic<size_t> __nrunning = 0;
		
		// start each thread
		for(unsigned long t=0;t<ctl.nthreads;t++) {
			Control ctl2 = ctl; ctl2.nthreads=1; // we'll make each thread just one
			threads[t] = std::thread(&ParallelInferenceInterface<Args...>::run_thread, this, ctl2, args...);
			__nrunning++;
		}
		
		
		
		
		// now 
		while(true) {
			
			
			
		}
		
		// wait for all to complete
		for(unsigned long t=0;t<ctl.nthreads;t++) {
			threads[t].join();
		}
	}
	
};