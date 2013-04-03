/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#include <cstring>
#include <cmath>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "power.h"
#include "config.h"
#include "ManufacturingDefect.h"
#include "Parquet.h"

#include "FailureMechanism.h"
#include "EMFailureMechanism.h"
#include "TCFailureMechanism.h"
#include "TDDBFailureMechanism.h"

#include "System.h"

System::System()
{
	// initialize random number generator
	rand_ln = gsl_rng_alloc(gsl_rng_taus);
	gsl_rng_set(rand_ln, LN_R_SEED);
	
	mcsVerbosity = 0;
	tempUpdate = false;
	initTemps = true;
	zeroPower = true;
	idealMTTF = false;
	buildFailures = true;
	measureYield = false;
	maxLinkBandwidth = BANDWIDTH_LIMIT;
	mappingPermutations  = N_PERMUTATIONS;
	
	operatingScenariosFound = false;
	initialTempsFound = false;
	externalRConvec = false;
	
	nextComponentID = 0;

	configFileName = "";
	floorplanFileName = "";
	taskGraphFileName = "";
	netlistFileName = "";
	databaseFileName = "";
	m_samples = N_SAMPLES;
	initialTaskMapping = new TaskMapping(this);
	reset();

	// default value
	r_convec = 15;
	
	setWorkingDirectory();

	// default floorplanning values
	fpIter = FP_ITER;
	fpAreaWeight = FP_AW;
	fpWireWeight = FP_WW;

	maxMemoryType = MEM2MB;
}

System::~System()
{
	// Nothing needs to be done here assuming that strings free themselves
}

float System::averageComponentTemperature() {
    float sum = 0;

    // iterate over all components, taking the average component temperature
    vector<Component*>::iterator iter = components.begin();
    while (iter != components.end()) {
	Component *c = *iter;
	sum += c->getCurrentTemperature();
	
	iter++;
    } // while

    return sum / (float) components.size();
} // averageComponentTemperature

void System::calibrateRConvec(int taskmapping) {
    float abs_min_rc = HS_MINRC;
    float min_rc = HS_MINRC;
    float rc = HS_DEFRC;
    float abs_max_rc = HS_MAXRC;
    float max_rc = HS_MAXRC;
    float zero = HS_ZERO;
    float charTemp = HS_CHART;
    
    // iterate until average component temperature is ~345K
    bool done = false;
    while (!done) {
	r_convec = rc;
	
	// run hotSpot
	run_single_HotSpot_simulation(this, taskmapping, (xsize >= ysize) ? xsize : ysize, r_convec);

	// read temperature file
	readTempsFromFile(taskmapping);

	// determine average component temperature
	float avgTemp = averageComponentTemperature();

	//cout << "&&& Calibrating r_convec: " << avgTemp << "K / [" << min_rc << ", " << r_convec << ", " << max_rc << "]" << endl;

	// if avgTemp == charTemp, we're done
	if (ABS(charTemp - avgTemp) < zero) {
	    done = true;
	} else {
	    if (avgTemp > charTemp) {
		// reset max
		max_rc = rc;
		// reduce rc
		rc -= (rc - min_rc) / 2;
		// reset min
		min_rc = rc - (max_rc - rc);
	    } else {
		// reset min
		min_rc = rc;
		// increase rc
		rc += (max_rc - rc) / 2;
		// reset max
		max_rc = rc + (rc - min_rc);
	    } // if/else

	    // check if we need a value out of bounds
	    if (ABS(min_rc - max_rc) < zero) {
		if (ABS(min_rc - abs_min_rc) < zero) {
		    // we need to set the low bound lower
		    abs_min_rc = abs_min_rc / 2;
		    min_rc = abs_min_rc;
		    rc -= (max_rc - min_rc)/2;
		} else if (ABS(max_rc - abs_max_rc) < zero) {
		    // we need to set the upper bound higher
		    abs_max_rc = abs_max_rc * 2;
		    max_rc = abs_max_rc;
		    rc += (max_rc - min_rc)/2;
		} // if/else
	    } // if

	    // check if r_convec needs to be zero to satisfy charTemp
	    if (rc < zero) {
		cerr << "*** Error: during r_convec calibration, r_convec -> zero" << endl;
		cleanUpAndExit(1);
	    } // if
	} // if/else
    } // while

    cout << "*** RConvec: " << r_convec << endl;
} // calibrateRConvec

void System::cleanUpAndExit(int status) {
    removeWorkingDirectory();
    exit(status);
}

void System::removeWorkingDirectory() {
    stringstream ss;
    ss << "rm -rf " << workingDirectory;
    string cmd = ss.str();
    
    system(cmd.c_str());
} 

void System::setWorkingDirectory() {
    stringstream ss;
    
    // create the base directory if it doesn't already exist
    ss << WDPATH;
    string baseDirectory = ss.str();
    
    mkdir(baseDirectory.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
    if (errno != EEXIST && errno != 0) {
	// directory creation failed, exit
	cerr << "Failed to create directory " << baseDirectory << " (" << errno << ")" << endl;
	cleanUpAndExit(1);
    }

    // construct a string with using the pid and date
    pid_t pid = getpid();
    time_t now;
    time(&now);
    struct tm *nowDate = localtime(&now);

    int year = nowDate->tm_year - 100;
    int month = nowDate->tm_mon;
    int day = nowDate->tm_mday;
    int hour = nowDate->tm_hour;
    int min = nowDate->tm_min;
    
    ss.fill('0');
    ss << "mcs_" << (int) pid << "_"
       << setw(2) << year
       << setw(2) << month
       << setw(2) << day
       << "."
       << setw(2) << hour
       << setw(2) << min
       << "/";
	
    workingDirectory = ss.str();

    //cout << workingDirectory << endl;
    
    // create the directory
    mkdir(workingDirectory.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
    if (errno != EEXIST && errno != 0) {
	// directory creation failed, exit
	cerr << "Failed to create directory " << workingDirectory << " (" << errno << ")" << endl;
	cleanUpAndExit(1);
    }

    // create sub directories
    string fpDirectory = workingDirectory;
    fpDirectory.append(FPPATH);

    // create the directory
    mkdir(fpDirectory.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
    if (errno != EEXIST && errno != 0) {
	// directory cremaation failed, exit
	cerr << "Failed to create directory " << fpDirectory << endl;
	cleanUpAndExit(1);
    }
    
    string hotSpotDirectory = workingDirectory;
    hotSpotDirectory.append(HSPATH);

    // create the directory
    mkdir(hotSpotDirectory.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
    if (errno != EEXIST && errno != 0) {
	// directory creation failed, exit
	cerr << "Failed to create directory " << hotSpotDirectory << endl;
	cleanUpAndExit(1);
    }
}

Component * System::addComponent(string cname, componentType type)
{
  // Create component
  Component *c = new Component(cname,componentLibrary,type,getNextComponentID(),initTemps);
  components.push_back(c);
  
  // Add EM failure mechanism
  EMFailureMechanism *em = new EMFailureMechanism("EM",c,rand_ln,componentLibrary[type].Aem);
  c->addFailureMechanism(em);
  
  // Add TDDB failure mechanism
  TDDBFailureMechanism *tddb = new TDDBFailureMechanism("TDDB",c,rand_ln,componentLibrary[type].vdd);
  c->addFailureMechanism(tddb);
  
  // Add TC failure mechanism
  TCFailureMechanism *tc = new TCFailureMechanism("TC",c,rand_ln);
  c->addFailureMechanism(tc);

  // add manufacturing defect mechanism
  ManufacturingDefect *md = new ManufacturingDefect("MD",c,rand_ln);
  c->setManufacturingDefect(md);
  
  return c;
}

int System::getNextComponentID()
{
  // Move to the next component ID
  nextComponentID++;

  // Return the last component ID
  return (nextComponentID - 1);
}

void System::sortComponents()
{
  sort(components.begin(),components.end(),compareComponentIDs());
  return;
}

Component *System::getComponent(string cname) 
{
	// find component by name
	Component *c;
	bool notFound = true;
	
	vector<Component*>::iterator iter = components.begin();
	while (notFound && iter != components.end()) {
		c = *iter;
		if (c->getName() == cname)
			notFound = false;
			
		iter++;
	}
	
	if (notFound)
		return NULL;
	else
		return c;
}

vector<Component*> System::getComponents()
{
	return components;
}

vector<Component*> System::getProcessors() {
	vector<Component*> processors;

	vector<Component*>::iterator iter = components.begin();
	while (iter != components.end()) {
		Component *c = *iter;

		if (c->getGType() == PROC)
			processors.push_back(c);

		iter++;
	} // while

	return processors;
}

vector<Component*> System::getMemories() {
	vector<Component*> memories;
	
	vector<Component*>::iterator iter = components.begin();
	while (iter != components.end()) {
		Component *c = *iter;

		if (c->getGType() == MEM)
			memories.push_back(c);
			
		iter++;
	} // while
	
	return memories;	
}

vector<Component*> System::getProcessorsRedundantCapacity() {
    vector<Component*> r_processors;
    vector<Component*> processors = getProcessors();
  
    vector<Component*>::iterator iter = processors.begin();
    while (iter != processors.end()) {
	Component *c = *iter;
    
	if (c->getCType() != c->getMType())
	    r_processors.push_back(c);
    
	iter++;
    } // while
  
    return r_processors;
}

vector<Component*> System::getProcessorsNoRedundantCapacity() {
    vector<Component*> nr_processors;
    vector<Component*> processors = getProcessors();
  
    vector<Component*>::iterator iter = processors.begin();
    while (iter != processors.end()) {
	Component *c = *iter;
    
	if (c->getCType() == c->getMType())
	    nr_processors.push_back(c);
    
	iter++;
    } // while
  
    return nr_processors;
}

vector<Component*> System::getMemoriesRedundantCapacity() {
    vector<Component*> r_memories;
    vector<Component*> memories = getMemories();
  
    vector<Component*>::iterator iter = memories.begin();
    while (iter != memories.end()) {
	Component *c = *iter;
    
	if (c->getCType() != c->getMType())
	    r_memories.push_back(c);
    
	iter++;
    } // while
  
    return r_memories;
}

vector<Component*> System::getMemoriesNoRedundantCapacity() {
    vector<Component*> nr_memories;
    vector<Component*> memories = getMemories();
    
    vector<Component*>::iterator iter = memories.begin();
    while (iter != memories.end()) {
	Component *c = *iter;
	
	if (c->getCType() == c->getMType())
	    nr_memories.push_back(c);
	
    iter++;
    } // while
    
    return nr_memories;
}

void System::allocateRedundancy(Component *c) {
	// sanity check
	if (c->getAType() == NONE) {
	    cerr << "*** Attempted to allocate redundancy to an unsupported component " << c->getName()
		 << " of type " << c->getCType()
		     << endl;
		cleanUpAndExit(1);
	}
	
	// change component type and update component parameters
	c->setComponentType(c->getAType());
}

void System::deallocateRedundancy(Component *c) {
	// sanity check
	if (c->getDType() == NONE) {
		cerr << "*** Attempted to deallocate redundancy from an unsupported component type " << c->getCType() << endl;
		cleanUpAndExit(1);
	}
	
	c->setComponentType(c->getDType());
}

void System::clearRedundancies(vector<Component*> selectedComponents) {
    vector<Component*>::iterator iter = selectedComponents.begin();

    while (iter != selectedComponents.end()) {
	Component *c = *iter;
	c->clearRedundancy();
	
	iter++;
    } // while
} // clearRedundancies

void System::clearAllRedundancies() {
    vector<Component*>::iterator iter = components.begin();

    while (iter != components.end()) {
	Component *c = *iter;
	c->clearRedundancy();
	
	iter++;
    } // while
} // clearAllRedundancies

void System::printComponentNames() {
    vector<Component*> processors = getProcessors();
    vector<Component*> memories = getMemories();

    // print processor names
    vector<Component*>::iterator iter = processors.begin();
    while (iter != processors.end()) {
	Component *c = *iter;
	cout << c->getName() << " ";
	
	iter++;
    } // while
    
    // print memory names
    iter = memories.begin();
    while (iter != memories.end()) {
	Component *c = *iter;
	cout << c->getName() << " ";
	
	iter++;
    } // while
} // printComponents

void System::printComponentTypes() {
    vector<componentType> procPerm = buildProcessorPermutation();
    vector<componentType> memPerm = buildMemoryPermutation();

    // print processor permutation
    printPermutation(procPerm);
    printPermutation(memPerm);
} // printComponentTypes

void System::printComponentCapacities() {
    vector<componentType> procPerm = buildProcessorPermutation();
    vector<componentType> memPerm = buildMemoryPermutation();

    vector<componentType>::iterator iter = procPerm.begin();
    while (iter != procPerm.end()) {
	componentType type = *iter;

	cout << componentTypeToCapacity(type) << " ";
	
	iter++;
    } // while
    
    iter = memPerm.begin();
    while (iter != memPerm.end()) {
	componentType type = *iter;

	cout << componentTypeToCapacity(type) << " ";
	
	iter++;
    } // while
} // printComponentCapacities

void System::addEmptyBlock(string emptyBlockName) {
	emptyBlocks.push_back(emptyBlockName);
	return;
}

vector<string> System::getEmptyBlocks()
{
	return emptyBlocks;
}

void System::setOperatingScenariosFound() {
	operatingScenariosFound = true;
	return;
}

void System::addTask(Task *t)
{
  tasks.push_back(t);
  return;
}

Task * System::getTask(string n)
{
  int x;

  // Step through the list of task objects until the desired one is found
  for(x = 0; x < (int)tasks.size(); x++) {
    if(tasks[x]->getName() == n) {
      return tasks[x];
    }
  }

  // Return NULL if the correct task object couldn't be found
  return NULL;
}

void System::buildAllScenarios()
{
  int x, y;
  
  // Loop over all possible combinations of components
  // Don't start at 0 since that would mean all components have failed
  for(x = 1; x < (1 << (int)components.size()); x++) {
    set<Component*,compareComponentIDs> curScenario;
    
    // Loop over each component
    for(y = 0; y < (int)components.size(); y++) {
      
      // Determine whether or not the current component should
      // be added to the current scenario
      if((x >> y) & 1) {
	curScenario.insert(components[y]);
      }
    }
    
    // Add the newly created scenario to the list of operaing scenarios
    allScenarios.push_back(curScenario);
  }
  
  if(mcsVerbosity > 0)
    cout << "Created " << allScenarios.size() << " operating scenarios" << endl;
  
  return;
}

void System::buildAllScenariosOrdered()
{
  int x, y, z, curNumScenarios;
  int numComponents = (int)components.size();
  vector<int> curPermutation;

  // Loop over each possible number of failed components
  for(x = 0; x < numComponents; x++) {
    
    // Create the initial sequence with x failed components
    curPermutation.clear();
    for(y = 0; y < x; y++) {
      curPermutation.push_back(0);
    }
    for(y = 0; y < (int)(numComponents - x); y++) {
      curPermutation.push_back(1);
    }

    // Calculate the number of permutations with x failed components
    curNumScenarios = factorial((long long)numComponents) / 
      (factorial((long long)(numComponents - x)) * factorial((long long)x));

    if(mcsVerbosity > 0) {
      printf("Creating %d scenarios based on: ",curNumScenarios);
      for(y = 0; y < numComponents; y++) {
	printf("%d ",curPermutation[y]);
      }
      printf("\n");
    }
    
    // Step through each permutation with x failed components
    for(y = 0; y < curNumScenarios; y++) {
      set<Component*,compareComponentIDs> newScenario;

      /*
      for(z = 0; z < numComponents; z++) {
	printf("%d ",curPermutation[z]);
      }
      printf("\n");
      */

      // Map the permutation to a set of components and place it in the list of all scenarios
      for(z = 0; z < numComponents; z++) {
	if(curPermutation[z] == 1) {
	  newScenario.insert(components[z]);
	}
      }
      allScenarios.push_back(newScenario);

      // Create the next permutation with x failed components
      curPermutation = generateNextPermutation(curPermutation);
    }
  }

  if(mcsVerbosity > 0)
    cout << "Created " << allScenarios.size() << " operating scenarios" << endl;

  return;
}

void System::buildFailureScenarios() {
  int x, y;
  bool curScenarioCovered = false;
  bool curComponentFound = false;
	
  Component *c;
	
  set<Component*,compareComponentIDs> curFailureScenario;
  set<Component*,compareComponentIDs> curScenario;
  set<Component*,compareComponentIDs> intersectionResult;
  insert_iterator<set<Component*,compareComponentIDs> > intBegin(intersectionResult,intersectionResult.begin());
  insert_iterator<set<Component*,compareComponentIDs> > intEnd(intersectionResult,intersectionResult.begin());
  set<Component*,compareComponentIDs> differenceResult;
	
  // Sort the list of all scenarios by number of failed components (smallest first)
  //sort(allScenarios.begin(),allScenarios.end(),compareNumFailedComponents);
	
  // Step through all possible scenarios
  for(x = 0; x < (int)allScenarios.size(); x++)  {
      //if(mcsVerbosity > 0)
	//cout << endl << "Looking at scenario " << x << endl;
		
    //if((x % 1000) == 0)
    //	cout << endl << "Looking at scenario " << x << endl;
		
    curScenario = allScenarios[x];
    curScenarioCovered = false;
		
    // Determine whether or not the current scenario is covered by one of the existing failure scenarios
    // Loop over all failure scenarios
    for(y = 0; y < (int)failureScenarios.size(); y++) {
      curFailureScenario = failureScenarios[y];

      intersectionResult.clear();
      intEnd = set_intersection(curFailureScenario.begin(),curFailureScenario.end(),
				curScenario.begin(),curScenario.end(),
				intBegin,compareComponentIDs());
			
      // If the current operating scenario failed, remove it from the list
      if((int)intersectionResult.size() == 0) {
	curScenarioCovered = true;
	break;
      }
    }
		
    // If the current scenario is not covered, attempt to find a task mapping for it
    if(!curScenarioCovered) {
      TaskMapping *tm = new TaskMapping(this,curScenario);
			
      // If no task mapping could be found, add the missing components as a failure scenario
      if(!tm->getMappingFound()) {

	// This double for loop represents a set difference of a vector and a set
	differenceResult.clear();
	for(y = 0; y < (int)components.size(); y++) {

	  curComponentFound = false;
	  for(set<Component*,compareComponentIDs>::iterator iter = curScenario.begin();
	      iter != curScenario.end(); iter++) {
	    c = *iter;	

	    if(c->getName() == components[y]->getName()) {
	      curComponentFound = true;
	      break;
	    }
	  }
					
	  if(!curComponentFound) {
	    differenceResult.insert(components[y]);
	  }
	}
				
	failureScenarios.push_back(differenceResult);
				
	if(mcsVerbosity > 0) {
	  cout << "Scenario " << x << " has been added to the list of failure scenarios" << endl;
	  for(set<Component*,compareComponentIDs>::iterator iter = differenceResult.begin();
	      iter != differenceResult.end(); iter++) {
	    Component *c = *iter;
	    cout << c->getName() << endl;
	  }
	}
      
	delete tm;
      }
      
      // If a task mapping wase found, add the scenario and task mapping to the appropriate lists
      else {
	string operatingScenarioKey = buildOperatingScenarioKey(curScenario);
	operatingScenarioLookup[operatingScenarioKey] = (int)operatingScenarios.size();
	operatingScenarios.push_back(curScenario);
	taskMappings.push_back(tm);
      }
    }
		
    // If the current scenario is covered, move on to the next scenario
    else {
      //cout << "Scenario " << x << " is already covered by failure scenario " << y << endl;
    }
  }
	
  if(mcsVerbosity > 0) {
    cout << "Found " << failureScenarios.size() << " failure scenarios" << endl;
    for(x = 0; x < (int)failureScenarios.size(); x++) {
      for(set<Component*,compareComponentIDs>::iterator iter = failureScenarios[x].begin();
	  iter != failureScenarios[x].end(); iter++) {
	Component *c = *iter;
	cout << c->getName() << " ";
      }
      cout << endl;
    }
  }

  return;
}

void System::clearFailureScenarios() {
	failureScenarios.clear();
}

void System::clearOperatingScenarios()
{
    operatingScenarios.clear();
    clearOperatingScenarioLookup();
    return;
}

void System::clearTaskMappings()
{
  int x;
  for(x = 0; x < (int)taskMappings.size(); x++) {
    delete taskMappings[x];
  }
  taskMappings.clear();

  // Should we also clear/delete initialTaskMapping here?
  return;
}

void System::clearOperatingScenarioLookup()
{
  operatingScenarioLookup.clear();
  return;
}

void System::clearEmptyBlocks() {
    emptyBlocks.clear();
} // clearEmptyBlocks

void System::createSingleTaskMapping(int pos) {
	TaskMapping *tm = new TaskMapping(this,operatingScenarios[pos]);
	taskMappings.push_back(tm);
	return;
}

void System::resetAvailableCapacitiesToInitial() {
  int x;
  
  for(x = 0; x < (int)components.size(); x++) {
    components[x]->resetAvailableCapacityToInitial();
  }
  
  return;
}

void System::resetPartialCapacitiesToInitial() {
  int x;
  
  for(x = 0; x < (int)components.size(); x++) {
    components[x]->resetPartialCapacityToInitial();
  }
  
  return;
}

void System::resetAvailableCapacitiesToPartial() {
  int x;
  
  for(x = 0; x < (int)components.size(); x++) {
    components[x]->resetAvailableCapacityToPartial();
  }
  
  return;
}

void System::storeNetlist() {
	FILE *netlistFile;
	char line[64]; // Assumes a single line in the netlist file is 63 characters or less
	char netlistTokens[4] = " :\n";
	int numConnections = 0;
	
	netlistFile = fopen(netlistFileName.c_str(),"r");
	if(!netlistFile) {
		cerr << "Couldn't open netlist file " << netlistFileName << " for reading" << endl;
		cleanUpAndExit(1);
	}

	while(fgets(line,sizeof(line),netlistFile)) {
		string curSrc, curDst, dirString;
		Component *srcComponent;
		Component *dstComponent;

		// If the current line is blank, move to the next line
		if(!strncmp("\n",line,1)) {
			continue;
		}
		// If the current line does not start with "NetDegree", move to the next line
		else if(strncmp("NetDegree",strtok(line,netlistTokens),9)) {
			continue;
		}
	
		// If the current line starts with "NetDegree", determine the number of connections to read
		else {
			numConnections = atoi(strtok(NULL,netlistTokens));
		}
	
		// Reading the connections of a net (current structure of net allows only 2 connections)
		// Read the first connection of the net
		if(!fgets(line,sizeof(line),netlistFile)) {
			cerr << "Incorrect netlist file specification, no source connection" << endl;
			cleanUpAndExit(1);
		}
		
		// Store the source connection and the directionality of the net
		curSrc = strtok(line,netlistTokens);
		dirString = strtok(NULL,netlistTokens);

		// Ensure the source corresponds to a valid component
		srcComponent = getComponent(curSrc);
		if(!srcComponent) {
		  cerr << "Could not find a component object corresponding to the name " << curSrc << endl;
		  cleanUpAndExit(1);
		}

		// Read the second connection of the net
		if(!fgets(line,sizeof(line),netlistFile)) {
			cerr << "Incorrect netlist file specification, no destination connection" << endl;
			cleanUpAndExit(1);
		}
		
		// Store the destination connection of the net
		curDst = strtok(line,netlistTokens);

		// Ensure the destination corresponds to a valid component
		dstComponent = getComponent(curDst);
		if(!dstComponent) {
		  cerr << "Could not find a component object corresponding to the name " << curDst << endl;
		  cleanUpAndExit(1);
		}

		// Check to make sure the specified directionalities of the net match
		if(strncmp(dirString.c_str(),strtok(NULL,netlistTokens),1)) {
			cerr << "Incorrect netlist file specification, mismatched directionalities" << endl;
			cleanUpAndExit(1);
		}

		ComponentNet *curNet = new ComponentNet(srcComponent,dstComponent,0,0,NO_DIR);
		
		// Set the directionality of the net based on the string in the netlist file
		if(!strncmp(dirString.c_str(),"U",1)) {
			curNet->setDirectionality(UNI);
		}
		else if(!strncmp(dirString.c_str(),"B",1)) {
			curNet->setDirectionality(BI);
		}
		else if(!strncmp(dirString.c_str(),"R",1)) {
			curNet->setDirectionality(REV);
		}
		else {
			cerr << "Incorrect netlist file specification, unknown directionality " << dirString << endl;
			cleanUpAndExit(1);
		}
		
		netlist.push_back(curNet);
	}
	
	fclose(netlistFile);
	return;
}

vector<ComponentNet*> System::copyNetlist() {
  int x;
  vector<ComponentNet*> newNetlist;
  
  for(x = 0; x < (int)netlist.size(); x++) {
    ComponentNet *curNet = new ComponentNet(netlist[x]->getSrc(),netlist[x]->getDst(),0,0,netlist[x]->getDirectionality());
    newNetlist.push_back(curNet);
  }
  
  return newNetlist;
}

void System::storeTaskGraph() {
    FILE *taskGraphFile;
    char line[64]; // Assumes a single line in the task graph file is 63 characters or less
    
    taskGraphFile = fopen(taskGraphFileName.c_str(),"r");
    if(!taskGraphFile) {
	cerr << "Couldn't open task graph file " << taskGraphFileName << " for reading" << endl;
	cleanUpAndExit(1);
    }
	
    int line_number = 0;
    TG_SECTIONTYPE section = TG_SECTION_NONE;
    
    while(fgets(line,sizeof(line),taskGraphFile)) {
	line_number++;
	
	char *tok = strtok(line, TOKENS);

	// check for blank lines
	if (tok == NULL) continue;
	// check for comments
	if (!strncmp(tok, "#", 1)) continue;

	// TODO: convert to lower case
	//g_strdown(tok);

	// based on the current section type, process line in different ways
	switch (section) {
	case TG_SECTION_NONE:
	    // check for the beginning of a new section
	    if (!strcmp(tok, "define")) {
		// see what sort of section this is
		tok = strtok(NULL, TOKENS);
		//g_strdown(tok);
			
		if (!strcmp(tok, "computation")) {
		    section = TG_SECTION_COMP;
		} else if (!strcmp(tok, "communication")) {
		    section = TG_SECTION_COMM;
		} else {
		    fprintf(stderr, "Unrecognized section, line %d\n",
			    line_number);
		    cleanUpAndExit(1);
		} 
	    } else {
		fprintf(stderr, "Unrecognized token, line %d\n", line_number);
		cleanUpAndExit(1);
	    }
	    break;

	case TG_SECTION_COMP:
	    // Leave the section if the end key is encountered
	    if(!strcmp(tok,"end")) {
		section = TG_SECTION_NONE;
	    } else {		
		// Get information about a new task
		string name = tok;
			
		// Second token is task requirement (MIPS or KBs)
		tok = strtok(NULL, TOKENS);

		if (tok == NULL) {
		    cerr << "Expected computational requirement, line " << line_number << endl;
		    cleanUpAndExit(1);
		} // if
		    
		int req = atoi(tok);
		
		// Create a new task and add it to the list if all info was found
		Task *newTask = new Task(name,req);
		addTask(newTask);
	    }
	    
	    break;
	    
	case TG_SECTION_COMM:
	    // Leave the section if the end key is encountered
	    if(!strcmp(tok,"end")) {
		section = TG_SECTION_NONE;
	    } else {
	    // Get information about a new task
		// First token is the communication source
		string curSrc = tok;
	       
		// Ensure the source corresponds to a valid task
		Task *srcTask = getTask(curSrc);
		if(!srcTask) {
		  cerr << "Could not find a task object corresponding to the name " << curSrc << endl;
		  cleanUpAndExit(1);
		}
		
		// Second token is the destination
		tok = strtok(NULL, TOKENS);
		
		if (tok == NULL) {
		    cerr << "Expected destination task, line " << line_number << endl;
		    cleanUpAndExit(1);
		} // if
		
		string curDst = tok;

		// Ensure the destination corresponds to a valid task
		Task *dstTask = getTask(curDst);
		if(!dstTask) {
		  cerr << "Could not find a task object corresponding to the name " << curDst << endl;
		  cleanUpAndExit(1);
		}

		// Third token is the bandwidth requirement
		tok = strtok(NULL, TOKENS);
		if (tok == NULL) {
		    cerr << "Expected communication bandwidth, line " << line_number << endl;
		    cleanUpAndExit(1);
		}
		
		int curBandwidth = atoi(tok);
		
		// Now create the net
		TaskNet *curNet = new TaskNet(srcTask,dstTask,curBandwidth);
		taskGraph.push_back(curNet);
	    }
	    
	    break;
	default:
	    fprintf(stderr, "Undefined or invalid section type\n");
	    cleanUpAndExit(1);
	    break;
	} // switch

    } // while
    
    fclose(taskGraphFile);
}

int System::getNSamples() const { return n_samples; }

// calculate statistics upon request
float System::getMTTF() const { return moment1; }
float System::getVar() const { return moment2 - moment1*moment1; }

float System::getSVar() const { 
	float var = moment2 - moment1*moment1;
	return var/n_samples; 
}

float System::getConfidence() const {
	return 1.96*sqrt(getSVar());
}

// set statistics from cached values
void System::setMTTF(float mttf) { moment1 = mttf; }

void System::resetStatistics() {
	n_samples = 0;
	moment1 = 0;
	moment2 = 0;
}

int System::getMaxSamples() const { return m_samples; }
void System::setMaxSamples(int samples) { m_samples = samples; }

void System::writeOperatingScenario(int pos)
{
	// It probably isn't necessary to keep a list of the operating scenarios in the config
	// file since they are now being created on demand
	return;
}

void System::setInitialComponentTemps()
{
  int x;
  set<Component*,compareComponentIDs> baseOperatingScenario;

  // always floorplan, for cost reporting; if we care about temperature, prepare
  // power and temperature information for hotSpot
  if(!initialTempsFound) {
      initialTempsFound = true;

      // Use Parquet to create a baseline floorplan
      floorplan();

      // Create the operating scenario for the fully working system
      for(x = 0; x < (int)components.size(); x++) {
	  baseOperatingScenario.insert(components[x]);
      }
      operatingScenarios.push_back(baseOperatingScenario);

      // Build the task mapping for the fully working system
      createSingleTaskMapping(0);

      string operatingScenarioKey = buildOperatingScenarioKey(baseOperatingScenario);
      operatingScenarioLookup[operatingScenarioKey] = 0;
      
      if (tempUpdate || initTemps) {	  
	  // Run the block filling algorithm to fill the empty spaces in the
	  // floorplan with dead blocks for HotSpot
	  if(fill_blocks(this)) {
	      cout << "fillBlocks() failed...now exiting" << endl;
	      cleanUpAndExit(1);
	  }
	  
	  // create the system power trace and set system power values
	  create_single_power_trace(this, 0);

	  // sum the power for the baseline configuration
	  float power = 0;
	  for (int i=0; i<(int) components.size(); i++) {
	      float cPower = taskMappings[0]->getPower(i);
	      power += cPower;

#ifdef STATS
	      // store initial component power dissipation
	      statsPower[i] = cPower;
#endif
	  } 

	  //cout << "*** Baseline power: " << power << endl;
	  
	  // calibrate convection resistance and determine baseline temperatures	  
	  if (externalRConvec) {
	      //cout << "*** Using external r_convec " << r_convec << endl;
	      run_single_HotSpot_simulation(this, 0, (xsize >= ysize) ? xsize : ysize, r_convec);
	      readTempsFromFile(0);
	  } else {
	      calibrateRConvec(0);
	  }
      }

      // Start with component temperatures of 345K, if -i 0 is specified on the command line
      if(!initTemps) {
	  for(x = 0; x < (int)components.size(); x++) {
	      components[x]->setCurrentTemperature(345.0);
	  }
      }      
  } else if (tempUpdate) {
      // Read back the initial component temperatures from HotSpot
      updatePowerValues(0);    
      readTempsFromFile(0);
  }
  
  return;
}

void System::resolvePreclusions(Component *failedComponent) {
    // resolve preclusions
    if (failedComponent->getNPreclusions() > 0) {
	list<Component*> preclusions = failedComponent->getPreclusions();
	
	for (list<Component*>::iterator iter = preclusions.begin();
	     iter != preclusions.end();iter++) {
	    Component *precludedComponent = *iter;
	    
	    // mark precluded component as failed
	    precludedComponent->setFailed();
	    failedComponents.insert(precludedComponent);
	    
	    //cout << "XXX Component preclusion: " << precludedComponent->getName() << endl;
	} // for
    } // if
}

bool System::resolveSystemFailure() {
    // is the scenario an operating scenario?

    //cout << "*** Resolving system failure: " << endl;
    
    // Build the key for the operating scenario being searched for
    set<Component*,compareComponentIDs> curScenario;
    insert_iterator<set<Component*,compareComponentIDs> > diffBegin(curScenario,curScenario.begin());
    insert_iterator<set<Component*,compareComponentIDs> > diffEnd(curScenario,curScenario.begin());
    curScenario.clear();
    
    // Add all components that aren't in failedComponents to a new set
    diffEnd = set_difference(components.begin(),components.end(),
			     failedComponents.begin(),failedComponents.end(),
			     diffBegin,compareComponentIDs());
    
    string operatingScenarioKey = buildOperatingScenarioKey(curScenario);

    //cout << "-" << operatingScenarioKey << "-" << endl;
    
    map<string, int>::iterator opScenIter = operatingScenarioLookup.find(operatingScenarioKey);
    if (opScenIter != operatingScenarioLookup.end()) {
	// scenario is an operating scenario, not a failure scenario
	//cout << "***   An existing operating scenario: " << opScenIter->second << endl;
	return false;
    }
    
    //cout << "***   Not an existing operating scenario" << endl;
    
    // we haven't recorded this scenario as an operating scenario yet,
    // so check if it is a failure scenario
    for (vector<set<Component*,compareComponentIDs> >::iterator fsiter = failureScenarios.begin();
	 fsiter != failureScenarios.end();fsiter++) {
	set<Component*,compareComponentIDs> fs = *fsiter;
	
	// check if the failure scenario is a subset of the failed components;
	// if so, then the system has failed
	if (includes(failedComponents.begin(),failedComponents.end(),fs.begin(),fs.end(),compareComponentIDs())) {
	    //cout << "***   An existing failure scenario" << endl;
	    return true;
	}
    }

    // this scenario isn't a subset of an existing failure scenario,
    // so try to build a task mapping    
    TaskMapping *tm = new TaskMapping(this,curScenario);
    //tm->setTempsCalculated(false);
    
    // If no task mapping could be found, add the missing components
    // as a failure scenario
    if(!tm->getMappingFound()) {
	// no task mapping, so this is a failure scenario
#ifdef CACHE_FAILSCN
	set<Component*,compareComponentIDs> failureScenario;

#ifndef CACHE_FAILSCN_SORTED
	insert_iterator<set<Component*,compareComponentIDs> > intBegin(failureScenario,failureScenario.begin());
	insert_iterator<set<Component*,compareComponentIDs> > intEnd(failureScenario,failureScenario.begin());
	failureScenario.clear();

	// Add all components that are in failedComponents to a new set
	intEnd = set_intersection(components.begin(),components.end(),
				  failedComponents.begin(),failedComponents.end(),
				  intBegin,compareComponentIDs());

	//cout << "*** New failure scenario" << endl;
	//cout << "*** " << failureScenarios.size() << " -" << buildOperatingScenarioKey(failureScenario) << "-" << endl;
	failureScenarios.push_back(failureScenario);
#else
	int failedCount = 0;
	for (int i=0; i<(int) components.size(); i++) {
	    Component *c = components[i];

	    if (c->isFailed()) {
		failureScenario.insert(c);
		failedCount++;
	    } // if
	} // for

	// look for the first failure scenario with the same number of
	// failed components, and insert scenario before
	vector<set<Component*,compareComponentIDs> >::iterator iter = failureScenarios.begin();

	while (iter != failureScenarios.end()) {
	    set<Component*,compareComponentIDs> fs = *iter;

	    if ((int) fs.size() == failedCount)
		break;
	    
	    iter++;
	} // while

	//cout << "*** " << failureScenarios.size() << " New failure scenario: ";
	//cout << "-" << buildOperatingScenarioKey(failureScenario) << "-" << endl;
	failureScenarios.insert(iter, failureScenario);
#endif
	
	if(mcsVerbosity > 0) {
	    cout << "Scenario has been added to the list of failure scenarios:" << endl << "  ";
	    for(set<Component*,compareComponentIDs>::iterator iter = failureScenario.begin();
		iter != failureScenario.end(); iter++) {
		Component *c = *iter;
		cout << c->getName() << " ";
	    }

	    cout << endl;
	}
#endif
	
	delete tm;
	return true;
    } else {
	// we found a task mapping, so this is an operating scenario
#ifdef CACHE_OPSCN
	//cout << "*** " << operatingScenarios.size() << " New operating scenario" << endl;
	//cout << "*** " << operatingScenarios.size() << " -" << operatingScenarioKey << "-" << endl;

	/*
	cout << "*** -";
	for(set<Component*,compareComponentIDs>::iterator iter = failedComponents.begin();
	    iter != failedComponents.end(); iter++) {
	    Component *c = *iter;
	    cout << c->getName() << " ";
	}
	cout << "-" << endl;
	*/
	
	operatingScenarioLookup[operatingScenarioKey] = (int) operatingScenarios.size();
	operatingScenarios.push_back(curScenario);

	taskMappings.push_back(tm);
#endif
	
	//delete tm;
	return false;
    } // if-else
}
    
void System::updateStatistics(float data) {      
    moment1 = (moment1*n_samples + data)/(n_samples+1);
    
    if (isnan((double)moment1) || isinf((double)moment1)) {
	cout << "*** Error: statistics diverge @ sample " << n_samples << " - " << data << endl;
	cleanUpAndExit(1);
    }
    
    moment2 = (moment2*n_samples + data*data)/(n_samples+1);
}

void System::sample() {
  //cout << "Sample " << n_samples << endl;
	
  bool matchedOperatingScenario = false;
	
  // clear set of failed components
  failedComponents.erase(failedComponents.begin(),failedComponents.end());

  vector<Component*> failureTimes = components;
	
  if (measureYield) {
      // generate a sample
      for (vector<Component*>::iterator iter = failureTimes.begin();
	   iter != failureTimes.end();iter++) {
	  Component *c = *iter;

	  c->sampleYield();
      } // for
      
      // process failed components
      for (vector<Component*>::iterator iter = failureTimes.begin();
	   iter != failureTimes.end();iter++) {
	  Component *c = *iter;

	  if (c->isFailed()) {
#ifdef STATS
	      // update statistics
	      updateSTATS(c, false);
#endif
	      
	      failedComponents.insert(c);
	      resolvePreclusions(c);
	  } // if
      } // for
  
      // is the system still functional?
      bool failure = resolveSystemFailure();

      // update distribution moments
      updateStatistics((float) (!failure));
      n_samples++;
      
  } else {
      // generate a sample
      for (vector<Component*>::iterator iter = failureTimes.begin(); 
	   iter != failureTimes.end();iter++) {
	  Component *c = *iter;
	  c->sampleLifetime();
	  //printf("%s: %f %f\n",c->getName().c_str(),c->getFailureTime(),c->getInitCurrentDensity());
      } // for
      //printf("\n");
      
      // sort the vector by component failure time
      sort(failureTimes.begin(), failureTimes.end(), compareFailureTimes);
      
      // Remove all memories from the list of failure times
      for(vector<Component*>::iterator iter = failureTimes.begin();
	  iter != failureTimes.end(); iter++) {
	  Component *c = *iter;
	  
	  if(c->getGType() == MEM) {
	      failureTimes.erase(iter);
	      
	      // Move the iterator back one since an element has been
	      // removed from the list
	      iter--;
	  }
      }
		
      // walk the vector until failure occurs
      float time = 0;
      bool failure = false; 
      
      vector<Component*>::iterator fiter = failureTimes.begin();
      while (!failure && fiter != failureTimes.end()) {
	  Component *failedComponent = *fiter; 
	  
	  // skip component if it has already failed due to preclusion
	  if (!failedComponent->isFailed()) {
	      //cout << "XXX Component failure: " << failedComponent->getName() << endl << endl;
	      
#ifdef STATS	
	      // updated statistics
	      if (failedComponents.empty())
		  updateSTATS(failedComponent, true);
	      else
		  updateSTATS(failedComponent, false);
#endif
	      
	      // mark component as failed (also resolves preclusions)
	      failedComponent->setFailed();
	      failedComponents.insert(failedComponent);

	      // resolve preclusions
	      resolvePreclusions(failedComponent);
	      
	      // advance time to the failure of the current component
	      if(tempUpdate) {
		  time += failedComponent->getFailureTime();
	      }
	      else {
		  time = failedComponent->getFailureTime();
	      }
	      //printf("Time is now %f\n",time);

	      failure = resolveSystemFailure();
	      
	      // update distribution moments if the system has failed
	      if (failure) {
		  //cout << "*** System is failed" << endl;
		  updateStatistics(time);
		  n_samples++;
		  //cout << "!!! System failure" << endl << endl;
	      }
	      
	      // If the system has not failed, update the temperatures (if specified in the command line)
	      else if(tempUpdate){
		  //cout << "*** System is not failed, updating temperatures" << endl;
		  
		  // Update wear for all components
		  for (vector<Component *>::iterator iter = fiter; iter != failureTimes.end(); iter++) {
		      Component *c = *iter;
		      c->updateWear(time);
		  }
		  
		  // Find the operating scenario that the system is in
		  int x;
		  FILE *tempFile;
		  //char *tempFileName;
		  char line[20];
		  string componentName;
		  float componentTemp;
				
		  /*
		  // Print the failed components
		  cout << "Failed components in sample " << n_samples << ":" << endl;
		  for(set<Component*,compareComponentIDs>::iterator iter = failedComponents.begin();
		  iter != failedComponents.end(); iter++) {
		  Component *c = *iter;
		  cout << c->getName() << endl;
		  }
		  */
	
		  // Build the key for the operating scenario being searched for
		  set<Component*,compareComponentIDs> diffResult;
		  insert_iterator<set<Component*,compareComponentIDs> > diffBegin(diffResult,diffResult.begin());
		  insert_iterator<set<Component*,compareComponentIDs> > diffEnd(diffResult,diffResult.begin());
		  diffResult.clear();
		  
		  // Add all componets that aren't in failedComponents to a new set
		  diffEnd = set_difference(components.begin(),components.end(),
					   failedComponents.begin(),failedComponents.end(),
					   diffBegin,compareComponentIDs());
		  
		  string operatingScenarioKey = buildOperatingScenarioKey(diffResult);
		  
		  // Loop over all operating scenarios
		  matchedOperatingScenario = false;
		  int operatingScenarioPos = -1;
		  operatingScenarioPos = operatingScenarioLookup[operatingScenarioKey];

		  //cout << "***   Operating scenario " << operatingScenarioPos << " (" << taskMappings[operatingScenarioPos]->getTempsCalculated() << ")" << endl;
		  
		  if(operatingScenarioPos != -1) {
		      x = operatingScenarioPos;
		      matchedOperatingScenario = true;
		      if(!taskMappings[operatingScenarioPos]->getTempsCalculated()) {
			  // Create a power trace for the new task mapping
			  create_single_power_trace(this,operatingScenarioPos);
			  
			  // Run HotSpot for the new operating scenario
			  run_single_HotSpot_simulation(this,operatingScenarioPos,(xsize >= ysize) ? xsize : ysize, r_convec);
			  
			  taskMappings[operatingScenarioPos]->setTempsCalculated(true);
		      }
		  }
		  
		  // If the current scenario was not previously found to be an operating scenario, set it up now
		  if(!matchedOperatingScenario) {
		      printf("This should never happen\n");
		      cleanUpAndExit(1);
		      set<Component*,compareComponentIDs> newOperatingScenario;
		      insert_iterator<set<Component*,compareComponentIDs> > diffBegin(newOperatingScenario,newOperatingScenario.begin());
		      insert_iterator<set<Component*,compareComponentIDs> > diffEnd(newOperatingScenario,newOperatingScenario.begin());
		      newOperatingScenario.clear();
		      
		      // Add all componets that aren't in failedComponents to a new set
		      diffEnd = set_difference(components.begin(),components.end(),
					       failedComponents.begin(),failedComponents.end(),
					       diffBegin,compareComponentIDs());
		      
		      // Add the new operating scenario to the list of operating scenarios
		      operatingScenarios.push_back(newOperatingScenario);
		      
		      /*
			printf("Components:\n");
			for(int a = 0; a < (int)components.size(); a++) {
			Component *b = components[a];
			printf("%s\n",b->getName().c_str());
			}
			printf("\n");
			
			printf("New operating scenario:\n");
			for(set<Component*,compareComponentIDs>::iterator a = newOperatingScenario.begin(); a != newOperatingScenario.end(); a++) {
			Component *b = *a;
			printf("%s\n",b->getName().c_str());
			}
			printf("\n");
			printf("added as scenario %d\n",operatingScenarios.size() - 1);
		      */
		      
		      // Write the new operating scenario back to the config file
		      //writeOperatingScenario(operatingScenarios.size() - 1);
		      
		      // Create a task mapping for the new operating scenario
		      createSingleTaskMapping(operatingScenarios.size() - 1);
		      
		      // Create a power trace for the new task mapping
		      create_single_power_trace(this,operatingScenarios.size() - 1);
		      
		      // Run HotSpot for the new operating scenario
		      run_single_HotSpot_simulation(this,operatingScenarios.size() - 1,(xsize >= ysize) ? xsize : ysize, r_convec);
		  }
		  
		  // Update the current power values for each component
		  //updatePowerValues(operatingScenarios.size() - 1);
		  updatePowerValues(operatingScenarioPos);
		  
		  // Open the HotSpot temperature file corresponding to the current operating scenario
		  string tempFileName;
		  stringstream ss;
		  ss << getWorkingDirectory() << HSPATH << operatingScenarioPos << ".temp";
		  tempFileName = ss.str();
		  
		  //tempFileName = (char *)malloc((strlen(configFileName.c_str()) + 12) * sizeof(char));
		  //sprintf(tempFileName,"%s.%05d.temp",configFileName.c_str(),x);
		  
		  tempFile = fopen(tempFileName.c_str(),"r");
		  
		  // Ensure the temperature file opened correctly
		  if(!tempFile) {
		      cerr << "Couldn't open temperature file " << tempFileName << " for reading...giving up on sample" << endl;
		      failure = true;
		  }
		  
		  // Step through each line in the temperature file
		  while(fgets(line,sizeof(line),tempFile)) {
		      componentName = strtok(line,"\t\n");
		      //componentTemp = atoff(strtok(NULL,"\t\n"));
		      componentTemp = atof(strtok(NULL,"\t\n"));
		      
		      // Step through all of the components in the system
		      for(x = 0; x < (int)components.size(); x++) {
			  
			  // Set the current temperature of the component to what is found in the temperature file
			  if(componentName == components[x]->getName()) {
			      components[x]->setCurrentTemperature(componentTemp);
			  }
		      }
		  }
		  
		  // Update failure times for all components
		  for (vector<Component *>::iterator iter = fiter; iter != failureTimes.end(); iter++) {
		      Component *c = *iter;
		      c->updateFailureTime();
		  }
				
		  fclose(tempFile);
		  //free(tempFileName);
	      }
	      
	      // Resort the vector of failure times
	      sort(fiter, failureTimes.end(), compareFailureTimes);
	  } // if
	  
	  // advance iterator
	  fiter++;
      } // while
      
      if (!failure)
	  cerr << "*** Error!  The system never failed!" << endl;
  } // sample lifetime
} // sample

bool System::samplingRun() {
    float mttf, area, wl;

    // build string representation for hashing
    string key = buildKey();
	
    // lookup MTTF
    mttf = lookupMTTF(key);
    
    if (mttf == -1) {
	//cout << "&&& Inserting ..." << endl;

	// reset the system to prepare to generate MTTF statistics
	reset();
	
	// mttf wasn't found, determine it
	for (int i=0; i<getMaxSamples(); i++) {
	    setInitialComponentTemps();
	    sample();
	}

	//cout << "&&& ... done sampling ..." << endl;

	// if we're working with yield, apply Y0
	if (measureYield)
	    setMTTF(getMTTF()*Y0);
	
	// get statistics	
	mttf = getMTTF();
		
	// hash the mttf
	setMTTF(key, mttf);
	//cout << "=== Inserted -" << key << "- : " << mttf << endl;

	// hash the area and wire length
	area = getArea();
	wl = getWL();
	setAreaWL(key, area, wl);
	//cout << "+++ Inserted -" << key << "- : " << area << " " << wl << endl;

	cout << "+++ ";
	printComponentCapacities();
	cout << area << " " << wl << " " << mttf << endl;
	
#ifdef STATS
	printSTATS();
#endif
	
	return false;
    } else {
	// mttf was found, update system variables
	//cout << "=== Found    -" << key << "- : " << mttf << endl; 
	
	pair<float, float> areawl = lookupAreaWL(key);

	if (areawl.first == 0 && areawl.second  == 0) {
	    cerr << "*** Redundancy allocation found in RAtoMTTF but not RAtoAreaWL" << endl;
	    cleanUpAndExit(1);
	}
	
	// update system cost
	setArea(areawl.first);
	setWL(areawl.second);
	
	//cout << "+++ Found    -" << key << "- : " << areawl.first << " " << areawl.second << endl;
	    	    
	// update system statistics
	setMTTF(mttf);
	
	return true;
    } // if/else
} // samplingRun

void System::initialize() {
    buildAllScenariosOrdered();
} // initialize

void System::reset() {
    // clear operating scenarios and dead block list
    clearOperatingScenarios();
    clearTaskMappings();
    clearEmptyBlocks();
    
    // build failure scenarios
    clearFailureScenarios();
    //buildFailureScenarios();

    // reset initial temps found to force floorplanning
    resetInitialTempsFound();
    
    // reset statistics
    resetStatistics();
} // reset

void System::writeBlocks(const string basename) {
    string filename = basename;
    filename.append(".blocks");
    
    fstream file(filename.c_str(), fstream::out);
    	
    file << "UCSC blocks  1.0" << endl << endl << endl;
    file << "NumSoftRectangularBlocks : \t" << components.size() << endl;
    file << "NumHardRectilinearBlocks : \t0"<< endl;
    file << "NumTerminals : \t0" << endl << endl;
    
    vector<Component*>::iterator iter = components.begin();
    while (iter != components.end()) {
	Component *c = *iter;
	file << c->getName() << " softrectangular " << c->getArea() << "\t"
	     << c->getHeight() / c->getWidth() << "\t"
	     << c->getHeight() / c->getWidth() << endl;
	
	iter++;
    } // while
    
    file.close();
} // writeBlocks

void System::writeNets(const string basename) {
    string filename = basename;
    filename.append(".nets");
    
    fstream file(filename.c_str(), fstream::out);

    file << "UCLA nets  1.0" << endl << endl << endl;

    // we assume that all nets have two pins
    file << "NumNets : " << netlist.size() << endl;
    file << "NumPins : " << netlist.size()*2 << endl << endl;

    vector<ComponentNet*>::iterator iter = netlist.begin();
    while (iter != netlist.end()) {
	ComponentNet *n = *iter;
	string srcName(n->getSrc()->getName());
	string dstName(n->getDst()->getName());

	// name the net
	string netName(srcName);
	netName.append("-");
	netName.append(dstName);

	file << "NetDegree : 2\t" << netName << endl;
	
	// directionality has no impact on floorplanning, just list the components
	file << srcName << " B" << endl;
	file << dstName << " B" << endl;
	
	iter++;
    } // while

    file.close();
} // writeNets

void System::floorplan() {
    string floorplanFileName;
    stringstream ss;
    ss << getWorkingDirectory() << FPPATH << FP_BASENAME;
    floorplanFileName = ss.str();
    
    // prepare ParquetFP input files
    writeNets(floorplanFileName);
    writeBlocks(floorplanFileName);

    int argc;
    char **argv;
    
    if (fpAreaMin) {
	// floorplan to minimize area
	
	// construct the command line for ParquetFP
	// count up the arguments (including "1" for the program name ...)
	argc = 9;
	argv = new char*[argc];

	for (int i=0; i<argc; i++) {
	    argv[i] = new char[128];
	} // for

	sprintf(argv[1], "-n");
	sprintf(argv[2], "%d", fpIter);
	sprintf(argv[3], "-f");
	sprintf(argv[4], "%s", floorplanFileName.c_str());
	sprintf(argv[5], "-save");
	sprintf(argv[6], "%s", floorplanFileName.c_str());
	sprintf(argv[7], "-timeInit");
	sprintf(argv[8], "%f", FP_TI);
    } else {
	// floorplan to minimize area and wire length
	
	// construct the command line for ParquetFP
	// count up the arguments (including "1" for the program name ...)
	argc = 14;
	argv = new char*[argc];

	for (int i=0; i<argc; i++) {
	    argv[i] = new char[128];
	} // for
    
	sprintf(argv[1], "-n");
	sprintf(argv[2], "%d", fpIter);
	sprintf(argv[3], "-minWL");
	sprintf(argv[4], "-wireWeight");
	sprintf(argv[5], "%f", fpWireWeight);
	sprintf(argv[6], "-areaWeight");
	sprintf(argv[7], "%f", fpAreaWeight);
	sprintf(argv[8], "-f");
	sprintf(argv[9], "%s", floorplanFileName.c_str());
	sprintf(argv[10], "-save");
	sprintf(argv[11], "%s", floorplanFileName.c_str());
	sprintf(argv[12], "-timeInit");
	sprintf(argv[13], "%f", FP_TI);
    }
    
    floorplanFileName.append(".pl");
    setFloorplanFileName(floorplanFileName);
    
    // create a new ParquetFP object ...
    Parquet engine(argc, argv, mcsVerbosity);
    engine.go();

    xsize = engine.getBestXSize();
    ysize = engine.getBestYSize();
    area = engine.getBestArea();
    wl = engine.getBestWL();

    return;
} // floorplan

void System::readTempsFromFile(int operatingScenarioIndex)
{
  int x;
  //char *tempFileName;
  FILE *tempFile;
  char line[20];
  string componentName;
  float componentTemp;

  // Open the HotSpot temperature file corresponding to the current operating scenario
  string tempFileName;
  stringstream ss;
  ss << getWorkingDirectory() << HSPATH << operatingScenarioIndex << ".temp";
  tempFileName = ss.str();

  //tempFileName = (char *)malloc((strlen(configFileName.c_str()) + 12) * sizeof(char));
  //sprintf(tempFileName,"%s.%05d.temp",configFileName.c_str(),operatingScenarioIndex);
  tempFile = fopen(tempFileName.c_str(),"r");
  
  // Ensure the temperature file opened correctly
  if(!tempFile) {
    cerr << "Couldn't open temperature file " << tempFileName << " for reading" << endl;
    cleanUpAndExit(1);
  }
  
  // Step through each line in the temperature file
  while(fgets(line,sizeof(line),tempFile)) {
    componentName = strtok(line,"\t\n");
    componentTemp = atof(strtok(NULL,"\t\n"));
    
    // Step through all of the components in the system
    for(x = 0; x < (int)components.size(); x++) {
      
      // Set the current temperature of the component to what is found in the temperature file
      if(componentName == components[x]->getName()) {
	components[x]->setCurrentTemperature(componentTemp);

#ifdef STATS
	// store initial component temperature
	statsTemp[x] = componentTemp;
#endif
      }
    }
  }

  // Should this be here when resetting the temps for the components?
  // If not, need a flag to take it out if this function will also be used in ::sample
  /*
  // Update failure times for all components
  for (vector<Component *>::iterator iter = fiter; iter != failureTimes.end(); iter++) {
    Component *c = *iter;
    c->updateFailureTime();
  }
  */
  
  fclose(tempFile);
  //free(tempFileName);
 
  return;
}

void System::updatePowerValues(int operatingScenarioIndex)
{
    TaskMapping *curTaskMapping = taskMappings[operatingScenarioIndex];
    
    // loop over all components in the system and get their power from the task mapping
    for (int y=0; y<(int) components.size(); y++) {
	Component *c = components[y];
	c->setCurPower(curTaskMapping->getPower(y));
    } // for
}

string System::buildOperatingScenarioKey(set<Component*,compareComponentIDs> scenario)
{
  string operatingScenarioKey = "";
 
  for(set<Component*,compareComponentIDs>::iterator iter = scenario.begin();
      iter != scenario.end(); iter++) {
    Component *c = *iter;
    operatingScenarioKey.append(c->getName());
  }

  return operatingScenarioKey;
}

void System::printPermutation(vector<componentType> permutation) {
    for (int c=0; c<(int) permutation.size(); c++) {
	cout << componentTypeToString(permutation[c]) << " ";
    } // for
} // printPermutation

vector<componentType> System::buildProcessorPermutation() {
    vector<Component*> processors = getProcessors();
    vector<componentType> permutation;
    
    vector<Component*>::iterator iter = processors.begin();
    while (iter != processors.end()) {
	Component *c = *iter;
	permutation.push_back(c->getCType());
	
	iter++;
    } // while

    return permutation;
} // buildProcessorPermutation

void System::buildProcessorPermutations() {
    // clear permutations list
    processorPermutations.clear();

    // save current permutation
    vector<Component*> processors = getProcessors();
    vector<componentType> curPermutation = buildProcessorPermutation();

    // prepare initial permutation
    clearRedundancies(processors);
    
    // calculate available MIPS in baseline permutation
    int sum = 0;
    for (int i = 0; i < (int) processors.size(); i++) {
	sum += processors[i]->getInitialCapacity();
    } // for

    initialProcessorCapacity = sum;
    
    // insert initial permutation
    processorPermutations.push_back(buildProcessorPermutation());

    // find all other processor permutations
    bool done = false;
    while(!done) {

	// try to allocate MIPS
	int idx = 0;
	bool allocated = false;
	while (!allocated) {
	    // can we allocate to the current processor?
	    if (processors[idx]->getAType() != NONE) {
		// allocate more MIPS
		processors[idx]->setComponentType(processors[idx]->getAType());
		allocated = true;
	    } else {
		// can't allocate to the current processor; is it the last?
		if (idx == (int) processors.size() - 1) {
		    // we're done!
		    allocated = true;
		    done = true;
		} else {
		    // fully deallocate this processor
		    processors[idx]->setComponentType(processors[idx]->getMType());

		    // try to allocate again with the next processor
		    idx++;
		} // if/else
	    } // if/else
	} // while (!allocated)
	
	// if we're not done, then add this permutation to the list
	if (!done)
	    processorPermutations.push_back(buildProcessorPermutation());
    } // while (!done)

    // restore original permutation
    applyProcessorPermutation(curPermutation);
} // buildProcessorPermutations

vector<vector<componentType> > System::getProcessorPermutations(int mips) {
    vector<vector<componentType> > selectedPermutations;

    // select those permutations that match the specified amount of total redundancy
    for (int i = 0; i < (int) processorPermutations.size(); i++) {
	vector<componentType> permutation = processorPermutations[i];

	// calculate MIPS available in current permutation
	int pSum = 0;
	for (int proc = 0; proc < (int) permutation.size(); proc++) {
	    pSum += componentLibrary[ permutation[proc] ].capacity;	    
	} // for

	// if the difference between pSum and sum is the same as mips, keep this permutation
	if (pSum - initialProcessorCapacity == mips)
	    selectedPermutations.push_back(permutation);
    } // for

    return selectedPermutations;
} // getProcessorPermutations

vector<vector<componentType> > System::findIncrementalProcessorPermutations() {
    vector<vector<componentType> > incrementalPermutations;
    
    // search for permutations where one processor has increased
    // capacity relative to the current permutation
    vector<componentType> curPermutation = buildProcessorPermutation();

    //cout << "*** Current permutation: ";
    //printPermutation(curPermutation);
    //cout << endl;
    
    for (int i=0; i<(int) processorPermutations.size(); i++) {
	vector<componentType> incPermutation = processorPermutations[i];

	int more = 0;
	int less = 0;
	for (int j=0; j<(int) curPermutation.size(); j++) {
	    if (curPermutation[j] + 1 == incPermutation[j])
		more++;
	    else if (curPermutation[j] - 1 == incPermutation[j])
		less++;
	} // for

	if (more == 1 && less == 0) {
	    incrementalPermutations.push_back(incPermutation);
	    //cout << "***   Found incremental permutation: ";
	    //printPermutation(incPermutation);
	    //cout << endl;
	} // if
    } // for
    
    return incrementalPermutations;
} // findIncrementalProcessorPermutations

vector<vector<componentType> > System::findIncrementalProcessorPermutations(vector<vector<componentType> > permutations) {
    vector<vector<componentType> > incrementalPermutations;
    
    // search for permutations where one processor has increased
    // capacity relative to the current permutation
    vector<componentType> curPermutation = buildProcessorPermutation();

    //cout << "*** Current permutation: ";
    //printPermutation(curPermutation);
    //cout << endl;
    
    for (int i=0; i<(int) permutations.size(); i++) {
	vector<componentType> incPermutation = permutations[i];

	int more = 0;
	int less = 0;
	for (int j=0; j<(int) curPermutation.size(); j++) {
	    if (curPermutation[j] < incPermutation[j])
		more++;
	    else if (curPermutation[j] > incPermutation[j])
		less++;
	} // for

	if (more > 0 && less == 0) {
	    incrementalPermutations.push_back(incPermutation);
	    //cout << "***   Found incremental permutation: ";
	    //printPermutation(incPermutation);
	    //cout << endl;
	} // if
    } // for
    
    return incrementalPermutations;
} // findIncrementalProcessorPermutations

void System::applyProcessorPermutation(vector<componentType> permutation) {
    // get processors
    vector<Component*> processors = getProcessors();

    //cout << "*** Applying initial permutation: ";
    //printPermutation(permutation);
    //cout << endl;
    
    // apply permutation
    vector<Component*>::iterator citer = processors.begin();
    vector<componentType>::iterator titer = permutation.begin();
    while (citer != processors.end()) {
	Component *c = *citer;
	c->setComponentType(*titer);
	
	citer++;
	titer++;
    } // while
} // applyProcessorPermutation

vector<componentType> System::buildMemoryPermutation() {
    vector<Component*> memories = getMemories();
    vector<componentType> permutation;

    vector<Component*>::iterator iter = memories.begin();
    while (iter != memories.end()) {
	Component *c = *iter;
	permutation.push_back(c->getCType());
	
	iter++;
    } // while

    return permutation;
} // buildMemoryPermutation

void System::buildMemoryPermutations() {
    // clear permutations list
    memoryPermutations.clear();
    
    // save current permutation and clear redundancy
    vector<Component*> memories = getMemories();
    vector<componentType> curPermutation = buildMemoryPermutation();

    // prepare initial permutation
    clearRedundancies(memories);
    
    // calculate available memory in baseline permutation
    int sum = 0;
    for (int i = 0; i < (int)memories.size(); i++) {
	sum += memories[i]->getInitialCapacity();
    } // for

    initialMemoryCapacity = sum;
    
    // insert initial permutation
    memoryPermutations.push_back(buildMemoryPermutation());

    // find all other memory permutations
    bool done = false;
    while(!done) {
	
	// try to allocate memory
	int idx = 0;
	bool allocated = false;
	while (!allocated) {
	    
	    // can we allocate to the current memory?
	    if (memories[idx]->getAType() != NONE && memories[idx]->getAType() <= maxMemoryType) {
		// allocate more memory
		memories[idx]->setComponentType(memories[idx]->getAType());
		allocated = true;
	    } else {
		// can't allocate to the current memory; is it the last?
		if (idx == (int)memories.size() - 1) {
		    // we're done!
		    allocated = true;
		    done = true;
		} else {
		    // fully deallocate this memory
		    memories[idx]->setComponentType(memories[idx]->getMType());

		    // try to allocate again with the next memory
		    idx++;
		} // if/else
	    } // if/else
	} // while (!allocated)
	
	// if we're not done, then add this permutation to the list
	if (!done)
	    memoryPermutations.push_back(buildMemoryPermutation());
    } // while (!done)    

    // restore original permutation
    applyMemoryPermutation(curPermutation);
} // buildMemoryPermutations

vector<vector<componentType> > System::getMemoryPermutations(int redundantMemory) {
    vector<vector<componentType> > selectedPermutations;
    
    // select those permutations that match the specified amount of total redundancy
    for (int i = 0; i < (int) memoryPermutations.size(); i++) {
	vector<componentType> permutation = memoryPermutations[i];
	
	//cout << "*** Current permutation (" << i << "/" << memoryPermutations.size() << "): " << endl;
	//printPermutation(permutation);
	//cout << endl;
	
	// calculate memory available in current permutation
	int pSum = 0;
	for (int mem = 0; mem < (int) permutation.size(); mem++) {
	    pSum += componentLibrary[ permutation[mem] ].capacity;
	} // for
	
	// if the difference between pSum and sum is the same as rMemory, keep this permutation
	if (pSum - initialMemoryCapacity == redundantMemory)
	    selectedPermutations.push_back(permutation);
    } // for
    
    return selectedPermutations;
} // gettMemoryPermutations

vector<vector<componentType> > System::findIncrementalMemoryPermutations(vector<vector<componentType> > permutations) {
    vector<vector<componentType> > incrementalPermutations;
    
    // search for permutations where one processor has increased
    // capacity relative to the current permutation
    vector<componentType> curPermutation = buildMemoryPermutation();

    //cout << "*** Current permutation: ";
    //printPermutation(curPermutation);
    //cout << endl;
    
    for (int i=0; i<(int) permutations.size(); i++) {
	vector<componentType> incPermutation = permutations[i];

	int more = 0;
	int less = 0;
	for (int j=0; j<(int) curPermutation.size(); j++) {
	    if (curPermutation[j] < incPermutation[j]) {
		//cout << "More: " << curPermutation[j] << "/" << incPermutation[j] << endl;
		more++;
	    } else if (curPermutation[j] > incPermutation[j]) {
		//cout << "Less: " << curPermutation[j] << "/" << incPermutation[j] << endl;
		less++;
	    } // if-else
	} // for

	if (more > 0 && less == 0) {
	    incrementalPermutations.push_back(incPermutation);
	    //cout << "***   Found incremental permutation: ";
	    //printPermutation(incPermutation);
	    //cout << endl;
	} // if
	else {
	    //cout << "***   Permutation not incremental: ";
	    //printPermutation(incPermutation);
	    //cout << endl;
	}
	    
    } // for
    
    return incrementalPermutations;
} // findIncrementalMemoryPermutations

void System::applyMemoryPermutation(vector<componentType> permutation) {
    // get memories
    vector<Component*> memories = getMemories();

    //cout << "*** Applying initial permutation: ";
    //printPermutation(permutation);
    //cout << endl;
    
    // apply permutation
    vector<Component*>::iterator miter = memories.begin();
    vector<componentType>::iterator piter = permutation.begin();
    while (miter != memories.end()) {
	Component *c = *miter;
	c->setComponentType(*piter);
	
	piter++;
	miter++;
    } // while
} // applyMemoryPermutation

int System::permutationCapacity(vector<componentType> permutation) {
    int sum = 0;
    for (int c=0;c<(int) permutation.size(); c++)
	sum += componentTypeToCapacity(permutation[c]);

    return sum;
}

int System::getProcessorAllocationLevel() {
    // sanity check
    if (initialProcessorCapacity == 0) {
	cerr << "*** Error: attempting to get processor allocation level when permutations have not yet been built" << endl;
	cleanUpAndExit(1);
    } // if

    vector<componentType> permutation = buildProcessorPermutation();
    int capacity = permutationCapacity(permutation);
    
    return capacity - initialProcessorCapacity;
} // getProcessorAllocationLevels

vector<int> System::getProcessorAllocationLevels() {
    vector<int> allocationLevels;

    // get the capacity of the default permutation
    int base = permutationCapacity(processorPermutations[0]);
    // insert a zero in the vector of allocation levels
    allocationLevels.push_back(0);
    
    for (int i=1; i<(int) processorPermutations.size(); i++) {
	// get permutation capacity
	int sum = permutationCapacity(processorPermutations[i]);
	int allocation = sum - base;

	// if we don't find the value, insert it
	if (find(allocationLevels.begin(), allocationLevels.end(), allocation) == allocationLevels.end())
	    allocationLevels.push_back(allocation);
    } // for

    // sort the vector
    sort(allocationLevels.begin(), allocationLevels.end());
    
    return allocationLevels;
} // getProcessorAllocationLevels

int System::getMemoryAllocationLevel() {
    // sanity check
    if (initialMemoryCapacity == 0) {
	cerr << "*** Error: attempting to get memory allocation level when permutations have not yet been built" << endl;
	cleanUpAndExit(1);
    } // if

    vector<componentType> permutation = buildMemoryPermutation();
    int capacity = permutationCapacity(permutation);
    
    return capacity - initialMemoryCapacity;
} // getMemoryAllocationLevel

vector<int> System::getMemoryAllocationLevels() {
    vector<int> allocationLevels;

    // get the capacity of the default permutation
    int base = permutationCapacity(memoryPermutations[0]);
    // insert a zero in the vector of allocation levels
    allocationLevels.push_back(0);
    
    for (int i=1; i<(int) memoryPermutations.size(); i++) {
	// get permutation capacity
	int sum = permutationCapacity(memoryPermutations[i]);
	int allocation = sum - base;

	// if we don't find the value, insert it
	if (find(allocationLevels.begin(), allocationLevels.end(), allocation) == allocationLevels.end())
	    allocationLevels.push_back(allocation);
    } // for

    // sort the vector
    sort(allocationLevels.begin(), allocationLevels.end());
    
    return allocationLevels;
} // getMemoryAllocationLevels

void System::storeDatabase()
{
  FILE *db;

  int numProcessors = getProcessors().size();
  int numMemories = getMemories().size();
  
  char curLine[200];
  int lineSize = 200;
  const char delims[2] = " ";
  char *curTok;

  int x, curLineNum = 0;

  // Stop if no database file name was specified
  if(databaseFileName.empty()) {
    return;
  }

  // Open the database file and make sure that it is not NULL
  db = fopen(databaseFileName.c_str(),"r");
  if(!db) {
    cerr << "Couldn't open database file <" << databaseFileName << "> for reading" << endl;
    cleanUpAndExit(1);
  }

  // Read all of the data lines in the file
  while(fgets(curLine,lineSize,db)) {
    string curKey;
    float curArea, curWirelength, curMTTF;
    pair<float,float> curAreaWL;

    curKey.clear();
    
    // Read the first token in the line (first processor redundancy value)
    curTok = strtok(curLine,delims);
    if(!curTok) {
      cerr << "Improperly formatted database file...couldn't read value for processor 0 at line " << curLineNum << endl;
      cleanUpAndExit(1);
    }
    
    curKey.append(curTok);
    // Add a space
    curKey.append(" ");

    // Read the remaining processor and memory tokens
    for(x = 0; x < numProcessors + numMemories - 1; x++) {
      curTok = strtok(NULL,delims);
      if(!curTok) {
	cerr << "Improperly formatted database file...couldn't read value for memory " << x << " at line " << curLineNum << endl;
	cleanUpAndExit(1);
      }
      
      curKey.append(curTok);
      // add a space
      curKey.append(" ");
    }

    //cout << "Generated key <" << curKey << ">" << endl;

    // Read the area value
    curTok = strtok(NULL,delims);
    if(!curTok) {
      cerr << "Improperly formatted database file...couldn't read the area value at line " << curLineNum << endl;
      cleanUpAndExit(1);
    }
    curArea = atof(curTok);
    
    // Read the wirelength value
    curTok = strtok(NULL,delims);
    if(!curTok) {
      cerr << "Improperly formatted database file...couldn't read the wirelength value at line " << curLineNum << endl;
      cleanUpAndExit(1);
    }
    curWirelength = atof(curTok);

    // Read the MTTF value
    curTok = strtok(NULL,delims);
    if(!curTok) {
      cerr << "Improperly formatted database file...couldn't read the MTTF value at line " << curLineNum << endl;
      cleanUpAndExit(1);
    }
    curMTTF = atof(curTok);

    // Insert the MTTF into the cache
    setMTTF(curKey, curMTTF);
    
    // Insert the <area,wirelength> pair into the cache
    setAreaWL(curKey, curArea, curWirelength);

    curLineNum++;
  }
  
  //cout << "Added " << curLineNum << " entries to the database" << endl;

  return;
}

string System::buildKey()
{
    stringstream ss;
    
    vector<Component*> procs = getProcessors();
    vector<Component*> mems = getMemories();
    
    // append processor capacity
    for(int x = 0; x < (int) procs.size(); x++)
	ss << procs[x]->getInitialCapacity() << " ";
    
    // append memory capacity
    for(int x = 0; x < (int) mems.size(); x++)
	ss << mems[x]->getInitialCapacity() << " ";
    
    return ss.str();
}

bool System::lookupExpl(string key) {
    return RAExpl[key];
} 

float System::lookupMTTF(string key) {
    // find an iterator pointing to the entry for key
    map<string, float>::iterator mttfIter = RAtoMTTF.find(key);
    // if it doesn't point to the end, return the value, else return -1
    if (mttfIter != RAtoMTTF.end())
	return mttfIter->second;
    else
	return -1;
}

pair<float, float> System::lookupAreaWL(string key) {
    // find an iterator pointing to the entry for key
    map<string, pair<float, float> >::iterator areawlIter = RAtoAreaWL.find(key);
    // if it doesn't point to the end, return the value, else return null
    if (areawlIter != RAtoAreaWL.end())
	return areawlIter->second;
    else {
	pair<float, float> empty;
	empty.first = 0;
	empty.second = 0;
	return empty;
    }
}

void System::setExpl(string key) {
    RAExpl[key] = true;
} 

void System::setMTTF(string key, float val) {
    RAtoMTTF[key] = val;
}

void System::setAreaWL(string key, float area, float wl) {
    pair<float, float> areawl;
    areawl.first = area;
    areawl.second = wl;
    RAtoAreaWL[key] = areawl;
}

void System::setDefectDensity(float defectDensityProc, float defectDensityMem) {
    for (int i=0; i<(int) components.size(); i++) {
	ManufacturingDefect *manDe = components[i]->getManufacturingDefect();

	if (components[i]->getGType() == PROC || components[i]->getGType() == SW)
	    manDe->setDefectDensity(defectDensityProc);
	else if (components[i]->getGType() == MEM)
	    manDe->setDefectDensity(defectDensityMem);
    } // for
} 

#ifdef STATS
void System::initializeSTATS() {
    statsFirstFailure = 0;
    statsNFailures = 0;
    
    int n_components = (int) components.size();
    
    // re-size vectors based on the number of components
    statsPower.resize(n_components);
    statsTemp.resize(n_components);

    statsFirstFailed.resize(n_components);
    statsFirstFailedMech.resize(n_components);
    
    statsFailed.resize(n_components);
    statsFailedMech.resize(n_components);

    // re-size vectors based on the number of failure mechanisms
    for (int i=0; i<n_components; i++) {
	statsFirstFailedMech[i].resize(NFM);
	statsFailedMech[i].resize(NFM);
    } // for
} // initializeSTATS

void System::updateSTATS(Component *failed, bool first) {
    int id = failed->getID();
    float cfTime = failed->getFailureTime();

    if (first) {
	statsFirstFailed[id]++;
	statsFirstFailure += cfTime;
    } // if
    
    statsNFailures++;
    statsFailed[id]++;

    // determine which failure mechanism caused component failure
    list<FailureMechanism*> fmechs = failed->getFailureMechanisms();
    int idx = -1;

    for (list<FailureMechanism*>::iterator iter = fmechs.begin();
	 iter != fmechs.end(); iter++) {

	FailureMechanism *fmech = *iter;
	float fmfTime = fmech->getFailureTime();

	if (fmfTime == cfTime) {
	    // determine failure mechanism index
	    string fmName = fmech->getName();

	    if (fmName.compare("EM") == 0)
		idx = EMFM;
	    else if (fmName.compare("TDDB") == 0)
		idx = TDDBFM;
	    else if (fmName.compare("TC") == 0)
		idx = TCFM;
	} // if
    } // for

    if (idx == -1) {
	cerr << "*** Error: failure mechanism for failed component " << failed->getName()
	     << " couldn't be determined" << endl;
	cleanUpAndExit(1);
    }

    if (first)
	statsFirstFailedMech[id][idx]++;
    
    statsFailedMech[id][idx]++;
} // updateSTATS

void System::printSTATS() {

    float power = 0;
    for (int i=0; i<(int) statsPower.size(); i++)
	power += statsPower[i];
    
    cout << "*** Failure Statistics ***" << endl;
    cout << "First failure: " << statsFirstFailure / (float) m_samples << endl;
    cout << "N failures: " << (float) statsNFailures / (float) m_samples << endl;
    cout << "Total power: " << power << endl << endl;
    
    cout << setw(9) << left << "name"
	 << setw(9) << left << "type"
	 << setw(9) << right << "power"
	 << setw(6) << right << "temp"
	 << setw(6) << right << "1st"
	 << setw(5) << right << "EM"
	 << setw(5) << right << "TDDB"
	 << setw(5) << right << "TC"
	 << setw(6) << right << "fail"
	 << setw(5) << right << "EM"
	 << setw(5) << right << "TDDB"
	 << setw(5) << right << "TC" << endl;

    for (int i=0; i<(int) components.size(); i++) {
	Component *c = components[i];
	
	cout << setw(9) << left << c->getName()
	     << setw(9) << left << componentTypeToString(c->getCType())
	     << setw(9) << right << scientific << setprecision(2) << statsPower[i]
	     << setw(6) << right << fixed << setprecision(1) << statsTemp[i]
	     << setw(6) << right << fixed << setprecision(2) << (float) statsFirstFailed[i] / (float) m_samples;

	for (int j=0; j<(int) statsFirstFailedMech[i].size(); j++)
	    cout << setw(5) << right << fixed << setprecision(2)
		 << (float) statsFirstFailedMech[i][j] / (float) statsFirstFailed[i];

	cout << setw(6) << right << fixed << setprecision(2) << (float) statsFailed[i] / (float) m_samples;

	for (int j=0; j<(int) statsFirstFailedMech[i].size(); j++)
	    cout << setw(5) << right << fixed << setprecision(2)
		 << (float) statsFailedMech[i][j] / (float) statsFailed[i];
	
	cout << endl;
    } // for    
} // printSTATS
#endif

long long factorial(long long x)
{
  if((x == 0) || (x == 1)) {
    return 1;
  }
  
  return (x * factorial(x - 1));
}

// push failed components from the front of the list toward the back of the list, 
// starting from the last failed component in the list
vector<int> generateNextPermutation(vector<int> initialSeq)
{
  int deallocCount = 0;
  int iter = (int)initialSeq.size() - 1;
  bool done = false;
  vector<int> nextPermutation = initialSeq;
		
    while(!done && (iter >= 0)) {
    
    // starting from the back of the list, find a failed component
    if(nextPermutation[iter] == 0) {

      // is this the last component?
        if(iter != (int)(initialSeq.size() - 1)) {

	// is the next component in the list (toward the back) not failed?
	if(nextPermutation[iter + 1] == 1) {

	  // swap failed/working for both components
	  nextPermutation[iter] = 1;
	  nextPermutation[iter + 1] = 0;
			
	  // count and un-fail all failed components (after the one just marked failed)
	  int deallocIter = (int)initialSeq.size() - 1;
	  while(deallocIter > (iter + 1)) {
	    if(nextPermutation[deallocIter] == 0) {
	      nextPermutation[deallocIter] = 1;
	      deallocCount++;
	    }
				
	    deallocIter--;
	  }

	  // fail the deallocIter components immediately following the one just marked failed
	  while(deallocCount > 0) {
	    nextPermutation[iter + 1 + deallocCount] = 0;
	    deallocCount--;
	  }
					
	  done = true;
	}
      }
    }
		
    iter--;
  }
	
  return nextPermutation;
}

bool compareNumFailedComponents(const set<Component*,compareComponentIDs> a, const set<Component*,compareComponentIDs> b)
{
  return a.size() > b.size();
}
