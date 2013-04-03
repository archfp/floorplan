/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#ifndef FAILUREMECHANISM_H_
#define FAILUREMECHANISM_H_

#include <gsl/gsl_rng.h>
#include <string>

#include "types.h"

using namespace std;

class FailureMechanism
{
	string name;			// failure mechanism name
	Component *owner;		// Component to which this mechanism belongs
	gsl_rng *rand_ln;		// log-normal random number generator for this failure mechanism
	
	// wear parameters
	float wear;				// accumulated wear
	float wearout;			// wear required for failure
	float failureTime;		// current failure time (function of wearout and distribution)
	float lastUpdateTime;   // the last simulation time at which the wear was updated
	
	// distribution parameters
	float mttf;		// mean time to failure of current distribution
	float mu;
	float sigma;
public:
	FailureMechanism(string fname, Component *owner, gsl_rng *rng);
	virtual ~FailureMechanism();
		
	// get various parameters
	float getFailureTime() { return failureTime; }
	float getMTTF() { return mttf; }
	string getName() { return name; }
	Component *getOwner() { return owner; }
	float getWear() { return wear; }
	float getWearout() { return wearout; }
	float getSigma() { return sigma; }
	
	// set various parameters
	void setMTTF(float time) { mttf = time; }
	void setTimeToFailure(float time) { failureTime = time; }

	// update component-dependent parameters
	virtual void updateParameters(); 
	
	// calculate MTTF using failure mechanism characteristics and
	// component temperature
	virtual float calculateMTTF();
	
	// set distribution parameter mu based on mttf
	float calculateMu();
	
	// get initial parameters and set mttf/mu
	virtual void initialize();

	// update the wear
	void updateWear(float time);
	
	// update parameters and determine mttf/mu/wear/failureTime
	virtual float updateFailureTime();	
	
	// initialize wear and generate a random sample for this failure mechanism
	float sample();
};

#endif /*FAILUREMECHANISM_H_*/
