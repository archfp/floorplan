/**************************************************************************
***    
*** Copyright (c) 2000-2006 Regents of the University of Michigan,
***               Saurabh N. Adya, Hayward Chan, Jarrod A. Roy
***               and Igor L. Markov
***
***  Contact author(s): sadya@umich.edu, imarkov@umich.edu
***  Original Affiliation:   University of Michigan, EECS Dept.
***                          Ann Arbor, MI 48109-2122 USA
***
***  Permission is hereby granted, free of charge, to any person obtaining 
***  a copy of this software and associated documentation files (the
***  "Software"), to deal in the Software without restriction, including
***  without limitation 
***  the rights to use, copy, modify, merge, publish, distribute, sublicense, 
***  and/or sell copies of the Software, and to permit persons to whom the 
***  Software is furnished to do so, subject to the following conditions:
***
***  The above copyright notice and this permission notice shall be included
***  in all copies or substantial portions of the Software.
***
*** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
*** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
*** OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
*** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
*** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
*** OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
*** THE USE OR OTHER DEALINGS IN THE SOFTWARE.
***
***
***************************************************************************/


#ifndef SEQPAIR_H
#define SEQPAIR_H

#include <ABKCommon/uofm_alloc.h>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace parquetfp
{
   class SeqPair
   {
   private:
      uofm::vector<unsigned> _XX;         //seqPair in the X direction
      uofm::vector<unsigned> _YY;         //seqPair in the Y direction

   public:
      // empty sequence-pair
      SeqPair(){}

      // random sequence-pair of length "size"
      SeqPair(unsigned size);

      // sequence-pair (X, Y)
      SeqPair(const uofm::vector<unsigned>& X, const uofm::vector<unsigned>& Y);

      inline const uofm::vector<unsigned>& getX() const;
      inline const uofm::vector<unsigned>& getY() const;
  
      inline void putX(const uofm::vector<unsigned>& X);
      inline void putY(const uofm::vector<unsigned>& Y);

      inline void printX(void) const;
      inline void printY(void) const;

      inline unsigned getSize(void) const;
   };

   // -----IMPLEMENTATIONS-----
   const uofm::vector<unsigned>& SeqPair::getX(void) const
   {   return _XX; }

   const uofm::vector<unsigned>& SeqPair::getY(void) const
   {   return _YY; }

   void SeqPair::putX(const uofm::vector<unsigned>& X)
   { _XX = X; }

   void SeqPair::putY(const uofm::vector<unsigned>& Y)
   { _YY = Y; }

   unsigned SeqPair::getSize(void) const
   {  return _XX.size(); }

   void SeqPair::printX(void) const
   {
      std::cout<<"X Seq Pair"<<std::endl;
      for(unsigned i=0; i<_XX.size(); ++i)
      {
         std::cout<<_XX[i]<<" ";
      }
      std::cout<<std::endl;
   }

   void SeqPair::printY(void) const
   {
      std::cout<<"Y Seq Pair"<<std::endl;
      for(unsigned i=0; i<_XX.size(); ++i)
      {
         std::cout<<_YY[i]<<" ";
      }
      std::cout<<std::endl;
   }
}
//using namespace parquetfp;

#endif
