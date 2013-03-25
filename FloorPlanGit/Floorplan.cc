/* -*- Mode: C ; indent-tabs-mode: nil ; c-file-style: "stroustrup" -*-

   Rapid Prototyping Floorplanner Project
   Author: Greg Faust

   File:   Floorplan.cc    Code for Floorplanner classes.

 */

#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <stdexcept>
#include <exception>
#include "MathUtil.hh"
#include "Floorplan.hh"


// This will be used to keep track of user's request for more output during layout.
bool verbose = true;
 
bool legalizing = true; // Flag for legalization

bool changeArea = true; // This is used to turn on/off whether we need to keep the same area after legalizing the layout.
bool rightMark = true;
bool topMark = true; // Once an item is placed at right or top, mark = false;

bool topBottomInversion = true; // TopBottom items can be inverted to BottomTop order

bool checkOverlap = true; // Overlap detection 
double expandHeight = 0, expandWidth = 0; //Overlap area expansion

// Temporary local for crazy mirror reflection stuff.
#define maxMirrorDepth 50
static bool xReflect = false;
static double xLeft[maxMirrorDepth];
static double xRight[maxMirrorDepth];
static int xMirrorDepth = 0;
static bool yReflect = false;
static double yBottom[maxMirrorDepth];
static double yTop[maxMirrorDepth];
static int yMirrorDepth = 0;
static bool printNames = true;

// Charles Maximum layout retries
#define maxRetry 100

struct OverlapException: public exception
{
  const char * what () const throw ()
  {
    return "Components overlapped";
  }
};

struct AreaException : public exception
{
  const char * what () const throw ()
  {
    return "Components need more area for layout";
  }
};


void setNameMode(bool flag) {
    printNames = flag;
}

void FPContainer::pushMirrorContext(double startX, double startY) {
    if (xMirror) {
        if (xMirrorDepth == maxMirrorDepth) throw out_of_range("X Mirror Depth exceeds maximum allowed.");
        xReflect = !xReflect;
        xLeft[xMirrorDepth] = startX + x;
        xRight[xMirrorDepth] = startX + x + width;
        xMirrorDepth += 1;
    }
    if (yMirror) {
        if (xMirrorDepth == maxMirrorDepth) throw out_of_range("Y Mirror Depth exceeds maximum allowed.");
        yReflect = !yReflect;
        yBottom[yMirrorDepth] = startY + y;
        yTop[yMirrorDepth] = startY + y + height;
        yMirrorDepth += 1;
    }
}

void FPContainer::popMirrorContext() {
    if (xMirror) {
        xReflect = !xReflect;
        xMirrorDepth -= 1;
    }
    if (yMirror) {
        yReflect = !yReflect;
        yMirrorDepth -= 1;
    }
}

inline double FPObject::calcX(double startX) {
    if (xReflect) {
        return xLeft[xMirrorDepth - 1] - (startX + x - xRight[xMirrorDepth - 1] + width);
    } else return startX + x;
}

inline double FPObject::calcY(double startY) {
    if (yReflect) {
        return yTop[yMirrorDepth - 1] - (startY + y - yBottom[yMirrorDepth - 1] + height);
    } else return startY + y;
}

// Map component types to names.
string TypeNames [] = {"Unknown", "Core", "Cache", "ICache", "DCache", "L1_", "L2_", "L3_", "RF", "FPRF", "ALU", "NoC", "Control", "CrossBar", "MemCtrl", "Grid", "Group", "Cluster"};
map<string, int> NameCounts;

// Methods for the dummy component class to help with some IO.

ostream& operator<<(ostream& s, dummyComponent& c) {
    s << "Component: " << c.getName() << " is of type: " << c.getType() << "\n";
    return s;
}

dummyComponent::dummyComponent(ComponentType typeArg) {
    type = typeArg;
    name = Type2Name(type);
}

dummyComponent::dummyComponent(string nameArg) {
    type = UnknownType;
    name = nameArg;
    for (int i = 0; i < ComponentTypeCount; i++)
        if (nameArg == TypeNames[i]) {
            type = (ComponentType) i;
            break;
        }
}

void dummyComponent::myPrint() {
    cout << "Component: " << getName() << " is of type: " << type << "\n";
}

// Methods for FPObject

FPObject::FPObject() {
    x = 0.0;
    y = 0.0;
    width = 0.0;
    height = 0.0;
    area = 0.0;
    type = UnknownType;
    name = Type2Name(type);
    hint = UnknownGeography;
    count = 1;
    refCount = 0;
    state = false;
}

void FPObject::setSize(double widthArg, double heightArg) {
    width = widthArg;
    height = heightArg;
    area = width*height;
}

void FPObject::setLocation(double xArg, double yArg) {
    x = xArg;
    y = yArg;
}

//Q: Why is there a case where one doesn't print the name?
//A: // These are too messy with names.  Just output the shapes.

void FPObject::outputHotSpotLayout(ostream& o, double startX, double startY) {
    string uname = (printNames) ? getUniqueName() : " ";

    o << uname << "\t" << getWidth() / 1000 << "\t" << getHeight() / 1000
            << "\t" << calcX(startX) / 1000 << "\t" << calcY(startY) / 1000 << "\n";
}


// Methods for the FPWrapper class.

FPCompWrapper::FPCompWrapper(dummyComponent* comp, double minAR, double maxAR, double areaArg, int countArg) {
    component = comp;
    minAspectRatio = minAR;
    maxAspectRatio = maxAR;
    area = areaArg;
    count = countArg;
    name = comp->getName();
    type = comp->getType();
}

FPCompWrapper::FPCompWrapper(string nameArg, double xArg, double yArg, double widthArg, double heightArg) {
    dummyComponent * DC = new dummyComponent(nameArg);
    component = DC;
    setLocation(xArg, yArg);
    setSize(widthArg, heightArg);
    minAspectRatio = width / height;
    maxAspectRatio = width / height;
    count = 1;
    name = DC->getName();
    type = DC->getType();
}

FPCompWrapper::~FPCompWrapper() {
    // cout << "deleting component.\n";
    delete component;
}
//==================================================//

double FPCompWrapper::ARInRange(double AR) {
    double maxAR = getMaxAR();
    if ((AR < 1 && maxAR > 1) || (AR > 1 && maxAR < 1)) flip();
    maxAR = getMaxAR();
    double minAR = getMinAR();
    double retval = AR;
    if (maxAR < 1) //If flipped
    {
        retval = MIN(retval, minAR);
        retval = MAX(retval, maxAR);
    } else {
        retval = MAX(retval, minAR);
        retval = MIN(retval, maxAR);
    }
    if (verbose)
        cout << "Target AR=" << AR << " minAR=" << minAR << " maxAR=" << maxAR << " returnAR=" << retval << "\n";
    return retval; //retval has to be in the range between minAR and maxAR
}

void FPCompWrapper::flip() {
    double temp = width;
    width = height;
    height = temp;
    minAspectRatio = 1 / minAspectRatio;
    maxAspectRatio = 1 / maxAspectRatio;
}

bool FPCompWrapper::layout(FPOptimization opt, double ratio) {
    // Make sure the ratio is within the stated constraints.
    double verifiedRatio = ARInRange(ratio);

    // Calculate width height from area and AR.
    // We have w/h = ratio => w = ratio*h.
    // and w*h = area => ratio*h*h = area => h^2 = area/ratio => h = sqrt(area/ratio)
    height = sqrt(area / verifiedRatio);
    width = area / height;

    // Charles: If the newRatio is out of bound, consider re-layout or extend the boundary.
    if (verifiedRatio != ratio) return false;

    return true;
}

// Methods for the FPcontainer class.
int FPContainer::maxItemCount = 50;

// Default constructor

FPContainer::FPContainer() {
    itemCount = 0;
    items = new FPObject*[maxItemCount];
    count = 1;
    yMirror = false;
    xMirror = false;
}

// Q: Why "~"?
// A: Destructor

FPContainer::~FPContainer() {
    // It's very important to only delete items when their refcount hits zero.
    // cout << "deleting container.\n";
    for (int i = 0; i < itemCount; i++) {
        FPObject * item = items[i];
        int newCount = item->decRefCount();
        if (newCount == 0) delete item;
    }
    delete [] items;
}

// To properly handle refCount, we will only allow one method to actually add (or remove) items from the item list.

void FPContainer::addComponentAtIndex(FPObject * comp, int index) {
    if (index < 0 || index > itemCount) throw invalid_argument("Attempt to add item to Container at illegal index.");
    if (itemCount == maxItemCount) throw out_of_range("Attempt to add more than the maximum items to a container.");
    // See if we need to move things to open space.
    if (index < itemCount) for (int i = itemCount; i > index; i--) items[i] = items[i - 1];
    items[index] = comp;
    itemCount += 1;
    comp->incRefCount();
}

FPObject * FPContainer::removeComponentAtIndex(int index) {
    if (index < 0 || index >= itemCount) throw invalid_argument("Attempt to add item to Container at illegal index.");
    FPObject * comp = items[index];
    // Now fill in the hole.
    for (int i = index; i < itemCount - 1; i++) items[i] = items[i + 1];
    itemCount -= 1;
    // Often this component is about to be added somewhere else.
    // So, if the refCount is now zero, don't handle it here.
    // If the caller doesn't put it somewhere else, they will have to delete it themselves.
    comp->decRefCount();
    return comp;
}

void FPContainer::replaceComponent(FPObject * comp, int index) {
    removeComponentAtIndex(index);
    addComponentAtIndex(comp, index);
}

void FPContainer::addComponent(FPObject * comp) {
    addComponentAtIndex(comp, itemCount);
}

void FPContainer::addComponentToFront(FPObject * comp) {
    addComponentAtIndex(comp, 0);
}

void FPContainer::addComponent(FPObject * comp, int countArg) {
    comp->setCount(countArg);
    addComponent(comp);
}

FPObject * FPContainer::addComponentCluster(ComponentType type, int count, double area, double maxAspectRatio, double minAspectRatio) {
    // First generate a dummy component with the correct information.
    dummyComponent * comp = new dummyComponent(type);
    // We first need to create a wrapper for the component.
    FPCompWrapper* wrapComp = new FPCompWrapper(comp, minAspectRatio, maxAspectRatio, area, count);
    addComponent(wrapComp);
    return wrapComp;
}

FPObject * FPContainer::addComponentCluster(string name, int count, double area, double maxAspectRatio, double minAspectRatio) {
    // First generate a dummy component with the correct information.
    dummyComponent * comp = new dummyComponent(name);
    // We first need to create a wrapper for the component.
    FPCompWrapper* wrapComp = new FPCompWrapper(comp, minAspectRatio, maxAspectRatio, area, count);
    addComponent(wrapComp);
    return wrapComp;
}

FPObject * FPContainer::removeComponent(int index) {
    return removeComponentAtIndex(index);
}

// We need to sort items by total area.
// There are so few items, just use bubble sort.
// We want a descending sort.
// We know this won't change item counts, so just it have at the item list.

void FPContainer::sortByArea() {
    int len = itemCount;
    for (int i = 0; i < len; i++) {
        double maxArea = -1;
        int maxAreaIndex = 0;
        for (int j = i; j < len; j++) {
            double newArea = items[j]->getArea() * items[j]->getCount();
            if (newArea > maxArea) {
                maxArea = newArea;
                maxAreaIndex = j;
            }
        }
        FPObject * temp = items[i];
        items[i] = items[maxAreaIndex];
        items[maxAreaIndex] = temp;
    }
}

// TODO.  Should this store the area in itself when done, or leave alone?

double FPContainer::totalArea() {
    // if (area != 0) return area;
    area = 0;
    int itemCount = getComponentCount();
    for (int i = 0; i < itemCount; i++) {
        FPObject * item = getComponent(i);
        area += item->totalArea();
    }
    area *= count;
    return area;
}

// Methods for the GridLayout class.

gridLayout::gridLayout() : FPContainer() {
    type = Grid;
    name = Type2Name(type);
}

bool gridLayout::layout(FPOptimization opt, double targetAR) {
    // We assume that a grid is a repeating unit of a single object.
    // However, that object can be either a leaf component or a container.
    if (getComponentCount() != 1) {
        cerr << "Attempt to layout a grid with other than one component.\n";
        return false;
    }
    double tarea = totalArea();
    double theight = sqrt(tarea / targetAR);
    double twidth = tarea / theight;
    if (verbose)
        cout << "Begin Grid Layout, TargetAR=" << targetAR << " My area=" << tarea << " Implied W=" << twidth << " H=" << theight << "\n";

    FPObject * obj = getComponent(0);
    int total = obj->getCount();

    // Calculate the grid size closest to the target AR.
    xCount = balanceFactors(total, targetAR);
    yCount = total / xCount;
    // We want the gridRatio to express the actual width to height.
    double gridRatio = ((double) xCount) / yCount;
    // Now see what we want as the component ratio.
    double ratio = targetAR / gridRatio;

    if (verbose) {
        cout << "In Grid Layout, xCount=" << xCount << " yCount=" << yCount << "\n";
        cout << "In Grid Layout, Asking my inferior for AR=" << ratio << "\n";
    }

    // Now layout whatever is below us.
    // We need to set the count to 1 to avoid double counting area.
    obj->setCount(1);
    bool isGoodAR = obj->layout(AspectRatio, ratio);
    obj->setCount(total);
    double compWidth = obj->getWidth();
    double compHeight = obj->getHeight();

    // Now we can set our own width and height.
    width = compWidth * xCount;
    height = compHeight * yCount;
    area = width * height;
    if (verbose)
        cout << "At End Grid Layout, TargetAR=" << targetAR << " actualAR=" << width / height << "\n";

    if (legalizing) return isGoodAR;
    else            return true;
}

void gridLayout::outputHotSpotLayout(ostream& o, double startX, double startY) {
    if (getComponentCount() != 1) {
        cerr << "Attempt to output a grid with other than one component.\n";
        return;
    }

    double compWidth, compHeight;
    string compName;
    FPObject * obj = getComponent(0);
    compWidth = obj->getWidth();
    compHeight = obj->getHeight();
    compName = obj->getName();

    int compCount = xCount*yCount;
    string GridName = getUniqueName();
    o << "# Start of " << GridName << " Layout.  There are " << compCount << " components in a " << xCount << " by " << yCount << " grid.\n";
    o << "# Total Grid Stats: X=" << calcX(startX) << " Y=" << calcY(startY) << " W=" << width
            << "mm H=" << height << "mm Area=" << area << "mm^2\n";
    int compNum = 1;
    for (int i = 0; i < yCount; i++) {
        double cy = (i * compHeight) + y + startY;
        for (int j = 0; j < xCount; j++) {
            double cx = (j * compWidth) + x + startX;
            obj->outputHotSpotLayout(o, cx, cy);
            compNum += 1;
        }
    }
    o << "# End of " << GridName << " Layout.\n";
}

// Methods for Baglayout Class

bagLayout::bagLayout() : FPContainer() {
    type = Group;
    name = Type2Name(type);
    locked = false;
}

bool bagLayout::layout(FPOptimization opt, double targetAR) {
    // If we are locked, don't layout.
    if (locked) return true;

    // Calculate our area, and the implied target width and height.
    area = totalArea();
    double remHeight = sqrt(area / targetAR);
    double remWidth = area / remHeight;
    double nextX = 0;
    double nextY = 0;

    if (verbose) {
        cout << "In BagLayout, Area=" << area << " Width= " << remWidth << " Height="
                << remHeight << " Target AR=" << targetAR << "\n";
    }

    // Sort the components, placing them in decreasing order of size.
    sortByArea();
    int itemCount = getComponentCount();
    bool isGoodAR;
    for (int i = 0; i < itemCount; i++) {
        FPObject * comp = getComponent(i);
        double compArea = comp->totalArea();

        // Take the largest component, place it in the min dimension.
        // Except for the last component which gets places in the max dimension.
        // First calculate the AR, and the rest fill follow.
        double AR = (remWidth > remHeight || i == itemCount - 1) ? (compArea / remHeight) / remHeight : remWidth / (compArea / remWidth);
        if (comp->getCount() > 1) {
            // We have more than one component of the same type, so let the grid layout do the work.
            gridLayout * GL = new gridLayout();
            GL->addComponent(comp);
            replaceComponent(GL, i);
            comp = GL;
        }
        isGoodAR = comp->layout(AspectRatio, AR);
        
        // Now we have the final component, we can set the location.
        comp->setLocation(nextX, nextY);
        // Prepare for the next round.
        
        // This outOfBound modification is to be verified.
        /*
        if (!isGoodAR) {
            if (remWidth > remHeight || i == itemCount - 1) {
                diffWidth = comp->getWidth() - remWidth;
            } else {
                diffHeight = comp->getHeight() - remHeight;
            }
        }
        */
        
        if (remWidth > remHeight || i == itemCount - 1) {
            double compWidth = comp->getWidth();            
            remWidth = remWidth - compWidth;// + diffWidth;
            nextX += compWidth;
        } else {
            double compHeight = comp->getHeight();
            remHeight = remHeight - compHeight;// + diffHeight;
            nextY += compHeight;
        }

        if (verbose)
            cout << " remWidth=" << remWidth << " remHeight=" << remHeight << " AR=" << AR << "\n";
    }

    recalcSize();

    if (verbose) {
        cout << "Total Container Width=" << width << " Height=" << height << " Area="
                << width * height << " AR=" << width / height << "\n";
    }
    
    if (legalizing) return isGoodAR;
    else            return true;
}

void bagLayout::recalcSize() {
    // Go through the layout components and find the containers width and height.
    width = 0;
    height = 0;
    for (int i = 0; i < getComponentCount(); i++) {
        FPObject * comp = getComponent(i);
        width = MAX(width, comp->getX() + comp->getWidth());
        height = MAX(height, comp->getY() + comp->getHeight());
    }
    area = width * height;
}

void bagLayout::outputHotSpotLayout(ostream& o, double startX, double startY) {
    pushMirrorContext(startX, startY);
    int itemCount = getComponentCount();
    string groupName;
    if (itemCount != 1) {
        groupName = getUniqueName();
        o << "# Start of " << groupName << " layout.\n";
        o << "# Total Group Stats: X=" << calcX(startX) << " Y=" << calcY(startY) << " W=" << width
                << "mm H=" << height << "mm Area=" << area << "mm^2\n";
    }
    for (int i = 0; i < getComponentCount(); i++) {
        FPObject * obj = getComponent(i);
        obj->outputHotSpotLayout(o, x + startX, y + startY);
    }
    if (itemCount != 1) o << "# End of " << groupName << " Layout.\n";
    popMirrorContext();
}

void fixedLayout::morph(double xFactor, double yFactor) {
    for (int i = 0; i < getComponentCount(); i++) {
        FPObject * comp = getComponent(i);
        comp->setLocation(comp->getX() * xFactor, comp->getY() * yFactor);
        comp->setSize(comp->getWidth() * xFactor, comp->getHeight() * yFactor);
    }
}

// This will read in a hotspot file, and create a bag layout that is locked.
// NOTE: In general, the layout method on a bag layout manager would not have been able to create the resultant layout.

fixedLayout::fixedLayout(const char * filename, double scalingFactor) : bagLayout() {
    locked = true;

    ifstream in(filename);

    // cout << "In read of fixedLayout\n";
    while (true) {
        char first = in.peek();
        if (in.eof() || in.bad() || in.fail()) break;
        if (first == '\n') {
            in.get();
            continue;
        }
        if (first == '#') // || first == ' ')
        {
            char getChar;
            while (!in.eof()) {
                getChar = in.get();
                // cout << getChar;
                if (getChar == '\n') break;
            }
            continue;
        }
        // Now we should have a real line.
        string name;
        if (in.peek() == ' ')
            name = "";
        else
            in >> name;
        // cout << "name is'" << name << "'\n";
        double x, y, width, height;
        in >> width >> height >> x >> y;
        this->addComponent(new FPCompWrapper(name, x * 1000, y * 1000, width * 1000, height * 1000));
    }

    // Let's find out if the baseline x,y positions are at 0, 0.
    // If not, renormalize all the locations.
    double minX = 2000000000;
    double minY = 2000000000;
    for (int i = 0; i < getComponentCount(); i++) {
        FPObject * comp = getComponent(i);
        minX = MIN(minX, comp->getX());
        minY = MIN(minY, comp->getY());
    }
    if (minX != 0.0 || minY != 0) {
        for (int i = 0; i < getComponentCount(); i++) {
            FPObject * comp = getComponent(i);
            comp->setLocation(comp->getX() - minX, comp->getY() - minY);
        }
    }

    // Perform scaling if needed.
    // NOTE.  This needs to happen after location normalization to avoid wierd offset effects.
    if (scalingFactor != 1.0) {
        morph(scalingFactor, scalingFactor);
    }

    // Now that the inferiors are renormalized, we can calculate the container size.
    recalcSize();
}

bool fixedLayout::layout(FPOptimization opt, double targetAR) {
    // Compare targetAR to our originalAR to calculate x and y scaling factors.
    double currentAR = width / height;
    double xFactor = sqrt(targetAR / currentAR);
    double yFactor = 1.0 / xFactor;
    morph(xFactor, yFactor);
    width = xFactor * width;
    height = yFactor * height;
    return true;
}

// Geographic Layout.

geogLayout::geogLayout() : FPContainer() {
    type = Cluster;
    name = Type2Name(type);
}

void geogLayout::addComponent(FPObject * comp, int count, GeographyHint hint) {
    comp->setHint(hint);
    FPContainer::addComponent(comp, count);
}

FPObject * geogLayout::addComponentCluster(ComponentType type, int count, double area, double maxARArg, double minARArg, GeographyHint hint) {
    FPObject * comp = FPContainer::addComponentCluster(type, count, area, maxARArg, minARArg);
    comp->setHint(hint);
    return comp;
}

FPObject * geogLayout::addComponentCluster(string name, int count, double area, double maxARArg, double minARArg, GeographyHint hint) {
    FPObject * comp = FPContainer::addComponentCluster(name, count, area, maxARArg, minARArg);
    comp->setHint(hint);
    return comp;
}

bool geogLayout::layout(FPOptimization opt, double targetAR) {
    // All this routine does is set up some data structures,
    //   and then call the helper to recursively do the layout work.
    // We will keep track of containers we make in a "stack".
    // We will also need to keep track of center items so they can be layed out last.

    // Assume no item will be more than split in two.
    int itemCount = getComponentCount();

    int maxArraySize = itemCount * 2;
    FPObject ** layoutStack = (FPObject **) malloc(sizeof (FPObject *) * maxArraySize);
    FPObject ** centerItems = (FPObject **) (malloc(sizeof (FPObject *) * FPContainer::maxItemCount));
    for (int i = 0; i < maxArraySize; i++) layoutStack[i] = 0;
    for (int i = 0; i < maxItemCount; i++) centerItems[i] = 0;

    // Backup all the items in case we need to re-layout
    FPObject ** backupItems = (FPObject **) malloc(sizeof (FPObject *) * itemCount);
    for (int i = 0; i < itemCount; i++) backupItems[i] = getComponent(i);

    // Create a counter to exit loop
    bool retval = true;
    int retryCount = 0;

    do {
        // Calculate the total area, and the implied target width and height.
        area = totalArea();
        double remHeight = sqrt(area / targetAR);
        double remWidth = area / remHeight;
        
        if (checkOverlap && legalizing) {
            remHeight += expandHeight;
            remWidth += expandWidth;
            area = remHeight * remWidth;
        }

        if (verbose)
            cout << "In geogLayout.  A=" << area << " W=" << remWidth << " H=" << remHeight << "\n";

        // Now do the real work.
        try {
            retval = layoutHelper(remWidth, remHeight, 0, 0, layoutStack, 0, centerItems, 0);
        } catch (OverlapException& e) { //Triggered if components overlap
            if (verbose) cout << e.what() << endl;
            
            //Clean up erroneous layout components
            for (int i = 0; i < maxArraySize; i++) layoutStack[i] = 0;
            for (int i = 0; i < maxItemCount; i++) centerItems[i] = 0;
            
            //Restore them into the items**[]
            for (int i = getComponentCount(); i > 0; i--) removeComponent(0);
            for (int i = itemCount; i > 0; i--) {
                GeographyHint compHint = backupItems[i-1]->getHint();
                if (compHint == LeftRight || compHint == TopBottom || compHint == LeftRightMirror || compHint == TopBottomMirror || compHint == LeftRight180 || compHint == TopBottom180) {                  
                    if (backupItems[i-1]->getState()) backupItems[i-1]->setCount(2*backupItems[i-1]->getCount());
                }
                addComponentToFront(backupItems[i-1]);
            }
            
            //Reset legalizing helpers
            rightMark = true;
            topMark = true;
            retval = false;            
            
            //Method 1: Sort by decreasing area
            //sortByArea();
            //checkOverlap = false;
            
        } catch (AreaException& e) { //Triggered if fixed area is desired
            if (verbose) cout << e.what() << endl;
            
            for (int i = 0; i < maxArraySize; i++) layoutStack[i] = 0;
            for (int i = 0; i < maxItemCount; i++) centerItems[i] = 0;
            for (int i = getComponentCount(); i > 0; i--) removeComponent(0);
            for (int i = itemCount; i > 0; i--) addComponentToFront(backupItems[i-1]);
            
            //Reset legalizing helpers
            rightMark = true;
            topMark = true;
            retval = false;
            
            //Re-defined targetAR based on geogHint and reset item list.
            targetAR = newAR;
            
        }
        
        retryCount++;
  
    } while (!retval && retryCount < maxRetry);

    cout << "retryCount = " << retryCount << "\n";
    
    // By now, the item list should be empty.
    if (getComponentCount() != 0) {
        cerr << "Non empty item list after recursive layout in geographic layout.\n";
        cerr << "Remaining Component count=" << getComponentCount() << "\n";
        for (int i = 0; i < getComponentCount(); i++) cerr << "Component " << i << " is of type " << getComponent(i)->getName() << "\n";
        return false;
    }

    // Now install the list of containers as our items.
    // Now go through the layout components and find the containers width and height.
    width = 0;
    height = 0;
    for (int i = 0; i < maxArraySize; i++) {
        FPObject * comp = layoutStack[i];
        if (!comp) break;
        FPContainer::addComponent(comp);
        width = MAX(width, comp->getX() + comp->getWidth());
        height = MAX(height, comp->getY() + comp->getHeight());
    }
    area = width * height;
    expandHeight = 0; expandWidth = 0;

    free(centerItems);
    free(layoutStack);
    free(backupItems);
    return retval;
}

bool geogLayout::layoutHelper(double remWidth, double remHeight, double curX, double curY, FPObject ** layoutStack, int curDepth, FPObject ** centerItems, int centerItemsCount) {
    int itemCount = getComponentCount();
    // Check for the end of the recursion.
    if (itemCount == 0 && centerItemsCount == 0) return true;

    // Check if it time to layout the center items.
    // For now we will just stick them in a bag layout, and then use all remaining area to lay it out.
    if (itemCount == 0 && centerItemsCount > 0) {
        // First find out if there are repeating units that need a grid to layout.
        // We will do this by finding the GCD of the counts in the collection of components.
        int gcd = centerItems[0]->getCount();
        for (int i = 1; i < centerItemsCount; i++) gcd = GCD(gcd, centerItems[i]->getCount());
        bagLayout * BL = new bagLayout();
        for (int i = 0; i < centerItemsCount; i++) {
            FPObject * item = centerItems[i];
            BL->addComponent(item, item->getCount() / gcd);
        }

        if (verbose)
            cout << "Laying out Center items.  Count=" << centerItemsCount << " GCD=" << gcd << "\n";

        // Use all remaining area.
        double targetAR = remWidth / remHeight;
        FPContainer * FPLayout = BL;
        if (gcd > 1) {
            // Put the bag container into a grid container and lay it out.
            gridLayout * GL = new gridLayout();
            GL->addComponent(BL, gcd);
            FPLayout = GL;
        }
        // Not sure if we need to set the hint, but when I missed this for the grid below, it was a bug.
        FPLayout->setHint(Center);
        FPLayout->layout(AspectRatio, targetAR);
        if (verbose)
            cout << "Laying out Center item(s).  Current x and y are(" << curX << "," << curY << ")\n";
        FPLayout->setLocation(curX, curY);
        // Put the layout on the stack.
        // DO we really need to do this????
        layoutStack[curDepth] = FPLayout;
        
        if (legalizing && checkOverlap) detectOverlap(layoutStack, curDepth, FPLayout);
        
        return true;
    }

    // Start by pealing off the first component cluster.
    // And getting the standard information about it.
    FPObject * comp = removeComponent(0);
    GeographyHint compHint = comp->getHint();

    // In this case, we will just save them up for now, and then lay them out last.
    if (compHint == Center) {
        centerItems[centerItemsCount] = comp;
        centerItemsCount += 1;
        layoutHelper(remWidth, remHeight, curX, curY, layoutStack, curDepth, centerItems, centerItemsCount);
    }

    if (compHint == LeftRight || compHint == TopBottom || compHint == LeftRightMirror || compHint == TopBottomMirror || compHint == LeftRight180 || compHint == TopBottom180) {
        // Check if the count is a multiple of two, and if half will fit on a side.
        // If not, just put it in one side.
        // If so, split the component into two, and handle the opposite sides separately.
        // In order to handle the split in a general way (with nested layouts)
        //    we will have to put the component into a bag for each side.
        // TODO Put some quick calc here to see if we can fit with half the items
        // If not, don't split.
        if (comp->getCount() % 2 == 0) {
            int newCount = comp->getCount() / 2;
            comp->setCount(newCount);
            comp->setState(true);
            bagLayout * BL1 = new bagLayout();
            bagLayout * BL2 = new bagLayout();
            BL1->addComponent(comp);
            BL2->addComponent(comp);
            if (compHint == LeftRight || compHint == LeftRightMirror || compHint == LeftRight180) {
                BL1->setHint(Left);
                BL2->setHint(Right);
                if (compHint == LeftRightMirror) BL2->xMirror = true;
            } else if (compHint == TopBottom || compHint == TopBottomMirror || compHint == TopBottom180) {
                
                //Legalization feature: Inverts Top/Bottom order to Bottom/Top to always start on Bottom or Left.
                //                      Prevents some legalizing issues.
                if (topBottomInversion) {
                    BL1->setHint(Bottom);
                    BL2->setHint(Top);
                } else {
                    BL1->setHint(Top);
                    BL2->setHint(Bottom);
                }
                
                if (compHint == TopBottomMirror) BL2->yMirror = true;
            }
            if (compHint == TopBottom180 || compHint == LeftRight180) {
                BL2->xMirror = true;
                BL2->yMirror = true;
            }
            comp = BL1;
            addComponentToFront(BL2);
        } else {
            if (compHint == LeftRight || compHint == LeftRightMirror || compHint == LeftRight180) {
                comp->setHint(Left);
            } else { //TopBottom and TBMirror and TB180
                if (topBottomInversion) comp->setHint(Bottom);
                else                    comp->setHint(Top);
            }
        }
        compHint = comp->getHint();
    }
    
    // Now handle left, right, top, bottom.
    double newX, newY;
    bool outOfBound, isGoodAR;
    if (compHint == Left || compHint == Top || compHint == Right || compHint == Bottom) {
        // Stuff the comp into a grid, and see if we can layout it out in the desired shape.
        // TODO TODO This assumes a component has an area.  For a container, it will not have an area until it gets layed out.
        double totalArea = comp->totalArea();
        
        // Legalizing: outOfBound is guaranteed if the component's area is greater than remaining area
        if (legalizing) {
            outOfBound = totalArea > remWidth*remHeight;
        } else {
            outOfBound = false;
        }

        if (verbose)
            cout << "In geog, for component " << comp->getName() << " total component area=" << totalArea << "\n";

        double targetWidth, targetHeight;
        newX = curX;
        newY = curY;
        if (compHint == Left || compHint == Right) {
            targetHeight = remHeight;
            targetWidth = totalArea / targetHeight;
        } else // if (compHint == Top || compHint == Bottom)
        {
            targetWidth = remWidth;
            targetHeight = totalArea / targetWidth;
        }
        //if (compHint == Left) newX += targetWidth;
        //if (compHint == Right) curX = curX + (remWidth - targetWidth);
        //if (compHint == Top) curY = curY + (remHeight - targetHeight);
        //if (compHint == Bottom) newY += targetHeight;

        // Charles TODO: Here can be a potential spot to check the sign of newX/Y and curX/Y (cannot be negative)

        double targetAR = targetWidth / targetHeight;
        FPObject * FPLayout = comp;
        if (comp->getCount() > 1) {
            gridLayout * grid = new gridLayout();
            grid->addComponent(comp);
            grid->setHint(compHint);
            FPLayout = grid;
        }


        // Charles Start of Add-on Feature
        // One may choose to re-layout if the targetAR cannot be respected due to constraints.
        isGoodAR = FPLayout->layout(AspectRatio, targetAR);
        double diffWidth = 0; double diffHeight = 0;
        
        if (!isGoodAR && legalizing) {
            if (compHint == Left || compHint == Right) {
                diffWidth = FPLayout->getWidth() - targetWidth;
                double newHeight = FPLayout->getHeight();

                //This getArea() returns the total area of all the components in this container.
                newAR = getArea() / pow(newHeight, 2);
            }
            else //(compHint == Top || compHint == Bottom)
            {
                diffHeight = FPLayout->getHeight() - targetHeight;
                double newWidth = FPLayout->getWidth();
                newAR = pow(newWidth, 2) / getArea();
            }

            // Skip it if we want to increase the total area without re-layout
            if (!changeArea && legalizing) throw AreaException();
        }

        // TODO: Need to figure out a way to deal with Left/Bottom outOfBound issues
        // TODO: Another case to check is when we have bad AR with Left/Bottom and TopRightMarked.
        if (compHint == Left) {
            newX += FPLayout->getWidth();
            remWidth -= FPLayout->getWidth();
        }
        
        if (compHint == Bottom) {
            newY += FPLayout->getHeight();
            remHeight -= FPLayout->getHeight();
        }

        //Allow the component to cross the original boundary.
        //targetWidth can be different than FPLayout->getWidth() if AR is limited by constraints.
        //We may choose to not cross the boundary by changing the - targetWidth to - getWidth().
        if (compHint == Right) {
            if (rightMark && legalizing) {
                if (!outOfBound) curX = curX + (remWidth - targetWidth);
                //remWidth -= targetWidth;
                remWidth = remWidth - FPLayout->getWidth() + diffWidth;
                rightMark = false;
            } else {
                if (!outOfBound) curX = curX + (remWidth - FPLayout->getWidth());
                remWidth -= FPLayout->getWidth();
            }

        }

        if (compHint == Top) {
            if (topMark && legalizing) {
                if (!outOfBound) curY = curY + (remHeight - targetHeight);
                //remHeight -= targetHeight; 
                remHeight = remHeight - FPLayout->getHeight() + diffHeight; 
                topMark = false;
            } else {
                if (!outOfBound) curY = curY + (remHeight - FPLayout->getHeight()); 
                remHeight -= FPLayout->getHeight();
            }
        }

        FPLayout->setLocation(curX, curY);
        // Put the layout on the stack.
        layoutStack[curDepth] = FPLayout;

        
        // Charles TODO: Q: Should we be able to detect overlapping components?
        // This is where Charles thinks to add an overlapping detection.
        // Let's start with something that's O(N^2), from the last (most recently added) layoutStack component.
        // Compare this last component's coordinate and size to all others in the layoutStack.
        // If overlap is detected, we have two options:
        // 1. Re-layout in different orders
        // 2. Look for deadspace and try to fit in (if can't find anything fit, we re-layout.
        
        
        // Overlap detection O(N^2) for now
        if (legalizing && checkOverlap && curDepth > 0) detectOverlap(layoutStack, curDepth, FPLayout);

        try {
            layoutHelper(remWidth, remHeight, newX, newY, layoutStack, curDepth + 1, centerItems, centerItemsCount);
            //if (!retval) return false;
        } catch (OverlapException e) {
            throw e;
        } catch (AreaException e) {
            throw e;
        }
    } else if (compHint != Center) {
        cerr << "Hint is not any of the recognized hints.  Hint=" << compHint << "\n";
        cerr << "Component is of type " << comp->getType() << "\n";
    }

    // TODO Here is where we should do bottom up fixups!!
    // For now the only recourse is to try more AR flex in components and relayout.


    return true;
}


bool FPContainer::detectOverlap(FPObject ** layoutStack, int curDepth, FPObject * FPLayout)
{
    double x1, x2, y1, y2, h1, h2, w1, w2, d;
    x2 = FPLayout->getX();
    y2 = FPLayout->getY();
    h2 = FPLayout->getHeight();
    w2 = FPLayout->getWidth();
    
    for (int i = curDepth - 1; i >= 0; i--) {

    // Only compare to the one before itself, O(N) now.
    //if (curDepth > 0) {        
        double widthOver = 0, heightOver = 0;
        bool isHeightOverlap = false, isWidthOverlap = false;
        FPObject * stackFPLayout = layoutStack[i]; //curDepth - 1

        x1 = stackFPLayout->getX(); 
        y1 = stackFPLayout->getY();
        h1 = stackFPLayout->getHeight();
        w1 = stackFPLayout->getWidth();
        d = 0.00000001; //for rounding error

        if (x2 > x1 && w1 - (x2 - x1) > d) {
            widthOver = w1 - (x2 - x1);
            isWidthOverlap = true;
        }               
        if (x2 < x1 && w2 - (x1 - x2) > d) {
            widthOver = w2 - (x1 - x2);
            isWidthOverlap = true;
        }
        
        if (x2 == x1) {
            isWidthOverlap = true;
            if (w2 >= w1) {
                widthOver = w1;
            } else {
                widthOver = w2;
            }
        }
        
        
        if (y2 > y1 && h1 - (y2 - y1) > d) {
            heightOver = h1 - (y2 - y1);
            isHeightOverlap = true;
        }
        if (y2 < y1 && h2 - (y1 - y2) > d) {
            heightOver = h2 - (y1 - y2);
            isHeightOverlap = true;
        }
        
        if (y2 == y1) {
            isHeightOverlap = true;
            if (h2 >= h1) {
                heightOver = h1;
            } else {
                heightOver = h2;
            }
        }

        if (isHeightOverlap && isWidthOverlap) {
            if (widthOver > heightOver) {
                expandHeight += heightOver;                       
            } else {
                expandWidth += widthOver;
            }

            if (verbose)
            cout << "In geog, detected overlap for component " << FPLayout->getName() 
                 << " expandHeight = " << expandHeight << "\n" << " expandWidth = " << expandWidth << "\n" ;
            throw OverlapException();
        } 
    }
    
    return true;
}


void geogLayout::outputHotSpotLayout(ostream& o, double startX, double startY) {
    pushMirrorContext(startX, startY);
    string layoutName = getUniqueName();
    o << "# Start of " << layoutName << " Layout.\n";
    o << "# Total Cluster Stats: X=" << calcX(startX) << " Y=" << calcY(startY) << " W=" << width
            << "mm H=" << height << "mm Area=" << area << "mm^2\n";
    for (int i = 0; i < getComponentCount(); i++) {
        FPObject * obj = getComponent(i);
        obj->outputHotSpotLayout(o, x + startX, y + startY);
    }
    o << "# End of " << layoutName << " Layout.\n";
    popMirrorContext();
}

/*
// Legalization
Legalize::Legalize() {
    isLegalizing = true;
    changeArea = false;
    topBottomInversion = false;
    
    topMark = true;
    rightMark = true;
}
*/

// Output Helpers.

ostream& outputHotSpotHeader(const char * filename) {
    cout << "Outputing floorplan to: " << filename << "\n";

    // Reset the name to counts map for this output.
    NameCounts.clear();

    ofstream& out = *(new ofstream(filename));
    out << "# FloorPlan output from ArchFP: UVA's Rapid Prototyping FloorPlanner.\n";
    out << "# Formatted for Input to HotSpot.\n";
    out << "# Line Format: <unit-name>\\t<width>\\t<height>\\t<left-x>\\t<bottom-y>\n";
    out << "# all dimensions are in meters\n";
    out << "# comment lines begin with a '#'\n";
    out << "# comments and empty lines are ignored\n\n";

    return out;
}

void outputHotSpotFooter(ostream& o) {
    // We don't need to close, as the destructor will do the close for us.
    // o.close();
    delete(&o);
}

string getStringFromInt(int in) {
    stringstream ss;
    ss << in;
    return ss.str();
}

/*
// Returns the difference between targetWidth/targetHeight and actual ones due to AR constraint
// TODO: Evaluate the newAR as well.
double calcDiff(GeographyHint compHint, double actualValue, double targetValue) {
    return FPLayout- - targetValue;
}
 */