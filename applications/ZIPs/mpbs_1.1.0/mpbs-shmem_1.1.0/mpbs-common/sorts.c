#include "types.h"
#include "sorts.h"

/********1*****|***2*********3*********4*********5*********6*********7**!

description:   Recursive implementations of the Quicksort algorithm 
               developed by C.A.R. Hoare.  This is an in-place
	       implementation that requires an additional O(log(n)) space
	       to accomodate the call stack.  Average case running time
	       is O(nlog(n)).

	       Uses a helper function, qsortui to keep the 
	       interface the same as the other sorting functions. 
	       quicksort shouldn't be called directly.  

 
  variables:   varible            description
               --------------     --------------------------------------
               n                - Array length

               a                - Array to be sorted.


       lmod:   10/06/11

*********1*****|***2*********3*********4*********5*********6*********7**/
void swap64(uint64 *x, uint64 *y) {
  uint64 temp = *x;
  *x = *y;
  *y = temp;
}

void quicksort64(uint64 *a, uint64 low, uint64 high) {
  if( low + 10 > high)
    isortui64(high - low + 1, &a[low]);
  else{
    uint64 pivot;
    uint64 middle = (low + high) / 2;
    if( a[middle] < a[low])
      swap64(&a[middle], &a[low]);
    if( a[high] < a[low])
      swap64(&a[high], &a[low]);
    if( a[high] < a[middle])
      swap64(&a[high], &a[middle]);
    
    swap64(&a[middle], &a[high-1]);
    pivot = a[high-1];
    
    uint64 i, j;
    i = low;
    j = high -1;
    while( 1 ) {
      while(a[++i] < pivot)
	;
      while(pivot < a[--j])
	;
      if(i >= j)
	break;
      swap64(&a[i], &a[j]);
    }

    swap64(&a[i], &a[high -1]);

    // Each recursive call to quicksort is independent, so allow
    // the OpenMP threads to execute them in parallel.
    #pragma omp task
    quicksort64(a, low, i-1);
    
    #pragma omp task
    quicksort64(a, i+1, high);
  }
}

void qsortui64( uint64 n, uint64 *a ) {
  // Spawn threads here, let one pass to start the quicksort.
  #pragma omp parallel
  {
    #pragma omp single
    quicksort64(a, 0, n-1);
  }
}

void
ssortui64( uint64 n, uint64 *a )
/********1*****|***2*********3*********4*********5*********6*********7**!

               *******************************************
               *   UNCLASSIFIED//FOR OFFICIAL USE ONLY   *
               *******************************************

source file:   sorts.src

description:   From Wikipedia:
               Selection sort is an in-place comparison sort. It has
               O(n2) complexity, making it inefficient on large lists,
               and generally performs worse than the similar insertion
               sort. Selection sort is noted for its simplicity, and
               also has performance advantages over more complicated
               algorithms in certain situations.

               The algorithm finds the minimum value, swaps it with
               the value in the first position, and repeats these steps
               for the remainder of the list. It does no more than n
               swaps, and thus is useful where swapping is very
               expensive.

               This function performs an ascending sort on a array of
               unsigned long 64bit integers.

 invoked by:   routine            description
               --------------     --------------------------------------
               mkfiles64        - void
                                  This subroutine creates the files for
                                  each process.

    invokes:   routine            description
               ---------------    --------------------------------------
               return           - void
                                  C library function that returns 
                                  control to calling function.

  variables:   varible            description
               --------------     --------------------------------------
               n                - uint32
                                  Number in array.

               a                - uint64 *
                                  Array to be sorted.

               i, j             - int
                                  Generic loop variables.

               temp             - uint64
                                  Variable for storing swapped int.

       lmod:   07/21/11

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   int i, j;
   uint64 temp;

   for( i = 0; i < n - 1; i++ )
   {
      for( j = i+1; j < n; j++ )
      {
         if( a[j] < a[i] )
         {
            temp = a[i];
            a[i] = a[j];
            a[j] = temp;
         }
      }
   }
   return;
}

void
isortui64( uint64 n, uint64 *a )
/********1*****|***2*********3*********4*********5*********6*********7**!

               *******************************************
               *   UNCLASSIFIED//FOR OFFICIAL USE ONLY   *
               *******************************************

source file:   sorts.src

description:   From Wikipedia:
               Insertion sort is a simple sorting algorithm that is
               relatively efficient for small lists and mostly-sorted
               lists, and often is used as part of more sophisticated
               algorithms. It works by taking elements from the list one
               by one and inserting them in their correct position into
               a new sorted list. In arrays, the new list and the
               remaining elements can share the array's space, but
               insertion is expensive, requiring shifting all following
               elements over by one.

               This function performs an ascending sort on a array of
               unsigned long 64bit integers.

 invoked by:   routine            description
               --------------     --------------------------------------
               mkfiles64        - void
                                  This subroutine creates the files for
                                  each process.

    invokes:   routine            description
               ---------------    --------------------------------------
               return           - void
                                  C library function that returns 
                                  control to calling function.

  variables:   varible            description
               --------------     --------------------------------------
               n                - uint32
                                  Number in array.

               a                - uint64 *
                                  Array to be sorted.

               i, j             - int
                                  Generic loop variables.

               look             - uint64
                                  Stores variable for comparison with
                                  subsequent values. 

       lmod:   07/21/11

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   int i, j;
   uint64 look;

   for( i = 1; i < n; i++ )
   {
      /* look at ith element */
      look = a[i];

      j = i - 1;
      while( j >=0 && a[j] > look )
      {
         a[j+1] = a[j];
         j--;
      }
      a[j+1] = look;
   }
   return;
}

void
bsortui64( uint64 n, uint64 *a )
/********1*****|***2*********3*********4*********5*********6*********7**!

               *******************************************
               *   UNCLASSIFIED//FOR OFFICIAL USE ONLY   *
               *******************************************

source file:   sorts.src

description:   From Wikipedia:
               Bubble sort is a simple sorting algorithm. The algorithm
               starts at the beginning of the data set. It compares the
               first two elements, and if the first is greater than the
               second, then it swaps them. It continues doing this for
               each pair of adjacent elements to the end of the data set.
               It then starts again with the first two elements,
               repeating until no swaps have occurred on the last pass.
               This algorithm's average and worst case performance is
               O(n^2), so it is rarely used to sort large, unordered,
               data sets. Bubble sort can be used to sort a small
               number of items (where its inefficiency is not a high
               penalty). Bubble sort may also be efficiently used on a
               list that is already sorted except for a very small
               number of elements. For example, if only one element is
               not in order, bubble sort will only take 2n time. If two
               elements are not in order, bubble sort will only take at
               most 3n time.

               Bubble sort average case and worst case are both O(n^2).

               This function performs an ascending sort on a array of
               unsigned long 64bit integers.

 invoked by:   routine            description
               --------------     --------------------------------------
               mkfiles64        - void
                                  This subroutine creates the files for
                                  each process.

    invokes:   routine            description
               ---------------    --------------------------------------
               return           - void
                                  C library function that returns 
                                  control to calling function.

  variables:   varible            description
               --------------     --------------------------------------
               n                - uint32
                                  Number in array.

               a                - uint64 *
                                  Array to be sorted.

               i                - int
                                  Generic loop variables.

               loop             - int
                                  Loop control variable( 0 or 1 );

               temp             - uint64
                                  Variable for storing swapped int.

       lmod:   07/21/11

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   int i;
   int loop;
   uint64 temp;

   do
   {
      loop = 0;
      for( i = 0; i < n - 1; i++ )
      {
         if( a[i+1] < a[i] )
         {
            temp = a[i];
            a[i] = a[i+1];
            a[i+1] = temp;
            loop = 1;
         }
      }
   } while( loop );
   return;
}


void
radixui64( uint32 r, uint64 n, uint64 *a )
/********1*****|***2*********3*********4*********5*********6*********7**!

               *******************************************
               *   UNCLASSIFIED//FOR OFFICIAL USE ONLY   *
               *******************************************

source file:   sorts.src

description:   From Wikipedia:
               Radix sort is an algorithm that sorts numbers by
               processing individual digits. n numbers consisting of k
               digits each are sorted in O(nk) time. Radix sort can
               either process digits of each number starting from the
               least significant digit (LSD) or the most significant
               digit (MSD). The LSD algorithm first sorts the list by
               the least significant digit while preserving their
               relative order using a stable sort. Then it sorts them
               by the next digit, and so on from the least significant
               to the most significant, ending up with a sorted list.
               While the LSD radix sort requires the use of a stable
               sort, the MSD radix sort algorithm does not (unless
               stable sorting is desired). In-place MSD radix sort is
               not stable. It is common for the counting sort algorithm
               to be used internally by the radix sort. Hybrid sorting
               approach, such as using insertion sort for small bins
               improves performance of radix sort significantly.

               This function performs an ascending sort on a array of
               unsigned long 64bit integers.

 invoked by:   routine            description
               --------------     --------------------------------------
               mkfiles64        - void
                                  This subroutine creates the files for
                                  each process.

 
  variables:   varible            description
               --------------     --------------------------------------
               r                - uint32
                                  Radix length.

               n                - uint32
                                  Number in array.

               a                - uint64 *
                                  Array to be sorted.

               i, j             - int
                                  Generic loop variables.

               k                - int
                                  Index control variable. 

               m                - uint64
                                  Bit selection variable.
 
               look             - uint64
                                  Variable for storing swapped int. 

       lmod:   07/21/11

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   int i, j, k;
   uint64 m;
   uint64 look;

   m = 0x1UL;

   for( i = 1; i <= r; i++ )
   {
      for( j = 1; j < n; j++ )      
      {
         /* look at ith element */
         look = a[j];
         k = j - 1;
         while( k >=0 && ((look&m) < (a[k]&m)) )
         {
            a[k+1] = a[k];
            k--;
         }
         a[k+1] = look;
      }
      m <<= 1;
   }
   return;
}
