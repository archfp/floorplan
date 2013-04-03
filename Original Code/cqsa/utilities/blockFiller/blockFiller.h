/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#ifndef BLOCKFILLER_H_
#define BLOCKFILLER_H_

#include <cmath>
#include "System.h"

// Debugging/verbosity linkage
//extern int mcsVerbosity;
#define DEBUG if(mcsVerbosity)

// Used to check for rounding errors between numbers
#define epsilon 0.0001

// Structure used for a node in a linked list
typedef struct node node;
struct node
{
  float value;
  node *next;
};

int fill_blocks(System *sys);

#endif
