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

#ifndef PSORT_SPLITTERS_H
#define PSORT_SPLITTERS_H

#include "psort_util.h"


namespace vpsort {
  using namespace std;

  template<typename SplitType>
    class Split {
    public:
    template<typename _RandomAccessIter, typename _Compare, typename _Distance>
      void split (_RandomAccessIter first, 
		  _RandomAccessIter last,
		  _Distance *dist,
		  _Compare comp,
		  vector< vector<_Distance> > &right_ends,
		  MPI_Datatype &MPI_valueType, MPI_Datatype &MPI_distanceType,
		  MPI_Comm comm) {
      
      SplitType *s = static_cast<SplitType *>(this);
      s->real_split(first, last, dist, comp, right_ends, 
		    MPI_valueType, MPI_distanceType, comm);
    }

    char *description () {
      SplitType *s = static_cast<SplitType *>(this);
      return s->real_description ();
    }
  };

  class MedianSplit : public Split<MedianSplit> {
  public:
    char *real_description () {
      string s ("Median splitter");
	return const_cast<char*>(s.c_str());
    }

    template<typename _RandomAccessIter, typename _Compare, typename _Distance>
      void real_split (_RandomAccessIter first, 
		       _RandomAccessIter last,
		       _Distance *dist,
		       _Compare comp,
		       vector< vector<_Distance> > &right_ends,
		       MPI_Datatype &MPI_valueType, 
		       MPI_Datatype &MPI_distanceType,
		       MPI_Comm comm) {

      typedef typename iterator_traits<_RandomAccessIter>::value_type _ValueType;

      int nproc, rank;
      MPI_Comm_size (comm, &nproc);
      MPI_Comm_rank (comm, &rank);

      int n_real = nproc;
      for (int i = 0; i < nproc; ++i)
	if (dist[i] == 0) { n_real = i; break; }

      copy (dist, dist + nproc, right_ends[nproc].begin());

      // union of [0, right_end[i+1]) on each processor produces dist[i] total values
	  // AL: conversion to remove dependence on C99 feature.
	  // original: _Distance targets[nproc-1];
	  vector<_Distance> targets_v(nproc-1);
      _Distance *targets = &targets_v.at(0);
      partial_sum (dist, dist + (nproc - 1), targets);

      // keep a list of ranges, trying to "activate" them at each branch
      vector< pair<_RandomAccessIter, _RandomAccessIter> > d_ranges(nproc - 1);
      vector< pair<_Distance *, _Distance *> > t_ranges(nproc - 1);
      d_ranges[0] = pair<_RandomAccessIter, _RandomAccessIter>(first, last);
      t_ranges[0] = pair<_Distance *, _Distance *>(targets, targets + (nproc - 1));
  
      // invariant: subdist[i][rank] == d_ranges[i].second - d_ranges[i].first
      // amount of data each proc still has in the search
      vector< vector<_Distance> > subdist(nproc - 1, vector<_Distance>(nproc));
      copy (dist, dist + nproc, subdist[0].begin());

      // for each processor, d_ranges - first
      vector< vector<_Distance> > outleft(nproc - 1, vector<_Distance>(nproc, 0));

#ifdef PSORTDEBUG
      double t_begin=0, t_query=0, t_bsearch=0, t_gather=0, t_finish=0;
      double t_allquery=0, t_allbsearch=0, t_allgather=0, t_allfinish=0;
#endif
      for (int n_act = 1; n_act > 0; ) {

	for (int k=0; k<n_act; ++k) {
	  assert(subdist[k][rank] == d_ranges[k].second - d_ranges[k].first);
        }

#ifdef PSORTDEBUG
	MPI_Barrier (MPI_COMM_WORLD);
        t_begin = MPI_Wtime();
#endif

	//------- generate n_act guesses
	// for the allgather, make a flat array of nproc chunks, each with n_act elts
	_ValueType * mymedians = new _ValueType[n_act];
	_ValueType * medians = new _ValueType[nproc*n_act];
	for (int k = 0; k < n_act; ++k) {
	  _ValueType *ptr = d_ranges[k].first;
	  _Distance index = subdist[k][rank] / 2;
	  mymedians[k] = ptr[index];
	}
	MPI_Allgather (mymedians, n_act, MPI_valueType, medians, n_act, MPI_valueType, comm);
	delete [] mymedians;

	// compute the weighted median of medians
	vector<_ValueType> queries(n_act);

	for (int k = 0; k < n_act; ++k) 
	{
	  // AL: conversion to remove dependence on C99 feature.
	  // original: _Distance ms_perm[n_real];
	  vector<_Distance> ms_perm_v(n_real);
	  _Distance *ms_perm = &ms_perm_v.at(0);
	  for (int i = 0; i < n_real; ++i) ms_perm[i] = i * n_act + k;
	  sort (ms_perm, ms_perm + n_real, 
		PermCompare< _ValueType, _Compare> (medians, comp));

	  _Distance mid = accumulate( subdist[k].begin(), 
				      subdist[k].end(), 
				      0) / 2;
	  _Distance query_ind = -1;
	  for (int i = 0; i < n_real; ++i) 
	  {
	    if (subdist[k][ms_perm[i] / n_act] == 0) continue;

	    mid -= subdist[k][ms_perm[i] / n_act];
	    if (mid <= 0) 
	    {
	      query_ind = ms_perm[i];
	      break;
	    }
	  }

	  assert(query_ind >= 0);
	  queries[k] = medians[query_ind];
	}
	delete [] medians;

#ifdef PSORTDEBUG
	MPI_Barrier (MPI_COMM_WORLD);
	t_query = MPI_Wtime() - t_begin;
#endif

	//------- find min and max ranks of the guesses
	// AL: conversion to remove dependence on C99 feature.
	// original: _Distance ind_local[2 * n_act];
	vector<_Distance> ind_local_v(2 * n_act);
	_Distance *ind_local = &ind_local_v.at(0);
	for (int k = 0; k < n_act; ++k) {
	  pair<_RandomAccessIter, _RandomAccessIter> 
	    ind_local_p = equal_range (d_ranges[k].first, 
				       d_ranges[k].second, 
				       queries[k], comp);

	  ind_local[2 * k] = ind_local_p.first - first;
	  ind_local[2 * k + 1] = ind_local_p.second - first;
	}

#ifdef PSORTDEBUG
	MPI_Barrier (MPI_COMM_WORLD);
	t_bsearch = MPI_Wtime() - t_begin - t_query;
#endif

	// AL: conversion to remove dependence on C99 feature.
	// original: _Distance ind_all[2 * n_act * nproc];
	vector<_Distance> ind_all_v(2 * n_act * nproc);
	_Distance *ind_all = &ind_all_v.at(0);
	MPI_Allgather (ind_local, 2 * n_act, MPI_distanceType,
		       ind_all, 2 * n_act, MPI_distanceType, comm);
	// sum to get the global range of indices
	vector<pair<_Distance, _Distance> > ind_global(n_act);
	for (int k = 0; k < n_act; ++k) {
	  ind_global[k] = make_pair(0, 0);
	  for (int i = 0; i < nproc; ++i) {
	    ind_global[k].first += ind_all[2 * (i * n_act + k)];
	    ind_global[k].second += ind_all[2 * (i * n_act + k) + 1];
	  }
	}

#ifdef PSORTDEBUG
	MPI_Barrier (MPI_COMM_WORLD);
	t_gather = MPI_Wtime() - t_begin - t_query - t_bsearch;
#endif

	// state to pass on to next iteration
	vector< pair<_RandomAccessIter, _RandomAccessIter> > d_ranges_x(nproc - 1);
	vector< pair<_Distance *, _Distance *> > t_ranges_x(nproc - 1);
	vector< vector<_Distance> > subdist_x(nproc - 1, vector<_Distance>(nproc));
	vector< vector<_Distance> > outleft_x(nproc - 1, vector<_Distance>(nproc, 0));
	int n_act_x = 0;

	for (int k = 0; k < n_act; ++k) {
	  _Distance *split_low = lower_bound (t_ranges[k].first, 
					      t_ranges[k].second,
					      ind_global[k].first);
	  _Distance *split_high = upper_bound (t_ranges[k].first, 
					       t_ranges[k].second,
					       ind_global[k].second);
      
	  // iterate over targets we hit
	  for (_Distance *s = split_low; s != split_high; ++s) {
	    assert (*s > 0);
	    // a bit sloppy: if more than one target in range, excess won't zero out
	    _Distance excess = *s - ind_global[k].first;
	    // low procs to high take excess for stability
	    for (int i = 0; i < nproc; ++i) {
	      _Distance amount = min (ind_all[2 * (i * n_act + k)] + excess, 
				      ind_all[2 * (i * n_act + k) + 1]);
	      right_ends[(s - targets) + 1][i] = amount;
	      excess -= amount - ind_all[2 * (i * n_act + k)];
	    }
	  }

	  if ((split_low - t_ranges[k].first) > 0) {
	    t_ranges_x[n_act_x] = make_pair (t_ranges[k].first, split_low);
	    // lop off local_ind_low..end
	    d_ranges_x[n_act_x] = make_pair (d_ranges[k].first, 
					     first + ind_local[2 * k]);
	    for (int i = 0; i < nproc; ++i) {
	      subdist_x[n_act_x][i] = ind_all[2 * (i * n_act + k)] - outleft[k][i];
	      outleft_x[n_act_x][i] = outleft[k][i];
	    }
	    ++n_act_x;
	  }

	  if ((t_ranges[k].second - split_high) > 0) {
	    t_ranges_x[n_act_x] = make_pair (split_high, t_ranges[k].second);
	    // lop off begin..local_ind_high
	    d_ranges_x[n_act_x] = make_pair (first + ind_local[2 * k + 1],
					     d_ranges[k].second); 
	    for (int i = 0; i < nproc; ++i) {
	      subdist_x[n_act_x][i] = outleft[k][i] 
		+ subdist[k][i] - ind_all[2 * (i * n_act + k) + 1];
	      outleft_x[n_act_x][i] = ind_all[2 * (i * n_act + k) + 1];
	    }
	    ++n_act_x;
	  }
	}

	t_ranges = t_ranges_x;
	d_ranges = d_ranges_x;
	subdist = subdist_x;
	outleft = outleft_x;
	n_act = n_act_x;
	
#ifdef PSORTDEBUG
	MPI_Barrier (MPI_COMM_WORLD);
        t_finish = MPI_Wtime() - t_begin - t_query - t_bsearch - t_gather;
        t_allquery += t_query;
        t_allbsearch += t_bsearch;
        t_allgather += t_gather;	
        t_allfinish += t_finish;
#endif
      }

#ifdef PSORTDEBUG
      if (rank == 0)
            std::cout << "t_query = " << t_allquery 	
                      << ", t_bsearch = " << t_allbsearch
                      << ", t_gather = " << t_allgather 
                      << ", t_finish = " << t_allfinish << std::endl;
#endif
    }

  private:
    template <typename T, typename _Compare>
      class PermCompare {
      private:
      T *weights;
      _Compare comp;
      public:
      PermCompare(T *w, _Compare c) : weights(w), comp(c) {}
      bool operator()(int a, int b) {
	return comp(weights[a], weights[b]);
      }
    };

  };


  class SampleSplit : public Split<SampleSplit> {
  public:
    char *real_description () {
      string s ("Sample splitter");
	return const_cast<char*>(s.c_str());
    }
    
    template<typename _RandomAccessIter, typename _Compare, typename _Distance>
      void real_split (_RandomAccessIter first, 
		       _RandomAccessIter last,
		       _Distance *dist,
		       _Compare comp,
		       vector< vector<_Distance> > &right_ends,
		       MPI_Datatype &MPI_valueType, 
		       MPI_Datatype &MPI_distanceType,
		       MPI_Comm comm) {
      
      int nproc, rank;
      MPI_Comm_size (comm, &nproc);
      MPI_Comm_rank (comm, &rank);
      
      // union of [0, right_end) on each processor produces split_ind total values
      _Distance targets[nproc-1];
      partial_sum (dist, dist + (nproc - 1), targets);
      
      fill (right_ends[0].begin(), right_ends[0].end(), 0);
      for (int i = 0; i < nproc-1; ++i) {
	sample_split_iter (first, last, dist, targets[i], comp,
			   right_ends[i + 1], 
			   MPI_valueType, MPI_distanceType, comm);
      }
      copy (dist, dist + nproc, right_ends[nproc].begin());      
    }
    
    
  private:
	// return an integer uniformly distributed from [0, max) 
	inline static int random_number(int max) {
#ifdef _MSC_VER
		return (int) (max * (double(rand()) / RAND_MAX));
#else
		return (int) (max * drand48());
#endif
	}
    
    template<typename _RandomAccessIter, typename _Compare, typename _Distance>
      static void sample_split_iter (_RandomAccessIter first, 
				     _RandomAccessIter last,
				     _Distance *dist,
				     _Distance target, 
				     _Compare comp,
				     vector<_Distance> &split_inds,
				     MPI_Datatype &MPI_valueType, 
				     MPI_Datatype &MPI_distanceType,
				     MPI_Comm comm) {

      typedef typename iterator_traits<_RandomAccessIter>::value_type _ValueType;

      int nproc, rank;
      MPI_Comm_size (comm, &nproc);
      MPI_Comm_rank (comm, &rank);

      // invariant: subdist[rank] == last - start
      _Distance subdist[nproc];  // amount of data each proc still has in the search
      _Distance outleft[nproc];  // keep track of start - first
      copy (dist, dist + nproc, subdist);
      fill (outleft, outleft + nproc, 0);

      _RandomAccessIter start = first;
      _ValueType query;
      _Distance sampler;

      while (true) {
	assert(subdist[rank] == last - start);

	// generate a guess
	// root decides who gets to guess
	if (!rank) {
	  _Distance distcum[nproc];
	  partial_sum (subdist, subdist + nproc, distcum);
	  // ensure sampler is a rank with data; lower is that first rank
	  _Distance lower = lower_bound(distcum, distcum + nproc, 1) - distcum;
	  sampler = lower_bound (distcum + lower, distcum + nproc,
				 random_number (distcum[nproc - 1]))
	    - distcum;
	}
	MPI_Bcast (&sampler, 1, MPI_distanceType, 0, comm);

	// take the guess
	if (rank == sampler) {
	  query = *(start + random_number (subdist[rank]));
	}
	MPI_Bcast (&query, 1, MPI_valueType, sampler, comm);

	// find min and max ranks of the guess
	pair<_RandomAccessIter, _RandomAccessIter> ind_range_p =
	  equal_range (start, last, query, comp);

	// get the absolute local index of the query
	_Distance ind_range[2] = {
	  ind_range_p.first - first, ind_range_p.second - first 
	};
      
	_Distance ind_all[2 * nproc];
	MPI_Allgather (ind_range, 2, MPI_distanceType,
		       ind_all, 2 , MPI_distanceType, comm);

	// scan to get the global range of indices of the query
	_Distance g_ind_min = 0, g_ind_max = 0;
	for (int i = 0; i < nproc; ++i) {
	  g_ind_min += ind_all[2 * i];
	  g_ind_max += ind_all[2 * i + 1];
	}

	// if target in range, low procs to high take excess for stability
	if (g_ind_min <= target && target <= g_ind_max) {
	  _Distance excess = target - g_ind_min;
	  for (int i = 0; i < nproc; ++i) {
	    split_inds[i] = min (ind_all[2 * i] + excess, ind_all[2 * i + 1]);
	    excess -= split_inds[i] - ind_all[2 * i];
	  }
	  return;
	}

	// set up for next iteration of binary search
	if (target < g_ind_min) {
	  last = first + ind_all[2 * rank];
	  assert(outleft[rank] == start - first);
	  for (int i = 0; i < nproc; ++i) {
	    subdist[i] = ind_all[2 * i] - outleft[i];
	  }
	} else {
	  start = first + ind_all[2 * rank + 1];
	  for (int i = 0; i < nproc; ++i) {
	    subdist[i] = outleft[i] + subdist[i] - ind_all[2 * i + 1];
	    outleft[i] = ind_all[2 * i + 1];
	  }
	}
      }
    }
  };

} /* namespace vpsort */

#endif /* PSORT_SPLITTERS_H */

