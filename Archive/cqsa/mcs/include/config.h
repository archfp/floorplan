/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#ifndef CONFIG_H_
#define CONFIG_H_

#include "System.h"

#define TOKENS " =,:\t\n"

typedef enum {SECTION_NONE,
	      SECTION_CTYPE,
	      SECTION_OBJS,
	      SECTION_PREC} SECTIONTYPE;

//extern int mcsVerbosity;
//extern bool tempUpdate;
//extern bool buildFailures;
//const extern char hotSpotConfigFileName[];

void parse_cmd_sys(int argc, char *argv[], System &sys);
void parse_config(const char *filename, System &sys);
//void run_HotSpot_simulations(System *sys);
void run_single_HotSpot_simulation(System *sys, int pos, float maxDimension, float r_convec);

#endif
