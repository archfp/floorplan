#include "SIM_power.h"
#include "SIM_power_static.h"


#if (PARM(TECH_POINT) == 18)
double NMOS_TAB[1] = {20.5e-9};
double PMOS_TAB[1] = {9.2e-9};
double NAND2_TAB[4] = {6.4e-10, 20.4e-9, 12.6e-9, 18.4e-9};
double NOR2_TAB[4] ={40.9e-9, 8.32e-9, 9.2e-9, 2.3e-10};
#elif (PARM(TECH_POINT) == 10)
double NMOS_TAB[1] = {22.7e-9};
double PMOS_TAB[1] = {18.0e-9};
double NAND2_TAB[4] = {1.2e-9, 22.6e-9, 11.4e-9, 35.9e-9};
double NOR2_TAB[4] ={45.1e-9, 11.5e-9, 17.9e-9, 1.8e-9};
#elif (PARM(TECH_POINT) == 7)
double NMOS_TAB[1] = {118.1e-9};
double PMOS_TAB[1] = {135.2e-9};
double NAND2_TAB[4] = {19.7e-9, 115.3e-9, 83.0e-9, 267.6e-9};
double NOR2_TAB[4] ={232.4e-9, 79.6e-9, 127.9e-9, 12.3e-9};
#endif
