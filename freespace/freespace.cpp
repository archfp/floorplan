#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <string>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <fstream>

using namespace std;
#include "subbox.h"
#include "mathmethod.h"

/*
4--3
|  |
1--2
*/

using namespace std;

///all the comment in this file are for testing purpose
int main(){

    OutputFileReset();

    ofstream outfile("D:/freespace/output.txt",ios::app);

    vector<subbox> complist;
    vector<subbox> UnitCompList;
    vector<subbox> DeadSpaceList;

    complist = Input_File_Process("D:/freespace/test4.flp");
    vector<double> xpartition=X_Partition(complist);
    vector<double> ypartition=Y_Partition(complist);

    outfile<<"Here are all the input components:"<<endl;
    PrintList(complist);
    outfile<<endl;

    UnitCompList = CreateUnitBox(xpartition,ypartition);
    outfile<<endl<<"The unit components are:"<<endl;
    PrintList(UnitCompList);

    DeadSpaceList=Generate_DS_List(complist,UnitCompList);
    DeadSpaceList=XBubble_SubSort(DeadSpaceList);

    outfile<<endl<<"The deadspace are:"<<endl;;
    PrintList(DeadSpaceList);

    outfile<<endl<<"after vertical reconstruct,"<<endl;
    outfile<<"the dead spaces are:"<<endl;
    DeadSpaceList=Vertical_Combine(DeadSpaceList);
    PrintList(DeadSpaceList);

    outfile<<endl<<"now horizontal reconstruct starts"<<endl;
    outfile<<"The dead spaces(Final result) are:"<<endl;
    DeadSpaceList=YBubble_SubSort(DeadSpaceList);
    DeadSpaceList=Horizontal_Combine(DeadSpaceList);
    PrintList(DeadSpaceList);

    outfile<<endl<<"the xpartition are:";
    for(int i=0;i<xpartition.size();i++){
        outfile<<xpartition[i]<<" ";
    }
    outfile<<endl<<"the ypartition are:";
    for(int i=0;i<ypartition.size();i++){
        outfile<<ypartition[i]<<" ";
    }

    outfile.close();

    ///Test Area///
//    vector<subbox> testcase;
//    vector<subbox> testcase2;
//    ///these are arbitrary deadspaces for testing purpose
//    subbox test1;
//
//    test1.set_values(1,5,6,1);
//    testcase.push_back(test1);
//    test1.set_values(1,1,2,2);
//    testcase.push_back(test1);
//    test1.set_values(1,1,8,8);
//    testcase.push_back(test1);
//
//    testcase2=XBubbleSort(testcase);
//    ///thses are arbitrary deadspaces for testing purpose
//
//
//    cout<<endl<<endl<<"Test Area"<<endl;
//    PrintList(testcase2);
    ///Test Area///
} //end here
