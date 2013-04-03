/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#ifndef TASK_H_
#define TASK_H_

#include <string>

using namespace std;

class Task
{
private:
  string name;
  int req;
	
public:
	
  // Constructors
  Task(string n, int r);
	
  // Destructor
  virtual ~Task();
	
  // Accessors
  string getName() { return name; }
  int getReq() { return req; }
  
};

#endif
