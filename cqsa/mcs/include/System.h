/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#ifndef SYSTEM_H_
#define SYSTEM_H_

#include <set>
#include <map>
#include <vector>
#include <string>

#include "ComponentLibrary.h"
#include "TaskMapping.h"
#include "Component.h"
#include "ComponentNet.h"
#include "TaskNet.h"
#include "Task.h"
#include "blockFiller.h"

using namespace std;

// caching settings
#define CACHE_OPSCN
//#define CACHE_OPSCN_LIMIT 0
#define CACHE_FAILSCN
#define CACHE_FAILSCN_SORTED
//#define CACHE_FAILSCN_LIMIT 0

// enhanced statistical output
//#define STATS

#ifdef STATS
// number of failure mechanisms
#define NFM (3)
// stats vector locations for each failure mechanism
#define EMFM (0)
#define TDDBFM (1)
#define TCFM (2)
#endif

#define N_SAMPLES 10000
#define N_PERMUTATIONS 500
#define N_FAILUREMECHANISMS 1
#define LN_R_SEED 0

//#define BASE_PROC ARM9
//#define BASE_MEM MEM1MB

// limit imposed on communication across links
#define BANDWIDTH_LIMIT (256*1024) // in kB/s
// actual limit given 100 MHz network clock, 1 4B flit per cycle, and saturation throughput of 0.838861
#define HARD_BANDWIDTH_LIMIT (312500) // in kB/s

#define WDPATH "/var/tmp/"
#define FPPATH "fp/"
#define HSPATH "hotSpot/"

#define FP_BASENAME "ParquetFP"
#define FP_ITER 100  // # of iterations
//#define FP_ITER 1
#define FP_WW 0.4    // wire weight in objective function
#define FP_AW 0.4    // area weight in objective function
//#define FP_TI 1e38   // initial temperature
#define FP_TI 3e5    // initial temperature

#define HS_MINRC 8;
#define HS_DEFRC 12;
#define HS_MAXRC 16;
#define HS_CHART 345;
#define HS_ZERO  1e-1;

// yield definitions
#define Y0 0.98

#define ABS(x) ((x) > 0 ? (x) : -(x))

struct ltstr
{
  bool operator()(const string s1, const string s2) const
  {
    return s1 < s2;
  }
};

typedef enum {TG_SECTION_NONE,
	      TG_SECTION_COMP,
	      TG_SECTION_COMM} TG_SECTIONTYPE;

class System
{
	// log-normal random number generator
	gsl_rng *rand_ln;
	
	// system components
	vector<Component*> components;

	// keeps track of the ID that will be assigned to the component that is created next
	int nextComponentID;

	// all valid permutations given initial configuration
	vector<vector<componentType> > processorPermutations;
	int initialProcessorCapacity;
	vector<vector<componentType> > memoryPermutations;
	int initialMemoryCapacity;

	// memories are not allowed to be allocated redundacy beyond this level
	componentType maxMemoryType;
	
	// system parameters
	int mcsVerbosity;
	bool tempUpdate;
	bool initTemps;
	bool zeroPower;
	bool idealMTTF;
	bool buildFailures;
	bool measureYield;           // false if MCS samples lifetime (default), true if MCS samples yield
	int maxLinkBandwidth;
	int mappingPermutations;
	int fpIter;
	bool fpAreaMin;
	float fpAreaWeight;
	float fpWireWeight;
	
	string workingDirectory; 	// working directory where temporary files are stored
	
	// component library
	componentLibraryEntry componentLibrary[N_COMPONENT_TYPES];
	
	set<Component*,compareComponentIDs> failedComponents;
	vector<set<Component*,compareComponentIDs> > failureScenarios;
	vector<set<Component*,compareComponentIDs> > allScenarios;
	
	// Parallel vectors for the operating scenarios and their corresponding task mappings
	map<const string,int,ltstr> operatingScenarioLookup;
	vector<set<Component*,compareComponentIDs> > operatingScenarios;
	vector<TaskMapping*> taskMappings;
	
	// Initial task mapping from the config file
	TaskMapping *initialTaskMapping;

	vector<ComponentNet*> netlist;
	vector<TaskNet*> taskGraph;
	vector<Task*> tasks;
	
	vector<string> emptyBlocks;
	
	// Indicates whether or not operating scenarios were found in the config file
	bool operatingScenariosFound;
	
	// system failure statistics
	int n_samples;			// number of samples generated
	int m_samples;			// maximum number of samples to generate
	float moment1;			// first moment
	float moment2;			// second moment

#ifdef STATS
	// store the average time of the first failure
	float statsFirstFailure;
	// store how many components fail (not including preclusions)
	int statsNFailures;
	
	// store the initial power of each component
	vector<float> statsPower;
	// store the initial temperature of each component
	vector<float> statsTemp;
	
	// store how many times each component was the first to fail
	vector<int> statsFirstFailed;
	// ... and what caused it to fail
	vector<vector<int> > statsFirstFailedMech;

	// store how many times each component failed (not including preclusions)
	vector<int> statsFailed;
	// ... and what caused it to fail
	vector<vector<int> > statsFailedMech;
#endif
	
	// results data bases
	map<string,float> RAtoMTTF;
	map<string,pair<float,float> > RAtoAreaWL;
	map<string,bool> RAExpl;
	
	// input files
	string configFileName;		// Configuration file name
	string floorplanFileName;	// Floorplan file name
	string taskGraphFileName;	// Task graph file name
	string netlistFileName;		// Netlist file name
	string databaseFileName;        // Result database file name
	
	// floorplanning parameters
	float xsize;                    // system x dimension
	float ysize;                    // system y dimension
	float area;                     // system area
	float wl;                       // system wire length

	// hotSpot parameters
	float r_convec;                 // convection resistance
	bool externalRConvec;           // true when r_convec is provided by the cmd line
	
	// indicates whether or not the initial component temps have been found
	bool initialTempsFound;
public:
	// constructor/destructor
	System();
	virtual ~System();

	// exit, cleaning up
	void cleanUpAndExit(int status);
	
	// calibrate system convection resistance
	float averageComponentTemperature();
	void calibrateRConvec(int taskmapping);
	float getRConvec() const { return r_convec; }
	void setRConvec(float rc) { r_convec = rc; }
	bool getExternalRCConvec() const { return externalRConvec; }
	void setExternalRConvec(bool extRC) { externalRConvec = true; }
	
	// set working directory
	void removeWorkingDirectory();
	void setWorkingDirectory();
	string getWorkingDirectory() const { return workingDirectory; }
		
	// get system parameters
	int getVerbosity() const { return mcsVerbosity; }
	int getTempUpdate() const { return tempUpdate; }
	bool getInitTemps() const { return initTemps; }
	bool getZeroPower() const { return zeroPower; }
	bool getIdealMTTF() const { return idealMTTF; }
	bool getMeasureYield() const { return measureYield; }
	int getBuildFailures() const { return buildFailures; }
	int getMaxLinkBandwidth() const { return maxLinkBandwidth; }
	int getFPIter() const { return fpIter; }
	bool getFPAreaMin() const { return fpAreaMin; }
	float getFPAreaWeight() const { return fpAreaWeight; }
	float getFPWireWeight() const { return fpWireWeight; }
	componentType getMaxMemoryType() { return maxMemoryType; }
	
	// set system parameters
	void incNSamples() { n_samples++; }
	
	void setVerbosity(int verbosity) { mcsVerbosity = verbosity; }
	void setTempUpdate(bool update) { tempUpdate = update; }
	void setInitTemps(bool tempInit) { initTemps = tempInit; }
	void setZeroPower(bool zPower) { zeroPower = zPower; }
	void setIdealMTTF(bool iMTTF) { idealMTTF = iMTTF; }
	void setMeasureYield(bool yield) { measureYield = yield; }
	void setBuildFailures(bool build) { buildFailures = build; }
	void setMaxLinkBandwidth(int limit) { maxLinkBandwidth = limit; }
	void setMaxMemoryType(componentType cType) { maxMemoryType = cType; }
	
	// sets the defect density of all components
	void setDefectDensity(float defectDensityProc, float defectDensityMem); 
	void setFPIter(int iterations) { fpIter = iterations; }
	void setFPAreaMin(bool min) { fpAreaMin = min; }
	void setFPAreaWeight(float aw) { fpAreaWeight = aw; }
	void setFPWireWeight(float ww) { fpWireWeight = ww; }
	
	// get various data structures
	vector<Component*> getComponents();		// get the vector of all components
	vector<Component*> getProcessors();		// get the vector of all processors
	vector<Component*> getMemories();		// get the vector of all memories
	
	vector<Component*> getProcessorsRedundantCapacity();	// get vector of processors with redundancy
	vector<Component*> getProcessorsNoRedundantCapacity();	// get vector of processors without redundancy
	vector<Component*> getMemoriesRedundantCapacity();	// get vector of memories with redundancy
	vector<Component*> getMemoriesNoRedundantCapacity();	// get vector of memories without redundancy

	gsl_rng *getRNG() { return rand_ln; }
	componentLibraryEntry *getComponentLibrary() { return componentLibrary; }
	
	// manipulate components
	Component *addComponent(string cname, componentType type);
	Component *getComponent(string cname); 	 // find a component by name
	void sortComponents();                   // sort components by name
	void allocateRedundancy(Component *c);	 // change component type to redundant component type
	void deallocateRedundancy(Component *c); // change component type to un-redundant component type
	// reset given components to initial types
	void clearRedundancies(vector<Component*> selectedComponents);
	void clearAllRedundancies();             // reset all component types
	void printComponentNames();
	void printComponentTypes();
	void printComponentCapacities();
	
	// manipulate processor and memory permutations
	void printPermutation(vector<componentType> permutation); // prints the component types in the given permutation
	
	vector<componentType> buildProcessorPermutation();        // returns current processor permutation
	void buildProcessorPermutations();                        // builds all processor permutations
	// returns permutations with given amount of execution slack
	vector<vector<componentType> > getProcessorPermutations(int execSlack);
	// returns permutations that differ from the current system by a single allocation to a single processor
	vector<vector<componentType> > findIncrementalProcessorPermutations();
	// returns permutations from the given vector that monotonically increase the slack at each component
	vector<vector<componentType> > findIncrementalProcessorPermutations(vector<vector<componentType> > permutations);
	void applyProcessorPermutation(vector<componentType>);    // applies given permutation

	vector<componentType> buildMemoryPermutation();      // returns current memory permutation
	void buildMemoryPermutations();                           // builds all memory permutations
	// returns permutations with given amount of storage slack
	vector<vector<componentType> > getMemoryPermutations(int storSlack);
	// returns permutations from the given vector that monotonically increase the slack at each component
	vector<vector<componentType> > findIncrementalMemoryPermutations(vector<vector<componentType> > permutations);
	void applyMemoryPermutation(vector<componentType>);       // applies given permutation

	int permutationCapacity(vector<componentType> permutation);  // returns the total capacity of the given permutation
	int getProcessorAllocationLevel();                           // returns current execution slack allocation level
	vector<int> getProcessorAllocationLevels();                  // returns all possible allocations of execution slack
	int getMemoryAllocationLevel();                              // returns current storage slack allocation level
	vector<int> getMemoryAllocationLevels();                     // returns all possible allocations of memory slack
	    
	// manipulate empty blocks
	void addEmptyBlock(string emptyBlockName);  // add the name of an empty block
	vector<string> getEmptyBlocks();	    // get the vector of all empty blocks
	void clearEmptyBlocks();                    // clear the empty block list

	// manipulate scenarios
	// get the list of operating scenarios
	vector<set<Component*,compareComponentIDs> > getOperatingScenarios() { return operatingScenarios; }	
	// get the list of all scenarios
	vector<set<Component*,compareComponentIDs> > getAllScenarios() { return allScenarios; }
	void buildAllScenarios(); 	     // build the list of all possible scenarios
	void buildAllScenariosOrdered();     // build the list of all possible scenarios in order (fewest component failures to most)
	void buildFailureScenarios();        // build the list of failure scenarios
	void clearFailureScenarios();        // clear the list of failure scenarios
	void clearOperatingScenarios();      // clear the list of operating scenarios
	void clearOperatingScenarioLookup(); // clear the operating scenario lookup map
	void setOperatingScenariosFound();   // set the value of operatingScenariosFound
	bool getOperatingScenariosFound() { return operatingScenariosFound; }	// get the value of operatingScenariosFOund
	void writeOperatingScenario(int pos);// write a particular operating scenario to the config file
	
	// manipulate task mappings
	vector<TaskMapping*> getTaskMappings() { return taskMappings; }      // get list of task mappings
	TaskMapping * getInitialTaskMapping() { return initialTaskMapping; } // get the initial task mapping
	void clearTaskMappings();                                            // clear the list of task mappings    
	//void createTaskMappings(); 	                                     // create a task mapping for each operating scneario
	void createSingleTaskMapping(int pos);	                             // create a task mapping for a particular operating scenario

	// manipulate tasks
	vector<TaskNet*> getTaskGraph() { return taskGraph; }  // get the task graph
	void addTask(Task *t); 	                               // add a task
	vector<Task *> getTasks() { return tasks; }	       // get the vector of all task objects
	Task * getTask(string n); 	                       // get a single task object by name
	void storeTaskGraph();                                 // create an internal representation of the task graph
	
	// reset the available redundancy of each component in the system
	void resetAvailableCapacitiesToInitial();
	void resetAvailableCapacitiesToPartial();

	// reset the partial redundancy of each component in the system
	void resetPartialCapacitiesToInitial();

	// manipulate the netlist
	vector<ComponentNet*> getNetlist() { return netlist; }
	void storeNetlist(); 	// create an internal representation of the netlist
	vector<ComponentNet*> copyNetlist(); 	// make a copy of the netlist
	
	// mapping permutations accessors
	int getMappingPermutations() const { return mappingPermutations; }
	void setMappingPermutations(int p) { mappingPermutations = p; }

	// sample generation and support functions
	int getMaxSamples() const; 	        // get max samples
	void setMaxSamples(int samples); 	// set max samples
	void resolvePreclusions(Component *c); 	// propagate failure to preclusions
	bool resolveSystemFailure(); 	        // determine if system is failed
	void updateStatistics(float data); 	// update statistics
	void sample();                  	// generate a sample
	// resets and generates maxSamples, using db'd results if available
	// returns true if system was found in the database, false otherwise
	bool samplingRun();
	// reset the system-- rebuilds operating and failure scenarios, forces floorplanning
	void reset(); 
	// builds the allscenarios list
	void initialize(); 
	
	// report statistics
	int getNSamples() const;
	float getConfidence() const;
	float getMTTF() const;
	float getSVar() const;
	float getVar() const;
	    
	// set statistics (used when the system returns to a cached state and MCS 
	// is not evaluated)
	void setMTTF(float mttf);
	
	// reset statistics
	void resetStatistics();

	// manipulate results database
	void storeDatabase();
	string buildKey();
	bool lookupExpl(string key);
	float lookupMTTF(string key);
	pair<float, float> lookupAreaWL(string key);
	void setExpl(string key);
	void setMTTF(string key, float val);
	void setAreaWL(string key, float area, float wl);
	
	// manipulate file names for input files
	void setConfigFileName(string fileName) { configFileName = fileName; }
	string getConfigFileName() { return configFileName; }
	void setFloorplanFileName(string fileName) { floorplanFileName = fileName; }
	string getFloorplanFileName() { return floorplanFileName; }
	void setTaskGraphFileName(string fileName) { taskGraphFileName = fileName; }
	string getTaskGraphFileName() { return taskGraphFileName; }
	void setNetlistFileName(string fileName) { netlistFileName = fileName; }
	string getNetlistFileName() { return netlistFileName; }
	void setDatabaseFileName(string fileName) { databaseFileName = fileName; }
	string getDatabaseFileName() { return databaseFileName; }
	
	// Gets the component temperatures for a fully operating system
	// or resets each component temperature to that value
	void setInitialComponentTemps();

	bool getInitialTempsFound() { return initialTempsFound; }
	void resetInitialTempsFound() { initialTempsFound = false; }
	
	// system floorplanning interface
	void writeBlocks(const string basename); // generate ParquetFP blocks
	void writeNets(const string basename);   // generate ParquetFP nets
	void floorplan();	                 // invoke ParquetFP
	float getArea() const { return area; }	 // get area and wire length
	float getWL() const { return wl; }       // get wire length
        void setArea(float a) { area = a; }      // set area (when previously generated)
	void setWL(float l) { wl = l; }          // set wire length (when previous generated)

#ifdef STATS
	// system statistics interface
	void initializeSTATS();
	void updateSTATS(Component *failed, bool first);
	void updateNFailureSTATS(int n_failed);
	void printSTATS();
#endif
	
 private:
	// Set each component temperature according to the given operating scenario
	void readTempsFromFile(int operatingScenarioIndex);

	// Set each component power according to the given operating scenario
	void updatePowerValues(int operatingScenarioIndex);

	string buildOperatingScenarioKey(set<Component*,compareComponentIDs> scenario);

	// Return the next unique component ID
	int getNextComponentID();
};

bool compareNumFailedComponents(const set<Component*,compareComponentIDs> a, const set<Component*,compareComponentIDs> b);
long long factorial(long long x);
vector<int> generateNextPermutation(vector<int> initialSeq);

#endif /*SYSTEM_H_*/
