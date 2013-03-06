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

#ifndef PSORT_H
#define PSORT_H

#include "psort_util.h"

namespace vpsort {
  using namespace std;

  /*
   SeqSort can be STLSort, STLStableSort
   Split can be MedianSplit, SampleSplit
   Merge can be FlatMerge, TreeMerge, OOPTreeMerge, FunnelMerge2, FunnelMerge4
  */
  template<typename _RandomAccessIter, typename _Compare,
    typename _SeqSortType, typename _SplitType, typename _MergeType>
    void parallel_sort (_RandomAccessIter first, _RandomAccessIter last,
			_Compare comp, long *dist_in, 
			SeqSort<_SeqSortType> &mysort, 
			Split<_SplitType> &mysplit, 
			Merge<_MergeType> &mymerge,
			MPI_Comm comm) {
    
    typedef typename iterator_traits<_RandomAccessIter>::value_type _ValueType;
    typedef typename iterator_traits<_RandomAccessIter>::difference_type _Distance;

    int nproc, rank;
    MPI_Comm_size (comm, &nproc);
    MPI_Comm_rank (comm, &rank);

    MPI_Datatype MPI_valueType, MPI_distanceType;
    MPI_Type_contiguous (sizeof(_ValueType), MPI_CHAR, &MPI_valueType);
    MPI_Type_commit (&MPI_valueType);	// ABAB: Any type committed needs to be freed to claim storage
    MPI_Type_contiguous (sizeof(_Distance), MPI_CHAR, &MPI_distanceType);
    MPI_Type_commit (&MPI_distanceType);
    
    _Distance *dist = new _Distance[nproc];
    for (int i=0; i<nproc; ++i) dist[i] = (_Distance) dist_in[i]; 

    // Sort the data locally 
    // ABAB: Progress calls use MPI::COMM_WORLD, instead of the passed communicator 
    // Since they are just debug prints, avoid them
    progress (rank, 0, mysort.description(), comm);
    mysort.seqsort (first, last, comp);
  
    if (nproc == 1)
    {
    	MPI_Type_free(&MPI_valueType);
    	MPI_Type_free(&MPI_distanceType);
	return;
    }

    // Find splitters
    progress (rank, 1, mysplit.description(), comm);
    // explicit vector ( size_type n, const T& value= T(), const Allocator& = Allocator() );
    // repetitive sequence constructor: Initializes the vector with its content set to a repetition, n times, of copies of value.
    vector< vector<_Distance> > right_ends(nproc + 1, vector<_Distance>(nproc, 0));
    // ABAB: The following hangs if some dist entries are zeros, i.e. first = last for some processors
    mysplit.split (first, last, dist, comp, right_ends, 
		   MPI_valueType, MPI_distanceType, comm);
    
    // Communicate to destination
    progress (rank, 2, const_cast<char *>(string("alltoall").c_str()), comm);
    _Distance n_loc = last - first;
    _ValueType *trans_data = new _ValueType[n_loc];
    _Distance *boundaries = new _Distance[nproc+1];
    alltoall (right_ends, first, last,
	      trans_data, boundaries, 
	      MPI_valueType, MPI_distanceType, comm);
    
    // Merge streams from all processors
    // progress (rank, 3, mymerge.description());
    mymerge.merge (trans_data, first, boundaries, nproc, comp);

	delete [] boundaries;
    delete [] dist;
    delete [] trans_data;
    MPI_Type_free (&MPI_valueType);
    MPI_Type_free (&MPI_distanceType);

    // Finish    
    progress (rank, 4, const_cast<char *>(string("finish").c_str()), comm);

    return;
  }

  template<typename _RandomAccessIter, typename _Compare>
    void parallel_sort (_RandomAccessIter first, 
			_RandomAccessIter last,
			_Compare comp, long *dist, MPI_Comm comm) {
    
    STLSort mysort;    
    MedianSplit mysplit;
    OOPTreeMerge mymerge;
    
    parallel_sort (first, last, comp, dist, mysort, mysplit, mymerge, comm);
  }

  template<typename _RandomAccessIter>
    void parallel_sort (_RandomAccessIter first, 
			_RandomAccessIter last,
			long *dist, MPI_Comm comm) {
    
    typedef typename iterator_traits<_RandomAccessIter>::value_type _ValueType;
    
    STLSort mysort;    
    MedianSplit mysplit;
    OOPTreeMerge mymerge;

    parallel_sort (first, last, less<_ValueType>(), 
		   dist, mysort, mysplit, mymerge, comm);
  }

}


#endif /* PSORT_H */
