/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#include <cmath>
#include <gsl/gsl_cdf.h>
#include <iostream>

#include "types.h"

#include "Component.h"
#include "FailureMechanism.h"
#include "EMFailureMechanism.h"
#include "parameters.h"

using namespace std;

EMFailureMechanism::EMFailureMechanism(string fname, Component *owner, gsl_rng *rng, float t_Aem) : FailureMechanism(fname, owner, rng)
{
	Aem = t_Aem;
}

EMFailureMechanism::~EMFailureMechanism()
{
}

void EMFailureMechanism::updateParameters() {
    componentType cType = getOwner()->getCType();
    componentLibraryEntry *compLib = getOwner()->getComponentLibrary();
    Aem = compLib[cType].Aem;
} // updateParameters

float EMFailureMechanism::calculateMTTF() {
  float time;
  
  if(getOwner()->getInitTemp()) {
    time = Aem *
      pow(currentDensity,(float)-PARAM_N) *
      exp(PARAM_EAEM / (PARAM_K * temperature));

    //cout << "EM: " << getOwner()->getName() << " " << time << endl;
  }
  else {
    time = 30.0;
  }


  setMTTF(time);
  return time;
}

float EMFailureMechanism::updateFailureTime() {
	float newFailTime, curTime, timeToFailure, mu, sigma;
	
	//printf("%s: %f --> ",getOwner()->getName().c_str(),temperature);
	temperature = getOwner()->getCurrentTemperature();
	//printf("%f\n",temperature);

	calculateMTTF();
	mu = calculateMu();
	sigma = this->getSigma();

	newFailTime = (float)gsl_cdf_lognormal_Pinv(this->getWearout(),mu,sigma);
	curTime = (float)gsl_cdf_lognormal_Pinv(this->getWear(),mu,sigma);
	
	timeToFailure = newFailTime - curTime;
	this->setTimeToFailure(timeToFailure);
	
	return timeToFailure;
}

void EMFailureMechanism::initialize()
{
	// get temperature and current density based on initial component values
	currentDensity = getOwner()->getCurrentCurrentDensity();
	//temperature = getOwner()->getInitTemperature();
	temperature = getOwner()->getCurrentTemperature();
	
	// initialize MTTF
	calculateMTTF();
	// initialize Mu
	calculateMu();
	
	return;
}
