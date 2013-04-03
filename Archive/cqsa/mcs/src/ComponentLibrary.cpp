/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#include <string>

#include "ComponentLibrary.h"

using namespace std;

// Aem = MTTF / ((power in W / vdd in V) / area in cm^2)^-1.1 * exp(0.9 / (8.62e-5 * nominal T in K))

componentType stringToComponentType(string name) {
    if (0 == name.compare(M3_STRING)) {
	// M3
	return M3;
    } else if(0 == name.compare(ARM9_STRING)) {
	// ARM9
	return ARM9;
    } else if(0 == name.compare(ARM11_STRING)) {
	// ARM11
	return ARM11;
    } else if(0 == name.compare(MEM64KB_STRING)) {
	// MEM64KB
	return MEM64KB;
    } else if(0 == name.compare(MEM96KB_STRING)) {
	// MEM96KB
	return MEM96KB;
    } else if(0 == name.compare(MEM128KB_STRING)) {
	// MEM128KB
	return MEM128KB;
    } else if(0 == name.compare(MEM192KB_STRING)) {
	// MEM192KB
	return MEM192KB;
    } else if(0 == name.compare(MEM256KB_STRING)) {
	// MEM256KB
	return MEM256KB;
    } else if(0 == name.compare(MEM384KB_STRING)) {
	// MEM384KB
	return MEM384KB;
    } else if(0 == name.compare(MEM512KB_STRING)) {
	// MEM512KB
	return MEM512KB;
    } else if(0 == name.compare(MEM1MB_STRING)) {
	// MEM1MB
	return MEM1MB;
    } else if(0 == name.compare(MEM2MB_STRING)) {
	// MEM2MB
	return MEM2MB;
    } else if(0 == name.compare(SW3X3_STRING)) {
	// SW3X3
	return SW3X3;
    } else if(0 == name.compare(SW4X4_STRING)) {
	// SW4X4
	return SW4X4;
    } else if(0 == name.compare(SW5X5_STRING)) {
	// SW5X5
	return SW5X5;
    } else {
	return NONE;
    }
} // stringToComponentType

string componentTypeToString(componentType type) {
    string str;

    switch (type) {
    case M3:
	str = "M3";
	break;
    case ARM9:
	str = "ARM9";
	break;
    case ARM11:
	str = "ARM11";
	break;
    case MEM64KB:
	str = "MEM64KB";
	break;
    case MEM96KB:
	str = "MEM96KB";
	break;
    case MEM128KB:
	str = "MEM128KB";
	break;
    case MEM192KB:
	str = "MEM192KB";
	break;
    case MEM256KB:
	str = "MEM256KB";
	break;
    case MEM384KB:
	str = "MEM384KB";
	break;
    case MEM512KB:
	str = "MEM512KB";
	break;
    case MEM1MB:
	str = "MEM1MB";
	break;
    case MEM2MB:
	str = "MEM2MB";
	break;
    case SW3X3:
	str = "SW3X3";
	break;
    case SW4X4:
	str = "SW4X4";
	break;
    case SW5X5:
	str = "SW5X5";
	break;
    case NONE:
	str = "";
	break;
    }

    return str;
} // componentTypeToString

int componentTypeToCapacity(componentType type) {
    switch (type) {
    case M3:
	return 125;
    case ARM9:
	return 250;
    case ARM11:
	return 500;
    case MEM64KB:
	return 64;
    case MEM96KB:
	return 96;
    case MEM128KB:
	return 128;
    case MEM192KB:
	return 192;
    case MEM256KB:
	return 256;
    case MEM384KB:
	return 384;
    case MEM512KB:
	return 512;
    case MEM1MB:
	return 1024;
    case MEM2MB:
	return 2048;
    default:
	return 0;
    }
} // componentTypeToCapacity

void initializeComponentLibrary(componentLibraryEntry *componentLibrary) {
	// M3
	componentLibrary[M3].vdd = 1;
	componentLibrary[M3].MIPPower = (12.5e-3/125);
	componentLibrary[M3].height = 0.781;
	componentLibrary[M3].width = 0.781;
	componentLibrary[M3].criticalFraction = 1;
	componentLibrary[M3].Aem = 4.75e-12;
	componentLibrary[M3].gType = PROC;
	componentLibrary[M3].capacity = 125;
	componentLibrary[M3].aType = ARM9;
	componentLibrary[M3].dType = NONE;
	componentLibrary[M3].inputs = -1;

	// ARM9
	componentLibrary[ARM9].vdd = 1;
	componentLibrary[ARM9].MIPPower = (42.5e-3/250);
	componentLibrary[ARM9].height = 1;
	componentLibrary[ARM9].width = 1;
	componentLibrary[ARM9].criticalFraction = 1;
	componentLibrary[ARM9].Aem = 1.06e-11;
	componentLibrary[ARM9].gType = PROC;
	componentLibrary[ARM9].capacity = 250;
	componentLibrary[ARM9].aType = ARM11;
	componentLibrary[ARM9].dType = M3;
	componentLibrary[ARM9].inputs = -1;
	
	// ARM11
	componentLibrary[ARM11].vdd = 1;
	componentLibrary[ARM11].MIPPower = (255e-3/500);
	componentLibrary[ARM11].height = 1.55;
	componentLibrary[ARM11].width = 1.55;
	componentLibrary[ARM11].criticalFraction = 1;
	componentLibrary[ARM11].Aem = 2.90e-11;
	componentLibrary[ARM11].gType = PROC; 
	componentLibrary[ARM11].capacity = 500;
	componentLibrary[ARM11].aType = NONE;
	componentLibrary[ARM11].dType = ARM9;
	componentLibrary[ARM11].inputs = -1;

	// 64KB memory (CACTI 5.3)
	componentLibrary[MEM64KB].vdd = 1.3;
	componentLibrary[MEM64KB].readEnergy = 0.143e-9;
	componentLibrary[MEM64KB].writeEnergy = 0.113e-9;
	componentLibrary[MEM64KB].staticPower = 0.00915e-3;
	componentLibrary[MEM64KB].height = 1.178;
	componentLibrary[MEM64KB].width = 0.863;
	componentLibrary[MEM64KB].criticalFraction = 1;
	componentLibrary[MEM64KB].Aem = 6.28E-13;
	componentLibrary[MEM64KB].gType = MEM;
	componentLibrary[MEM64KB].capacity = 64;
	componentLibrary[MEM64KB].aType = MEM96KB;
	componentLibrary[MEM64KB].dType = NONE;
	componentLibrary[MEM64KB].inputs = -1; 

	// 96KB memory (CACTI 5.3)
	componentLibrary[MEM96KB].vdd = 1.3;
	componentLibrary[MEM96KB].readEnergy = 0.121e-9;
	componentLibrary[MEM96KB].writeEnergy = 0.098e-9;
	componentLibrary[MEM96KB].staticPower = 0.0137e-3;
	componentLibrary[MEM96KB].height = 1.178;
	componentLibrary[MEM96KB].width = 1.280;
	componentLibrary[MEM96KB].criticalFraction = 1;
	componentLibrary[MEM96KB].Aem = 3.43E-13;
	componentLibrary[MEM96KB].gType = MEM;
	componentLibrary[MEM96KB].capacity = 96;
	componentLibrary[MEM96KB].aType = MEM128KB;
	componentLibrary[MEM96KB].dType = MEM64KB;
	componentLibrary[MEM96KB].inputs = -1; 
	
	// 128KB memory (CACTI 5.3)
	componentLibrary[MEM128KB].vdd = 1.3;
	componentLibrary[MEM128KB].readEnergy = 0.143e-9;
	componentLibrary[MEM128KB].writeEnergy = 0.113e-9;
	componentLibrary[MEM128KB].staticPower = 0.0183e-3;
	componentLibrary[MEM128KB].height = 1.178;
	componentLibrary[MEM128KB].width = 1.618;
	componentLibrary[MEM128KB].criticalFraction = 1;
	componentLibrary[MEM128KB].Aem = 3.15E-13;
	componentLibrary[MEM128KB].gType = MEM;
	componentLibrary[MEM128KB].capacity = 128;
	componentLibrary[MEM128KB].aType = MEM192KB;
	componentLibrary[MEM128KB].dType = MEM96KB;
	componentLibrary[MEM128KB].inputs = -1;

	// 192KB memory (CACTI 5.3)
	componentLibrary[MEM192KB].vdd = 1.3;
	componentLibrary[MEM192KB].readEnergy = 0.216e-9;
	componentLibrary[MEM192KB].writeEnergy = 0.146e-9;
	componentLibrary[MEM192KB].staticPower = 0.0263e-3;
	componentLibrary[MEM192KB].height = 2.259;
	componentLibrary[MEM192KB].width = 1.278;
	componentLibrary[MEM192KB].criticalFraction = 1;
	componentLibrary[MEM192KB].Aem = 2.92E-13;
	componentLibrary[MEM192KB].gType = MEM;
	componentLibrary[MEM192KB].capacity = 192;
	componentLibrary[MEM192KB].aType = MEM256KB;
	componentLibrary[MEM192KB].dType = MEM128KB;
	componentLibrary[MEM192KB].inputs = -1;
	
	// 256KB memory (CACTI 5.3)
	componentLibrary[MEM256KB].vdd = 1.3;
	componentLibrary[MEM256KB].readEnergy = 0.245e-9;
	componentLibrary[MEM256KB].writeEnergy = 0.158e-9;
	componentLibrary[MEM256KB].staticPower = 0.0350e-3;
	componentLibrary[MEM256KB].height = 2.259;
	componentLibrary[MEM256KB].width = 1.627;
	componentLibrary[MEM256KB].criticalFraction = 1;
	componentLibrary[MEM256KB].Aem = 2.53E-13;
	componentLibrary[MEM256KB].gType = MEM;
	componentLibrary[MEM256KB].capacity = 256;
	componentLibrary[MEM256KB].aType = MEM384KB;
	componentLibrary[MEM256KB].dType = MEM192KB;
	componentLibrary[MEM256KB].inputs = -1;

	// 384KB memory (CACTI 5.3)
	componentLibrary[MEM384KB].vdd = 1.3;
	componentLibrary[MEM384KB].readEnergy = 0.316e-9;
	componentLibrary[MEM384KB].writeEnergy = 0.188e-9;
	componentLibrary[MEM384KB].staticPower = 0.0523e-3;
	componentLibrary[MEM384KB].height = 2.259;
	componentLibrary[MEM384KB].width = 2.301;
	componentLibrary[MEM384KB].criticalFraction = 1;
	componentLibrary[MEM384KB].Aem = 2.21E-13;
	componentLibrary[MEM384KB].gType = MEM;
	componentLibrary[MEM384KB].capacity = 384;
	componentLibrary[MEM384KB].aType = MEM512KB;
	componentLibrary[MEM384KB].dType = MEM256KB;
	componentLibrary[MEM384KB].inputs = -1;	

	// 512KB memory (CACTI 5.3)
	componentLibrary[MEM512KB].vdd = 1.3;
	componentLibrary[MEM512KB].readEnergy = 0.386e-9;
	componentLibrary[MEM512KB].writeEnergy = 0.216e-9;
	componentLibrary[MEM512KB].staticPower = 0.0698e-3;
	componentLibrary[MEM512KB].height = 2.259;
	componentLibrary[MEM512KB].width = 2.975;
	componentLibrary[MEM512KB].criticalFraction = 1;
	componentLibrary[MEM512KB].Aem = 2.03E-13;
	componentLibrary[MEM512KB].gType = MEM;
	componentLibrary[MEM512KB].capacity = 512;
	componentLibrary[MEM512KB].aType = MEM1MB;
	componentLibrary[MEM512KB].dType = MEM384KB;
	componentLibrary[MEM512KB].inputs = -1;	
	
	// 1MB memory (CACTI 5.3)
	componentLibrary[MEM1MB].vdd = 1.3;
	componentLibrary[MEM1MB].readEnergy = 0.462e-9;
	componentLibrary[MEM1MB].writeEnergy = 0.291e-9;
	componentLibrary[MEM1MB].staticPower = 0.140e-3;
	componentLibrary[MEM1MB].height = 2.368;
	componentLibrary[MEM1MB].width = 5.841;
	componentLibrary[MEM1MB].criticalFraction = 1;
	componentLibrary[MEM1MB].Aem = 1.18E-13;
	componentLibrary[MEM1MB].gType = MEM;
	componentLibrary[MEM1MB].capacity = 1024;
	componentLibrary[MEM1MB].aType = MEM2MB;
	componentLibrary[MEM1MB].dType = MEM512KB;
	componentLibrary[MEM1MB].inputs = -1;
	
	// 2MB memory (CACTI 5.3)
	componentLibrary[MEM2MB].vdd = 1.3;
	componentLibrary[MEM2MB].readEnergy = 1.21e-9;
	componentLibrary[MEM2MB].writeEnergy = 0.418e-9;
	componentLibrary[MEM2MB].staticPower = 0.273e-3;
	componentLibrary[MEM2MB].height = 4.419;
	componentLibrary[MEM2MB].width = 5.689;
	componentLibrary[MEM2MB].criticalFraction = 1;
	componentLibrary[MEM2MB].Aem = 1.42E-13;
	componentLibrary[MEM2MB].gType = MEM;
	componentLibrary[MEM2MB].capacity = 2048;
	componentLibrary[MEM2MB].aType = NONE;
	componentLibrary[MEM2MB].dType = MEM1MB;
	componentLibrary[MEM2MB].inputs = -1;
	
	// 3x3 switch
	componentLibrary[SW3X3].vdd = 1;
	componentLibrary[SW3X3].height = 0.593;
	componentLibrary[SW3X3].width = 0.593;
	componentLibrary[SW3X3].criticalFraction = 1;
	componentLibrary[SW3X3].Aem = 5.30E-11;
	componentLibrary[SW3X3].gType = SW;
	componentLibrary[SW3X3].capacity = 0;
	componentLibrary[SW3X3].aType = NONE;
	componentLibrary[SW3X3].dType = NONE;
	componentLibrary[SW3X3].inputs = 3;	

	// 4x4 switch
	componentLibrary[SW4X4].vdd = 1;
	componentLibrary[SW4X4].height = 0.686;
	componentLibrary[SW4X4].width = 0.686;
	componentLibrary[SW4X4].criticalFraction = 1;
	componentLibrary[SW4X4].Aem = 5.13E-11;
	componentLibrary[SW4X4].gType = SW;
	componentLibrary[SW4X4].capacity = 0;
	componentLibrary[SW4X4].aType = NONE;
	componentLibrary[SW4X4].dType = NONE;
	componentLibrary[SW4X4].inputs = 4;

	// 5x5 switch
	componentLibrary[SW5X5].vdd = 1;
	componentLibrary[SW5X5].height = 0.768;
	componentLibrary[SW5X5].width = 0.768;
	componentLibrary[SW5X5].criticalFraction = 1;
	componentLibrary[SW5X5].Aem = 5.06E-11;
	componentLibrary[SW5X5].gType = SW;
	componentLibrary[SW5X5].capacity = 0;
	componentLibrary[SW5X5].aType = NONE;
	componentLibrary[SW5X5].dType = NONE;
	componentLibrary[SW5X5].inputs = 5;

	return;
}
