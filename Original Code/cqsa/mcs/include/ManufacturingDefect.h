/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#ifndef MANUFACTURINGDEFECT_H_
#define MANUFACTURINGDEFECT_H_

#include <gsl/gsl_rng.h>
#include <string>

#include "types.h"

#define DEFECTDENSITY 0.163 // number of defects per cm^2
//#define DEFECTDENSITY 1.63 // number of defects per cm^2

using namespace std;

class ManufacturingDefect
{
    string name;            // manufacturing defect name
    Component *owner;       // Component to which this defect model belongs
    gsl_rng *rand_u;        // uniform random number generator
    
    // failure parameters
    float yield;            // component average yield based on defect density and area
    bool defect;            // true if there is a defect
    
    // defect density parameters
    float defectDensity;    // defect density in cm^2

 public:
    ManufacturingDefect(string dname, Component *downer, gsl_rng *rng);
    
    // get various parameters
    string getName() { return name; }
    Component *getOwner() { return owner; }
    bool getDefect() { return defect; }
    
    // set various parametrs
    void setDefectDensity(float density) { defectDensity = density; }

    void initialize();       // initialize expected yield
    bool sample();           // generate a random defect sample
};

#endif /*MANUFACTURINGDEFECT_H_*/
