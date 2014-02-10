/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#ifndef TCFAILUREMECHANISM_H_
#define TCFAILUREMECHANISM_H_

#include <gsl/gsl_rng.h>

#include "FailureMechanism.h"

// TODO: calculate current density for processors/memories

class TCFailureMechanism : public FailureMechanism
{
	float temperature;
	
public:
	TCFailureMechanism(string fname, Component *owner, gsl_rng *rng);
	virtual ~TCFailureMechanism();

	void updateParameters();
	float calculateMTTF();
	float updateFailureTime();
	void initialize();
};

#endif
