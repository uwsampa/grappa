/*
Copyright (c) 2009, David Cheng, Viral B. Shah.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef PSORT_UTIL_H
#define PSORT_UTIL_H

#include <mpi.h>
#include <cassert>
#include <stdexcept>
#include <limits>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <functional>
#include <iterator>
#include <numeric>
#include <algorithm>
#include <vector>

#ifdef PSORTDEBUG
#define PSORT_DEBUG(_a) _a
#else
#define PSORT_DEBUG(_a)
#endif

#include "psort_seqsort.h"
#include "psort_splitters.h"
#include "psort_alltoall.h"
#include "psort_merge.h"

namespace vpsort {
  using namespace std;

  static double psort_timing[10];

  template <typename _RandomAccessIter, typename _Compare>
    bool is_sorted (_RandomAccessIter first,
		    _RandomAccessIter last,
		    _Compare comp, 
		    MPI_Comm comm) {
    
    int nproc, rank;
    MPI_Comm_size (comm, &nproc);
    MPI_Comm_rank (comm, &rank);

    int not_sorted = 0;

    typedef typename iterator_traits<_RandomAccessIter>::pointer _IterType;
    typedef typename iterator_traits<_RandomAccessIter>::value_type _ValueType;

    for (_IterType iter=first+1; iter<last; ++iter) {
      if (comp(*(iter), *(iter-1)) == true) {
	not_sorted = 1;
	break;
      }
    }

    _ValueType *left_boundary = new _ValueType[nproc];
    _ValueType *right_boundary = new _ValueType[nproc];

    _ValueType left = *first;
    MPI_Allgather (&left, sizeof (_ValueType), MPI_CHAR,
		   left_boundary, sizeof (_ValueType), MPI_CHAR,
		   comm);

    _ValueType right = *(last-1);
    MPI_Allgather (&right, sizeof (_ValueType), MPI_CHAR,
		   right_boundary, sizeof (_ValueType), MPI_CHAR,
		   comm);

    for (int i=0; i<nproc-1; ++i) {
      if (comp(left_boundary[i+1], right_boundary[i]) == true) {
	not_sorted = 1;
	break;
      }
    }

    delete [] left_boundary;
    delete [] right_boundary;

    int result[nproc];
    MPI_Allgather (&not_sorted, 1, MPI_INT,
		   result, 1, MPI_INT, comm);
    
    int all_result = accumulate (result, result + nproc, 0);
    if (all_result == 0) 
      return true;
    else 
      return false;

  }

  template <typename _RandomAccessIter, typename _Compare>
    bool is_sorted (_RandomAccessIter first,
		    _RandomAccessIter last,
		    MPI_Comm comm) {

    typedef typename iterator_traits<_RandomAccessIter>::value_type _ValueType;
    return is_sorted (first, last, std::less<_ValueType>(), comm);
  }


  static void progress (int rank, int step, char *s, MPI_Comm comm) {
    MPI_Barrier (comm);
    psort_timing[step] = MPI_Wtime ();
    if (rank == 0) {
      PSORT_DEBUG (cout << step << ". " << s << endl;);
    }
  }


  template <typename _Distance>
    void print_perf_data (_Distance *dist, MPI_Comm comm) {
    
    STLSort stl_sort;
    MedianSplit median_split;
    OOPTreeMerge oop_tree_merge;
    
    print_perf_data (dist, stl_sort, median_split, oop_tree_merge, comm);
  }

  template <typename _SeqSortType, typename _SplitType, typename _MergeType, typename _Distance>
    void print_perf_data (_Distance *dist, 
			  SeqSort<_SeqSortType> &mysort,
			  Split<_SplitType> &mysplit,
			  Merge<_MergeType> &mymerge,
			  MPI_Comm comm) {
    
    int rank, nproc;
    MPI_Comm_size (comm, &nproc);
    MPI_Comm_rank (comm, &rank);

    char **stage = new char*[5];
    stage[1] = mysort.description();
    stage[2] = mysplit.description();
    stage[3] = "alltoall";
    stage[4] = mymerge.description();
    
    if (rank == 0) cout << endl;
    double rtime[5];
    for (int i=1; i<=4; ++i) {
      double time_i = psort_timing[i] - psort_timing[i-1];
      if (rank == 0) {
        cout << i << ". " 
	     << setw(30) << left << stage[i] 
	     << setw(10) << right << ": " 
	     << setprecision(6) << time_i << " sec" << endl;
        rtime[i] = time_i;
      }    
    }

    long n_sort = 0L; for (int i=0; i<nproc; ++i) n_sort += dist[i];
    double total_time = rtime[1] + rtime[2] + rtime[3] + rtime[4];
    double mkeys_per_sec;
    double mkeys_per_sec_proc;
    if (rank == 0) {
      mkeys_per_sec = ((n_sort * log2(n_sort)) / total_time) / 1e6;
      mkeys_per_sec_proc = (((n_sort * log2(n_sort)) / total_time) / nproc) / 1e6;

      cout << setprecision(6) << endl;
      cout << setw(33) << left << "*  MKeys/sec" 
	   << setw(10) << right << ": " << mkeys_per_sec << endl;
      cout << setw(33) << left << "*  MKeys/sec/proc" 
	   << setw(10) << right << ": " << mkeys_per_sec_proc << endl;
      cout << endl;
    }

    if (rank == 0) {
      ofstream results;
      results.open ("./results", ios::app);

      results << mysort.description() << ", " 
	      << mysplit.description() << ", " 
	      << mymerge.description() << ", " 
	      << nproc << ", " 
	      << n_sort << ", " 
	      << rtime[1] << ", " 
	      << rtime[2] << ", " 
	      << rtime[3] << ", " 
	      << rtime[4] << ", " 
	      << total_time << ", " 
	      << mkeys_per_sec << ", " 
	      << mkeys_per_sec_proc 
	      << endl;

      results.close ();
    }
    
  }


  template <typename _SeqSortType, typename _Distance>
    void print_perf_data_samplesort (_Distance *dist, 
				     SeqSort<_SeqSortType> &mysort,
				     MPI_Comm comm) {
    
    int rank, nproc;
    MPI_Comm_size (comm, &nproc);
    MPI_Comm_rank (comm, &rank);

    char **stage = new char*[6];
    stage[1] = "sample splitters";
    stage[2] = "partition";
    stage[3] = "alltoall";
    stage[4] = mysort.description();
    stage[5] = "adjust boundaries";

    if (rank == 0) cout << endl;
    double rtime[6];
    for (int i=1; i<=5; ++i) {
      double time_i = psort_timing[i] - psort_timing[i-1];
      if (rank == 0) {
        cout << i << ". " 
	     << setw(30) << left << stage[i] 
	     << setw(10) << right << ": " 
	     << setprecision(6) << time_i << " sec" << endl;
        rtime[i] = time_i;
      }    
    }

    long n_sort = 0L; for (int i=0; i<nproc; ++i) n_sort += dist[i];
    double total_time = rtime[1] + rtime[2] + rtime[3] + rtime[4];
    double mkeys_per_sec;
    double mkeys_per_sec_proc;
    if (rank == 0) {
      mkeys_per_sec = ((n_sort * log2(n_sort)) / total_time) / 1e6;
      mkeys_per_sec_proc = (((n_sort * log2(n_sort)) / total_time) / nproc) / 1e6;

      cout << setprecision(6) << endl;
      cout << setw(33) << left << "*  MKeys/sec" 
	   << setw(10) << right << ": " << mkeys_per_sec << endl;
      cout << setw(33) << left << "*  MKeys/sec/proc" 
	   << setw(10) << right << ": " << mkeys_per_sec_proc << endl;
      cout << endl;
    }

    if (rank == 0) {
      ofstream results;
      results.open ("./results", ios::app);

      results << "sample sort" << ", "
	      << mysort.description() << ", " 
	      << nproc << ", " 
	      << n_sort << ", " 
	      << rtime[1] << ", " 
	      << rtime[2] << ", " 
	      << rtime[3] << ", " 
	      << rtime[4] << ", " 
	      << rtime[5] << ", "
	      << total_time << ", " 
	      << mkeys_per_sec << ", " 
	      << mkeys_per_sec_proc 
	      << endl;

      results.close ();
    }    
  }

}


#endif /* PSORT_UTIL_H */
