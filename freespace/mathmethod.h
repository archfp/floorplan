#ifndef FREESPACE_H_INCLUDED
#define FREESPACE_H_INCLUDED
#include "subbox.h"

//#define epislon 0.0000001
//#define multiplier 1

#define epislon 0.8
#define multiplier 10000000


bool If_Equal(double a,double b){
    if ((a-b)>=0){if (a-b<epislon)return true;else return false;}
    else {if (b-a<epislon)return true;else return false;}
    }

int IfAddRect (subbox rect_in_1,subbox rect_in_2){
    if(
    If_Equal(rect_in_1.p2xr(),rect_in_2.p1xr())&&   //12
    If_Equal(rect_in_1.p2yr(),rect_in_2.p1yr())&&
    If_Equal(rect_in_1.p3xr(),rect_in_2.p4xr())&&
    If_Equal(rect_in_1.p3yr(),rect_in_2.p4yr())&&
    (!If_Equal(rect_in_1.p1xr(),rect_in_2.p2xr()))
) {return 1;}
    else if(
    If_Equal(rect_in_1.p1xr(),rect_in_2.p4xr())&&   //1
    If_Equal(rect_in_1.p1yr(),rect_in_2.p4yr())&&   //2
    If_Equal(rect_in_1.p2xr(),rect_in_2.p3xr())&&
    If_Equal(rect_in_1.p2yr(),rect_in_2.p3yr())&&
    (!If_Equal(rect_in_1.p4yr(),rect_in_2.p1yr()))
) {return 2;}
    else if(
    If_Equal(rect_in_1.p4xr(),rect_in_2.p1xr())&&   //2
    If_Equal(rect_in_1.p4yr(),rect_in_2.p1yr())&&   //1
    If_Equal(rect_in_1.p3xr(),rect_in_2.p2xr())&&
    If_Equal(rect_in_1.p3yr(),rect_in_2.p2yr())&&
    (!If_Equal(rect_in_1.p1yr(),rect_in_2.p4yr()))
) {return 3;}
    else if(
    If_Equal(rect_in_1.p1xr(),rect_in_2.p2xr())&&   //21
    If_Equal(rect_in_1.p1yr(),rect_in_2.p2yr())&&
    If_Equal(rect_in_1.p4xr(),rect_in_2.p3xr())&&
    If_Equal(rect_in_1.p4yr(),rect_in_2.p3yr())&&
    (!If_Equal(rect_in_1.p2xr(),rect_in_2.p1xr()))
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

vector <subbox> Input_File_Process (string FilePath){

    vector <subbox> componentlist;
    subbox wholebox;

  string str = "";
  ifstream infile;
  infile.open(FilePath.c_str()); ///D:/freespace/McPAT_32x1.flp
  while (!infile.eof())
  {
  getline(infile, str);
  //string str=" 	0.000316891	0.00164094	0	0";
  if (str[0]!='#'){

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

    float ww=atof(attributes[0].c_str())*multiplier;
    float hh=atof(attributes[1].c_str())*multiplier;
    float xx=atof(attributes[2].c_str())*multiplier;
    if (xx<0){xx=0;}
    float yy=atof(attributes[3].c_str())*multiplier;
    if (yy<0){yy=0;}

    wholebox.set_values(ww,hh,xx,yy);
    componentlist.push_back (wholebox);
  }
  }
  infile.close();
  cout << "Read file completed" << endl;
  return componentlist;
}

//determine if a (x,y) ccord is inside or out
bool ifinside (vector <subbox> mylist,double xcord,double ycord){
             //check each componentlist, to see if this point is inside these components
             for (int i=0;i<mylist.size();i=i+1){
             if ((mylist[i].p1xr()<=xcord)&&(mylist[i].p1yr()<=ycord)&&(mylist[i].p2xr()>=xcord)
                 &&(mylist[i].p2yr()<=ycord)&&(mylist[i].p3xr()>=xcord)&&(mylist[i].p3yr()>=ycord)
                 &&(mylist[i].p4xr()<=xcord)&&(mylist[i].p4yr()>=ycord))
                 {return true;}
             }
             return false;
}

bool ifvaluein(vector <double> mylist,double value){
            for (int i=0;i<mylist.size();i=i+1){
             if (If_Equal(mylist[i],value)){return true;}
             }
             return false;
}

bool ifboxinside(vector <subbox> mylist,subbox box){
             if (ifinside(mylist,box.CenterXr(),box.CenterYr()))
             {return true;}
             return false;
}

vector <subbox> CreateUnitBox(vector<double> xpartition,vector<double> ypartition){
        vector<subbox> UnitBoxList;
        for(int y = 0; y < ypartition.size()-1; y++) {
            for(int x = 0; x < xpartition.size()-1; x++) {
            subbox temp;
            temp.set_values(xpartition[x+1]-xpartition[x],ypartition[y+1]-ypartition[y],xpartition[x],ypartition[y]); //need to push additional 0 into the ypartion
            temp.MakeOut(); //set the "in" bit to 0
            UnitBoxList.push_back (temp);
            }
        }
        return UnitBoxList;
}

vector <subbox> XBubbleSort(vector<subbox> mylist)//Bubble sort function
{
    int i,j;
    for(i=0;i<mylist.size();i++)
    {
        for(j=0;j<i;j++)
        {
            if(mylist[i].p1xr()<mylist[j].p1xr())
            {
                subbox temp=mylist[i]; //swap
                mylist[i]=mylist[j];
                mylist[j]=temp;
            }
        }
    }
    return mylist;
}

vector <subbox> YBubbleSort(vector<subbox> mylist)//Bubble sort function
{
    int i,j;
    for(i=0;i<mylist.size();i++)
    {
        for(j=0;j<i;j++)
        {
            if(mylist[i].p1yr()<mylist[j].p1yr())
            {
                subbox temp=mylist[i]; //swap
                mylist[i]=mylist[j];
                mylist[j]=temp;
            }
        }
    }
    return mylist;
}

void PrintList(vector<subbox> InputList){

    ofstream outfile("D:/freespace/output.txt",ios::app);
    if(!outfile)
    {
        cerr<<"open output.txt error!\n";
    }

    for (int i=0;i<InputList.size();i=i+1){
            InputList[i].PrintInfo();
    }
    int j=InputList.size()+1;
    outfile<<"total number: "<<(InputList.size())<<endl;
}

///to sort the list in x and y order
///1st
///Algorighm verified Feb 16th at home
vector <subbox> XBubble_SubSort(vector<subbox> InputList)//Bubble sort function
{
    vector<subbox> mylist;

    ///IMPORTANT MESSAGE
    mylist=XBubbleSort(InputList);
    ///IMPORTANT MESSAGE

//    cout<<endl<<"Here is the X-Sorted result:"<<endl;
//    PrintList(mylist);
//    cout<<"X-Sorted ends here"<<endl<<endl;

    vector<subbox> temp2;
    vector<subbox> temp3;

    ///the following content is for Sub-Y sorting, not combining
    while(mylist.size()>1){
        if (!(If_Equal(mylist[1].p1xr(),mylist[0].p1xr()))){
            temp3.push_back(mylist[0]);
            mylist.erase(mylist.begin());
        }
        else {

            ///Take all the elements with the same X(outter bracket)
            temp2.push_back(mylist[0]);
            while((If_Equal(mylist[1].p1xr(),mylist[0].p1xr()))&&(mylist.size()>1)){
                    mylist.erase(mylist.begin());
                    temp2.push_back(mylist[0]);
            }
            temp2=YBubbleSort(temp2);
            mylist.erase(mylist.begin());

            ///Testing Purpose
//            cout<<"test"<<endl;
//            PrintList(temp2);
            ///

            for (int k=0;k<temp2.size();k=k+1){
                temp3.push_back(temp2[k]);
            }
            temp2.clear();
        }
    }

    ///to add the last element
    if(!If_Equal(mylist[0].p1xr(),temp3[temp3.size()-1].p1xr())){
        temp3.push_back(mylist[0]);
    }

    return temp3;
}

///2nd
///Algorighm verified Feb 16th at home
vector <subbox> YBubble_SubSort(vector<subbox> InputList)//Bubble sort function
{
    vector<subbox> mylist;

    ///IMPORTANT MESSAGE
    mylist=YBubbleSort(InputList);
    ///IMPORTANT MESSAGE

//    cout<<endl<<"Here is the Y-Sorted result:"<<endl;
//    PrintList(mylist);
//    cout<<"Y-Sorted ends here"<<endl<<endl;

    vector<subbox> temp2;
    vector<subbox> temp3;

    ///the following content is for Sub-X sorting, not combining
    while(mylist.size()>1){
        if (!(If_Equal(mylist[1].p1yr(),mylist[0].p1yr()))){
            temp3.push_back(mylist[0]);
            mylist.erase(mylist.begin());
        }
        else {
            ///Take all the elements with the same Y(outter bracket)
            temp2.push_back(mylist[0]);
            while((If_Equal(mylist[1].p1yr(),mylist[0].p1yr()))&&(mylist.size()>1)){
                    mylist.erase(mylist.begin());
                    temp2.push_back(mylist[0]);
            }

            temp2=XBubbleSort(temp2);

            ///to avoid the overlap for the first if statement
            mylist.erase(mylist.begin());

            ///Testing Purpose
//            cout<<"test2"<<endl;
//            PrintList(temp2);
            ///

            for (int k=0;k<temp2.size();k=k+1){
                temp3.push_back(temp2[k]);
            }
            temp2.clear();
        }
    }

    ///to add the last element
    if(!If_Equal(mylist[0].p1yr(),temp3[temp3.size()-1].p1yr())){
        temp3.push_back(mylist[0]);
    }

    return temp3;
}

///input list is already sorted
vector <subbox> Vertical_Combine(vector<subbox> mylist)//Bubble sort function
{
    vector<subbox> temp;
    temp=mylist;
    vector<subbox> FinalList;
    int j=0;
    while(j<mylist.size()){

        ///single case
        if (!If_Equal(temp[1].p1xr(),temp[0].p1xr())){
        FinalList.push_back(temp[0]);
        temp.erase(temp.begin());
        }

        ///combine
        else{
            if (IfAddRect(temp[1],temp[0])!=5){
            temp[0]=AddRect(temp[0],temp[1]);
            temp.erase(temp.begin()+1);
            }
            else{
            FinalList.push_back(temp[0]);
            temp.erase(temp.begin());
            }
        }
        j=j+1;
    }
    return FinalList;
}

///this process has to be executed after a vertical combine
vector <subbox> Horizontal_Combine(vector<subbox> mylist)//Bubble sort function
{
    vector<subbox> temp;
    temp=mylist;
    vector<subbox> FinalList;
    int j=0;
    while(j<mylist.size()){

        ///single case
        if (!If_Equal(temp[1].p1yr(),temp[0].p1yr())){
        FinalList.push_back(temp[0]);
        temp.erase(temp.begin());
        }

        ///combine
        else{
            if (IfAddRect(temp[1],temp[0])!=5){
            temp[0]=AddRect(temp[0],temp[1]);
            temp.erase(temp.begin()+1);
            }
            else{
            FinalList.push_back(temp[0]);
            temp.erase(temp.begin());
            }
        }
        j=j+1;
    }
    return FinalList;
}

///
vector<double> X_Partition(vector<subbox> complist)
{
    vector<double> temp;
    for (int i=0;i<complist.size();i=i+1){
        if (!ifvaluein(temp,complist[i].p1xr())){temp.push_back(complist[i].p1xr());}
        if (!ifvaluein(temp,complist[i].p2xr())){temp.push_back(complist[i].p2xr());}
    }
    sort (temp.begin(), temp.end());
    return temp;
}

vector<double> Y_Partition(vector<subbox> complist)
{
    vector<double> temp;
    for (int i=0;i<complist.size();i=i+1){
        if (!ifvaluein(temp,complist[i].p1yr())){temp.push_back(complist[i].p1yr());}
        if (!ifvaluein(temp,complist[i].p4yr())){temp.push_back(complist[i].p4yr());}
    }
    sort (temp.begin(), temp.end());
    return temp;
}

vector<subbox> Generate_DS_List(vector<subbox> complist,vector<subbox> UnitCompList)
{
    vector<subbox> temp;
    for (int i=0;i<UnitCompList.size();i=i+1){
        if (!ifboxinside(complist,UnitCompList[i])){
                temp.push_back(UnitCompList[i]);
        }
    }
    return temp;
}

void OutputFileReset()
{
ofstream outfile("D:/freespace/output.txt",ios::trunc);
outfile.close();
}


#endif // FREESPACE_H_INCLUDED
