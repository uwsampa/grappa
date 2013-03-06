// benchmark with (insert insert delMin)^N (insert delMin delMin)^N
// just in time RNG
// version 3: use easier to handle binary heaps


#include <limits.h>
#include <iostream.h>
#include <stdlib.h>
#include <math.h>

#define DEBUGLEVEL 0
#include "util.h"
#define KNH
//#define H2
//#define H4
//#define HSLOW

#ifdef KNH
#  include "knheap.C"
#  define HTYPE KNHeap<int, int> 
#  define HINIT heap(INT_MAX, -INT_MAX)
#else
#  ifdef H4
#    include "heap4.h"
#    define HTYPE Heap4<int, int> 
#    define HINIT heap(INT_MAX, -INT_MAX, n)
#  else
#    ifdef H2
#      include "heap2.h"
#      define HTYPE Heap2<int, int> 
#      define HINIT heap(INT_MAX, -INT_MAX, n)
#    else 
#      ifdef HSLOW
#        include "heap-CLR.h"
#        define HTYPE Heap2<int, int> 
#        define HINIT heap(INT_MAX, -INT_MAX, n)
#      endif
#    endif
#  endif
#endif


#define rand32() (ran32State = 1664525 * ran32State + 1013904223)
#define getRandom() ((rand32()), (int)(ran32State >> 2))

inline void onePass(HTYPE& heap, int n)
{ int j, newElem, k, v;
  double insertSum=0, deleteSum=0;
  static int ran32State = 42 << 20;
  
  Debug3(cout << heap.getSize());
  Assert(heap.getSize() == 0);
  for (j = 0;  j < n;  j++) {   
    newElem = getRandom();
    heap.insert(newElem, j);
    Debug0(insertSum += newElem);
    Debug3(cout << newElem << "in ");
    
    heap.deleteMin(&k, &v);
    Debug0(deleteSum += k);
    Debug3(cout << k << "out ");
    
    newElem = getRandom();
    heap.insert(newElem, j + 1);
    Debug0(insertSum += newElem);
    Debug3(cout << newElem << "in ");
  }
  Assert0(heap.getSize() == n);
  for (j = 0;  j < n;  j++) {   
    heap.deleteMin(&k, &v);
    Debug0(deleteSum += k);
    Debug3(cout << k << "out ");

    newElem = getRandom();
    heap.insert(newElem, j);
    Debug0(insertSum += newElem);
    Debug3(cout << newElem << "in ");
    
    heap.deleteMin(&k, &v);
    Debug0(deleteSum += k);
    Debug3(cout << k << "out ");
  }
  Assert0(deleteSum == insertSum);
  Assert(heap.getSize() == 0);
}


int main(int argc, char **argv)
{ 
  Assert(argc > 1);
  int n = atoi(argv[1]);
  int i;
  int ran32State = 42 << 20;
  int repeat = 1;
  double startTime, endTime;
  if (argc > 2) repeat = atoi(argv[2]);
  //#ifdef H2
  //HTYPE *temp = new HTYPE(INT_MAX, -INT_MAX);
  //HTYPE& heap =*temp;
  //#else
  HTYPE HINIT;
  //#endif

  // warmup
  onePass(heap, n);

  startTime = cpuTime();
  for (i = 0;  i < repeat;  i++) {
    onePass(heap, n);
  }
  endTime = cpuTime();

  // output time per insert-delete-pair and this time over log n
  double timePerPair = (endTime - startTime) / n / repeat / 3.0;
  double timePerCompare = timePerPair / (log((double)n) / log(2.0));
  cout << n << " " << timePerPair * 1e9 << " " << timePerCompare * 1e9 << endl;
}

