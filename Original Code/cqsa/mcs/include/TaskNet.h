/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#ifndef TASK_NET_H_
#define TASK_NET_H_

#include "Task.h"

using namespace std;

class TaskNet
{
private:
	Task *src;
	Task *dst;
	int bandwidth;
	
public:
	
	// Constructors
	TaskNet();
	TaskNet(Task *tSrc, Task *tDst, int tBandwidth);
	
	// Destructor
	virtual ~TaskNet();
	
	// Source node accessors
	void setSrc(Task *tSrc) { src = tSrc; }
	Task * getSrc() { return src; }
	
	// Destination node accessors
	void setDst(Task *tDst) { dst = tDst; }
	Task * getDst() { return dst; }
	
	// Bandwidth accessors
	void setBandwidth(int tBandwidth);
	int getBandwidth() { return bandwidth; }
};

#endif
