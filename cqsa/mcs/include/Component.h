/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#ifndef COMPONENT_H_
#define COMPONENT_H_

#include <gsl/gsl_rng.h>
#include <list>
#include <string>

#include "types.h"
#include "ComponentLibrary.h"
//#include "System.h"
#include "Task.h"

// memory output width in bytes
#define MEM_OUTPUT_WIDTH 16

using namespace std;

class Component
{
	// wear variables
	bool failed;		// true if component has failed
	float failureTime;	// time of failure under current distribution

	// switch- and memory-specific power fields
	int fwdBandwidth;       // data moving away from component
	int revBandwidth;       // data moving toward the component
	
	// memory-specific power fields
	float readEnergy;       // energy required per read in memory
	float writeEnergy;      // energy required per write in memory
	float staticPower;      // leakage for memories
	
	// processor-specific power fields
	float MIPPower;         // power required per MIP
	
	// component parameters and variables
	float initTemperature;	// initial temperature in K
	float curTemperature;	// current temperature in K
	bool initTemp;	

	float curPower;         // current power consumption

	float vdd;		// operating voltage for component
	float height, width;	// component dimensions
	float criticalFraction; // fraction of area that is critical (w.r.t. manufacturing defects)

	componentLibraryEntry *componentLibrary;
	
	generalComponentType gType; // general component type {processor, memory, switch}
	int inputs;             // number of inputs (only relevant for switches)
	componentType cType;	// the specific type of this component
	componentType aType;	// type component becomes when allocated redundnacy
	componentType dType;	// type component becomes when deallocated redundancy
	componentType mType;    // the "minimum" component type this component is allowed to be
	
	int availableCapacity;
	int partialCapacity;
	int initialCapacity;
	
	// component name
	string name;		// name of this component

	// component ID
	int id;
	
	// failure mechanisms
	int n_failureMechanisms;
	list<FailureMechanism*> failureMechanisms;	// list of failure mechanisms for this component

	// manufacturing defects
	ManufacturingDefect* defectMechanism;           // defect mechanism for this component
	
	// preclusions
	int n_preclusions;
	list<Component*> preclusions;				// list of components that fail if this component fails

	// initial tasks, defined at configuration time
	list<Task*> initialTasks;
public:
	Component(string cname, componentLibraryEntry *compLib, componentType cType, int cID, bool sysInitTemp);
	virtual ~Component();

	void setComponentType(componentType cType);
	void clearRedundancy();
	
	// status
	bool isFailed() const { return failed; }
	void setFailed() { failed = true; }

	// get various parameters
	float getArea() const { return height*width; }
	float getFailureTime() const { return failureTime; }
	float getHeight() const { return height; }
	float getCriticalFraction() const { return criticalFraction; }
	string getName() const { return name; }
	int getID() const { return id; }
	float getVdd() const { return vdd; }
	float getWidth() const { return width; }
	generalComponentType getGType() const { return gType; }
	int getInputs() const { return inputs; }
	int getInitialCapacity() const { return initialCapacity; }
	int getAvailableCapacity() const { return availableCapacity; }
	int getPartialCapacity() const { return partialCapacity; }
	componentLibraryEntry *getComponentLibrary() { return componentLibrary; }
	componentType getCType() const { return cType; }
	componentType getAType() const { return aType; }
	componentType getDType() const { return dType; }
	componentType getMType() const { return mType; }
	bool isRedundant() const;
	
	// get initial values of parameters
	//float getInitCurrentDensity() const { return (initPower / vdd) / ((height * 0.1) * (width * 0.1)); }
	float getCurrentCurrentDensity() const { return (curPower / vdd) / ((height * 0.1) * (width * 0.1)); }
	float getCurPower() const { return curPower; }
	float getInitTemperature() const { return initTemperature; }
	float getCurrentTemperature() const { return curTemperature; }
	bool getInitTemp() const { return initTemp; }

	int getFwdBandwidth() const { return fwdBandwidth; }
	int getRevBandwidth() const { return revBandwidth; }
	float getReadEnergy() const { return readEnergy; }
	float getWriteEnergy() const { return writeEnergy; }
	float getStaticPower() const { return staticPower; }
	float getMIPPower() const { return MIPPower; }
	
	// set various parameters
	void setInitialTemperature(float temp) { initTemperature = temp; }
	void setCurrentTemperature(float temp) { curTemperature = temp; }
	void setCurPower(float power) { curPower = power; }
	void setAvailableCapacity(int tCapacity) { availableCapacity = tCapacity; }

	void setFwdBandwidth(int bandwidth) { fwdBandwidth = bandwidth; }
	void setRevBandwidth(int bandwidth) { revBandwidth = bandwidth; }
	
	// resets available capacity based on initially mapped tasks
	void resetAvailableCapacity(); 
	void resetAvailableCapacityToInitial() { availableCapacity = initialCapacity; }
	void resetAvailableCapacityToPartial() { availableCapacity = partialCapacity; }
	void setPartialCapacity(int pCapacity) { partialCapacity = pCapacity; }
	void resetPartialCapacityToInitial() { partialCapacity = initialCapacity; }
	
	// add a new failure mechanism to this component
	void addFailureMechanism(FailureMechanism *fmech);
	int getNFailureMechanisms() const;
	list<FailureMechanism*> getFailureMechanisms() { return failureMechanisms; }

	// manufacturing defect functions
	ManufacturingDefect *getManufacturingDefect() const { return defectMechanism; }
	void setManufacturingDefect(ManufacturingDefect *dmech);
	
	// manipulate preclusions
	void addPreclusion(Component *c);
	list<Component*> getPreclusions() const { return preclusions; }
	int getNPreclusions() const { return n_preclusions; }

	// manipulate initial tasks
	void addInitialTask(Task *t);
	list<Task*> getInitialTasks() const { return initialTasks; }
	bool validateInitialMapping();
	
	// reset wear and generate a random failure time based on
	// component's failure mechanisms
	float sampleLifetime();

	// reset manufacturing defects and generate a random yield sample
	bool sampleYield();
	
	// Update the wear from each of the failure mechanisms for the component
	void updateWear(float time);
	
	// Update the failure time of the component
	void updateFailureTime();
};

// used with std::sort to sort a vector of pointers to Components
bool compareFailureTimes(const Component *a, const Component *b);

typedef struct compareComponentIDs
{
  bool operator()(const Component *a, const Component *b) const
  {
    return a->getID() < b->getID();
  }
} compareComponentIDs;

#endif /*COMPONENT_H_*/
