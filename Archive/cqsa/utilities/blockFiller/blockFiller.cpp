/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#include <cstdlib>
#include <cstdio>
#include <string>

#include "blockFiller.h"

void printList(node *list, int mcsVerbosity)
{
  node *curNode = list;
  
  while(curNode) {
    DEBUG printf("%f\n",curNode->value);
    curNode = curNode->next;
  }
  DEBUG printf("\n");

  return;
}

int listLength(node *list)
{
  node *curNode = list;
  int x = 0;
  
  while(curNode != NULL) {
    x++;
    curNode = curNode->next;
  }
  
  return x;
}

int findValue(node *list, float value)
{
  node *curNode = list;
  int x = 0;

  while(curNode != NULL) {
    if(fabsf(curNode->value - value) < epsilon) {
      return x;
    }
    else {
      x++;
    }
    curNode = curNode->next;
  }
      
  return -1;
}

float getValue(node *list, int index)
{
  node *curNode = list;
  int x;
  
  for(x = 0; x < index; x++) {
    curNode = curNode->next;
  }

  return curNode->value;
}

// Function to insert a value into a linked list in sorted order
// Only one copy of each value will be maintained in the list
void insertValueInList(node *list, float value)
{
  node *curNode = list;

  // Step through all of the nodes in the list
  while(curNode != NULL) {
    
    // Stop if the value is found in the list
    // fabsf is used to get around weird rounding issues
    if(fabsf(curNode->value - value) < epsilon) {
      return;
    }
    
    // If the next value is larger than the value to insert or the current value is at the end of the list,
    // create a new node and insert the value in the list; fabsf is used to get around weird rounding issues
    if((curNode->next == NULL) || ((curNode->next->value > value) && (fabsf(curNode->next->value - value) > epsilon))) {
      node *newNode = (node *)malloc(sizeof(node));
      newNode->value = value;
      newNode->next = curNode->next;
      curNode->next = newNode;
      return;
    }

    // Move to the next node in the list
    curNode = curNode->next;
  }

  return;
}

// Prints a matrix with element (0,0) in the bottom-left corner
void printMatrix(int *mat, int xSize, int ySize, int mcsVerbosity) 
{
  int x, y, toPrint;

  for(y = ySize - 1; y >= 0; y--) {
    for(x = 0; x < xSize; x++) {
      toPrint = mat[x + y * xSize];
      if(toPrint) {
	DEBUG printf("%c ",toPrint);
      }
      else {
	DEBUG printf("- ");
      }
    }
    DEBUG printf("\n");
  }
  DEBUG printf("\n");
	
  return;
}

int fill_blocks(System *sys)
{
  FILE *inFile;
  FILE *outFile;
  
  char outputFileName[106]; /* Assume that the input file name will not be longer than 99 characters */ 
	
  char curLine[100]; /* Assumes that line lengths will not be longer than 99 characters */
  char curBlockName[100]; /* Assumes that block names will not be longer than 99 characters */
  char emptyBlockString[4] = "_eb";
	
  float xCoord, yCoord, xSize, ySize;
  node *xPartitions = (node *)malloc(sizeof(node));
  node *yPartitions = (node *)malloc(sizeof(node));
  int xElements, yElements;
  int *occupancyMat;
  int blockChar = 'a';
  int x, y, z;
  int countRight, countUp;
  int rowSum;

  int mcsVerbosity = sys->getVerbosity();
	
  // Initialize the linked lists
  xPartitions->value = 0;
  xPartitions->next = NULL;
  yPartitions->value = 0;
  yPartitions->next = NULL;
  
  // Open the input file for reading
  inFile = fopen(sys->getFloorplanFileName().c_str(),"r");
  if(!inFile) {
    printf("Couldn't open input file %s\n",sys->getFloorplanFileName().c_str());
    return 1;
  }

  // Create the output file name
  sprintf(outputFileName,"%s.filled",sys->getFloorplanFileName().c_str());
	
  // Open the output file for writing
  outFile = fopen(outputFileName,"w");
  if(!outFile) {
    printf("Couldn't open output file %s\n",outputFileName);
    return 1;
  }

  // Read through all lines in the input file to build the xPartitions and yPartitions
  while(fgets(curLine,200,inFile) != NULL) {
    DEBUG printf("read the line\n\t%s\n",curLine);

    // Scan the current line for block coordinates and size and output existing blocks in HotSpot format
    if(sscanf(curLine,"%s\t%f\t%f\tDIMS = (%f, %f)\t: %*s\n",curBlockName,&xCoord,&yCoord,&xSize,&ySize) != 5) {
      DEBUG printf("The following line did not follow the expected input pattern and will be ignored:\n\t%s",curLine);
    }
    else {
      insertValueInList(xPartitions,xCoord);
      insertValueInList(xPartitions,xCoord + xSize);
      insertValueInList(yPartitions,yCoord);
      insertValueInList(yPartitions,yCoord + ySize);
      fprintf(outFile,"%s\t%f\t%f\t%f\t%f\n",
	      curBlockName,
	      xSize * 0.001,
	      ySize * 0.001,
	      xCoord * 0.001,
	      yCoord * 0.001);
    }
  }

  // Print information about partition lists
  DEBUG printf("xPartitions:\n");
  DEBUG printList(xPartitions, mcsVerbosity);
  DEBUG printf("yPartitions:\n");
  DEBUG printList(yPartitions, mcsVerbosity);

  xElements = listLength(xPartitions) - 1;
  yElements = listLength(yPartitions) - 1;
  DEBUG printf("xElements = %d, yElements = %d\n\n",xElements,yElements);
	
  // Allocate occupancyMat to have len(xPartitions) * len(yPartitions) elements
  occupancyMat = (int *)calloc((xElements) * (yElements),sizeof(int));

  // Read through all lines in the input file to fill in occupancyMat
  rewind(inFile);
  while(fgets(curLine,200,inFile) != NULL) {
    // Scan the current line for block coordinates and size
    if(sscanf(curLine,"%*s\t%f\t%f\tDIMS = (%f, %f)\t: %*s\n",&xCoord,&yCoord,&xSize,&ySize) == 4) {
      for(y = findValue(yPartitions,yCoord); (((yCoord + ySize) - getValue(yPartitions,y)) > epsilon) && (y < yElements); y++) {
	for(x = findValue(xPartitions,xCoord); (((xCoord + xSize) - getValue(xPartitions,x)) > epsilon) && (x < xElements); x++) {
	  occupancyMat[x + y * xElements] = blockChar;
	}
      }
      blockChar++;
    }
  }
	
  // Print occupancyMat with blanks unfilled
  DEBUG printf("occupancyMat with blanks unfilled:\n\n");
  DEBUG printMatrix(occupancyMat,xElements,yElements,mcsVerbosity);
	
  // Step through all elements of occupancyMat to find empty spaces
  blockChar = 'A';
  for(y = 0; y < yElements; y++) {
    for(x = 0; x < xElements; x++) {
			
      // If the current block is filled, move to the next one
      if(occupancyMat[x + y * xElements] != 0) {
	continue;
      }
			
      // If the current block is empty, create an empty block and expand it
      else {
	countRight = 0;
	countUp = 1;
				
	// Determine the width of the new empty block and fill in the first row
	while(((x + countRight) < xElements) && (occupancyMat[x + countRight + y * xElements] == 0)) {
	  occupancyMat[x + countRight + y * xElements] = blockChar;
	  countRight++;
	}
				
	// Determine the height of the new empty block and fill in the remaining rows
	while((y + countUp) < yElements) {
					
	  // Sum all elements in the current row to determine whether or not they are all 0
	  rowSum = 0;
	  for(z = 0; z < countRight; z++) {
	    rowSum += occupancyMat[x + z + (y + countUp) * xElements];
	  }
					
	  // If the current row is empty, fill it in and continue up to the next one
	  if(rowSum == 0) {
	    for(z = 0; z < countRight; z++) {
	      occupancyMat[x + z + (y + countUp) * xElements] = blockChar;
	    }
	    countUp++;
	  }
	  else {
	    break;
	  }
	}
	
	if(countRight == 0) {
	  printf("countRight is broken\n");
	  exit(1);
	}

	if(countUp == 0) {
	  printf("countUp is broken\n");
	  exit(1);
	}
	
	// All information about a new empty block now exists
	DEBUG printf("countRight was %d countUp was %d\n",countRight,countUp);
	DEBUG printf("Created blank block %s%c at (%.10f,%.10f) with dimensions (%.10f,%.10f)\n",
	       "_eb",
	       blockChar,
	       0.001 * getValue(xPartitions,x),
	       0.001 * getValue(yPartitions,y),
	       0.001 * (getValue(xPartitions,x + countRight) - getValue(xPartitions,x)),
	       0.001 * (getValue(yPartitions,y + countUp) - getValue(yPartitions,y)));
	DEBUG printf("xRight %.10f xLeft %.10f, yTop %.10f, yBottom %.10f\n",
	       getValue(xPartitions,x + countRight),
	       getValue(xPartitions,x),
	       getValue(yPartitions,y + countUp),
	       getValue(yPartitions,y));
	
	fprintf(outFile,"%s%c\t%.10f\t%.10f\t%.10f\t%.10f\n",
		"_eb",
		blockChar,
		0.001 * (getValue(xPartitions,x + countRight) - getValue(xPartitions,x)),
		0.001 * (getValue(yPartitions,y + countUp) - getValue(yPartitions,y)),
		0.001 * getValue(xPartitions,x),
		0.001 * getValue(yPartitions,y));
				
	// Add the name of the newly created block to the system
	sprintf(emptyBlockString,"%s%c","_eb",blockChar);
	sys->addEmptyBlock(emptyBlockString);
				
	blockChar++;
      }
    }
  }
	
  // Print occupancyMat with blanks filled
  DEBUG printf("\noccupancyMat with blanks filled:\n\n");
  DEBUG printMatrix(occupancyMat,xElements,yElements,mcsVerbosity);
	
  // Cleanup and exit
  // Create a function to free linked lists
  // cleanList(xPartitions);
  // cleanList(yPartitions);
  free(occupancyMat);
  fclose(inFile);
  fclose(outFile);
  return 0;
}
