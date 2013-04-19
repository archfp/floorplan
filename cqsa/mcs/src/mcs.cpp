/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#include <sys/time.h>
#include <gsl/gsl_rng.h>
#include <iostream>

#include "config.h"
#include "power.h"
#include "mcs.h"
#include "System.h"
#include "blockFiller.h"

using namespace std;

//#define SAMPLE_CONFIDENCE
#define INTERVAL 1000

int main(int argc, char* argv[]) {
    System sys;

#ifdef SAMPLE_CONFIDENCE
    // Initialize timer
    timeval start, sample;
    gettimeofday(&start, NULL);
#endif
    
	// Initialize the component library
	initializeComponentLibrary(sys.getComponentLibrary());
	
	// parse input arguments
	parse_cmd_sys(argc, argv, sys);

	/*
	if(sys.getBuildFailures()) {
	  // Build all possible scenarios for the purpose of automatically determining failure scenarios
	  sys.buildAllScenariosOrdered();	  
	  
	  // Automatically build the list of failure scenarios
	  sys.buildFailureScenarios();
	}
	*/

	// generate samples
	for (int i=0;i<sys.getMaxSamples();i++) {
	    sys.setInitialComponentTemps();
	    sys.sample();

#ifdef SAMPLE_CONFIDENCE
		if (i > 0 && i % INTERVAL == 0) {
		    // report statistics
		    gettimeofday(&sample, NULL);
		    float diff = (sample.tv_sec + 1e-6*sample.tv_usec) -
			(start.tv_sec + 1e-6*start.tv_usec);
		    
		    float mttf = sys.getMTTF();
		    float var = sys.getVar();
		    float s_var = sys.getSVar();
		    float conf = sys.getConfidence();

		    printf("%3f ", diff);
		    printf("%5d ", i);
		    cout << mttf << " " << s_var << " " << var << " " << conf << endl;
		} // if
#endif

	} // for

	// if we're sampling yield, apply Y0
	if (sys.getMeasureYield())
	    sys.setMTTF(sys.getMTTF()*Y0);
	
	// report statistics
#ifdef SAMPLE_CONFIDENCE
	gettimeofday(&sample, NULL);
	float diff = (sample.tv_sec + 1e-6*sample.tv_usec) -
	    (start.tv_sec + 1e-6*start.tv_usec);
#endif
	
	float mttf = sys.getMTTF();
	float var = sys.getVar();
	float s_var = sys.getSVar();
	float conf = sys.getConfidence();

#ifdef SAMPLE_CONFIDENCE
	printf("%3.2f ", diff);
#endif
	cout << sys.getMaxSamples() << " " << mttf << " " << s_var << " " << var << " " << conf << endl;

#ifdef STATS
	sys.printSTATS();
#endif
	
	//removeWorkingDirectory();
	return 0;
} // main
