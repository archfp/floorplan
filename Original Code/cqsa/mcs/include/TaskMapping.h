/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#ifndef TASK_MAPPING_H_
#define TASK_MAPPING_H_

#include <set>

#include "Component.h"
#include "ComponentNet.h"
#include "Task.h"
#include "System.h"

using namespace std;

typedef struct componentDistance {
	Component *component;
	int hops;
} componentDistance;

class TaskMapping
{
private:
	System *sys;
	int mcsVerbosity;
	
	// Parallel vectors of components and the tasks mapped to them
	vector<Component*> components;
	vector<set<Task *> > tasks;
	vector<ComponentNet*> netlist;
	vector<ComponentNet*> initialNetlist;
	vector<set<Task *> > baseTaskMapping;

	// vector parallels main component vector, saves component power
	vector<float> power;
	
	// Indicates whether or not a valid task mapping was found during object creation
	bool mappingFound;

	// Indicates whether or not HotSpot has been used to calculate temperatures for this task mapping
	bool tempsCalculated;

	// Compares available capacity in the system to the required capacity for all tasks to be remapped to
	// determine whether or not any remapping is possible
	bool checkAvailableCapacity(vector<Component *> componentsToBeMapped, vector<set<Task *> > tasksToBeMapped);
	
	// Creates a list of candidate components for each component to be remapped
	vector<vector<componentDistance> > findCandidateComponents(vector<Component*> componentsToBeMapped);

	// Sorts each of the candidate orderings by number of hops
	vector<vector<componentDistance> > sortCandidateComponents(vector<vector<componentDistance> > candidateComponents);

	// Removes entries from the netlist which touch components that are currently unmapped
	void cullNetlist(vector<Component*> componentsToBeMapped);
	
	// Initial remapping algorithm
	bool remap1(vector<Component*> componentsToBeMapped, vector<set<Task *> > tasksToBeMapped, vector<vector<componentDistance> > candidateComponents, vector<int> p);
	
	// Finds a path between components src and dst based on the nets in netlist
	// Updates netlist with the specified bandwidth on the links along the path
	bool findPath(Component *src, Component *dst, int bandwidth, bool useBandwidth, int *numHops);
	
	void placeBandwidth(Component *bandwidthSrc, Component *bandwidthDst, int bandwidth);
	
	// Save a copy of netlist to initialNetlist
	void saveInitialNetlist();
	
	// Place the contents of initialNetlist into netlist
	void revertToInitialNetlist();
	
	vector<int> knuthShuffle(int N);
	void printNetlist();
	
public:
	// Constructs a blank task mapping
	TaskMapping(System *sys);
	
	// Constructs a task mapping from an operating scenario (sys.operatingScenarios[pos])
	TaskMapping(System *sys, set<Component*,compareComponentIDs> curScenario);
	
	// Task mapping destructor
	virtual ~TaskMapping();
	
	// Adds a single component-tasks mapping to the task mapping
	void addSingleMapping(Component *tComponent, set<Task *> tTasks);
	
	// Populate the netlist with bandwidths for all tasks except those which are unmapped
	bool populateMappedBandwidth(vector<set<Task *> > tasksToBeMapped);
	
	// Get the components in this task mapping
	vector<Component*> getComponents() { return components; }
	
	// Get the tasks in this task mapping
	vector<set<Task *> > getTasks() { return tasks; }
	
	// Get the netlist (including bandwidth values) for this task mapping
	vector<ComponentNet *> getNetlist() { return netlist; }
	
	// Get the value of the mappingFound variable for this task mapping
	bool getMappingFound() { return mappingFound; }

	// Get the value of the tempsCalculated variable for this task mapping
	bool getTempsCalculated() { return tempsCalculated; }

	// Set the value of the tempsCalculated variable for this task mapping
	void setTempsCalculated(bool tCalc) { tempsCalculated = tCalc; }

	void printTasks();

	// push a power value on the power vector
	void pushPower(float cPower) { power.push_back(cPower); }
	float getPower(int idx) { return power[idx]; }
};

bool compareComponentDistances(const componentDistance a, const componentDistance b);

#endif
