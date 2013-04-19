/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#ifndef TDDBFAILUREMECHANISM_H_
#define TDDBFAILUREMECHANISM_H_

#include "FailureMechanism.h"

// TODO: calculate current density for processors/memories

class TDDBFailureMechanism : public FailureMechanism
{
	float temperature;
	float vdd;
	float Atddb;
	
public:
	TDDBFailureMechanism(string fname, Component *owner, gsl_rng *rng, float t_vdd);
	virtual ~TDDBFailureMechanism();

	void updateAtddb();
	void updateParameters();
	float calculateMTTF();
	float updateFailureTime();
	void initialize();
};

#endif
