/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#ifndef COMPONENT_NET_H_
#define COMPONENT_NET_H_

#include "Component.h"

using namespace std;

typedef enum
{
	UNI = 0,
	BI,
	REV,
	NO_DIR
} directionality;

class ComponentNet
{
private:
	Component *src;
	Component *dst;
	int fwdBandwidth;
	int revBandwidth;
	directionality dir;
	
public:
	
	// Constructors
	ComponentNet();
	ComponentNet(Component *tSrc, Component *tDst, int tFwdBandwidth, int tRevBandwidth, directionality tDir);
	
	// Destructor
	virtual ~ComponentNet();
	
	// Source node accessors
	void setSrc(Component *tSrc) { src = tSrc; }
	Component * getSrc() { return src; }
	
	// Destination node accessors
	void setDst(Component *tDst) { dst = tDst; }
	Component * getDst() { return dst; }
	
	// Bandwidth accessors
	void setFwdBandwidth(int tFwdBandwidth);
	int getFwdBandwidth() { return fwdBandwidth; }
	void setRevBandwidth(int tRevBandwidth);
	int getRevBandwidth() { return revBandwidth; }
	
	// Directionality accessors
	void setDirectionality(directionality tDir) { dir = tDir; }
	directionality getDirectionality() { return dir; }
};

#endif
