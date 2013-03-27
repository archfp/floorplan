#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <string>
#include <iomanip>
#include <sstream>
#define epislon 0.0001

using namespace std;

/*
4--3
|  |
1--2
*/

//predeclared variables
bool Gridbool[39999][39999];

bool equallol(double a,double b){
    if ((a-b)>=0){if (a-b<epislon)return true;else return false;}
    else {if (b-a<epislon)return true;else return false;}
    }

class subbox {
    double p1x,p1y,p2x,p2y,p3x,p3y,p4x,p4y;
    double w,h;
    bool UnitIn;

    public:
    void set_values (double,double,double,double);
    void set_values (double,double,double,double,bool);

    void MakeIn() {UnitIn=1;};
    void MakeOut() {UnitIn=0;};

    double p1xr() {return p1x;};
    double p1yr() {return p1y;};
    double p2xr() {return p2x;};
    double p2yr() {return p2y;};
    double p3xr() {return p3x;};
    double p3yr() {return p3y;};
    double p4xr() {return p4x;};
    double p4yr() {return p4y;};
    double wr() {return w;};
    double hr() {return h;};

    double min_length() {if ((p2x-p1x)>=(p4y-p1y)) return ((p4y-p1y));else return ((p2x-p1x));};
};

void subbox::set_values(double width, double height, double x, double y){
p1x=x;
p1y=y;
p2x=x+width;
p2y=y;
p3x=x+width;
p3y=y+height;
p4x=x;
p4y=y+height;
w=width;
h=height;
}

void subbox::set_values(double width, double height, double x, double y,bool in){
p1x=x;
p1y=y;
p2x=x+width;
p2y=y;
p3x=x+width;
p3y=y+height;
p4x=x;
p4y=y+height;
w=width;
h=height;
UnitIn=in;
}

int IfAddRect (subbox rect_in_1,subbox rect_in_2){
    if(
    equallol(rect_in_1.p2xr(),rect_in_2.p1xr())&&   //12
    equallol(rect_in_1.p2yr(),rect_in_2.p1yr())&&
    equallol(rect_in_1.p3xr(),rect_in_2.p4xr())&&
    equallol(rect_in_1.p3yr(),rect_in_2.p4yr())&&
    (!equallol(rect_in_1.p1xr(),rect_in_2.p2xr()))
) {return 1;}
    else if(
    equallol(rect_in_1.p1xr(),rect_in_2.p4xr())&&   //1
    equallol(rect_in_1.p1yr(),rect_in_2.p4yr())&&   //2
    equallol(rect_in_1.p2xr(),rect_in_2.p3xr())&&
    equallol(rect_in_1.p2yr(),rect_in_2.p3yr())&&
    (!equallol(rect_in_1.p4yr(),rect_in_2.p1yr()))
) {return 2;}
    else if(
    equallol(rect_in_1.p4xr(),rect_in_2.p1xr())&&   //2
    equallol(rect_in_1.p4yr(),rect_in_2.p1yr())&&   //1
    equallol(rect_in_1.p3xr(),rect_in_2.p2xr())&&
    equallol(rect_in_1.p3yr(),rect_in_2.p2yr())&&
    (!equallol(rect_in_1.p1yr(),rect_in_2.p4yr()))
) {return 3;}
    else if(
    equallol(rect_in_1.p1xr(),rect_in_2.p2xr())&&   //21
    equallol(rect_in_1.p1yr(),rect_in_2.p2yr())&&
    equallol(rect_in_1.p4xr(),rect_in_2.p3xr())&&
    equallol(rect_in_1.p4yr(),rect_in_2.p3yr())&&
    (!equallol(rect_in_1.p2xr(),rect_in_2.p1xr()))
) {return 4;}
    else {return 5;}
};

subbox AddRect (subbox rect_in_1,subbox rect_in_2){
   if(IfAddRect(rect_in_1,rect_in_2)==1){//12
    subbox temp_box;
    temp_box.set_values(rect_in_1.wr()+rect_in_2.wr(),rect_in_1.hr(),rect_in_1.p1xr(),rect_in_1.p1yr());
    return temp_box;}
   else if (IfAddRect(rect_in_1,rect_in_2)==2){//1up 2down
    subbox temp_box;
    temp_box.set_values(rect_in_1.wr(),rect_in_1.hr()+rect_in_2.hr(),rect_in_2.p1xr(),rect_in_2.p1yr());
    return temp_box;}
   else if (IfAddRect(rect_in_1,rect_in_2)==3){//1down 2up
    subbox temp_box;
    temp_box.set_values(rect_in_1.wr(),rect_in_1.hr()+rect_in_2.hr(),rect_in_1.p1xr(),rect_in_1.p1yr());
    return temp_box;}
   else if (IfAddRect(rect_in_1,rect_in_2)==4){//21
    subbox temp_box;
    temp_box.set_values(rect_in_1.wr()+rect_in_2.wr(),rect_in_1.hr(),rect_in_2.p1xr(),rect_in_2.p1yr());
    return temp_box;}
   else return rect_in_1;
};

int main()
{
  double totalW=4; //mm
  double totalH=3; //mm
  //int CompNumber=3;
  int gridxNo;   //the x number
  int gridyNo;   //the y number

  double w1,h1,x1,y1;
  double w2,h2,x2,y2;
  double w3,h3,x3,y3;

  vector <subbox> componentlist;
  subbox wholebox;

  string str = "";
  ifstream infile;
  infile.open("D:/freespace/McPAT_32x1.flp");
  while (!infile.eof())
  {
  getline(infile, str);
  if (str[0]!='#'){

    //string str="1.213 jioawef 2.55 wjaoi 3.44 jnoawifej  3.22";
    string attributes[4];
    int status=0;
    int i=0;
    int j=0;
    for (int i=0;i<str.length();i=i+1){
    if (status==0){
        if ((str.at(i) >= '0' && str.at(i) <= '9')||(str.at(i) == '-')){
        attributes[j]=attributes[j]+str.at(i);
        status=1;
        }
    }
    else{
        if (!((str.at(i) >= '0' && str.at(i) <= '9')||(str.at(i)=='.')||(str.at(i)=='e')||(str.at(i)=='-'))){
        status=0; // stop drag
        j=j+1;
        }
        else{attributes[j]=attributes[j]+str.at(i);
        }
    }
    }

    float ww=atof(attributes[0].c_str())*1000;
    float hh=atof(attributes[1].c_str())*1000;
    float xx=atof(attributes[2].c_str())*1000;
    if (xx<0){xx=0;}
    float yy=atof(attributes[3].c_str())*1000;
    if (yy<0){yy=0;}

    wholebox.set_values(ww,hh,xx,yy);
    componentlist.push_back (wholebox);
  }
  }
  infile.close();
  cout << "Read file completed" << endl;

  //determine the unit grid length among all the components
  int i; //iterator for the componentlist
  double GlobalMinLength = componentlist[1].min_length();
  for (i=1;i<=componentlist.size()-2;i=i+1){
  if (componentlist[i].min_length()<GlobalMinLength) {GlobalMinLength=componentlist[i].min_length();};
  };

  double GridUnitLength = GlobalMinLength/10;

  int grid_xMax;
  int grid_yMax;

  grid_xMax=(int)(totalW/GridUnitLength);
  grid_yMax=(int)(totalH/GridUnitLength);

  vector <subbox> UnitList;
  //for loop for points
  //start from 1 because 0 is the edge of the graph

  for (gridyNo=1;gridyNo<=grid_yMax;gridyNo++){
        for (gridxNo=1;gridxNo<=grid_xMax;gridxNo++){

             int i; //iterator for the componentlist

             double xcord= GridUnitLength*gridxNo;
             double ycord= GridUnitLength*gridyNo;
             int status=0;

             //check each componentlist, to see if this point is inside these components
             for (i=1;i<=componentlist.size()-2;i=i+1){
             if ((componentlist[i].p1xr()<=xcord)&&(componentlist[i].p1yr()<=ycord)&&(componentlist[i].p2xr()>=xcord)
                 &&(componentlist[i].p2yr()<=ycord)&&(componentlist[i].p3xr()>=xcord)&&(componentlist[i].p3yr()>=ycord)
                 &&(componentlist[i].p4xr()<=xcord)&&(componentlist[i].p4yr()>=ycord))
                 {
                    subbox unitbox;
                    unitbox.set_values(GridUnitLength,GridUnitLength,xcord-GridUnitLength/2,ycord-GridUnitLength/2,true);
                    UnitList.push_back (unitbox);
                    status=1;
                    break;
                 }
             }
             if (status==0){
             subbox unitbox;
             unitbox.set_values(GridUnitLength,GridUnitLength,xcord-GridUnitLength/2,ycord-GridUnitLength/2,false);
             UnitList.push_back (unitbox);}
         };
  };

  cout<<"ComponentList Size: "<<componentlist.size()<<endl;
  cout<<"UnitList Size: "<<UnitList.size()<<endl;
  cout<<"the 500th unit element X is "<<UnitList[5960].p1xr()<<endl;
  cout<<"the 500th unit element Y is "<<UnitList[5960].p1yr()<<endl;

  cout<<"totalH  "<<totalH<<endl;
  cout<<"totalW  "<<totalW<<endl;
  cout<<"GridUnitLength  "<<GridUnitLength<<endl;
  cout<<"componentlist[2].min_length()  "<<componentlist[2].min_length()<<endl;

  cout<<"grid_xMax  "<<grid_xMax<<endl;
  cout<<"grid_yMax  "<<grid_yMax<<endl;
  cout<<"GridUnitLength  "<<GridUnitLength<<endl;

  cout<<Gridbool[29][28]<<endl<<endl;

  int which=96;
  cout<<"component3 w: "<<componentlist[which].wr()<<endl;
  cout<<"component3 h: "<<componentlist[which].hr()<<endl;
  cout<<"component3 p1xr: "<<componentlist[which].p1xr()<<endl;
  cout<<"component3 p1yr: "<<componentlist[which].p1yr()<<endl;
//  cout<<Gridbool[3][5];

  w2=5;h2=10;x2=5;y2=5;//e11
  w1=5;h1=20;x1=5;y1=15;//e41
  w3=0.002;h3=0.0015;x3=0;y3=0;//e21
  double Area;

  //testing addrect
  subbox AddRect_testing1;
  subbox AddRect_testing2;

  AddRect_testing1.set_values(w1,h1,x1,y1);
  AddRect_testing2.set_values(w2,h2,x2,y2);

  subbox AddTest=AddRect(AddRect_testing1,AddRect_testing2);
  cout<<endl<<"add test"<<endl;
  cout<<AddRect_testing1.p1xr()<<endl;
  cout<<AddRect_testing1.p1yr()<<endl;
  cout<<AddRect_testing1.p2xr()<<endl;
  cout<<AddRect_testing1.p2yr()<<endl;
  cout<<AddRect_testing1.p3xr()<<endl;
  cout<<AddRect_testing1.p3yr()<<endl;
  cout<<AddRect_testing1.p4xr()<<endl;
  cout<<AddRect_testing1.p4yr()<<endl<<endl;

  cout<<AddRect_testing2.p1xr()<<endl;
  cout<<AddRect_testing2.p1yr()<<endl;
  cout<<AddRect_testing2.p2xr()<<endl;
  cout<<AddRect_testing2.p2yr()<<endl;
  cout<<AddRect_testing2.p3xr()<<endl;
  cout<<AddRect_testing2.p3yr()<<endl;
  cout<<AddRect_testing2.p4xr()<<endl;
  cout<<AddRect_testing2.p4yr()<<endl<<endl;

  cout<<AddTest.p1xr()<<endl;
  cout<<AddTest.p1yr()<<endl;
  cout<<AddTest.p2xr()<<endl;
  cout<<AddTest.p2yr()<<endl;
  cout<<AddTest.p3xr()<<endl;
  cout<<AddTest.p3yr()<<endl;
  cout<<AddTest.p4xr()<<endl;
  cout<<AddTest.p4yr()<<endl<<endl;
  cout<<AddTest.wr()<<endl;
  cout<<AddTest.hr()<<endl;
  cout<<equallol(1,0);




}; //end here
