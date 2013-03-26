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

#ifndef PSORT_ALLTOALL_H
#define PSORT_ALLTOALL_H

#include "psort_util.h"

namespace vpsort {  
  using namespace std;

  template <typename _RandomAccessIter, typename _ValueType, typename _Distance>
    static void alltoall (vector< vector<_Distance> > &right_ends, 
			  _RandomAccessIter first,
			  _RandomAccessIter last,
			  _ValueType *trans_data,
			  _Distance *boundaries,
			  MPI_Datatype &MPI_valueType, 
			  MPI_Datatype &MPI_distanceType,
			  MPI_Comm comm) {

    int nproc, rank;
    MPI_Comm_size (comm, &nproc);
    MPI_Comm_rank (comm, &rank);
    
    // Should be _Distance, but MPI wants ints
    char errMsg[] = "32-bit limit for MPI has overflowed";
    _Distance n_loc_ = last - first; 
    if (n_loc_ > INT_MAX) throw std::overflow_error(errMsg);
    int n_loc = static_cast<int> (n_loc_);

    // Calculate the counts for redistributing data 
    int *send_counts = new int[nproc];
	int *send_disps = new int[nproc];
    for (int i = 0; i < nproc; ++i) {
      _Distance scount = right_ends[i + 1][rank] - right_ends[i][rank];
      if (scount > INT_MAX) throw std::overflow_error(errMsg);
      send_counts[i] = static_cast<int> (scount);
    }
    send_disps[0] = 0;
    partial_sum (send_counts, send_counts + nproc - 1, send_disps + 1);
    
    int *recv_counts = new int[nproc];
	int *recv_disps = new int[nproc];
    for (int i = 0; i < nproc; ++i) {
      _Distance rcount = right_ends[rank + 1][i] - right_ends[rank][i];
      if (rcount > INT_MAX) throw std::overflow_error(errMsg);
      recv_counts[i] = static_cast<int> (rcount);
    }
    
    recv_disps[0] = 0;
    partial_sum (recv_counts, recv_counts + nproc - 1, recv_disps + 1);
    
    assert (accumulate (recv_counts, recv_counts + nproc, 0) == n_loc);

    // Do the transpose
    MPI_Alltoallv (first, send_counts, send_disps, MPI_valueType,
                   trans_data, recv_counts, recv_disps, MPI_valueType, comm);

    for (int i=0; i<nproc; ++i) boundaries[i] = (_Distance) recv_disps[i];
    boundaries[nproc] = (_Distance) n_loc;  // for the merging  

	delete [] recv_counts;
	delete [] recv_disps;
 
	delete [] send_counts;
	delete [] send_disps;
    return;
  }

}


#endif /* PSORT_ALLTOALL_H */
