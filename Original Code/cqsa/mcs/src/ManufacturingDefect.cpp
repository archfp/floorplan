/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#include <cmath>
#include <iostream>
#include <string>

#include "Component.h"
#include "ManufacturingDefect.h"

using namespace std;

ManufacturingDefect::ManufacturingDefect(string dname, Component *downer, gsl_rng *rng) {
    // set parameters
    owner = downer;
    rand_u = rng;
    name.append(dname);

    // initialize parameter values
    defect = false;
    defectDensity = DEFECTDENSITY;
}

void ManufacturingDefect::initialize() {
    // calculate average yield
    yield = (float) pow((double) (1 + owner->getArea() * owner->getCriticalFraction() * defectDensity), (double) -14.5);
    //cout << "@@@ " << owner->getName() << ": yield for defect >" << name << "< is " << yield << "%" << endl;
}

bool ManufacturingDefect::sample() {
    // determine whether component is defective
    float d = (float) gsl_rng_uniform(rand_u);

    if (d > yield) {
	defect = true;
	//cout << "@@@ " << owner->getName() << ": DEFECT due to >" << name << "< (avg yield: " << yield << ")" << endl;
    } else {
	defect = false;
	//cout << "@@@ " << owner->getName() << ": no defect due to >" << name << "< " << endl;
    }

    return defect;
} 
