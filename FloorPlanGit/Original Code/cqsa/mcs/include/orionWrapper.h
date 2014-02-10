/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#ifndef ORIONWRAPPER_H_
#define ORIONWRAPPER_H_

#include "SIM_power.h"
#include "SIM_power_router.h"
#include "SIM_router_power.h"

// network clock frequency (in Hz)
#define NETWORK_CLOCK (1e8)

double getRouterPower(int degree, double load);

#endif
