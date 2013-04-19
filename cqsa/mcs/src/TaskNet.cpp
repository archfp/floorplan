/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#include <cstdlib>
#include <iostream>

#include "TaskNet.h"

TaskNet::TaskNet()
{
  src = NULL;
  dst = NULL;
  bandwidth = -1;
}

TaskNet::TaskNet(Task *tSrc, Task *tDst, int tBandwidth)
{
  if(tBandwidth <= 0) {
    cerr << "Tried to create a TaskNet with a non-positive bandwidth" << tBandwidth << endl;
    exit(1);
  }

  src = tSrc;
  dst = tDst;
  bandwidth = tBandwidth;
}

TaskNet::~TaskNet()
{
  // Nothing to do here...don't want to delete the task objects associated with the net
}

void TaskNet::setBandwidth(int tBandwidth)
{
  if(tBandwidth > 0) {
    bandwidth = tBandwidth;
  }
  else {
    cerr << "Tried to set bandwidth to a non-positive value " << tBandwidth << endl;
    exit(1);
  }
  
  return;
}
