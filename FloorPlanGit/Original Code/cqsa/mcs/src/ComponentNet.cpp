/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#include <cstdlib>
#include <iostream>

#include "ComponentNet.h"

ComponentNet::ComponentNet()
{
  src = NULL;
  dst = NULL;
  fwdBandwidth = -1;
  revBandwidth = -1;
  dir = NO_DIR;
}

ComponentNet::ComponentNet(Component *tSrc, Component *tDst, int tFwdBandwidth, int tRevBandwidth, directionality tDir)
{
  src = tSrc;
  dst = tDst;
  fwdBandwidth = tFwdBandwidth;
  revBandwidth = tRevBandwidth;
  dir = tDir;
}

ComponentNet::~ComponentNet()
{
  // Nothing to do here...don't want to delete the component objects associated with the net
}

void ComponentNet::setFwdBandwidth(int tFwdBandwidth)
{
  if((dir == UNI) || (dir == BI)) {
    fwdBandwidth = tFwdBandwidth;
  }
  else {
    cerr << "Tried to set forward bandwidth on a net of incorrect type " << dir << endl;
    exit(1);
  }
  
  return;
}

void ComponentNet::setRevBandwidth(int tRevBandwidth)
{
  if((dir == REV) || (dir == BI)) {
    revBandwidth = tRevBandwidth;
  }
  else {
    cerr << "Tried to set reverse bandwidth on a net of incorrect type " << dir << endl;
    exit(1);
  }
  
  return;
}
