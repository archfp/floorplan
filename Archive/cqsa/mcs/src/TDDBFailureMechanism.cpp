/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#include <cmath>
#include <gsl/gsl_cdf.h>
#include <gsl/gsl_rng.h>
#include <iostream>

#include "types.h"

#include "Component.h"
#include "FailureMechanism.h"
#include "TDDBFailureMechanism.h"
#include "parameters.h"

using namespace std;

TDDBFailureMechanism::TDDBFailureMechanism(string fname, Component *owner, gsl_rng *rng, float t_vdd) : FailureMechanism(fname, owner, rng)
{
	vdd = t_vdd;
	updateAtddb();
}

TDDBFailureMechanism::~TDDBFailureMechanism()
{
}

void TDDBFailureMechanism::updateAtddb() {
    Atddb = 30.0 / (pow(1.0 / vdd,PARAM_A - (PARAM_B * T_CHAR)) *
		    exp((PARAM_X + (PARAM_Y / T_CHAR) + (PARAM_Z * T_CHAR)) / (PARAM_K * T_CHAR)));
} // updateAtddb

void TDDBFailureMechanism::updateParameters() {
    vdd = getOwner()->getVdd();
    updateAtddb();
} // updateParameters

float TDDBFailureMechanism::calculateMTTF() {
  float time;

  if(getOwner()->getInitTemp()) {
    time = Atddb *
      pow(1.0 / vdd,PARAM_A - (PARAM_B * temperature)) *
      exp((PARAM_X + (PARAM_Y / temperature) + (PARAM_Z * temperature)) / (PARAM_K * temperature));

    //cout << "TDDB: " << getOwner()->getName() << " " << time << endl;
  }
  else {
    time = 30.0;
  }

  setMTTF(time);
  return time;
}

float TDDBFailureMechanism::updateFailureTime() {
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

void TDDBFailureMechanism::initialize()
{
	// get temperature based on initial component values
	temperature = getOwner()->getCurrentTemperature();
	
	// initialize MTTF
	calculateMTTF();
	// initialize Mu
	calculateMu();
	
	return;
}
