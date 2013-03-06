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

#ifndef PSORT_SAMPLE_H

#define PSORT_SAMPLE_H

#include "psort_util.h"

#define MPI_PSORT_TAG 31337

namespace vpsort {
  using namespace std;

  // sample n_out elements from array in _without_ replacement
  // done with two sweeps of size n_out
  // in is touched, but restored
  template<typename _RandomAccessIter, typename _ValueType, typename _Distance>
  static void sample_splitters(_RandomAccessIter in, _Distance n_in, 
			       _ValueType *out, long n_out) {
    if (n_out >= n_in) {
      // TODO
    } else {
      _Distance *picks = new _Distance[n_out];

      for (int i=0; i<n_out; ++i) {
	picks[i] = static_cast<_Distance> ((n_in - i) * drand48());
	out[i] = *(in + picks[i]);
	*(in + picks[i]) = *(in + (n_out - i - 1));
      }

      for (int i=n_out-1; i>=0; --i) {
	*(in + (n_out - i - 1)) = *(in + picks[i]);
	*(in + picks[i]) = out[i];
      }

      delete [] picks;
    }
  }

  static void set_splitters(long *part_gsize, long n_parts, int nproc, long *dist, long *out) {
    // impl 1: get all but last about correct size; last might pick up slack
    long ind = 0, cum = 0;
    for (long i=0; i<n_parts; ++i) {
      cum += part_gsize[i];
      if (cum > dist[ind]) {
	out[ind] = i;
	if (cum - dist[ind] > dist[ind] - cum + part_gsize[i]) {
	  --out[ind];
	  cum = part_gsize[i];
	} else {
	  cum = 0;
	}
	if (++ind >= nproc - 1) break;
      }
    }
  }

  template<typename _ValueType>
  static void redist (int *from_dist, _ValueType *current,
		      int *to_dist, _ValueType *final,
		      int rank, int nproc, 
		      MPI_Datatype &MPI_valueType, MPI_Comm comm)
  { 
    int starch[nproc][nproc];
    
    for (int i = 0; i < nproc; ++i) { 
      for (int j = 0; j < nproc; ++j) { 
	starch[i][j] = 0; 
      } 
    } 
    
    int i = 0, j=0; 
    int from_delta = from_dist[0], to_delta = to_dist[0]; 
    
    while ((i < nproc) && (j < nproc)) { 
      
      int diff = from_delta - to_delta; 
      
      if (diff == 0) { 
	starch[i][j] = from_delta; 
	++i; 
	++j; 
	from_delta = from_dist[i]; 
	to_delta = to_dist[j]; 
      } else if (diff > 0) { 
	starch[i][j] = to_delta; 
	++j; 
	from_delta -= to_delta; 
	to_delta = to_dist[j]; 
      } else { 
	starch[i][j] = from_delta; 
	++i; 
	to_delta -= from_delta; 
	from_delta = from_dist[i]; 
      } 
    }     
    
    MPI_Request send_req[nproc], recv_req[nproc];
    MPI_Status status;
  
    int sendl = 0, recvl = 0;

    for (int p = 0; p <= rank; ++p) {

      int torecv = starch[p][rank];
      if (torecv) {
	MPI_Irecv (&final[recvl], torecv,
		   MPI_valueType, p, MPI_PSORT_TAG,
		   comm, &recv_req[p]);
	recvl += torecv;
      }

      int tosend = starch[rank][p];
      if (tosend) {
	MPI_Isend (&current[sendl], tosend,
		   MPI_valueType, p, MPI_PSORT_TAG,
		   comm, &send_req[p]);
	sendl += tosend;
      }
    
    }

    int sendr = 0, recvr = 0;

    for (int p = nproc-1; p > rank; --p) {

      int torecv = starch[p][rank];
      if (torecv) {
	recvr += torecv;
	MPI_Irecv (&final[to_dist[rank]-recvr], torecv,
		   MPI_valueType, p, MPI_PSORT_TAG,
		   comm, &recv_req[p]);
      }

      int tosend = starch[rank][p];
      if (tosend) {
	sendr += tosend;
	MPI_Isend (&current[from_dist[rank]-sendr], tosend,
		   MPI_valueType, p, MPI_PSORT_TAG,
		   comm, &send_req[p]);
      }
    }
    
    for (int p = 0; p < nproc; ++p) {
      if ( starch[rank][p] )
	MPI_Wait(&send_req[p], &status);
      if ( starch[p][rank] )
	MPI_Wait(&recv_req[p], &status);
    }      

  }


  template<typename _RandomAccessIter, typename _Compare,
	   typename _SeqSortType>
  void parallel_samplesort(_RandomAccessIter first, _RandomAccessIter last,
			    _Compare comp, long *dist, 
			    SeqSort<_SeqSortType> &mysort,
			    long s, long k, 
			    MPI_Comm comm) {
  
    typedef typename iterator_traits<_RandomAccessIter>::value_type _ValueType;
    typedef typename iterator_traits<_RandomAccessIter>::difference_type _Distance;

    int nproc, rank;
    MPI_Comm_size(comm, &nproc);
    MPI_Comm_rank(comm, &rank);

    MPI_Datatype MPI_valueType, MPI_distanceType;
    MPI_Type_contiguous (sizeof(_ValueType), MPI_CHAR, &MPI_valueType);
    MPI_Type_commit (&MPI_valueType);
    MPI_Type_contiguous (sizeof(_Distance), MPI_CHAR, &MPI_distanceType);
    MPI_Type_commit (&MPI_distanceType);

    _Distance n_loc = last - first;

    progress (rank, 0, "Sample splitters", comm);

    // randomly pick s*k sample splitters
    long n_samp = s*k;
    _ValueType *s_splitters = new _ValueType[n_samp];
    sample_splitters(first, n_loc, s_splitters, n_samp);

    // send the sample splitters to the root
    long n_parts = nproc * k - 1;
    _ValueType *p_splitters = new _ValueType[n_parts];
    if (rank != 0) {
      MPI_Gather(s_splitters, n_samp, MPI_valueType,
		 NULL, 0, MPI_valueType, 0, comm);
    } else {
      _ValueType *gath_splitters = new _ValueType[nproc * n_samp];
      MPI_Gather(s_splitters, n_samp, MPI_valueType,
		 gath_splitters, n_samp, MPI_valueType,
		 0, comm);

      // root sorts sample splitters serially
      sort(gath_splitters, gath_splitters + (nproc * n_samp), comp);

      // root picks pk-1 splitters, broadcasts
      double stride = (double) (nproc * n_samp) / (nproc * k);
      for (int i=0; i<n_parts; ++i) {
	p_splitters[i] = gath_splitters[(int) (stride * (double) (i + 1))];
      }
      delete [] gath_splitters;
    }

    MPI_Bcast(p_splitters, n_parts, MPI_valueType, 0, comm);

    delete [] s_splitters;

    progress (rank, 1, "Partition", comm);

    // apply the splitters
    //  impl1: binary search each element over the splitters
    //  TODO: impl2: recurse, partitioning dfs order over splitters
    _Distance *part_ind = new _Distance[n_loc];
    long *part_lsize = new long[n_parts + 1];
    for (long i=0; i<=n_parts; ++i) part_lsize[i] = 0;

    for (long i=0; i<n_loc; ++i) {
      // TODO: load balance when splitters have same value
      part_ind[i] = lower_bound(p_splitters, p_splitters + n_parts, *(first + i))
	- p_splitters;
      ++part_lsize[part_ind[i]];
    }

    delete [] p_splitters;

    long *boundaries = new long[nproc - 1];
    if (rank != 0) {
      MPI_Reduce(part_lsize, NULL, n_parts + 1, MPI_LONG, MPI_SUM, 0, comm);
    } else {
      // determine global partition sizes, starch-minimizing boundaries
      long *part_gsize = new long[n_parts + 1];
      MPI_Reduce(part_lsize, part_gsize, n_parts + 1, MPI_LONG, MPI_SUM, 0, comm);
      set_splitters (part_gsize, n_parts + 1, nproc, dist, boundaries);
      delete [] part_gsize;
    }
    MPI_Bcast(boundaries, nproc - 1, MPI_LONG, 0, comm);

    // send data according to boundaries
    // part_proc[i] says which processor partition i goes to
    long *part_proc = new long[n_parts + 1];
    int out_counts[nproc];
    for (int i=0; i<nproc; ++i) out_counts[i] = 0;
    long cur = 0;
    for (long i=0; i<=n_parts; ++i) {
      if (cur < nproc - 1 && i > boundaries[cur]) {
	++cur;
      }
      part_proc[i] = cur;
      out_counts[cur] += part_lsize[i];
    }

    delete [] boundaries;
    delete [] part_lsize;

    long curs[nproc];
    _ValueType **s_bufs = new _ValueType*[nproc];
    for (int i=0; i<nproc; ++i) {
      curs[i] = 0;
      s_bufs[i] = new _ValueType[out_counts[i]];
    }
    for (int i=0; i<n_loc; ++i) {
      int dest = part_proc[part_ind[i]];
      s_bufs[dest][curs[dest]++] = *(first + i);
    }

    delete [] part_ind;
    delete [] part_proc;

    progress (rank, 2, "Alltoall", comm);

    // tell each processor how many elements to expect from m
    int in_counts[nproc];
    MPI_Alltoall(out_counts, 1, MPI_INT, in_counts, 1, MPI_INT, comm);

    int offsets[nproc + 1];
    offsets[0] = 0;
    for (int i=1; i<=nproc; ++i) {
      offsets[i] = offsets[i-1] + in_counts[i-1];
    }
    int n_loct = offsets[nproc];

    MPI_Request send_reqs[nproc];
    MPI_Request recv_reqs[nproc];
    _ValueType *trans_data = new _ValueType[n_loct];

    for (int i=0; i<nproc; ++i) {
      MPI_Irecv(&trans_data[offsets[i]], in_counts[i],
		MPI_valueType, i, MPI_PSORT_TAG, comm, &recv_reqs[i]);
    }
    for (int i=1; i<=nproc; ++i) {
      int dest = (rank + i) % nproc;
      MPI_Isend(s_bufs[dest], out_counts[dest], MPI_valueType,
		dest, MPI_PSORT_TAG, comm, &send_reqs[dest]);
    }

    MPI_Waitall(nproc, send_reqs, MPI_STATUS_IGNORE);
    for (int i=0; i < nproc; ++i) {
      delete [] s_bufs[i];
    }
    delete [] s_bufs;

    MPI_Waitall (nproc, recv_reqs, MPI_STATUS_IGNORE);

    progress (rank, 3, "Sequential sort", comm);    

    mysort.seqsort (trans_data, trans_data + n_loct, comp);

    progress (rank, 4, "Adjust boundaries", comm);    

    // starch
    int trans_dist[nproc], dist_out[nproc];
    MPI_Allgather(&n_loct, 1, MPI_INT, trans_dist, 1, MPI_INT, comm);
    for (int i=0; i<nproc; ++i) dist_out[i] = dist[i];
    redist(trans_dist, trans_data, dist_out, first, rank, nproc, MPI_valueType, comm);
    delete [] trans_data;

    progress (rank, 5, "Finish", comm);    

    MPI_Type_free (&MPI_valueType);
  }

  template<typename _RandomAccessIter>
    void parallel_samplesort(_RandomAccessIter first, _RandomAccessIter last,
			     long *dist, MPI_Comm comm) {
    
    typedef typename iterator_traits<_RandomAccessIter>::value_type _ValueType;

    STLSort stl_sort;    
    
    int nproc;
    MPI_Comm_size (comm, &nproc);
    long s = nproc, k = nproc;
    parallel_samplesort (first, last, less<_ValueType>(), dist, stl_sort, s, k, comm);
    
  }

} /* namespace vpsort */

#endif /* PSORT_SAMPLE_H */
