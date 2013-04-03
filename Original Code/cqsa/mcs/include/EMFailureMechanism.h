/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#ifndef EMFAILUREMECHANISM_H_
#define EMFAILUREMECHANISM_H_

#include <gsl/gsl_rng.h>

#include "FailureMechanism.h"

// TODO: calculate current density for processors/memories

class EMFailureMechanism : public FailureMechanism
{
	float currentDensity;
	float temperature;
	float Aem;
	
public:
	EMFailureMechanism(string fname, Component *owner, gsl_rng *rng, float t_Aem);
	virtual ~EMFailureMechanism();

	void updateParameters();
	float calculateMTTF();
	float updateFailureTime();
	void initialize();
};

#endif /*EMFAILUREMECHANISM_H_*/
