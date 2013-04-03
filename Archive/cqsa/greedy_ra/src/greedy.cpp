/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#include <cmath>
#include <gsl/gsl_rng.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "config.h"
#include "Component.h"
#include "ComponentLibrary.h"
#include "greedy.h"
#include "mcs.h"

#define MF 2

//#define DISTANCE
//#define GREEDY
//#define CQMEM
//#define CQPROC
//#define CQ2ND

#define MWD 1
#define MPEG 2
#define MPEG3P 3
#define MPEGCPL2ND 4
#define MWD_OLD 5

//#define APP MWD
#define APP MWD_OLD
//#define APP MPEG
//#define APP MPEG3P
//#define APP MPEGCPL2ND
//#define ARCH 3
#define ARCH 4
//#define ARCH 5
//#define ARCH 10

using namespace std;

bool checkMemoryPermutation(System *sys, vector<componentType> permutation)
{
  int x;
  bool permutationApplicable = false;
  vector<Component*> mems = sys->getMemories();

  if(mems.size() != permutation.size()) {
    cerr << "Number of memories in the system does not match number of memories in the permutation" << endl;
    sys->cleanUpAndExit(1);
  }

  // Step through all of the memories in the system
  for(x = 0; x < (int)mems.size(); x++) {

    // This if statement assumes that memory types are enumerated in order of increasing size
    // Thus, if all component types in the system are greater than or equal to the types in the
    // permutation, the current state of the system has used permutation as a base assignment
    if(mems[x]->getCType() < permutation[x]) {
      return false;
    }

    // At least one system component type must be greater than permutations for the
    // current permutation to be applicable
    if(mems[x]->getCType() > permutation[x]) {
      permutationApplicable = true;
    }
  }

  return permutationApplicable;
}

inline float distance(float x1, float y1, float x2, float y2) {
    return (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2);
} // distance

int exhaustiveExecSlackAllocation(System *sys, vector<vector<componentType> > permutations) {
#ifdef DISTANCE
    float bestDist = 1e9;
#else
    float bestMTTF = 0;
#endif
    
    int nExpl = 0;
    
    vector<vector<componentType> >::iterator bestPermutation = permutations.begin();
    vector<vector<componentType> >::iterator curPermutation = permutations.begin();
    while (curPermutation != permutations.end()) {
	//nExpl++;
	
	// apply the new permutation
	sys->applyProcessorPermutation(*curPermutation);

	// if this design hasn't already been explored, increment counter
	string key = sys->buildKey();
	if (sys->lookupExpl(key) == false) {
	    nExpl++;
	    sys->setExpl(key);
	}

	// evaluate
	sys->samplingRun();
	sys->printComponentCapacities();
	cout << sys->getArea() << " " << sys->getWL() << " " << sys->getMTTF() << endl;

#ifdef DISTANCE
	float dist = distance(0, 0, sys->getArea(), 1.0/sys->getMTTF());

	if (dist < bestDist) {
	    bestDist = dist;
	    bestPermutation = curPermutation;
	} // if
#else
	float mttf = sys->getMTTF();
	
	if (mttf > bestMTTF) {
	    bestMTTF = mttf;
	    bestPermutation = curPermutation;
	} // if
#endif
	
	// advance to next permutation
	curPermutation++;
    } // while
    
    // re-apply the best permutation
    sys->applyProcessorPermutation(*bestPermutation);
    
    // regenerate statistics-- sampling run is guaranteed not to perform floorplanning
    // or MCS because the design has already been evaluated
    sys->samplingRun();

    return nExpl;
} // exhaustiveExecSlackAllocation

int exhaustiveStorSlackAllocation(System *sys, vector<vector<componentType> > permutations) {
#ifdef DISTANCE
    float bestDist = 1e9;
#else
    float bestMTTF = 0;
#endif
    
    int nExpl = 0;
    
    vector<vector<componentType> >::iterator bestPermutation = permutations.begin();
    vector<vector<componentType> >::iterator curPermutation = permutations.begin();
    while (curPermutation != permutations.end()) {
	//nExpl++;
	
	// apply the new permutation
	sys->applyMemoryPermutation(*curPermutation);

	// if this design hasn't already been explored, increment counter
	string key = sys->buildKey();
	if (sys->lookupExpl(key) == false) {
	    nExpl++;
	    sys->setExpl(key);
	}

	// evaluate
	sys->samplingRun();
	sys->printComponentCapacities();
	cout << sys->getArea() << " " << sys->getWL() << " " << sys->getMTTF() << endl;

#ifdef DISTANCE
	float dist = distance(0, 0, sys->getArea(), 1.0/sys->getMTTF());

	if (dist < bestDist) {
	    bestDist = dist;
	    bestPermutation = curPermutation;
	} // if
#else
	float mttf = sys->getMTTF();
    
	if (mttf > bestMTTF) {
	    bestMTTF = mttf;
	    bestPermutation = curPermutation;
	} // if
#endif
	
	// advance to next permutation
	curPermutation++;
    } // while
    
    // re-apply the best permutation
    sys->applyMemoryPermutation(*bestPermutation);
    
    // regenerate statistics-- sampling run is guaranteed not to perform floorplanning
    // or MCS because the design has already been evaluated
    sys->samplingRun();

    return nExpl;
} // exhaustiveStorSlackAllocation

int exhaustiveExecStorSlackAllocation(System *sys,
				      vector<vector<componentType> > procPermutations,
				      vector<vector<componentType> > memPermutations) {
#ifdef DISTANCE
    float bestDist = 1e9;
#else
    float bestMTTF = 0;
#endif
    
    int nExpl = 0;
    
    vector<vector<componentType> >::iterator bestMemPermutation = memPermutations.begin();
    vector<vector<componentType> >::iterator curMemPermutation = memPermutations.begin();
    vector<componentType> bestProcPermutation;

    while (curMemPermutation != memPermutations.end()) {
	//nExpl++;
	
	// apply the new permutation
	sys->applyMemoryPermutation(*curMemPermutation);

	// find the best processor permutation given this memory permutation
	nExpl += exhaustiveExecSlackAllocation(sys, procPermutations);

	//cout << "###   Evaluated " << nExpl << " designs with fixed memory permutation" << endl;
	
	// evaluate
#ifdef DISTANCE
	float dist = distance(0, 0, sys->getArea(), 1.0/sys->getMTTF());

	if (dist < bestDist) {
	    bestDist = dist;
	    bestMemPermutation = curMemPermutation;
	    bestProcPermutation = sys->buildProcessorPermutation();
	} // if
#else
	float mttf = sys->getMTTF();
	
	if (mttf > bestMTTF) {
	    bestMTTF = mttf;
	    bestMemPermutation = curMemPermutation;
	    bestProcPermutation = sys->buildProcessorPermutation();
	} // if
#endif
	
	// advance to next permutation
	curMemPermutation++;
    } // while

    //cout << "###   " << nExpl << " designs explored exhaustively in Stage 2 so far" << endl;
    
    // re-apply the best permutations
    sys->applyProcessorPermutation(bestProcPermutation);
    sys->applyMemoryPermutation(*bestMemPermutation);
    
    // regenerate statistics-- sampling run is guaranteed not to perform floorplanning
    // or MCS because the design has already been evaluated
    sys->samplingRun();

    return nExpl;
}
    
int findAllocationLevelCeiling(int slack, vector<int> allocationLevels) {
    int idx = 0;
    vector<int>::iterator iter = allocationLevels.begin();
    while (*iter < slack) {	
	idx++;
	iter++;
    } // while
    
    return idx;
} // findAllocationLevelCeiling

inline int fastfactorial(int n) {
  if (n > 1)
    return n*factorial(n-1);
  else
    return 1;
} // factorial

int choose(int n, int k) {
  return fastfactorial(n) / (fastfactorial(k) * (fastfactorial(n - k)));
} // choose

void pruneProcessorAllocationLevels(System &sys, vector<int> &processorAllocationLevels) {
    // save initial permutation to restore later
    vector<componentType> initialPermutation = sys.buildProcessorPermutation();

    // determine the baseline memory configuration
    sys.clearRedundancies(sys.getProcessors());
    vector<componentType> basePermutation = sys.buildProcessorPermutation();

    vector<int> indices, baseCapacity;
    indices.resize(basePermutation.size());
    baseCapacity.resize(basePermutation.size());

    // build a vector of processor sizes / in parallel, set up the indices
    for (int i=0; i<(int) basePermutation.size(); i++) {
	baseCapacity[i] = componentTypeToCapacity(basePermutation[i]);
	indices[i] = i;
    } // for

    // build a list of slack allocation levels that result from adding
    // combinations of system processors
    // - there sum_{i=1}^n NChooseR(n, i) allocation levels (not all necessarily unique)

    cout << "~~~ Building the list of valid processor levels" << endl;
    
    map<int,bool> cqAllocations;
    cqAllocations[0] = true;
    
    int nfact = factorial(basePermutation.size());
    for (int i=0; i<nfact; i++) {
	vector<int>::iterator begin = indices.begin();
	vector<int>::iterator end = indices.end();
	
	// for the first j entries of the permutation, see if that
	// combination is in the combinations vector
	int sum = 0;
	for (int j=0; j<(int) basePermutation.size(); j++) {
	    sum += baseCapacity[ indices[j] ];

	    if (!cqAllocations[sum]) {
		cqAllocations[sum] = true;
		//cout << "~~~   " << sum << endl;
	    }
	} 
	
	next_permutation(begin, end);
    } // for

    //cout << "~~~ Pruning processorAllocationLevels of invalid levels" << endl;
    
    // now remove any entry in the processorAllocationList that is not in cqAllocations
    vector<int>::iterator iter = processorAllocationLevels.begin();
    while (iter != processorAllocationLevels.end()) {
	int level = *iter;
	
	if (!cqAllocations[level]) {
	    iter = processorAllocationLevels.erase(iter);
	    iter--;

	    //cout << "~~~   Removed " << level << endl;
	} // if

	iter++;
    } // while
    
    // restore original processor permutation
    sys.applyProcessorPermutation(initialPermutation);
} // pruneProcessorAllocationLevels

void pruneMemoryAllocationLevels(System &sys, vector<int> &memoryAllocationLevels) {
    // save initial permutation to restore later
    vector<componentType> initialPermutation = sys.buildMemoryPermutation();

    // determine the baseline memory configuration
    sys.clearRedundancies(sys.getMemories());
    vector<componentType> basePermutation = sys.buildMemoryPermutation();

    vector<int> indices, baseCapacity;
    indices.resize(basePermutation.size());
    baseCapacity.resize(basePermutation.size());

    // build a vector of memory sizes / in parallel, set up the indices
    for (int i=0; i<(int) basePermutation.size(); i++) {
	baseCapacity[i] = componentTypeToCapacity(basePermutation[i]);
	indices[i] = i;
    } // for

    // build a list of slack allocation levels that result from adding
    // combinations of system memories
    // - there sum_{i=1}^n NChooseR(n, i) allocation levels (not all necessarily unique)

    //cout << "~~~ Building the list of valid memory levels" << endl;
    
    map<int,bool> cqAllocations;
    cqAllocations[0] = true;
    
    int nfact = factorial(basePermutation.size());
    for (int i=0; i<nfact; i++) {
	vector<int>::iterator begin = indices.begin();
	vector<int>::iterator end = indices.end();
	
	// for the first j entries of the permutation, see if that
	// combination is in the combinations vector
	int sum = 0;
	for (int j=0; j<(int) basePermutation.size(); j++) {
	    sum += baseCapacity[ indices[j] ];

	    if (!cqAllocations[sum]) {
		cqAllocations[sum] = true;
		//cout << "~~~   " << sum << endl;
	    }
	} 
	
	next_permutation(begin, end);
    } // for

    //cout << "~~~ Pruning memoryAllocationLevels of invalid levels" << endl;
    
    // now remove any entry in the memoryAllocationList that is not in cqAllocations
    vector<int>::iterator iter = memoryAllocationLevels.begin();
    while (iter != memoryAllocationLevels.end()) {
	int level = *iter;
	
	if (!cqAllocations[level]) {
	    iter = memoryAllocationLevels.erase(iter);
	    iter--;

	    //cout << "~~~   Removed " << level << endl;
	} // if

	iter++;
    } // while
    
    // restore original memory permutation
    sys.applyMemoryPermutation(initialPermutation);
} // pruneMemoryAllocationLevels

void pruneMemoryAllocationLevelsRobust(System &sys, vector<int> &memoryAllocationLevels) {
    // save initial permutation to restore later
    vector<componentType> initialPermutation = sys.buildMemoryPermutation();

    // determine the baseline memory configuration
    sys.clearRedundancies(sys.getMemories());
    vector<componentType> basePermutation = sys.buildMemoryPermutation();

    vector<int> baseCapacity;
    baseCapacity.resize(basePermutation.size());

    // build a vector of memory sizes
    for (int i=0; i<(int) basePermutation.size(); i++) {
	baseCapacity[i] = componentTypeToCapacity(basePermutation[i]);
    } // for

    // build a list of valid slack allocation levels that result from
    // adding combinations of system memories
    map<int,bool> cqAllocations;
    // the baseline-- no slack-- is automatically valid
    cqAllocations[0] = true;

    // search for combinations of up to MF*nMemories of memories
    vector<int> indices;
    for (int i=1; i<=(int) basePermutation.size()*MF; i++) {
	// initialize the index vector
	indices.clear();
	indices.resize(i, 0);

	cout << "~~~ " << i << endl;
	
	// explore all assignments of indices to memories
	for (int j=0; (float) j<powf(basePermutation.size(),i); j++) {
	    // calculate the sum of the capacities referred to by the current indices
	    int sum = 0;

	    cout << "~~~  " << j << ": ";
	    for (int k=0; k<(int) indices.size(); k++) {
		// print current indices
		cout << indices[k] << " ";
		sum += baseCapacity[ indices[k] ];
	    } // for
	    cout << "/ " << sum << " ";

	    // check if we've already observed this combination
	    if (!cqAllocations[sum]) {
		cqAllocations[sum] = true;
		cout << "+++";
	    } // if
	    cout << endl;

	    // advance to the next combination
	    bool next = false;
	    int idx = indices.size()-1;

	    while (!next) {		
		// advance the current index
		if (indices[idx] < (int) baseCapacity.size()-1) {
		    indices[idx]++;
		    next = true;
		} else {
		    // can't advance, so reset it, and prepare to advance the previous index
		    indices[idx] = 0;
		    idx--;

		    // check if we're at the last combination
		    if (idx < 0)
			break;
		} // if-else
	    } // while
	} // for
    } // for

    cout << "~~~ Pruning memoryAllocationLevels of invalid levels" << endl;
    
    // now remove any entry in the memoryAllocationList that is not in cqAllocations
    vector<int>::iterator iter = memoryAllocationLevels.begin();
    while (iter != memoryAllocationLevels.end()) {
	int level = *iter;
	
	if (!cqAllocations[level]) {
	    iter = memoryAllocationLevels.erase(iter);
	    iter--;

	    cout << "~~~   Removed " << level << endl;
	} // if

	iter++;
    } // while
    
    // restore original memory permutation
    sys.applyMemoryPermutation(initialPermutation);
} // pruneMemoryAllocationLevelsRobust

int main(int argc, char* argv[]) {
    System sys;
    
    // Initialize the component library
    initializeComponentLibrary(sys.getComponentLibrary());
    
    // parse input arguments
    parse_cmd_sys(argc, argv, sys);
    
    // initialize the system
    sys.initialize();

#if APP == MWD
    sys.setMaxMemoryType(MEM2MB);
#elif APP == MWD_OLD
    sys.setMaxMemoryType(MEM2MB);
#else
    sys.setMaxMemoryType(MEM512KB);
#endif
    
    // build processor and memory permutations
    sys.buildProcessorPermutations();
    sys.buildMemoryPermutations();
    
    vector<int> processorAllocationLevels = sys.getProcessorAllocationLevels();
    vector<int> memoryAllocationLevels = sys.getMemoryAllocationLevels();

    cout << "### Memory allocation levels: ";
    for (int i=0; i < (int) memoryAllocationLevels.size(); i++)
	cout << memoryAllocationLevels[i] << " ";
    cout << endl;
    
#ifndef GREEDY
    //pruneProcessorAllocationLevels(sys, processorAllocationLevels);
    //pruneMemoryAllocationLevelsRobust(sys, memoryAllocationLevels);
#endif
    
    cout << "### Processor allocation levels: ";
    for (int i=0; i < (int) processorAllocationLevels.size(); i++)
	cout << processorAllocationLevels[i] << " ";
    cout << endl;

    cout << "### Memory allocation levels: ";
    for (int i=0; i < (int) memoryAllocationLevels.size(); i++)
	cout << memoryAllocationLevels[i] << " ";
    cout << endl;    

    // Build the list of numbers of critical processors
    vector<int> criticalProcessorsList;
    criticalProcessorsList.clear();
    
    // Build the list of numbers of critical processors/memory
    vector<pair<int,int> > criticalPMList;
    criticalPMList.clear();

    pair<int,int> p;
    
    // if we're just doing greedy search, the application and architecture
    // doesn't matter
#ifdef GREEDY
    criticalProcessorsList.resize(0);
    criticalPMList.resize(1);
    criticalPMList[0].first = 0;
    criticalPMList[0].second = 0;


#elif APP == MWD && ARCH == 3

#ifdef CQMEM
    // first-order mem (1)
    p.first = 0;
    p.second = 1024;
    criticalPMList.push_back(p);

#ifdef CQ2ND
    // second-order mem-mem (1)
    p.first = 0;
    p.second = 1024*2;
    criticalPMList.push_back(p);
#endif
#endif

    // first-order switch
    
#ifdef CQPROC
    // first-order proc (2)
    criticalProcessorsList.push_back(250);
    criticalProcessorsList.push_back(500);

#ifdef CQ2ND
    // second-order proc-proc (4)
    criticalProcessorsList.push_back(375);
    criticalProcessorsList.push_back(625);
    criticalProcessorsList.push_back(750);
    criticalProcessorsList.push_back(1000);

#ifdef CQMEM (3)
    // second-order proc-mem
    p.first = 125;
    p.second = 1024;
    criticalPMList.push_back(p);

    p.first = 250;
    criticalPMList.push_back(p);

    p.first = 500;
    criticalPMList.push_back(p);    
#endif
#endif
#endif
    

#elif APP == MWD && ARCH == 4

#ifdef CQMEM
    // first-order mem
    p.first = 0;
    p.second = 1024;
    criticalPMList.push_back(p);

#ifdef CQ2ND
    // second-order mem-mem
    p.first = 0;
    p.second = 1024*2;
    criticalPMList.push_back(p);
#endif
#endif

    // first-order switch
    criticalProcessorsList.push_back(750);

    p.first = 250;
    p.second = 1024;
    criticalPMList.push_back(p);

    p.first = 375;
    criticalPMList.push_back(p);
	
    p.first = 625;
    criticalPMList.push_back(p);
    
#ifdef CQPROC
    // first-order proc
    criticalProcessorsList.push_back(250);
    criticalProcessorsList.push_back(500);

#ifdef CQ2ND
    // second-order proc-proc
    criticalProcessorsList.push_back(375);
    criticalProcessorsList.push_back(625);
    criticalProcessorsList.push_back(1000);

#ifdef CQMEM
    // second-order proc-mem
    p.first = 125;
    p.second = 1024;
    criticalPMList.push_back(p);

    p.first = 500;
    criticalPMList.push_back(p);    
#endif
#endif
#endif

#elif APP == MWD_OLD && ARCH == 4

    // first-order switch    
    criticalProcessorsList.push_back(750);

    p.first = 500;
    p.second = 1024;
    criticalPMList.push_back(p);

#elif APP == MPEG3P && ARCH == 4

#ifdef CQMEM
    // first-order mem (3)
    p.first = 0;
    p.second = 64;
    criticalPMList.push_back(p);

    p.second = 96;
    criticalPMList.push_back(p);
    
    p.second = 256;
    criticalPMList.push_back(p);
    
#ifdef CQ2ND
    // second-order mem-mem (4)
    p.second = 64*2;
    criticalPMList.push_back(p);
    
    p.second = 64+96;
    criticalPMList.push_back(p);
    
    p.second = 64+256;
    criticalPMList.push_back(p);
    
    p.second = 96+256;
    criticalPMList.push_back(p);
#endif
#endif

    // first-order switch (2)
    p.first = 250;
    p.second = 64;
    criticalPMList.push_back(p);
    
    p.first = 750;
    p.second = 256;
    criticalPMList.push_back(p);

#ifdef CQPROC
    // first-order proc (2)
    criticalProcessorsList.push_back(250);
    criticalProcessorsList.push_back(500);    

#ifdef CQ2ND
    // second-order proc-proc (4)
    criticalProcessorsList.push_back(375);
    criticalProcessorsList.push_back(625);
    criticalProcessorsList.push_back(750);
    criticalProcessorsList.push_back(1000);

#ifdef CQMEM
    // second-order proc-mem (9)
    p.first = 125;
    p.second = 64;
    criticalPMList.push_back(p);

    p.second = 96;
    criticalPMList.push_back(p);

    p.second = 256;
    criticalPMList.push_back(p);

    p.first = 250;
    p.second = 64;
    criticalPMList.push_back(p);

    p.second = 96;
    criticalPMList.push_back(p);

    p.second = 256;
    criticalPMList.push_back(p);

    p.first = 500;
    p.second = 64;
    criticalPMList.push_back(p);

    p.second = 96;
    criticalPMList.push_back(p);

    p.second = 256;
    criticalPMList.push_back(p);    
#endif
#endif
#endif

    
#elif APP == MPEG3P && ARCH == 5

#ifdef CQMEM
    // first-order mem (3)
    p.first = 0;
    p.second = 64;
    criticalPMList.push_back(p);

    p.second = 96;
    criticalPMList.push_back(p);
    
    p.second = 256;
    criticalPMList.push_back(p);

#ifdef CQ2ND
    // second-order mem-mem (4)
    p.second = 64*2;
    criticalPMList.push_back(p);
    
    p.second = 64+96;
    criticalPMList.push_back(p);
    
    p.second = 64+256;
    criticalPMList.push_back(p);
    
    p.second = 96+256;
    criticalPMList.push_back(p);    
#endif
#endif

    // first-order switch (5)
    criticalProcessorsList.push_back(625);
    
    p.first = 250;
    p.second = 64;
    criticalPMList.push_back(p);
    
    p.first = 500;
    criticalPMList.push_back(p);    
    
    p.first = 125;
    p.second = 96;
    criticalPMList.push_back(p);
    
    p.first = 250;
    p.second = 256;
    criticalPMList.push_back(p);

#ifdef CQPROC
    // first-order proc (2)
    criticalProcessorsList.push_back(250);
    criticalProcessorsList.push_back(500);    

#ifdef CQ2ND
    // second-order proc-proc (3)
    criticalProcessorsList.push_back(375);
    criticalProcessorsList.push_back(750);
    criticalProcessorsList.push_back(1000);

#ifdef CQMEM
    // second-order proc-mem (9)
    p.first = 125;
    p.second = 64;
    criticalPMList.push_back(p);

    p.second = 96;
    criticalPMList.push_back(p);

    p.second = 256;
    criticalPMList.push_back(p);

    p.first = 250;
    p.second = 64;
    criticalPMList.push_back(p);

    p.second = 96;
    criticalPMList.push_back(p);

    p.second = 256;
    criticalPMList.push_back(p);

    p.first = 500;
    p.second = 64;
    criticalPMList.push_back(p);

    p.second = 96;
    criticalPMList.push_back(p);

    p.second = 256;
    criticalPMList.push_back(p);
#endif
#endif
#endif

    
#elif APP == MPEGCPL2ND && ARCH == 10
    sys.setExternalRConvec(true);
    sys.setRConvec(29.775);

#ifdef CQMEM
    // first-order mem (3)
    p.first = 0;
    p.second = 128;
    criticalPMList.push_back(p);
    
    p.second = 192;
    criticalPMList.push_back(p);
    
    p.second = 384;
    criticalPMList.push_back(p);

#ifdef CQ2ND
    // second-order mem-mem (5)
    p.second = 128*2;
    criticalPMList.push_back(p);

    p.second = 128+192;
    criticalPMList.push_back(p);
    
    p.second = 128+384;
    criticalPMList.push_back(p);
    
    p.second = 192+384;
    criticalPMList.push_back(p);
    
    p.second = 384*2;
    criticalPMList.push_back(p);
#endif
#endif

    // first-order switch (7)
    criticalProcessorsList.push_back(375);
    criticalProcessorsList.push_back(750);

    // 125/128
    p.first = 125;
    p.second = 128;    
    criticalPMList.push_back(p);

    // 250/128
    p.first = 250;
    criticalPMList.push_back(p);
    
    // 125/192
    p.first = 125;
    p.second = 192;
    criticalPMList.push_back(p);

    // 250/192
    p.first = 250;
    criticalPMList.push_back(p);

    // 250/384
    p.second = 384;
    criticalPMList.push_back(p);
    
#ifdef CQPROC
    // first-order proc (2)
    criticalProcessorsList.push_back(250);
    criticalProcessorsList.push_back(500);
    
#ifdef CQ2ND
    // second-order proc-proc (2)
    criticalProcessorsList.push_back(625);
    criticalProcessorsList.push_back(1000);    
    
#ifdef CQMEM
    // second-order proc-mem (5)
    p.first = 125;
    p.second = 384;
    criticalPMList.push_back(p);
    
    p.first = 250;
    p.second = 128;
    criticalPMList.push_back(p);

    p.first = 500;
    p.second = 128;
    criticalPMList.push_back(p);
    
    p.second = 192;
    criticalPMList.push_back(p);
    
    p.second = 384;
    criticalPMList.push_back(p);
#endif
#endif
#endif

    
#else
    cerr << "*** Error: undefined APP / ARCH pair" << endl;
    sys.cleanUpAndExit(1);
#endif
  
    // Stage 0
    // Allocate redundancy greedily up to first amount of critical execution slack

    cout << "### Stage 0 results" << endl;
    sys.printComponentNames();
    cout << "area wl mttf" << endl;

    // evaluate initial design
    int nExpl = 1;

    string key = sys.buildKey();
    sys.setExpl(key);
    
    sys.samplingRun();    
    sys.printComponentCapacities();
    cout << sys.getArea() << " " << sys.getWL() << " " << sys.getMTTF() << endl;
    
    int procAllocLevel = 1;
    int execSlack = processorAllocationLevels[procAllocLevel];

#ifndef GREEDY
    int stage0Limit;
    if (criticalProcessorsList.size() > 0)
	stage0Limit = criticalProcessorsList[0];
    else
	stage0Limit = processorAllocationLevels[processorAllocationLevels.size()-1];
    
    while (execSlack < stage0Limit) {
	// find permutations that minimally increase the slack of any one processor
	cout << "###   " << execSlack << " MIPS" << endl;
	
	vector<vector<componentType> > fixedAllocPermutations =
	    sys.getProcessorPermutations(execSlack);
	// remove permutations that don't differ in a single allocation
	vector<vector<componentType> > incrementalPermutations =
	    sys.findIncrementalProcessorPermutations(fixedAllocPermutations);	

	if (incrementalPermutations.size() > 0) {
	    // iterate over all possible permutations, keeping track of the one that maximizes MTTF
	    nExpl += exhaustiveExecSlackAllocation(&sys, incrementalPermutations);
	    
	    // report permutation and statistics
	    cout << "*** ";
	    sys.printComponentCapacities();
	    cout << sys.getArea() << " " << sys.getWL() << " " << sys.getMTTF() << endl;
	} // if

	// advance to next allocation level
	procAllocLevel++;
	if (procAllocLevel < (int) processorAllocationLevels.size())
	    execSlack = processorAllocationLevels[procAllocLevel];
	else
	    execSlack++;
    } // while
#endif
    
    cout << "###   " << nExpl << " designs explored so far" << endl;
    
    // Stage 1 loop
    cout << "### Stage 1 results" << endl;
    
    for(int x = 0; x < (int) criticalProcessorsList.size(); x++) {
	execSlack = criticalProcessorsList[x];

	cout << "###   " << execSlack << " MIPS" << endl;
	
	// find the least allocation level that is larger than execSlack
	procAllocLevel = findAllocationLevelCeiling(execSlack, processorAllocationLevels);
	execSlack = processorAllocationLevels[procAllocLevel];

	// get all the permutations at that allocation level
	vector<vector<componentType> > permutations =
	    sys.getProcessorPermutations(execSlack);
	// exhaustively search for the best one
	nExpl += exhaustiveExecSlackAllocation(&sys, permutations);

	// report permutation and statistics
	cout << "*** ";
	sys.printComponentCapacities();
	cout << sys.getArea() << " " << sys.getWL() << " " << sys.getMTTF() << endl;

	// allocate slack greedily up to the maximum processor allocation level
	procAllocLevel++;
	if (procAllocLevel < (int) processorAllocationLevels.size())
	    execSlack = processorAllocationLevels[procAllocLevel];
	else
	    execSlack++;	
	
	while (execSlack <= processorAllocationLevels[processorAllocationLevels.size()-1]) {
	    vector<vector<componentType> > fixedAllocPermutations =
		sys.getProcessorPermutations(execSlack);
	    // remove permutations that don't differ in a single allocation
	    vector<vector<componentType> > incrementalPermutations =
		sys.findIncrementalProcessorPermutations(fixedAllocPermutations);	

	    cout << "###     " << execSlack << " MIPS" << endl;
	    
	    if (incrementalPermutations.size() > 0) {
		// iterate over all possible permutations, keeping track of the one that maximizes MTTF
		nExpl += exhaustiveExecSlackAllocation(&sys, incrementalPermutations);
		
		// report permutation and statistics
		cout << "*** ";
		sys.printComponentCapacities();
		cout << sys.getArea() << " " << sys.getWL() << " " << sys.getMTTF() << endl;
	    } // if
	    
	    // advance to next allocation level
	    procAllocLevel++;
	    if (procAllocLevel < (int) processorAllocationLevels.size())
		execSlack = processorAllocationLevels[procAllocLevel];
	    else
		execSlack++;
	} // while
    } // for

    cout << "###   " << nExpl << " designs explored so far" << endl;
    
    // Stage 2 loop
    cout << "### Stage 2 results" << endl;
    
    for(int x = 0; x < (int) criticalPMList.size(); x++) {
	execSlack = criticalPMList[x].first;
	int storSlack = criticalPMList[x].second;

	cout << "###   " << execSlack << " MIPS / " << storSlack << " KB" << endl;

	// find the least allocation level that is larger than execSlack
	procAllocLevel = findAllocationLevelCeiling(execSlack, processorAllocationLevels);
	execSlack = processorAllocationLevels[procAllocLevel];
	// find the least allocation level that is larger than storSlack
	int memAllocLevel = findAllocationLevelCeiling(storSlack, memoryAllocationLevels);
	storSlack = memoryAllocationLevels[memAllocLevel];

	//cout << "Before gathering permutations: " << execSlack << " / " << storSlack << endl;
	
	// get all processor and memory permutations at these levels
	vector<vector<componentType> > procPermutations =
	    sys.getProcessorPermutations(execSlack);

	//cout << "Before getting memory permutations" << endl;
	
	vector<vector<componentType> > memPermutations =
	    sys.getMemoryPermutations(storSlack);

	//cout << "Before exhaustive search" << endl;
	
	// exhaustively search for the best one
	nExpl += exhaustiveExecStorSlackAllocation(&sys, procPermutations, memPermutations);

	//cout << "After exhaustive search" << endl;
	
	// report permutation and statistics
	cout << "*** ";
	sys.printComponentCapacities();
	cout << sys.getArea() << " " << sys.getWL() << " " << sys.getMTTF() << endl;

	float lastMTTF = sys.getMTTF();
	float lastCost = GREEDY_COST(sys.getArea(), sys.getWL());

	vector<componentType> lastProcPermutation = sys.buildProcessorPermutation();
	vector<componentType> lastMemPermutation = sys.buildMemoryPermutation();
	
	// allocate slack greedily while there are still gains and allocation levels to attempt
	bool done = false;
	while (!done) {	    
	    // FIND THE BEST INCREMENTAL PROCESSOR PERMUTATION
	    bool procPermutations = false;
	    
	    procAllocLevel++;
	    execSlack = processorAllocationLevels[procAllocLevel];

	    while (!procPermutations &&
		   execSlack <= processorAllocationLevels[processorAllocationLevels.size()-1]) {
		vector<vector<componentType> > fixedAllocPermutations =
		    sys.getProcessorPermutations(execSlack);
		// remove permutations that don't differ in a single allocation
		vector<vector<componentType> > incrementalPermutations =
		    sys.findIncrementalProcessorPermutations(fixedAllocPermutations);	
		
		if (incrementalPermutations.size() > 0) {
		    // iterate over all possible permutations, keeping track of the one that maximizes MTTF
		    cout << "###   Execution slack permutation (" << execSlack << "):" << endl;
		    nExpl += exhaustiveExecSlackAllocation(&sys, incrementalPermutations);

		    // report permutation and statistics
		    cout << "*** ";
		    sys.printComponentCapacities();
		    cout << sys.getArea() << " " << sys.getWL() << " " << sys.getMTTF() << endl;

		    procPermutations = true;
		} else {		
		    // advance to next allocation level
		    procAllocLevel++;
		    if (procAllocLevel < (int) processorAllocationLevels.size())
			execSlack = processorAllocationLevels[procAllocLevel];
		    else
			execSlack++;
		}
	    } // while

	    // save the best processor permutation
	    vector<componentType> bestProcPermutation = sys.buildProcessorPermutation();
	    
	    // save statistics for the best processor permutation
	    float bestProcMTTF, bestProcCost;
	    
	    if (procPermutations == true) {
		bestProcMTTF = sys.getMTTF();
		bestProcCost = GREEDY_COST(sys.getArea(), sys.getWL());
	    } else {
		// didn't find an incremental permutation, so nothing to compare against
		bestProcMTTF = lastMTTF;
		bestProcCost = lastCost;
	    } // if-else

	    // restore system
	    sys.applyProcessorPermutation(lastProcPermutation);
	    
	    // FIND INCREMENTAL MEMORY PERMUTATION
	    bool memPermutations = false;

	    memAllocLevel++;
	    storSlack = memoryAllocationLevels[memAllocLevel];

	    while (!memPermutations &&
		   storSlack <= memoryAllocationLevels[memoryAllocationLevels.size()-1]) {
		vector<vector<componentType> > fixedAllocPermutations =
		    sys.getMemoryPermutations(storSlack);
		// remove permutations that don't differ in a single allocation
		vector<vector<componentType> > incrementalPermutations =
		    sys.findIncrementalMemoryPermutations(fixedAllocPermutations);	
		
		if (incrementalPermutations.size() > 0) {
		    // iterate over all possible permutations, keeping track of the one that maximizes MTTF
		    cout << "###   Storage slack permutation (" << storSlack << "):" << endl;
		    nExpl += exhaustiveStorSlackAllocation(&sys, incrementalPermutations);

		    // report permutation and statistics
		    cout << "*** ";
		    sys.printComponentCapacities();
		    cout << sys.getArea() << " " << sys.getWL() << " " << sys.getMTTF() << endl;

		    memPermutations = true;
		} else {		
		    // advance to next allocation level
		    memAllocLevel++;
		    if (memAllocLevel < (int) memoryAllocationLevels.size())
			storSlack = memoryAllocationLevels[memAllocLevel];
		    else
			storSlack++;
		}
	    } // while

	    // save the best processor permutation
	    vector<componentType> bestMemPermutation = sys.buildMemoryPermutation();
	    
	    // save statistics for the best processor permutation
	    float bestMemMTTF, bestMemCost;
	    
	    if (memPermutations == true) {
		bestMemMTTF = sys.getMTTF();
		bestMemCost = GREEDY_COST(sys.getArea(), sys.getWL());
	    } else {
		// didn't find an incremental permutation, so nothing to compare against
		bestMemMTTF = lastMTTF;
		bestMemCost = lastCost;
	    } // if-else

	    // restore system
	    sys.applyMemoryPermutation(lastMemPermutation);

	    // COMPARE PROCESSOR AND MEMORY PERMUTATIONS

	    // Determine whether the processor or memory redundancy
	    // should be selected
	    int winner = 0; // 0 == neither, 1 == processor, 2 == memory

	    // first check that a new permutation was evaluated in each case--
	    // if not, select the option that considered a new permutation
	    if (procPermutations && !memPermutations) {
		winner = 1;
	    } else if (!procPermutations && memPermutations) {
		winner = 2;
	    } else if (!procPermutations && !memPermutations) {
		cerr << "*** No new permutation evaluated!" << endl;
		sys.cleanUpAndExit(1);
	    } else {
#ifdef DISTANCE
		// Calculate the distance from each point to the origin
		float procDist = distance(0, 0, bestProcCost, 1.0/bestProcMTTF);
		float memDist = distance(0, 0, bestMemCost, 1.0/bestMemMTTF);
		
		winner = procDist <= memDist ? 1 : 2;
#else
		// Calculate the slope of the best processor allocation
		float procDeltaMTTF = bestProcMTTF - lastMTTF;
		float procDeltaCost = bestProcCost - lastCost;
		// protect against divide by zero
		float procSlope = 0;
		if (procDeltaCost != 0)
		    procSlope = procDeltaMTTF / procDeltaCost;
		
		// Calculate the slope of the best memory allocation
		float memDeltaMTTF = bestMemMTTF - lastMTTF;
		float memDeltaCost = bestMemCost - lastCost;
		// protect against divide by zero
		float memSlope = 0;
		if (memDeltaCost != 0)
		    memSlope = memDeltaMTTF / memDeltaCost;
		
		// a permutation was evaluated in each case, so select
		// the best of the two
		
		if((procSlope > 0) && (memSlope > 0)) {
		    if((procDeltaMTTF > 0) && (memDeltaMTTF > 0)) {
			winner = procSlope > memSlope ? 1 : 2;
		    }
		    else if((procDeltaMTTF < 0) && (memDeltaMTTF < 0)) {
			winner = procSlope > memSlope ? 2 : 1;
		    }
		    else if((procDeltaMTTF > 0) && (memDeltaMTTF < 0)) {
			winner = 1;
		    }
		    else if((procDeltaMTTF < 0) && (memDeltaMTTF > 0)) {
			winner = 2;
		    }
		    else {
			cerr << "Unknown case 1" << endl;
			sys.cleanUpAndExit(1);
		    }
		}
		else if((procSlope > 0) && (memSlope <= 0)) {
		    //winner = memDeltaMTTF > 0 ? 2 : (procDeltaMTTF > 0 ? 1 : 0);
		    winner = memDeltaMTTF > 0 ? 2 : 1;
		}
		else if((procSlope <= 0) && (memSlope > 0)) {
		    winner = procDeltaMTTF > 0 ? 1 : 2;
		}
		else if((procSlope <= 0) && (memSlope <= 0)) {		    
		    if((procDeltaCost < 0) && (memDeltaCost < 0)) {
			winner = procSlope > memSlope ? 2 : 1;
		    }
		    else if(procDeltaCost < 0) {
			winner = 1;
		    }
		    else if(memDeltaCost < 0) {
			winner = 2;
		    }
		    else if((procDeltaCost >= 0) && (procDeltaCost >= 0)) {
			//winner = 0;
			winner = procDeltaMTTF > memDeltaMTTF ? 1 : 2;
		    }
		    else {
			cout << "Unknown case 2: "
			     << procSlope << " / "
			     << memSlope << " | "
			     << procDeltaCost << " / "
			     << memDeltaCost << " | "
			     << procDeltaMTTF << " / "
			     << memDeltaMTTF
			     << endl;
			sys.cleanUpAndExit(1);
		    }
		}
		else {
		    cerr << "Couldn't pick a winner...procSlope = " << procSlope << " memSlope = " << memSlope << endl;
		    sys.cleanUpAndExit(1);
		}
#endif
	    }
	    
	    // Apply the better of the greedy allocations
	    if(winner == 1) {
		// processor permutation wins, so reapply it
		sys.applyProcessorPermutation(bestProcPermutation);
		// save permutations
		lastProcPermutation = bestProcPermutation;
		// save statistics
		lastMTTF = bestProcMTTF;
		lastCost = bestProcCost;
		// restore memAllocLevel
		memAllocLevel--;

		cout << "###   Execution slack allocation selected" << endl;
	    }
	    else if(winner == 2) {
		// memory permutation wins, so reapply it
		sys.applyMemoryPermutation(bestMemPermutation);
		// save permutations
		lastMemPermutation = bestMemPermutation;
		// save statistics
		lastMTTF = bestMemMTTF;
		lastCost = bestMemCost;
		// restore procAllocLevel
		procAllocLevel--;

		cout << "###   Storage slack allocation selected" << endl;

	    }
	    else {
		done = true;

		cout << "###   Neither slack allocation selected, terminating" << endl;
	    }
	    
	    if (storSlack >=
		memoryAllocationLevels[memoryAllocationLevels.size()-1] &&
		execSlack >=
		processorAllocationLevels[processorAllocationLevels.size()-1])
		done = true;
	    
	    // restore statistics
	    //sys.samplingRun();
	    //sys.printComponentCapacities();
	    //cout << sys.getArea() << " " << sys.getWL() << " " << sys.getMTTF() << endl;
	}
    }
  
    cout << "### Total number of designs evaluated: " << nExpl << endl;
    return 0;
}
