/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#include <cstdlib>
#include <iostream>
#include <sstream>

#include "power.h"
#include "System.h"
#include "TaskMapping.h"

void create_power_traces(System *sys) {
	int x;

    // Loop over all operating scenarios
	for(x = 0; x < (int)sys->getOperatingScenarios().size(); x++) {
		create_single_power_trace(sys,x);
	}
	
	return;
}

void create_single_power_trace(System *sys, int pos)
{
	int y;
	//char curTraceName[100];
	FILE *curTraceFile;
	

	vector<Component *> localComponents = sys->getComponents();
	vector<string> localEmptyBlocks = sys->getEmptyBlocks();
	set<Component*,compareComponentIDs> curOperatingScenario;
	TaskMapping *curTaskMapping;
	vector<Component *> curTaskMappingComponents;
	vector<set<Task *> > curTaskMappingTasks;
	vector<ComponentNet *> curTaskMappingNetlist;
	//Component *c;
	
	curOperatingScenario = sys->getOperatingScenarios()[pos];
	curTaskMapping = sys->getTaskMappings()[pos];
	curTaskMappingComponents = curTaskMapping->getComponents();
	curTaskMappingTasks = curTaskMapping->getTasks();
	curTaskMappingNetlist = curTaskMapping->getNetlist();

	/*
	curTaskMapping->printTasks();
	for(int x = 0; x < (int)curTaskMappingComponents.size(); x++) {
	  Component *b = curTaskMappingComponents[x];
	  printf("%s\n",b->getName().c_str());
	}
	printf("\n");
	*/

	// Create the file name for this power trace
	//sprintf(curTraceName,"%s.%05d.ptrace",sys->getConfigFileName().c_str(),pos);
	string curTraceName;
	stringstream ss;
	ss << sys->getWorkingDirectory() << HSPATH << pos << ".ptrace";
	curTraceName = ss.str();
	
	// Open the current power trace file
	curTraceFile = fopen(curTraceName.c_str(),"w");
	if(!curTraceFile) {
		cerr << "Failed to open power trace file " << curTraceName << " for writing" << endl;
		sys->cleanUpAndExit(1);
	}
	
	// Loop over all components in the system to print the first line of the power trace
	for(y = 0; y < (int)localComponents.size(); y++) {
		fprintf(curTraceFile,"%s\t\t",localComponents[y]->getName().c_str());
	}
	
	// Loop over all empty blocks in the system to complete the first line
	for(y = 0; y < (int)localEmptyBlocks.size(); y++) {
		fprintf(curTraceFile,"%s\t",localEmptyBlocks[y].c_str());
	}
	fprintf(curTraceFile,"\n");

	vector<Component*> components = sys->getComponents();
	
	// loop over all nets and clear their foward and reverse bandwidths
	for (y=0; y < (int)components.size(); y++) {
	    components[y]->setFwdBandwidth(0);
	    components[y]->setRevBandwidth(0);
	} // for
	
	// loop over all nets to update forward and reverse bandwidth requirements for each component
	//cout << "*** Checking bandwidth in power.cpp" << endl;
	for (y=0; y<(int)curTaskMappingNetlist.size(); y++) {
	    Component *src = curTaskMappingNetlist[y]->getSrc();
	    Component *dst = curTaskMappingNetlist[y]->getDst();

	    directionality dir = curTaskMappingNetlist[y]->getDirectionality();
	    int fwdB = curTaskMappingNetlist[y]->getFwdBandwidth();
	    int revB = curTaskMappingNetlist[y]->getRevBandwidth();

	    /*
	    if (dst->getGType() == MEM)
		cout << src->getName() << "-" << dst->getName() << ": "
		     << fwdB << ", " << revB << endl; // " -- ";
	    */
		     
	    switch (dir) {
	    case UNI:
		src->setFwdBandwidth(src->getFwdBandwidth()+fwdB);
		dst->setRevBandwidth(dst->getRevBandwidth()+fwdB);
		break;
	    case BI:
		src->setFwdBandwidth(src->getFwdBandwidth()+fwdB);
		src->setRevBandwidth(src->getRevBandwidth()+revB);
		
		dst->setFwdBandwidth(dst->getFwdBandwidth()+revB);
		dst->setRevBandwidth(dst->getRevBandwidth()+fwdB);
		break;
	    case REV:
		src->setRevBandwidth(src->getRevBandwidth()+revB);
		dst->setFwdBandwidth(dst->getFwdBandwidth()+revB);
		break;
	    default:
		cerr << "Net has no directionality, exiting" << endl;
		sys->cleanUpAndExit(1);
	    }		
	}

	//cout << endl;
	
	// loop over all components in the system and calculate their power
	for(vector<Component *>::iterator iter = localComponents.begin(); iter != localComponents.end(); iter++) {
	    Component *c = *iter;

	    // if the component is in the operating scenario, calculate its power
	    if(includes(curOperatingScenario.begin(),
			curOperatingScenario.end(),
			iter,
			iter + 1,
			compareComponentIDs())) {		

		bool componentFound;
		int idx = -1;
		int utilization;
		float procPower, memReadPower, memWritePower, memPower, switchPower;
		int switchBandwidth;
		
		switch (c->getGType()) {
		case PROC:
		    //c->resetAvailableCapacity();

		    // find the processor in the task mapping
		    componentFound = false;
		    for (y=0; y<(int) curTaskMappingComponents.size(); y++) {
			if (curTaskMappingComponents[y]->getID() == c->getID()) {
			    componentFound = true;
			    idx = y;
			    break;
			} // if
		    } // for

		    if (componentFound == false) {
			cerr << "*** Component in operating scenario not found in task mapping "
			     << "during power trace generation" << endl;
			sys->cleanUpAndExit(1);
		    }
		    
		    // sum up the MIPS of the tasks assigned to this resource
		    utilization = 0;
		    for (set<Task *>::iterator titer=curTaskMappingTasks[idx].begin();
			 titer != curTaskMappingTasks[idx].end();
			 titer++) {
			Task *t = *titer;
			utilization += t->getReq();
		    }
		    
		    procPower = c->getMIPPower()*utilization;
		    //cout << c->getName() << ": " << c->getMIPPower() << " * (" << c->getInitialCapacity() << " - "
		    // << c->getAvailableCapacity() << ")  = " << procPower << endl;
		    c->setCurPower(procPower);
		    break;
		case MEM:
		    // bandwidth in KB
		    memReadPower = (float) c->getFwdBandwidth() * (float) (1024/MEM_OUTPUT_WIDTH) * (float) c->getReadEnergy();
		    memWritePower = (float) c->getRevBandwidth() * (float) (1024/MEM_OUTPUT_WIDTH) * (float) c->getWriteEnergy();
		    memPower = c->getStaticPower() + memReadPower + memWritePower;

		    /*
		    cout << c->getName() << " " << c->getCType() << ": "
		    	 << c->getStaticPower() << "; "
		    	 << c->getRevBandwidth() << ", " << memReadPower << "; "
		    	 << c->getFwdBandwidth() << ", " << memWritePower << endl;
		    */
		    
		    c->setCurPower(memPower);
		    break;
		case SW:
		    // only count the bandwidth requested (which is, of course, equal to the bandwidth provided)
		  switchBandwidth = c->getRevBandwidth();
		  switchPower = getRouterPower(c->getInputs(),
						       (double) switchBandwidth /
						       (double) (HARD_BANDWIDTH_LIMIT * c->getInputs()));

		    //cout << c->getName() << " " << c->getCType() << ": "
		    //	 << c->getFwdBandwidth() << "; "
		    //	 << c->getRevBandwidth() << "; "
		    //	 << c->getInputs() << "; "
		    //	 << (double) switchBandwidth / (double) (HARD_BANDWIDTH_LIMIT * c->getInputs()) << endl;
		    
		    c->setCurPower(switchPower);
		    break;
		case NO_TYPE:
		    break;
		} // switch

	    } else {
		// else, set power to 0
		
		// TODO update to support set zero power
		c->setCurPower(0);
	    }

	    // update power vector in task mapping
	    curTaskMapping->pushPower(c->getCurPower());
	} // for
	
	// loop over all components in the system and print their power
	for (y=0; y<(int) components.size(); y++) {
	    Component *c = components[y];
	    fprintf(curTraceFile, "%.4e\t", c->getCurPower());
	}

	/* TODO old code that handled set zero power flag)
	// If the current component is not included in the current operating scenario,
	// set its value in the power trace to 0
	else {
	if(sys->getZeroPower()) {
	fprintf(curTraceFile,"0.0\t");
	}
	else {
	fprintf(curTraceFile,"%f\t",c->getInitPower());
	}
	*/
	
	// Print 0.0 for each of the empty blocks
	for(y = 0; y < (int)localEmptyBlocks.size(); y++) {
		fprintf(curTraceFile,"0.0\t");
	}
	fprintf(curTraceFile,"\n");
	
	// Close the current trace file
	fclose(curTraceFile);
	return;
}
