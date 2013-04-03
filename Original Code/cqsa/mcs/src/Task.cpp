/*                                                                              
   Copyright 2009 Carnegie Mellon University.                                   
                                                                                
   This software developed under GRC contract 2008-HJ-1795 funded by            
   the Semiconductor Research Corporation.                                      
*/

#include <cstdlib>

#include "Task.h"

Task::Task(string n, int r)
{
  name = n;
  req = r;
}

Task::~Task()
{
  // Nothing to do here...assuming "strings" clean themselves up automatically
}
