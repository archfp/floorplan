/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#include <gsl/gsl_rng.h>
#include <iostream>
#include <list>
#include <string>

#include "Component.h"
#include "ComponentLibrary.h"
#include "EMFailureMechanism.h"
#include "FailureMechanism.h"
#include "ManufacturingDefect.h"
#include "parameters.h"
#include "System.h"

using namespace std;

// set name and default parameters at creation
Component::Component(string cname, componentLibraryEntry *compLib, componentType type, int cID, bool sysInitTemp)
{
  // initialize component
  name.append(cname);
  id = cID;
  failed = false;
  initTemperature = T_AMBIENT;
  failureTime = 0;
  n_failureMechanisms = 0;
  initTemp = sysInitTemp;
  
  // set parameters using component type
  componentLibrary = compLib;
  setComponentType(type);
  
  // set mType
  mType = type;
}

Component::~Component()
{
	// TODO: destruct failure mechanisms?
}

void Component::setComponentType(componentType type) {
	vdd = componentLibrary[type].vdd;

	MIPPower = componentLibrary[type].MIPPower;
	readEnergy = componentLibrary[type].readEnergy;
	writeEnergy = componentLibrary[type].writeEnergy;
	staticPower = componentLibrary[type].staticPower;
	
	height = componentLibrary[type].height;
	width = componentLibrary[type].width;
	criticalFraction = componentLibrary[type].criticalFraction;
	gType = componentLibrary[type].gType;
	inputs = componentLibrary[type].inputs;
	cType = type;
	aType = componentLibrary[type].aType;
	dType = componentLibrary[type].dType;
	availableCapacity = partialCapacity = initialCapacity = componentLibrary[type].capacity;

	// update failure mechanism parameters
	list<FailureMechanism*>::iterator iter = failureMechanisms.begin();
	while (iter != failureMechanisms.end()) {
	    FailureMechanism *fm = *iter;
	    fm->updateParameters();
	    
	    iter++;
	} // while
}

void Component::clearRedundancy() {
    setComponentType(mType);
} 

bool Component::isRedundant() const {
    if (cType == mType)
	return false;
    else
	return true;
} // isRedundant

void Component::addFailureMechanism(FailureMechanism *fmech)
{
	n_failureMechanisms++;
	failureMechanisms.push_back(fmech);
} 

int Component::getNFailureMechanisms() const { return n_failureMechanisms; }

void Component::setManufacturingDefect(ManufacturingDefect *dmech) {
    defectMechanism = dmech;
}

void Component::addPreclusion(Component *c) {
	preclusions.push_back(c);
	n_preclusions++;
}

void Component::addInitialTask(Task *t) {
    initialTasks.push_back(t);
}

bool Component::validateInitialMapping() {
    int initialMappingRequirement = 0;

    list<Task*>::iterator iter = initialTasks.begin();
    while (iter != initialTasks.end()) {
	Task *t = *iter;
	
	initialMappingRequirement += t->getReq();
	iter++;
    } // while

    if (initialMappingRequirement > initialCapacity)
	return false;
    else
	return true;
}

void Component::resetAvailableCapacity() {
    availableCapacity = initialCapacity;
    
    list<Task*>::iterator iter = initialTasks.begin();
    while (iter != initialTasks.end()) {
	Task *t = *iter;
	
	availableCapacity -= t->getReq();
	iter++;
    } // while
}

float Component::sampleLifetime()
{
	failed = false;
	failureTime = -1;
	
	// initialize all failure mechanisms, and record lowest failure time
	for (list<FailureMechanism*>::iterator iter = failureMechanisms.begin();
		 iter != failureMechanisms.end();iter++) {
		FailureMechanism *fm = *iter;
		
		// generate a random failure time
		float mechFailTime = fm->sample();
		
		//cout << "### Mechanism " << fm->getName() << " " << " fails at t=" << mechFailTime << endl;
		
		// if failureTime = -1, accept new mechFailTime
		if (failureTime == -1)
			failureTime = mechFailTime;
		// otherwise, if mechFailTime < failureTime, accept
		else if (mechFailTime < failureTime)
			failureTime = mechFailTime;
	} // for

	//cout << "### Component " << getName() << " " << " fails at t=" << failureTime
	//     << " (" << curTemperature << ")" << endl;

	return failureTime;	 
}

bool Component::sampleYield() {
    failed = false;

    // initialize manufacturing defect
    defectMechanism->initialize();

    // generate a random sample
    failed = defectMechanism->sample();
    
    return failed;
} 

void Component::updateWear(float time)
{
	// Step through all of the failure mechanisms for this componenet
	for(list<FailureMechanism*>::iterator iter = failureMechanisms.begin();
		iter != failureMechanisms.end(); iter++) {
		FailureMechanism *curFailureMechanism = *iter;
		
		// Update the wear for this failure mechanism
		curFailureMechanism->updateWear(time);
	}
	
	return;
}

void Component::updateFailureTime()
{
	float minFailureTime = -1;
	float curFailureTime;
	
	// Step through all of the failure mechanisms for this componenet
	for(list<FailureMechanism*>::iterator iter = failureMechanisms.begin();
		iter != failureMechanisms.end(); iter++) {
		FailureMechanism *curFailureMechanism = *iter;	
		
		// Update the failure time for this failure mechanism
		curFailureTime = curFailureMechanism->updateFailureTime();
		
		// Store the new failure time if it is lower than all others
		if((curFailureTime < minFailureTime) || (minFailureTime == -1)) {
			minFailureTime = curFailureTime;
		}
	}
	
	failureTime = minFailureTime;
	return;
}

bool compareFailureTimes(const Component *a, const Component *b) {
	return a->getFailureTime() < b->getFailureTime();
	return true;
}
