#ifndef SUBBOX_H_INCLUDED
#define SUBBOX_H_INCLUDED

class subbox {
    double p1x,p1y,p2x,p2y,p3x,p3y,p4x,p4y;
    double w,h;
    bool UnitIn;

    public:
    void set_values (double,double,double,double);
    void set_values (double,double,double,double,bool);
    void MakeIn() {UnitIn=1;};
    void MakeOut() {UnitIn=0;};
    bool inr(){return UnitIn;}
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
    double CenterXr() {return (p1x+p2x)/2;}
    double CenterYr() {return (p1y+p4y)/2;}
    double min_length() {if ((p2x-p1x)>=(p4y-p1y)) return ((p4y-p1y));else return ((p2x-p1x));}
    void PrintInfo() {
        ofstream outfile("D:/freespace/output.txt",ios::app);
        outfile<<"w:"<<w<<" h:"<<h<<" x:"<<p1x<<" y:"<<p1y<<endl;};
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
#endif // SUBBOX_H_INCLUDED
