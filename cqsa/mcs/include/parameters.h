/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#ifndef PARAMETERS_H_
#define PARAMETERS_H_

// temperature parameters
#define T_AMBIENT 300.0
#define T_CHAR 345.0

// failure mechanism parameters

/*** GENERAL ***/
#define PARAM_K 8.62e-5

/*** EM ***/
#define PARAM_N 1.1
#define PARAM_EAEM 0.9

/*** TDDB ***/
#define PARAM_A 78.0
#define PARAM_B -0.081
#define PARAM_X 0.759
#define PARAM_Y -66.8
#define PARAM_Z -8.37e-4
#define PARAM_ATDDB 719.392

/*** TC ***/
#define PARAM_Q 2.35
#define PARAM_ATC 230234.64

#endif /*PARAMETERS_H_*/
