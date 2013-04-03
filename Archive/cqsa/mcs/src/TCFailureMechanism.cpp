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
#include "TCFailureMechanism.h"
#include "parameters.h"

using namespace std;

TCFailureMechanism::TCFailureMechanism(string fname, Component *owner, gsl_rng *rng) : FailureMechanism(fname, owner, rng)
{
}

TCFailureMechanism::~TCFailureMechanism()
{
}

void TCFailureMechanism::updateParameters() {
    return;
}

float TCFailureMechanism::calculateMTTF() {
  float time;

  if(getOwner()->getInitTemp()) {
    time = PARAM_ATC *
      pow(1.0 / (temperature - T_AMBIENT),PARAM_Q);

    //cout << "TC: " << getOwner()->getName() << " " << time << endl;
  }
  else {
    time = 30.0;
  }

  setMTTF(time);
  return time;
}

float TCFailureMechanism::updateFailureTime() {
	float newFailTime, curTime, timeToFailure, mu, sigma;
	
	temperature = getOwner()->getCurrentTemperature();
	calculateMTTF();
	mu = calculateMu();
	sigma = this->getSigma();

	newFailTime = (float)gsl_cdf_lognormal_Pinv(this->getWearout(),mu,sigma);
	curTime = (float)gsl_cdf_lognormal_Pinv(this->getWear(),mu,sigma);
	
	timeToFailure = newFailTime - curTime;
	this->setTimeToFailure(timeToFailure);
	
	return timeToFailure;
}

void TCFailureMechanism::initialize()
{
	// get temperature based on initial component values
	temperature = getOwner()->getCurrentTemperature();
	
	// initialize MTTF
	calculateMTTF();
	// initialize Mu
	calculateMu();
	
	return;
}
