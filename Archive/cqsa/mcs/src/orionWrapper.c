/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#include "orionWrapper.h"

double getRouterPower(int degree, double load)
{
  char name[2048] = "router\0";
  FUNC(SIM_router_power_init, &GLOB(router_info), &GLOB(router_power));

  // Set switch parameters based on switch type
  GLOB(router_info).n_in = degree;
  GLOB(router_info).n_total_in -= (4 - degree);
  GLOB(router_info).n_out = degree;
  GLOB(router_info).n_total_out -= (4 - degree);
  GLOB(router_info).degree = degree;

  return SIM_router_stat_energy(&GLOB(router_info),
				&GLOB(router_power),
				0,
				name,
				AVG_ENERGY,
				load,
				0,
				NETWORK_CLOCK);
}
