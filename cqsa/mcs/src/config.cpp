/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include "config.h"
#include "System.h"
#include "hotSpot/hotspot.h"
#include "ComponentLibrary.h"

using namespace std;

//int verbosity = 0; /* this is now at the mcs level */
	
void parse_initTemp(const char *temp, System &sys) {
	FILE *in;
	
	int line_number;
	char *tok;
	char line[64];
	
	int mcsVerbosity = sys.getVerbosity();
	
	// sanity check
	if (temp == NULL) {
		fprintf(stderr, "Unable to read empty filename\n");
		sys.cleanUpAndExit(1);
	}
	
	// try to open the temperature profile
	if ((in = fopen(temp, "r")) == NULL) {
		fprintf(stderr, "Could not open file %s for read\n", temp);
		sys.cleanUpAndExit(1);
	}
	
	if (mcsVerbosity > 0)
		printf("::: Reading from temperature profile %s\n", temp);
	
	line_number = 0;
	while (fgets(line, sizeof(line), in) != NULL) {
		line_number++;
		
		tok = strtok(line, TOKENS);
		
		// check for blank lines
		if (tok == NULL) continue;
		// check for comments
		if (!strncmp(tok, "#", 1)) continue;
		
		// check for first line
		if (!strcmp(tok, "Unit\tSteady(Kelvin)")) continue;
		
		// first token is the name, so get the name and look it up
		Component *c = sys.getComponent(tok);
		
		// if the component isn't recognized, continue
		// TODO: optimize by only reading the first N+1 lines where
		// N is the number of components in the system
		if (c != NULL) continue;
		
		// second token is the temperature
		float temperature = strtod(tok, NULL);
		c->setInitialTemperature(temperature);
	} // while
}

void parse_config(const char *config, System &sys) {
    FILE *in;

    int line_number;
    char *tok;
    char line[100]; // was 64...but operating scenarios could be much longer
    int numInitialTasks = 0;
    int x;

    int mcsVerbosity = sys.getVerbosity();
    
    // sanity check
    if (config == NULL) {
    	fprintf(stderr, "Unable to read empty filename\n");
    	sys.cleanUpAndExit(1);
    } 
    
    // try to open the configuration file
    if ((in = fopen(config, "r")) == NULL) {
    	fprintf(stderr, "Could not open file %s for read\n", config);
    	sys.cleanUpAndExit(1);
    }

    if (mcsVerbosity > 0)
    	printf("::: Reading from configuration file %s\n", config);

    line_number = 0;
    SECTIONTYPE section = SECTION_NONE;

    // read the file
    while (fgets(line, sizeof(line), in) != NULL) {
    	line_number++;
	
	tok = strtok(line, TOKENS);

	// check for blank lines
	if (tok == NULL) continue;
	// check for comments
	if (!strncmp(tok, "#", 1)) continue;

	// TODO: convert to lower case
	//g_strdown(tok);

	// based on the current section type, process line in different ways
	switch (section) {
	case SECTION_NONE:
	    // check for the beginning of a new section
	    if (!strcmp(tok, "define")) {
		// see what sort of section this is
		tok = strtok(NULL, TOKENS);
		//g_strdown(tok);
			
		if (!strcmp(tok, "components")) {
		    section = SECTION_OBJS;
		} else if (!strcmp(tok, "preclusions")) {
		    section = SECTION_PREC;
		} else {
		    fprintf(stderr, "Unrecognized section, line %d\n",
			    line_number);
		    sys.cleanUpAndExit(1);
		} 
	    } else {
		fprintf(stderr, "Unrecognized token, line %d\n", line_number);
		sys.cleanUpAndExit(1);
	    }
	    break;
	case SECTION_OBJS:
	    // check for the end
	    if (!strcmp(tok, "end")) {
		section = SECTION_NONE;
	    } else {
		// if we haven't ended, we have a new component
		// first token is the component name
		char *name = tok;
			
		// second token is component type
		tok = strtok(NULL, TOKENS);
		componentType curComponentType;
			
		// Compare the token to the list of known component types
		if (tok != NULL) {
		    string cTypeName = tok;
		    curComponentType = stringToComponentType(cTypeName);

		    if (curComponentType == NONE) {
			fprintf(stderr, "Invalid component type (%s), line %d\n", tok, line_number);
			sys.cleanUpAndExit(1);
		    }
		} 
		else {
		    fprintf(stderr, "Expected component type, line %d\n", line_number);
		    sys.cleanUpAndExit(1);
		}
			
		// third token is number of initially mapped tasks
		tok = strtok(NULL,TOKENS);
		if(tok != NULL) {
		    numInitialTasks = atoi(tok);
		}
		else {
		    fprintf(stderr,"Expected number of initially mapped tasks, line %d\n",line_number);
		    sys.cleanUpAndExit(1);
		}

		// create component
		Component *curComponent;
		curComponent = sys.addComponent(name,curComponentType);

		// remaining tokens are initially mapped tasks
		set<Task *> curInitialTasks;
		for(x = 0; x < numInitialTasks; x++) {
		  Task *curTask;
		  
		  tok = strtok(NULL,TOKENS);
		  if(tok != NULL) {	
		    
		    // Ensure the current task name corresponds to a Task object
		    curTask = sys.getTask(tok);
		    if(!curTask) {
		      fprintf(stderr,"Couldn't find a Task object for the task named %s in the config file\n",tok);
		      sys.cleanUpAndExit(1);
		    }

		    curInitialTasks.insert(curTask);

		    // keep track of the tasks initially mapped to the
		    // component
		    curComponent->addInitialTask(curTask);
		  }
		  else {
		    fprintf(stderr,"Expected more initially mapped tasks, line %d\n",line_number);
		    sys.cleanUpAndExit(1);
		  }
		}

		// add the component to the initial task mapping
		sys.getInitialTaskMapping()->addSingleMapping(curComponent,curInitialTasks);

		// check that the component has enough capacity to map the task
		if (!curComponent->validateInitialMapping()) {
		    cerr << "*** Error: initial mapping exceeds component capacity, line " << line_number << endl;
		    sys.cleanUpAndExit(1);
		}
	    } // if/else
		    
	    break;
	case SECTION_PREC:
	    if (!strcmp(tok, "end")) {
		section = SECTION_NONE;
	    } else {
		// the first token identifies the failing component
		Component *failedComponent = sys.getComponent(tok);
		if (!failedComponent) {
		    fprintf(stderr, "Unrecognized initial component, line %d\n",
			    line_number);
		    sys.cleanUpAndExit(1);
		}
			
		if (mcsVerbosity> 0)
		    cout << "::: Adding preclusions for component ", failedComponent->getName();
			
		// the subsequent tokens identify precluded components
		while ((tok = strtok(NULL, TOKENS)) != NULL) {
		    Component *precludedComponent;
			    
		    //g_strdown(tok);
		    precludedComponent = sys.getComponent(tok);
		    if (!precludedComponent) {
			fprintf(stderr, "Unrecognized precluded component, line %d\n",
				line_number);
			sys.cleanUpAndExit(1);
		    }
			    
		    if (mcsVerbosity> 0)
			cout << precludedComponent->getName() << " ";
			    
		    failedComponent->addPreclusion(precludedComponent);
		} // while
			
		if (mcsVerbosity> 0)
		    cout << endl;
	    } // if/else
	    break;
		    
	default:
	    fprintf(stderr, "Undefined or invalid section type\n");
	    sys.cleanUpAndExit(1);
	    break;
	} // switch
    } // while
    
    fclose(in);
    return;
} // parse_config

void write_operating_scenarios(const char *config, System &sys) {
    FILE *configFile;
    int x;
    set<Component*,compareComponentIDs> curOperatingScenario;
    
    int mcsVerbosity = sys.getVerbosity();
    
    // sanity check
    if (config == NULL) {
    	fprintf(stderr, "Unable to read empty filename\n");
    	sys.cleanUpAndExit(1);
    } 
    
    // try to open the configuration file for appending
    if ((configFile = fopen(config, "a")) == NULL) {
    	fprintf(stderr, "Could not open file %s for append\n", config);
    	sys.cleanUpAndExit(1);
    }

    if (mcsVerbosity > 0)
    	printf("::: Appending to configuration file %s\n", config);
	
    // Print the section header to the file
    fprintf(configFile,"\n\ndefine sysactive\n");
	
    // Loop over all operating scenarios
	for(x = 0; x < (int)sys.getOperatingScenarios().size(); x++) {
		curOperatingScenario = sys.getOperatingScenarios()[x];
		
		// Loop over each component in the operatig scenario
		for(set<Component*,compareComponentIDs>::iterator iter = curOperatingScenario.begin();
			iter != curOperatingScenario.end(); iter++) {
			
			Component *c = *iter;
			fprintf(configFile,"%s ",c->getName().c_str());
		}
		fprintf(configFile,"\n");
	}
	
	fprintf(configFile,"end\n");
	fclose(configFile);
	return;
}

void parse_cmd_sys(int argc, char *argv[], System &sys) {
	int x;
	bool configFileSpecified = false;
	bool taskGraphFileSpecified = false;
	bool netlistFileSpecified = false;
	bool databaseFileSpecified = false;

	float defectDensityProc = 0;
	float defectDensityMem = 0;
	
	// Determine whether or not the command line has the correct number of parameters
	if(argc < 7) {
		cout << "Invalid command line specified...usage is as follows" << endl;
		cout << argv[0] << " -c <configFile> -n <netlistFile> -t <taskGraphFile> [-d <databaseFile>] [-u 0/1] [-b 0/1] [-i 0/1] [-z 0/1] [-I 0/1] [-s numSamples] [-f fpIterations] [-w areaWeight wireWeight] [-v verbosity] [-r numPermutations] [-y ddp ddm]" << endl;
		sys.cleanUpAndExit(1);
	}
	
	for(x = 1; x < argc; x++) {
		// -c flag specifies configuration file
		if(!strncmp("-c",argv[x],2)) {
			sys.setConfigFileName(argv[x + 1]);
			configFileSpecified = true;
		}
		
		// -t flag specifies task graph file
		if(!strncmp("-t",argv[x],2)) {
			sys.setTaskGraphFileName(argv[x + 1]);
			taskGraphFileSpecified = true;
		}
		
		// -n flag specifies netlist file
		if(!strncmp("-n",argv[x],2)) {
			sys.setNetlistFileName(argv[x + 1]);
			netlistFileSpecified = true;
		}

		// -d specifies the database file name
		if (!strncmp("-d", argv[x], 2)) {
		    sys.setDatabaseFileName(argv[x + 1]);
		    databaseFileSpecified = true;
		} // if
		
		// -s flag specifies number of samples
		if(!strncmp("-s",argv[x],2)) {
			if(atoi(argv[x + 1]) <= 0) {
				cout << "Maximum samples must be greater than 0...using default value instead" << endl;
			}
			else {
				sys.setMaxSamples(atoi(argv[x + 1]));
			}
		}

		// -f flag specifies the number of floorplanning iterations to be conducted
		if(!strncmp("-f",argv[x],2)) {
		  if(atoi(argv[x + 1]) < 0) {
		    cout << "Number of floorplanning iterations must be greater than or equal to 0...using default value instead" << endl;
		  }
		  else {
		    sys.setFPIter(atoi(argv[x + 1]));
		  }
		}				

		// -a 1 indicates that area minimization should be performed during floorplanning (no WL minimization)
		if(!strncmp("-a",argv[x],2)) {
			if((atoi(argv[x + 1]) != 0) && (atoi(argv[x + 1]) != 1)) {
				cout << "Invalid value for area minimization flag...using default value of false" << endl;
			}
			else {
				sys.setFPAreaMin((atoi(argv[x + 1]) == 1) ? true : false);
			}
		}

		// -w x y sets area weight and wire length weight during floorplanning to x and y respectively
		// only valid when -a 0, and when 0 <= x + y <= 1
		if(!strncmp("-w",argv[x],2)) {
		    float areaWeight = strtof(argv[x + 1], NULL);
		    float wireWeight = strtof(argv[x + 2], NULL);
		    
		    if (areaWeight < 0 || wireWeight < 0 || areaWeight + wireWeight > (1+1e-6)) {
			cerr << "Invalid area and/or wire weights specified: " << areaWeight << " / " << wireWeight;
			cerr << "; both must be >= 0 and their sum must be <= 1" << endl;
			sys.cleanUpAndExit(1);
		    } else {
			sys.setFPAreaWeight(areaWeight);
			sys.setFPWireWeight(wireWeight);
		    }
		}
		
		// -r flag specifies number of permutations to be tried during task re-mapping
		if(!strncmp("-r",argv[x],2)) {
		  if(atoi(argv[x + 1]) <= 0) {
		    cout << "Number of task re-mapping permutations must be greater than 0...using default value instead" << endl;
		  }
		  else {
		    sys.setMappingPermutations(atoi(argv[x + 1]));
		  }
		}		

		// -v flag specifies verbosity
		if(!strncmp("-v",argv[x],2)) {
			sys.setVerbosity(atoi(argv[x + 1]));
		}
		
		if(!strncmp("-u",argv[x],2)) {
			if((atoi(argv[x + 1]) != 0) && (atoi(argv[x + 1]) != 1)) {
				cout << "Invalid value for temperature update flag...using default value of false" << endl;
			}
			else {
				sys.setTempUpdate((atoi(argv[x + 1]) == 1) ? true : false);
			}
		}
		
		if(!strncmp("-b",argv[x],2)) {
			if((atoi(argv[x + 1]) != 0) && (atoi(argv[x + 1]) != 1)) {
				cout << "Invalid value for failure building flag...using default value of true" << endl;
			}
			else {
				sys.setBuildFailures((atoi(argv[x + 1]) == 1) ? true : false);
			}
		}

		if(!strncmp("-i",argv[x],2)) {
		  if((atoi(argv[x + 1]) != 0) && (atoi(argv[x + 1]) != 1)) {
		    cout << "Invalid value for temperature initialization flag...using default value of true" << endl;
		  }
		  else {
		    sys.setInitTemps((atoi(argv[x + 1]) == 1) ? true : false);
		  }
		}

		if(!strncmp("-z",argv[x],2)) {
		    cerr << "Set zero power flag (-z) currently unsupported" << endl;
		    sys.cleanUpAndExit(1);

		    /*
		      if((atoi(argv[x + 1]) != 0) && (atoi(argv[x + 1]) != 1)) {
		      cout << "Invalid value for component power flag...using default value of true" << endl;
		      }
		      else {
		      sys.setZeroPower((atoi(argv[x + 1]) == 1) ? true : false);
		      }
		    */
		}
		
		if(!strncmp("-I",argv[x],2)) {
		  if((atoi(argv[x + 1]) != 0) && (atoi(argv[x + 1]) != 1)) {
		    cout << "Invalid value for ideal MTTF flag...using default value of false" << endl;
		  }
		  else {
		    sys.setIdealMTTF((atoi(argv[x + 1]) == 1) ? true : false);
		  }
		}
		
		if(!strncmp("-y",argv[x],2)) {
		    defectDensityProc = strtof(argv[x + 1], NULL);
		    defectDensityMem = strtof(argv[x + 2], NULL);
		    
		    // defect density in units per mm^2
		    if(defectDensityProc < 0 || defectDensityMem < 0) {
			    cerr << "Invalid defect density, please specify a value >= 0" << endl;
			    sys.cleanUpAndExit(1);
		    }
		    else if (defectDensityProc == 0 && defectDensityMem == 0) {
			sys.setMeasureYield(false);
		    } else {
			sys.setMeasureYield(true);
		    }
		}

		if (!strncmp("-h", argv[x],2)) {
		    float r_convec = strtof(argv[x + 1], NULL);

		    if (r_convec <= 0) {
			cerr << "Invalid r_convec, please specify a value >= 0" << endl;
			sys.cleanUpAndExit(1);
		    } else {
			sys.setExternalRConvec(true);
			sys.setRConvec(r_convec);
		    }
		}
	}
	
	if(!(configFileSpecified && taskGraphFileSpecified && netlistFileSpecified)) {
		cout << "Missing the following required arguments: ";
		if(!configFileSpecified) {
			cout << "-c <configFile> ";
		}
		if(!taskGraphFileSpecified) {
			cout << "-t <taskGraphFile> ";
		}
		if(!netlistFileSpecified) {
			cout << "-n <netlistFile>";
		}
		
		cout << endl;
		sys.cleanUpAndExit(1);
	}

	// Cannot have flags set which indicate ideal MTTF calculation and temperature updating
	if(sys.getIdealMTTF() && sys.getTempUpdate()) {
	  cout << "Cannot calculate ideal MTTF and update temperatures..change at least one of the -u or -I flags" << endl;
	  sys.cleanUpAndExit(1);
	}

	// Read the task graph information out of the file
	// This must be done before the configuration file is read so that Task object exist
	sys.storeTaskGraph();

	// parse configuration file
	parse_config(sys.getConfigFileName().c_str(), sys);
	sys.sortComponents();

	// Read the netlist information out of the file
	sys.storeNetlist();

	// if a database was provided, read it
	if (databaseFileSpecified)
	    sys.storeDatabase();

	// if a defect density was specified, set it
	if (sys.getMeasureYield())
	    sys.setDefectDensity(defectDensityProc, defectDensityMem);

#ifdef STATS
	// now that the system is configured, initialize statistics objects
	sys.initializeSTATS();
#endif
	
	return;
} // parse_cmd_sys

void run_single_HotSpot_simulation(System *sys, int pos, float maxDimension, float r_convec)
{
    stringstream ss;
    string floorplanFileName, powerTraceFileName, outputFileName;

    ss << sys->getWorkingDirectory() << FPPATH << FP_BASENAME << ".pl.filled";
    floorplanFileName = ss.str();
    ss.clear();
    ss.str("");
    
    ss << sys->getWorkingDirectory() << HSPATH << pos << ".ptrace";
    powerTraceFileName = ss.str();
    ss.clear();
    ss.str("");

    ss << sys->getWorkingDirectory() << HSPATH << pos << ".temp";
    outputFileName = ss.str();
    
    // Allocate space for the floorplan file name to be passed to HotSpot (8 -> "filled." + NULL)
    //char *floorplanFileName = (char *)malloc((8 + strlen(sys->getFloorplanFileName().c_str())) * sizeof(char));
    //sprintf(floorplanFileName,"%s.filled",sys->getFloorplanFileName().c_str());
    
    // Allocate space for the power trace file name to be passed to HotSpot (14 -> ".#####.ptrace + NULL)
    //char *powerTraceFileName = (char *)malloc((14 + strlen(sys->getConfigFileName().c_str())) * sizeof(char));
    //sprintf(powerTraceFileName,"%s.%05d.ptrace",sys->getConfigFileName().c_str(),pos);
    
    // Allocate space for the output file name to be passed to HotSpot (12 -> ".#####.temp + NULL)
    //char *outputFileName = (char *)malloc((12 + strlen(sys->getConfigFileName().c_str())) * sizeof(char));
    //sprintf(outputFileName,"%s.%05d.temp",sys->getConfigFileName().c_str(),pos);
    
    hotSpot_main(
		 floorplanFileName.c_str(),
		 powerTraceFileName.c_str(),
		 outputFileName.c_str(),
		 maxDimension,
		 r_convec
		 );

    //free(floorplanFileName);
    //free(powerTraceFileName);
    //free(outputFileName);
    
    return;
}
