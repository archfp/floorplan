/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#include <cstdlib>
#include <iostream>

#include "System.h"
#include "Component.h"
#include "TaskMapping.h"

TaskMapping::TaskMapping(System *theSystem)
{
  components.clear();
  tasks.clear();
  netlist.clear();
  
  sys = theSystem;
  mcsVerbosity = sys->getVerbosity();
}

TaskMapping::TaskMapping(System *theSystem, set<Component*,compareComponentIDs> curScenario)
{
  int x, y, numPermutations, precludedComponentPos;
  bool componentFound = false;
  bool precludedComponentFound;
	
  sys = theSystem;
  mcsVerbosity = sys->getVerbosity();
	
  TaskMapping *initialTaskMapping = sys->getInitialTaskMapping();
	
  vector<Component*> componentsToBeMapped;
  vector<set<Task *> > tasksToBeMapped;
  vector<vector<componentDistance> > candidateComponents;
  vector<int> curPermutation;
  vector<Component*> switchesToAdd;
  vector<Component*> allPrecludedComponents;
		
  // Ensure private member variables are empty
  components.clear();
  tasks.clear();
  netlist.clear();
  allPrecludedComponents.clear();
  mappingFound = true;
  tempsCalculated = false;
	
  // Copy the blank system netlist to the netlist in this object
  netlist = sys->copyNetlist();

  // The current mapping is being created for the initial scenario, no remapping is required
  if((size_t)sys->getComponents().size() == (size_t)curScenario.size()) {
    if(mcsVerbosity > 0)
      cout << "This is the initial scenario...copying directly to the new task mapping" << endl;
		
    for(x = 0; x < (int)initialTaskMapping->getComponents().size(); x++) {
	components.push_back(initialTaskMapping->getComponents()[x]);
	tasks.push_back(initialTaskMapping->getTasks()[x]);
    }
  }
	
  // The current mapping is being created for some degraded operating scenario, some remapping is required
  else {
    // Step through all of the components in the initial mapping
    for(x = 0; x < (int)initialTaskMapping->getComponents().size(); x++) {
      componentFound = false;
			
      // Step through all of the components in the current operating scenario
      for(set<Component*>::iterator iter = curScenario.begin();
	  iter != curScenario.end(); iter++) {				
	Component *c = *iter;

	// If the component is found, copy the component and its mapped tasks to this task mapping
	if(initialTaskMapping->getComponents()[x]->getName() == c->getName()) {
	  components.push_back(initialTaskMapping->getComponents()[x]);
	  tasks.push_back(initialTaskMapping->getTasks()[x]);
	  componentFound = true;
	  break;
	}
      }
		
      // If the component wasn't found, save the component and its mapped tasks
      // to a new set of parallel vectors
      if(!componentFound) {
   	componentsToBeMapped.push_back(initialTaskMapping->getComponents()[x]);
	tasksToBeMapped.push_back(initialTaskMapping->getTasks()[x]);
				
	// Add the component's precluded components to the list of components to be mapped
	// TODO: shouldn't need the second part of the if clause, but somehow random components
	// say that they have random number of preclusions when the actual preclusions list is empty
	if((initialTaskMapping->getComponents()[x]->getNPreclusions() > 0) &&
	   (initialTaskMapping->getComponents()[x]->getGType() == SW)) {
					
	  // If there are precluded components, the failed component must have been a switch
	  // Save the switch's component object to be added to the list of components just
	  // before candidate components are found
	  switchesToAdd.push_back(initialTaskMapping->getComponents()[x]);		
					
	  // Step through each precluded component
	  list<Component*> precludedComponents = initialTaskMapping->getComponents()[x]->getPreclusions();
	  for(list<Component*>::iterator componentIter = precludedComponents.begin();
	      componentIter != precludedComponents.end(); componentIter++) {
						
	    Component *curComponent = *componentIter;
	    allPrecludedComponents.push_back(curComponent);
	  }
	}
      }
    }
		
    if(mcsVerbosity > 0) {
      cout << "The failed components are: ";
      for(x = 0; x < (int)componentsToBeMapped.size(); x++) {
	cout << componentsToBeMapped[x]->getName() << " ";
      }
      cout << endl;
    }
		
    // Step through all precluded components that need to be removed from the mapping
    for(x = 0; x < (int)allPrecludedComponents.size(); x++) {
			
      // Remove the precluded component and its tasks from the mapping
      precludedComponentPos = 0;
      precludedComponentFound = false;
      for(vector<Component*>::iterator componentIter = components.begin();
	  componentIter != components.end(); componentIter++) {
	Component *curComponent = *componentIter;
				
	if(curComponent->getName() == allPrecludedComponents[x]->getName()) {
	  precludedComponentFound = true;
	  components.erase(componentIter);
	  tasks.erase(tasks.begin() + precludedComponentPos);
					
	  componentsToBeMapped.push_back(allPrecludedComponents[x]);
	  if(mcsVerbosity > 0)
	    cout << "Added " << curComponent->getName() << " to be mapped via preclusion" << endl;
					
	  // Find the tasks mapped to the precluded component and add those to the list of tasks to be mapped
	  for(y = 0; y < (int)initialTaskMapping->getComponents().size(); y++) {
	    if(allPrecludedComponents[x]->getName() == initialTaskMapping->getComponents()[y]->getName()) {
	      tasksToBeMapped.push_back(initialTaskMapping->getTasks()[y]);
	      break;
	    }
	  }
	  break;
	}
	precludedComponentPos++;
      }
			
      if(!precludedComponentFound) {
	if(mcsVerbosity > 0) {
	  cout << "Couldn't find precluded component " << allPrecludedComponents[x]->getName() << " in the list of components" << endl;
	  cout << "Assuming it is already in the xxxToBeMapped lists" << endl;
	}
      }
    }
		
    // The base task mapping is complete at this point...save it
    baseTaskMapping = tasks;

    // Set available capacities for each component based on the base task mapping
    sys->resetPartialCapacitiesToInitial();
    for(x = 0; x < (int)components.size(); x++) {
	//cout << components[x]->getName() << " ";
	
      for(set<Task *>::iterator iter = baseTaskMapping[x].begin(); iter != baseTaskMapping[x].end(); iter++) {
	Task *curTask = *iter;

	//cout << curTaskName << " " << sys->getTask(curTaskName)->getReq() << " ";
	
	components[x]->setPartialCapacity(components[x]->getPartialCapacity() - curTask->getReq());
      }

      //cout << endl;
    }
    sys->resetAvailableCapacitiesToPartial();

    // Compare the capacity to be remapped to the available capacity in the system
    mappingFound = checkAvailableCapacity(componentsToBeMapped,tasksToBeMapped);
    if(!mappingFound) {
      if(mcsVerbosity > 0) {
	cout << "System has insufficient available capacity to allow any task remapping" << endl;
      }
      
      return;
    }
		
    // Temporarily add switches to the component list
    for(x = 0; x < (int)switchesToAdd.size(); x++) {
      components.push_back(switchesToAdd[x]);
    }
		
    // Make lists of candidate components for each component to be mapped
    candidateComponents = findCandidateComponents(componentsToBeMapped);
    candidateComponents = sortCandidateComponents(candidateComponents);
		
    if(mcsVerbosity > 0) {
      cout << "Candidate components: " << endl;
      for(x = 0; x < (int)candidateComponents.size(); x++) {
	for(y = 0; y < (int)candidateComponents[x].size(); y++) {
	    cout << (candidateComponents[x])[y].component->getName() << " "
		 << candidateComponents[x][y].component->getAvailableCapacity() << " "
		 << (candidateComponents[x])[y].hops << endl;
	}
	cout << endl;
      }
    }
		
    // Remove any switches that were temporarily added to the components list
    for(x = 0; x < (int)switchesToAdd.size(); x++) {
      components.erase(components.end() - 1);
    }
		
    // Remove nets connected to unmapped components from the netlist
    cullNetlist(componentsToBeMapped);
  }

  // Remap the components in the ToBeRemapped vectors
  if(mcsVerbosity > 0)
    cout << "Found " << componentsToBeMapped.size() << " components with tasks to be remapped" << endl;
	
  // Populate the netlist with bandwidth information for all tasks except those requiring remapping
  mappingFound = populateMappedBandwidth(tasksToBeMapped);
  if(!mappingFound) {
    if(mcsVerbosity > 0)
      cout << "Could not populate all initial bandwidth" << endl;
		
    return;
  }
  saveInitialNetlist();

  /* No longer necessary since the Task object has been fully integrated
  // Build a vector<set<Task *> > out of a vector<set<string> >
  vector<set<Task *> > newTasksToBeMapped;
  for(x = 0; x < (int)tasksToBeMapped.size(); x++) {
    set<Task *> curTaskSet;
    for(set<string>::iterator iter = tasksToBeMapped[x].begin(); iter != tasksToBeMapped[x].end(); iter++) {
      string curTaskName = *iter;
      curTaskSet.insert(theSystem->getTask(curTaskName));
    }
    newTasksToBeMapped.push_back(curTaskSet);
  }
  */
	
  // Remap tasks as necessary
  if((int)componentsToBeMapped.size() > 0) {
		
    // First, try to remap only the required tasks in order of vector
    numPermutations = factorial((int)componentsToBeMapped.size());
    if(numPermutations > sys->getMappingPermutations())
      numPermutations = sys->getMappingPermutations();
    for(x = 0; x < (int)componentsToBeMapped.size(); x++) {
      curPermutation.push_back(x);
    }
		
    mappingFound = false;
    for(x = 0; x < numPermutations; x++) {
      if(mcsVerbosity > 0) {
	cout << "Trying permutation " << x << endl;
	for(y = 0; y < (int)curPermutation.size(); y++) {
	  cout << curPermutation[y] << " ";
	}
	cout << endl;
      }

      mappingFound = remap1(componentsToBeMapped,tasksToBeMapped,candidateComponents,curPermutation);
	
      if(mappingFound) {
	if(mcsVerbosity > 0)
	  cout << "Mapping found with permutation " << x << endl;
	break;
      }
      //next_permutation(curPermutation.begin(),curPermutation.end());
      curPermutation = knuthShuffle((int)componentsToBeMapped.size());
      revertToInitialNetlist();
      tasks = baseTaskMapping;
      sys->resetAvailableCapacitiesToPartial();
    }
		
    if(!mappingFound) {
      if(mcsVerbosity > 0)
	cout << "Could not find a valid task remapping with remap1 in " << numPermutations << " permutations" << endl;
			
      return;
    }
    // Second, try to remap only the required tasks in all orders
    // Finally, remap all tasks in all orders
  }
	
  if(mcsVerbosity > 0)
    printNetlist();
	
  return;
}

TaskMapping::~TaskMapping()
{
	int x;
	
	for(x = 0; x < (int)netlist.size(); x++) {
		delete netlist[x];
	}
	netlist.clear();
	
	for(x = 0; x < (int)initialNetlist.size(); x++) {
		delete initialNetlist[x];
	}
	initialNetlist.clear();
	
	return;
}

void TaskMapping::addSingleMapping(Component *tComponent, set<Task *> tTasks)
{
	components.push_back(tComponent);
	tasks.push_back(tTasks);
	return;
}

bool TaskMapping::checkAvailableCapacity(vector<Component *> componentsToBeMapped, vector<set<Task *> > tasksToBeMapped)
{
  int x, y, curRequiredCapacity, curAvailableCapacity, maxRequired, maxAvailable;

  // Perform the capacity checks for each component type
  for(x = 0; x < N_G_COMPONENT_TYPES; x++) {
    
    // Calculate the total required capacity for the current component type
    curRequiredCapacity = 0;
    maxRequired = 0;
    for(y = 0; y < (int)componentsToBeMapped.size(); y++) {
      if(componentsToBeMapped[y]->getGType() == x) {
	for(set<Task *>::iterator iter = tasksToBeMapped[y].begin(); iter != tasksToBeMapped[y].end(); iter++) {
	  Task *curTask = *iter;
	  int curReq = curTask->getReq();

	  curRequiredCapacity += curReq;

	  // Keep track of the largest single required capacity
	  if(curReq > maxRequired) {
	    maxRequired = curReq;
	  }
	}
      }
    }

    // Calculate the total available capacity for the current component type
    curAvailableCapacity = 0;
    maxAvailable = 0;
    for(y = 0; y < (int)components.size(); y++) {
      if(components[y]->getGType() == x) {
	int curCapacity = components[y]->getAvailableCapacity();
	
	curAvailableCapacity += curCapacity;
	
	// Keep track of the largest single available capacity
	if(curCapacity > maxAvailable) {
	  maxAvailable = curCapacity;
	}
      }
    }

    // Check to see whether or not the total required capacity for the current component type is greater
    // than the total available capacity for the current component type
    if(curAvailableCapacity < curRequiredCapacity) {
      return false;
    }

    // Check to see whether or not the largest chuck of required capacity for the current component type
    // is greater than the largest chunk of available capacity for the current component type
    if(maxAvailable < maxRequired) {
      return false;
    }
  }

  return true;
}

vector<vector<componentDistance> > TaskMapping::findCandidateComponents(vector<Component*> componentsToBeMapped)
{
  int x, y, curHops;
  vector<vector<componentDistance> > allOrderings;
	
  // Step through all components to be mapped
  for(x = 0; x < (int)componentsToBeMapped.size(); x++) {
    vector<componentDistance> curOrdering;	
		
    // Build the list of candidate components for each component to be remapped
    for(y = 0; y < (int)components.size(); y++) {
      componentDistance curCandidateDistance;
			
      // Check to see if the component types match and if there is available redundancy
      if((componentsToBeMapped[x]->getGType() == components[y]->getGType()) &&
	 (components[y]->getAvailableCapacity() != 0)) {
				
	// Find the number of hops between the current component to be remapped
	// and the potential candidate
				
	if(findPath(componentsToBeMapped[x],components[y],0,false,&curHops)) {
	  if(mcsVerbosity > 0) {
	    cout << curHops << " hops between " << componentsToBeMapped[x]->getName();
	    cout << " and " << components[y]->getName() << endl;
	  }
	  
	  curCandidateDistance.component = components[y];
	  curCandidateDistance.hops = curHops;
	  curOrdering.push_back(curCandidateDistance);
	}
	else {
	  if(mcsVerbosity > 0) {
	    cout << "No path found between " << componentsToBeMapped[x]->getName();
	    cout << " and " << components[y]->getName() << endl;	
	  }
	}				
      }
    }
	
		
    allOrderings.push_back(curOrdering);
  }
	
  return allOrderings;
}

void TaskMapping::cullNetlist(vector<Component*> componentsToBeMapped)
{
  int x;
  
  // Step through each net in the netlist
  for(vector<ComponentNet*>::iterator iter = netlist.begin(); iter != netlist.end(); iter++) {
    ComponentNet *curNet = *iter;
    
    // Step through each component in the vector of components currently without mappings
    for(x = 0; x < (int)componentsToBeMapped.size(); x++) {
      
      // If the current component name matches a source or destination in a net, remove the net
      if((componentsToBeMapped[x] == curNet->getSrc()) ||
	 (componentsToBeMapped[x] == curNet->getDst())) {
	netlist.erase(iter);
	iter--; // Iterator must be decremented because all elements shift down after an erase
	break;
      }
    }
  }
  
  return;
}

bool TaskMapping::populateMappedBandwidth(vector<set<Task *> > tasksToBeMapped)
{
  int x, y;
  bool mainTaskUnmapped = false;
  bool componentFound = false;
  Component *srcComponent;
  Component *dstComponent;
  
  // Step through all of the tasks in the main task graph
  for(x = 0; x < (int)sys->getTaskGraph().size(); x++) {
    
    // Step through each set in the exclusion vector
    mainTaskUnmapped = false;
    for(y = 0; y < (int)tasksToBeMapped.size(); y++) {
      
      // Step through each task in the current set
      for(set<Task *>::iterator taskIter = tasksToBeMapped[y].begin();
	  taskIter != tasksToBeMapped[y].end(); taskIter++) {
	Task *curTask = *taskIter;
	
	if((curTask == sys->getTaskGraph()[x]->getSrc()) ||
	   (curTask == sys->getTaskGraph()[x]->getDst())) {
	  mainTaskUnmapped = true;
	  break;
	}
      }
      
      if(mainTaskUnmapped) {
	break;
      }
    }
    
    // If the current task from the main list was not found in exclusion vector,
    // add its bandwidth to this object's netlist
    if(!mainTaskUnmapped) {
      
      // Get the source component to which the source task is mapped
      // Loop over all components currently in the mapping
      componentFound = false;
      for(y = 0; y < (int)components.size(); y++) {
	
	// Loop over all tasks mapped to the current component
	for(set<Task *>::iterator taskIter = tasks[y].begin();
	    taskIter != tasks[y].end(); taskIter++) {
	  Task *curTask = *taskIter;
	  
	  if(sys->getTaskGraph()[x]->getSrc() == curTask) {
	    srcComponent = components[y];
	    componentFound = true;
	    break;
	  }
	}
	
	if(componentFound) {
	  break;
	}
      }
      
      if(!componentFound) {
	if(mcsVerbosity > 0)
	  cout << "Couldn't find the component to which source task " << sys->getTaskGraph()[x]->getSrc() << " is mapped in the initial mapping" << endl;
	return false;
      }
      
      // Get the destination component to which the destination task is mapped
      componentFound = false;
      for(y = 0; y < (int)components.size(); y++) {
	
	// Loop over all tasks mapped to the current component
	for(set<Task *>::iterator taskIter = tasks[y].begin();
	    taskIter != tasks[y].end(); taskIter++) {
	  Task *curTask = *taskIter;
	  
	  if(sys->getTaskGraph()[x]->getDst() == curTask) {
	    dstComponent = components[y];
	    componentFound = true;
	    break;
	  }
	}
	
	if(componentFound) {
	  break;
	}
      }
      
      if(!componentFound) {
	if(mcsVerbosity > 0)
	  cout << "Couldn't find the component to which destination task " << sys->getTaskGraph()[x]->getDst() << " is mapped in the initial mapping" << endl;
	return false;
      }
      
      if(!findPath(srcComponent,dstComponent,sys->getTaskGraph()[x]->getBandwidth(),true,NULL)) {
	if(mcsVerbosity > 0) {
	  cout << "Couldn't find a path from component " << srcComponent->getName() << " to component ";
	  cout << dstComponent->getName() << " during initial bandwidth population" << endl;
	}
	return false;
      }
    }
  }
  
  return true;
}

bool TaskMapping::remap1(vector<Component*> componentsToBeMapped, vector<set<Task *> > tasksToBeMapped, vector<vector<componentDistance> > candidateComponents, vector<int> p)
{
  int w, x, y, z, outerLoop;
  bool dstComponentFound = false;
  bool srcComponentFound = false;
  bool allTasksRemapped = true;
  bool curTaskRemapped = false;
  bool allPathsExist = true;
	
  Component *candidateComponent;
	
  vector<Task *> dstTasks, srcTasks;
  vector<Component*> dstComponents, srcComponents;
  vector<int> dstBandwidths, srcBandwidths;
		
  // Loop over all components with tasks requiring remapping
  for(outerLoop = 0; outerLoop < (int)componentsToBeMapped.size(); outerLoop++) {
    x = p[outerLoop];

    // Loop over all tasks to be remapped for the current component
    allTasksRemapped = true;
    for(set<Task *>::iterator taskIter1 = tasksToBeMapped[x].begin();
	taskIter1 != tasksToBeMapped[x].end(); taskIter1++) {
      Task *curTask1 = *taskIter1;
      int curTask1Req = curTask1->getReq();
		
      // Find the destination and source task(s) of the task being remapped
      dstTasks.clear();
      srcTasks.clear();
      dstBandwidths.clear();
      srcBandwidths.clear();
      for(w = 0; w < (int)sys->getTaskGraph().size(); w++) {
	// The task being remapped is a source for some other task
	if(sys->getTaskGraph()[w]->getSrc() == curTask1) {
	  dstTasks.push_back(sys->getTaskGraph()[w]->getDst());
	  dstBandwidths.push_back(sys->getTaskGraph()[w]->getBandwidth());
	}
	// The task being remapped is a destination for some other task
	if(sys->getTaskGraph()[w]->getDst() == curTask1) {
	  srcTasks.push_back(sys->getTaskGraph()[w]->getSrc());
	  srcBandwidths.push_back(sys->getTaskGraph()[w]->getBandwidth());
	}
      }

      // Find the destination component(s) to which the destination task(s) is/are mapped
      // Loop over all destination tasks
      dstComponents.clear();
      for(w = 0; w < (int)dstTasks.size(); w++) {
	dstComponentFound = false;
				
	// Loop over all components currently in the mapping
	for(y = 0; y < (int)components.size(); y++) {
					
	  // Loop over all tasks mapped to the current component
	  for(set<Task *>::iterator taskIter2 = tasks[y].begin();
	      taskIter2 != tasks[y].end(); taskIter2++) {
	    Task *curTask2 = *taskIter2;
						
	    if(dstTasks[w] == curTask2) {
	      dstComponents.push_back(components[y]);
	      dstComponentFound = true;
	    }
	  }
	}
				
	if(!dstComponentFound) {
	  if(mcsVerbosity > 0) {
	    cout << "Couldn't find the component mapped to destination task " << dstTasks[w] << endl;
	    cout << "This is probably due to the fact that the destination task also needs to be remapped" << endl;
	    cout << "The current remapping algorithm does not handle this scenario" << endl;
	  }
	  //exit(1);
	  //return false;
	}
      }

      // Find the source component(s) to which the source task(s) is/are mapped
      // Loop over all source tasks
      srcComponents.clear();
      for(w = 0; w < (int)srcTasks.size(); w++) {
	srcComponentFound = false;
				
	// Loop over all components currently in the mapping
	for(y = 0; y < (int)components.size(); y++) {
					
	  // Loop over all tasks mapped to the current component
	  for(set<Task *>::iterator taskIter2 = tasks[y].begin();
	      taskIter2 != tasks[y].end(); taskIter2++) {
	    Task *curTask2 = *taskIter2;
						
	    if(srcTasks[w] == curTask2) {
	      srcComponents.push_back(components[y]);
	      srcComponentFound = true;
	    }
	  }
	}
				
	if(!srcComponentFound) {
	  if(mcsVerbosity > 0) {
	    cout << "Couldn't find the component mapped to source task " << srcTasks[w] << endl;
	    cout << "This is probably due to the fact that the source task also needs to be remapped" << endl;
	    cout << "The current remapping algorithm does not handle this scenario" << endl;
	  }
	  //exit(1);
	  //return false;
	}
      }
			
      if(mcsVerbosity > 0)
	cout << "Now attempting to find a home for task " << curTask1->getName() << " which has " << dstComponents.size() << " destination components and " << srcComponents.size() << " source components" << endl;
			
      // Loop over all components currently in the mapping to find a suitable replacement
      curTaskRemapped = false;
      for(z = 0; z < (int)candidateComponents[x].size(); z++) {
	candidateComponent = (candidateComponents[x])[z].component;
				
	// Check to see if the component types match and if there is available redundancy
	if((componentsToBeMapped[x]->getGType() == candidateComponent->getGType()) &&
	   (candidateComponent->getAvailableCapacity() >= curTask1Req)) {
	 
	  allPathsExist = true;
	  if(!sys->getIdealMTTF()) {

	    if(mcsVerbosity > 0)
	      cout << "Try " << z << ": " << candidateComponent->getName() << endl;
	    
	    // Loop over all destination components
	    for(w = 0; w < (int)dstComponents.size(); w++) {
	      // Determine whether or not there is a path between the current candidate component and
	      // the current destination component and place bandwidth along that path
	      allPathsExist = allPathsExist &
		findPath(candidateComponent,dstComponents[w],dstBandwidths[w],true,NULL);
	    }
	    
	    // Loop over all source components
	    for(w = 0; w < (int)srcComponents.size(); w++) {
	      // Determine whether or not there is a path between the current source component and
	      // the current candidate component and place bandwidth along that path
	      allPathsExist = allPathsExist &
		findPath(srcComponents[w],candidateComponent,srcBandwidths[w],true,NULL);
	    }
	  }
	}
	   
	// If the types do not match or there is no available redundancy, move to the next candidate
	else {
	  continue;
	}
	
	if(allPathsExist) {
	  if(mcsVerbosity > 0)
	    cout << "The task " << curTask1->getName() << " can be remapped to component " << candidateComponent->getName() << endl;
					
	  // Reduce the available redundancy of the component that the task has been remapped to
	  // This should have a global effect since components hold pointers to the original component objects
	  // So, inital available redundancy values must be reset before another task mapping is attempted
	  candidateComponent->setAvailableCapacity(candidateComponent->getAvailableCapacity() - curTask1Req);
					
	  // Add the remapped task to the set of the component's tasks in the task mapping
	  bool taskInserted = false;
	  for(w = 0; w < (int)components.size(); w++) {
	    if(candidateComponent->getName() == components[w]->getName()) {
	      tasks[w].insert(curTask1);
	      taskInserted = true;
	    }
	  }
					
	  if(!taskInserted) {
	    cout << "Couldn't insert the remapped task in the mapping" << endl;
	    exit(0);
	  }
					
	  curTaskRemapped = true;
	  //printNetlist();
	  break; // This moves to the next task that must be remapped
	}
	else {
	  if(mcsVerbosity > 0)
	    cout << "The task " << curTask1->getName() << " cannot be remapped to component " << candidateComponent->getName() << endl;
	}
      }
			
      if(!curTaskRemapped) {
	if(mcsVerbosity > 0)
	  cout << "Could not remap task " << curTask1->getName() << endl;
	allTasksRemapped = false;
	break; // Give up if any task cannot be remapped
      }
    }
		
    if(!allTasksRemapped) {
      if(mcsVerbosity > 0)
	cout << "Task remapping failed" << endl;
      return false;
    }
  }
	
  return true;
}

bool TaskMapping::findPath(Component *src, Component *dst, int bandwidth, bool useBandwidth, int *numHops) {
	
  int x, y, srcPos, minDist, minPos, neighborPos, newDist, stepsTaken;
  bool neighborPosFound = false;
  vector<int> dist;
  vector<Component *> prev;
  vector<Component *> tempComponents = components;
	
  Component *searchComponent;
  Component *bandwidthSrc;
  Component *bandwidthDst;
	
  vector<Component *> neighbors;
  vector<int> neighborDist;
		
  //cout << "Starting to find a path from component " << src->getName() << " to " << dst->getName() << endl;
	
  // Short-circuit the function if the source and destination are equal
  if(src == dst) {
    if(useBandwidth) {
      if(mcsVerbosity > 0)
	cout << "Source and destination are the same...automatic success" << endl;
      return true;
    }
    else {
      cout << "Same source and destination when trying to find number of hops...something is probably wrong" << endl;
      return false;
    }
  }
	
  // If findPath is being used to find candidate components, add the src to the components vector
  if(!useBandwidth) {
    dist.push_back(-1);
    prev.push_back(NULL);
    tempComponents.push_back(src);
  }
	
  // Initialize the distance and previous node vectors (which are parallel to the components vector)
  srcPos = -1;
  for(x = 0; x < (int)tempComponents.size(); x++) {
    dist.push_back(-1);
    prev.push_back(NULL);

    // Determine the position of the source node in the vectors
    if(tempComponents[x] == src) {
      srcPos = x;
    }
  }

  if(srcPos == -1) {
    cout << "Couldn't find " << src->getName() << " in tempComponents" << endl;
  }
  dist[srcPos] = 0;
	
  // The main loop for Dijkstra's algorithm
  stepsTaken = 0;
  while(!tempComponents.empty()) {
		
    // Find the node with the smallest distance
    minDist = -1;
    minPos = -1;
    for(x = 0; x < (int)dist.size(); x++) {
      if((((minDist == -1) && (dist[x] != -1)) || ((dist[x] < minDist) && (dist[x] != -1))) && 
	 (tempComponents[x] != NULL)) {
	minDist = dist[x];
	minPos = x;
      }
    }

    // If minDist was not found to be something other than -1, there is no path
    if(minDist == -1) {
      if(mcsVerbosity > 0)
	cout << "No path found between component " << src->getName() << " and " << dst->getName() << endl;
      return false;
    }
		
    searchComponent = tempComponents[minPos];
		
    // Exit the main loop if the destination node is reached
    if((stepsTaken > 0) && (tempComponents[minPos] == dst)) {
      break;
    }
		
    // If this is not the first iteration, ensure that the component is a
    // switch before finding its neighbors
    if((stepsTaken > 0) && (tempComponents[minPos]->getGType() != SW)) {
      tempComponents[minPos] = NULL;
      continue;
    }
		
    // Find all neighbors of the node with the minimum distance
    for(x = 0; x < (int)netlist.size(); x++) {
      bool componentFound;

      // If the current node is a source, push its destination as a neighbor
      if((netlist[x]->getSrc() == searchComponent) && 
	 ((netlist[x]->getDirectionality() == UNI) || (netlist[x]->getDirectionality() == BI))) {

	// Ensure the destination is not NULL in the components list
	componentFound = false;
	for(y = 0; y < (int)tempComponents.size(); y++) {
	  if(tempComponents[y] == netlist[x]->getDst()) {
	    componentFound = true;
	    break;
	  }
	}
	
	if(componentFound) {
	  neighbors.push_back(netlist[x]->getDst());
	  
	  if(mcsVerbosity > 10)
	    cout << netlist[x]->getDst()->getName() << " added as a neighbor of " << searchComponent->getName() << endl;
	  
	  neighborDist.push_back(netlist[x]->getFwdBandwidth());
	}
      }

      // If the current node is a destination, push its source as a neighbor
      if((netlist[x]->getDst() == searchComponent) &&
	 ((netlist[x]->getDirectionality() == REV) || (netlist[x]->getDirectionality() == BI))) {

	// Ensure the source is not NULL in the components list
	componentFound = false;
	for(y = 0; y < (int)tempComponents.size(); y++) {
	  if(tempComponents[y] == netlist[x]->getSrc()) {
	    componentFound = true;
	    break;
	  }
	}

	if(componentFound) {
	  neighbors.push_back(netlist[x]->getSrc());
	  
	  if(mcsVerbosity > 10)
	    cout << netlist[x]->getSrc()->getName() << " added as a neighbor of " << searchComponent->getName() << endl;
	  
	  neighborDist.push_back(netlist[x]->getRevBandwidth());
	}
      }
    }
		
    // Loop over all neighbors
    for(x = 0; x < (int)neighbors.size(); x++) {
			
      if(useBandwidth) {
	newDist = minDist + neighborDist[x];
      }
      else {
	newDist = minDist + 1;
      }
			
      // If the maximum link bandwidth would be exceeded, skip this neighbor
      if(useBandwidth && ((bandwidth + neighborDist[x]) > sys->getMaxLinkBandwidth())) {
	continue;
      }

      // Determine where the current neighbor is in the components vector
      neighborPosFound = false;
      for(y = 0; y < (int)components.size(); y++) {
	if(components[y] == neighbors[x]) {
	  neighborPos = y;
	  neighborPosFound = true;
	  break;
	}
      }
			
      if(!neighborPosFound) {
	cerr << "Could not find neighbor " << neighbors[x]->getName() << " in the list of components" << endl;
	cerr << "This is likely caused by a mismatch between component names in the config and netlist files" << endl;
	exit(1);
	//continue;
      }
			
      if((newDist < dist[neighborPos]) || (dist[neighborPos] == -1)) {
	dist[neighborPos] = newDist;
	prev[neighborPos] = tempComponents[minPos];
      }
    }
		
    // Remove the node with the smallest distance from the set of nodes
    tempComponents[minPos] = NULL;
    stepsTaken++;
  }
	
  if(!useBandwidth) {
    *numHops = stepsTaken;
  }
	
  if(useBandwidth) {
	
    Component *tmpDst = dst;
    int dstPos;
    for(x = 0; x < (int)components.size(); x++) {
      if(components[x] == tmpDst) {
	dstPos = x;
	break;
      }
    }
		
    while(prev[dstPos] != NULL) {
      bandwidthDst = tmpDst;
      tmpDst = prev[dstPos];
      bandwidthSrc = tmpDst;
      placeBandwidth(bandwidthSrc,bandwidthDst,bandwidth);
			
      for(x = 0; x < (int)components.size(); x++) {
	if(components[x] == tmpDst) {
	  dstPos = x;
	  break;
	}
      }	
    }
  }

  return true;
}

void TaskMapping::placeBandwidth(Component *bandwidthSrc, Component *bandwidthDst, int bandwidth)
{
  int x;
  bool bandwidthPlaced = false;
	
  for(x = 0; x < (int)netlist.size(); x++) {
	
    switch(netlist[x]->getDirectionality()) {
    case UNI:
      if((netlist[x]->getSrc() == bandwidthSrc) && (netlist[x]->getDst() == bandwidthDst)) {
	netlist[x]->setFwdBandwidth(netlist[x]->getFwdBandwidth() + bandwidth);
	bandwidthPlaced = true;
      }
      break;
    case BI:
      if((netlist[x]->getSrc() == bandwidthSrc) && (netlist[x]->getDst() == bandwidthDst)) {
	netlist[x]->setFwdBandwidth(netlist[x]->getFwdBandwidth() + bandwidth);
	bandwidthPlaced = true;
      }
      else if((netlist[x]->getSrc() == bandwidthDst) && (netlist[x]->getDst() == bandwidthSrc)) {
	netlist[x]->setRevBandwidth(netlist[x]->getRevBandwidth() + bandwidth);
	bandwidthPlaced = true;
      }
      break;
    case REV:
      if((netlist[x]->getSrc() == bandwidthDst) && (netlist[x]->getDst() == bandwidthSrc)) {
	netlist[x]->setRevBandwidth(netlist[x]->getRevBandwidth() + bandwidth);
	bandwidthPlaced = true;
      }
      break;
    case NO_DIR:
    default:
      cout << "Invalid directionality specified for net between " << netlist[x]->getSrc() << " and " << netlist[x]->getDst() << endl;
    }
		
    if(bandwidthPlaced) {
      break;	
    }
  }
	
  if(!bandwidthPlaced) {
    cout << "Could not find a net in the netlist between " << bandwidthSrc << " and " << bandwidthDst << endl;
    exit(1);
  }
	
  return;
}

void TaskMapping::saveInitialNetlist()
{
  int x;
  
  for(x = 0; x < (int)netlist.size(); x++) {
    ComponentNet *curNet = netlist[x];
    ComponentNet *newNet = new ComponentNet(curNet->getSrc(),curNet->getDst(),
					    curNet->getFwdBandwidth(),curNet->getRevBandwidth(),
					    curNet->getDirectionality());
    initialNetlist.push_back(newNet);
  }
  
  return;
}

void TaskMapping::revertToInitialNetlist()
{
  int x;
  
  // Destroy all current netlist entries
  for(x = 0; x < (int)netlist.size(); x++) {
    delete netlist[x];
  }
  
  // Remove all entries from the current netlist
  netlist.clear();
  
  for(x = 0; x < (int)initialNetlist.size(); x++) {
    ComponentNet *curNet = initialNetlist[x];
    ComponentNet *newNet = new ComponentNet(curNet->getSrc(),curNet->getDst(),
					    curNet->getFwdBandwidth(),curNet->getRevBandwidth(),
					    curNet->getDirectionality());
    netlist.push_back(newNet);
  }
  
  return;
}

void TaskMapping::printNetlist()
{
  int x;
  for(x = 0; x < (int)netlist.size(); x++) {
    cout << netlist[x]->getSrc()->getName() << " " << netlist[x]->getDst()->getName() << " " << netlist[x]->getDirectionality() << " ";
    cout << netlist[x]->getFwdBandwidth() << " " << netlist[x]->getRevBandwidth() << endl;
  }
  return;
}

void TaskMapping::printTasks()
{
  int x;
  
  for(x = 0; x < (int)tasks.size(); x++) {
    for(set<Task *>::iterator taskIter = tasks[x].begin();
	taskIter != tasks[x].end(); taskIter++) {
      Task *curTask = *taskIter;
      cout << curTask->getName() << " ";
    }
    cout << endl;
  }
  cout << endl;
  
  return;
}

vector<int> TaskMapping::knuthShuffle(int N)
{
  int x, k, t;
  int n = N;
  vector<int> thePermutation;

  for(x = 0; x < n; x++) {
    thePermutation.push_back(x);
  }

  while(n > 1) {
    k = rand() % n;
    n--;
    t = thePermutation[n];
    thePermutation[n] = thePermutation[k];
    thePermutation[k] = t;
  }

  return thePermutation;
}


vector<vector<componentDistance> > TaskMapping::sortCandidateComponents(vector<vector<componentDistance> > candidateComponents)
{
	int x;
	
	for(x = 0; x < (int)candidateComponents.size(); x++) {
		sort(candidateComponents[x].begin(),candidateComponents[x].end(),compareComponentDistances);
	}
	
	return candidateComponents;
}

bool compareComponentDistances(const componentDistance a, const componentDistance b) {
	return a.hops < b.hops;
}
