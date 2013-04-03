/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#ifndef COMPONENTLIBRARY_H_
#define COMPONENTLIBRARY_H_

#include <string>

using namespace std;

// Total number of component types
#define N_COMPONENT_TYPES 15

// Total number of general component types (which may require remapping)
#define N_G_COMPONENT_TYPES 2

// Strings representing each component type in a .config file
#define M3_STRING "M3"
#define M3_STRING_LENGTH 2

#define ARM9_STRING "ARM9"
#define ARM9_STRING_LENGTH 4

#define ARM11_STRING "ARM11"
#define ARM11_STRING_LENGTH 5

#define MEM64KB_STRING "MEM64KB"
#define MEM64KB_STRING_LENGTH 7

#define MEM96KB_STRING "MEM96KB"
#define MEM96KB_STRING_LENGTH 7

#define MEM128KB_STRING "MEM128KB"
#define MEM128KB_STRING_LENGTH 8

#define MEM192KB_STRING "MEM192KB"
#define MEM192KB_STRING_LENGTH 8

#define MEM256KB_STRING "MEM256KB"
#define MEM256KB_STRING_LENGTH 8

#define MEM384KB_STRING "MEM384KB"
#define MEM384KB_STRING_LENGTH 8

#define MEM512KB_STRING "MEM512KB"
#define MEM512KB_STRING_LENGTH 8

#define MEM1MB_STRING "MEM1MB"
#define MEM1MB_STRING_LENGTH 6

#define MEM2MB_STRING "MEM2MB"
#define MEM2MB_STRING_LENGTH 6

#define SW3X3_STRING "SW3X3"
#define SW3X3_STRING_LENGTH 5

#define SW4X4_STRING "SW4X4"
#define SW4X4_STRING_LENGTH 5

#define SW5X5_STRING "SW5X5"
#define SW5X5_STRING_LENGTH 5

// Enumeration of general component types
// All types which may require remapping must be listed before those
// not requiring remapping
typedef enum {
    PROC = 0,
    MEM,
    SW,
    NO_TYPE
} generalComponentType;

// Enumeration of specific component types
typedef enum {
    NONE = -1,
    M3,
    ARM9,
    ARM11,
    MEM64KB,
    MEM96KB,
    MEM128KB,
    MEM192KB,
    MEM256KB,
    MEM384KB,
    MEM512KB,
    MEM1MB,
    MEM2MB,
    SW3X3,
    SW4X4,
    SW5X5
} componentType;

// Structure to hold the properties of each component
typedef struct componentLibraryEntry {
    float vdd;    // volts

    float MIPPower;    // power per MIP for processors-- watts
    float readEnergy;  // energy per read for memories-- joules
    float writeEnergy; // energy per write for memories-- joules
    float staticPower; // leakage for memories-- watts

    float height; // millimeters
    float width;  // millimeters
    float criticalFraction; // percent
    float Aem;
    generalComponentType gType;
    int capacity; // MIPS for processors, KBs for memories, meaningless for switches
    componentType aType; // component this component becomes when allocated redundancy
    componentType dType; // component this component becomes when deallocated redundancy
    int inputs;
} componentLibraryEntry;

// given a string, returns the corresponding component type
componentType stringToComponentType(string name);

// given a component type, returns a string
string componentTypeToString(componentType type);

// given a component type, returns the full capacity of that type
int componentTypeToCapacity(componentType type);

// Sets the values of each property for each component in the library
void initializeComponentLibrary(componentLibraryEntry *componentLibrary);

#endif
