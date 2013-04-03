/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#include <cmath>
#include <gsl/gsl_cdf.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_rng.h>
#include <iostream>
#include <string>

#include "Component.h"
#include "FailureMechanism.h"

using namespace std;

//extern gsl_rng *rand_ln;

// default constructor sets default parameters
FailureMechanism::FailureMechanism(string fname, Component *fowner, gsl_rng *rng)
{
	// set owner
	owner = fowner;
	
	// set random number generator
	rand_ln = rng;
	
	// initialize parameter values
	failureTime = 0;
	wear = 0;
	wearout = 0;
	lastUpdateTime = 0;
	
	// set name
	name.append(fname);
	
	// 30 y MTTF default
	mttf = 30;
	mu = 3.2762;
	sigma = 0.5;

	//cout << "^^^ Created failure mechanism " << name << endl;
}

// nothing to free
FailureMechanism::~FailureMechanism()
{
}

/*
void FailureMechanism::setInitialTemperature(float temperature)
{
	// update initial temperature
	initTemperature = temperature;
	curTemperature = temperature;
	// update mttf
	float mttf = calculateMTTF(); 
	// update mu
	float mu = calculateMu();
	
	cout << "### " << getName() << " MTTF/Mu updated: " << mttf << "/" << mu << endl;
}
*/

// update component-dependent parameters
void FailureMechanism::updateParameters() {
  cerr << "Called the virtual updateParameters() function" << endl;
  exit(1);
} 

// generic MTTF calculation
float FailureMechanism::calculateMTTF()
{
  printf("Should never be calling FailureMechanism::calculateMTTF()\n");
  exit(1);
  mttf = 30;
  return mttf;
}

// based on provided MTTF and fixed sigma, set mu
float FailureMechanism::calculateMu()
{
	// MTTF = exp(mu + sigma^2/2)
	float c = (float) exp(sigma*sigma/2);
	mu = (float) log(mttf/c);
	
	return mu;
}

// initialize failure mechanism by generating a random failure time 
void FailureMechanism::initialize() 
{
  cerr << "Called the virtual initialize() function" << endl;
  exit(1);
}

void FailureMechanism::updateWear(float time)
{
	float lastTimeInCurDist;
	float timeSpentInCurDist;

	// make sure this should be Pinv during checkin
	lastTimeInCurDist = (float)gsl_cdf_lognormal_Pinv(wear,mu,sigma);
	timeSpentInCurDist = time - lastUpdateTime;
	wear = (float)gsl_cdf_lognormal_P(lastTimeInCurDist + timeSpentInCurDist,mu,sigma);
	lastUpdateTime = time;
	
	return;
}

float FailureMechanism::updateFailureTime() 
{
  cerr << "Called the virtual updateFailureTime() function" << endl;
  exit(1);
}

float FailureMechanism::sample() 
{
	// initialize mechanism parameters
	initialize();

	// generate random failure time
	failureTime = (float) gsl_ran_lognormal(rand_ln, mu, sigma);
	
	// initialize wear, wearout, and the time at which the wear was last updated
	wear = 0;
	wearout = (float) gsl_cdf_lognormal_P(failureTime, mu, sigma);
	lastUpdateTime = 0;
	
	return failureTime;		
}
