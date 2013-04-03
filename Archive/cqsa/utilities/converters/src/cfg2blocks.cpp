#include <iostream>

#include "config.h"
#include "System.h"

void parse(int argc, char* argv[], System &sys) {
    if (argc < 4) {
	cerr << "Usage: " << argv[0] << " <input cfg file> <input tg file> <output basename>" << endl;
	exit(1);
    }

    // store and parse the task graph file (not needed to generate
    // blocks, but needed to avoid writing new code to parse config
    // files)
    sys.setTaskGraphFileName(argv[2]);
    sys.storeTaskGraph();
    
    // store and parse the config file
    sys.setConfigFileName(argv[1]);
    parse_config(sys.getConfigFileName().c_str(), sys);
    sys.sortComponents();
} // parse

int main(int argc, char* argv[]) {
    System sys;

    // initialize the component library
    initializeComponentLibrary(sys.getComponentLibrary());
    
    // parse input arguments
    parse(argc, argv, sys);

    // write output
    sys.writeBlocks(argv[3]);
} // main

