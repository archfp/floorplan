/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#ifndef POWER_H_
#define POWER_H_

#include "System.h"
extern "C" double getRouterPower(int degree, double load);

//extern int mcsVerbosity;

void create_power_traces(System *sys);
void create_single_power_trace(System *sys, int pos);

#endif
