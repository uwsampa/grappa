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

#ifndef PSORT_MERGE_H
#define PSORT_MERGE_H

#ifdef USE_FUNNEL
#include "funnel.h"
#endif

namespace vpsort {
  template<typename MergeType>
    class Merge {
    public:
    template<typename _ValueType, typename _Compare, typename _Distance>
      void merge (_ValueType *in, _ValueType *out,
		  _Distance *disps, int nproc, _Compare comp) {
      
      MergeType *m = static_cast<MergeType *>(this);
      m->real_merge (in, out, disps, nproc, comp);
    }
    
    char *description () {
      MergeType *m = static_cast<MergeType *>(this);
      return m->real_description ();
    }
  };
  
  // p-way flat merge
  class FlatMerge : public Merge<FlatMerge> {
  public:
    char *real_description () {
      string s("Flat merge");
      return const_cast<char*>(s.c_str());
    };

    template<typename _ValueType, typename _Compare, typename _Distance>
      void real_merge (_ValueType *in, _ValueType *out,
		       _Distance *disps, int nproc, _Compare comp) {
      
      _Distance heads[nproc];
      copy (disps, disps + nproc, heads);
      for (int i = 0; i < disps[nproc]; ++i) {
	int min_head = -1;
	for (int j = 0; j < nproc; ++j) {
	  if (heads[j] < disps[j+1]
	      && (min_head < 0
		  || comp (in[heads[j]], in[heads[min_head]]))) {
	    min_head = j;
	  }
	}
	out[i] = in[heads[min_head]++];
      }
    }    
  };


  // A tree merge
  class TreeMerge : public Merge<TreeMerge> {
  public:
    char *real_description () {
	string s("Tree merge");
      return const_cast<char*>(s.c_str());
    };

    template<typename _ValueType, typename _Compare, typename _Distance>
      void real_merge (_ValueType *in, _ValueType *out,
		       _Distance *disps, int nproc, _Compare comp) {
      
      // round nproc up to next power of two, pad disps
      int nproc_p;
      for (nproc_p = 1; nproc_p < nproc; nproc_p *= 2);
      _Distance disps_p[nproc_p + 1];
      copy (disps, disps + nproc + 1, disps_p);
      fill (disps_p + nproc + 1, disps_p + nproc_p + 1, disps[nproc]);
      
      int merged = 0;
      for (int i = 1; i * 2 < nproc_p + 1; i = i * 2) {
	for (int j = 0; j + 2*i < nproc_p + 1; j += 2*i) {
	  inplace_merge (in + disps_p[j], 
			 in + disps_p[j + i],
			 in + disps_p[j + 2*i], 
			 comp);
	}
	merged = 2*i;
      }
      std::merge (in, in + disps_p[merged],
		  in + disps_p[merged], in + disps_p[nproc_p],
		  out, comp);
    }
  };

  // An out of place tree merge
  class OOPTreeMerge : public Merge<OOPTreeMerge> {
  public:
    char *real_description () {
      string s ("Out-of-place tree merge");
	return const_cast<char*>(s.c_str());
    };


    template<typename _RandomAccessIter, typename _Compare, typename _Distance>
      void real_merge (_RandomAccessIter in, _RandomAccessIter out,
			      _Distance *disps, int nproc, _Compare comp) {

      if (nproc == 1) {
	copy (in, in + disps[nproc], out);
	return;
      }
    
      _RandomAccessIter bufs[2] = { in, out };
      _Distance *locs = new _Distance[nproc];
      for (int i = 0; i < nproc; ++i) {
	locs[i] = 0;
      }
    
      _Distance next = 1;
      while (true) {
	_Distance stride = next * 2;
	if (stride >= nproc)
	  break;
      
	for (_Distance i = 0; i + next < nproc; i += stride) {
	  _Distance end_ind = min (i + stride, (_Distance) nproc);
        
	  std::merge (bufs[locs[i]] + disps[i], 
		      bufs[locs[i]] + disps[i + next],
		      bufs[locs[i + next]] + disps[i + next], 
		      bufs[locs[i + next]] + disps[end_ind],
		      bufs[1 - locs[i]] + disps[i],
		      comp);
	  locs[i] = 1 - locs[i];
	}
      
	next = stride;
      }
    
      // now have 4 cases for final merge
      if (locs[0] == 0) {
	// 00, 01 => out of place
	std::merge (in, in + disps[next],
		    bufs[locs[next]] + disps[next],
		    bufs[locs[next]] + disps[nproc],
		    out, comp);
      } else if (locs[next] == 0) {
	// 10 => backwards out of place
	std::merge (reverse_iterator<_RandomAccessIter> (in + disps[nproc]),
		    reverse_iterator<_RandomAccessIter> (in + disps[next]),
		    reverse_iterator<_RandomAccessIter> (out + disps[next]),
		    reverse_iterator<_RandomAccessIter> (out),
		    reverse_iterator<_RandomAccessIter> (out + disps[nproc]),
		    not2 (comp));
      } else {
	// 11 => in-place
	std::inplace_merge (out, out + disps[next], out + disps[nproc], comp);
      }
	  delete [] locs;
    }
  };  


#ifdef USE_FUNNEL

  class FunnelMerge2 : public Merge<FunnelMerge2> {
  public:
    char *real_description () {
      return ("Funnel(2) merge");
    };

    template<typename _RandomAccessIter, typename _Compare, typename _Distance>
      void real_merge (_RandomAccessIter in, _RandomAccessIter out,
			      _Distance *disps, int nproc, _Compare comp) {

      if (nproc == 1) {
	copy (in, in + disps[nproc], out);
	return;
      }

      iosort::merge_tree<_RandomAccessIter, 2> merger (nproc);

      for (int i=0; i<nproc; ++i) {
	merger.add_stream (in + disps[i], in + disps[i+1]);
      }

      merger (out);
    }
  };

  class FunnelMerge4 : public Merge<FunnelMerge4> {
  public:
    char *real_description () {
      std::string s ("Funnel(4) merge");
	return const_cast<char*>(s.c_str());
    };

    template<typename _RandomAccessIter, typename _Compare, typename _Distance>
      void real_merge (_RandomAccessIter in, _RandomAccessIter out,
			      _Distance *disps, int nproc, _Compare comp) {

      if (nproc == 1) {
	copy (in, in + disps[nproc], out);
	return;
      }

      iosort::merge_tree<_RandomAccessIter, 4> merger (nproc);

      for (int i=0; i<nproc; ++i) {
	merger.add_stream (in + disps[i], in + disps[i+1]);
      }

      merger (out);
    }
  };
#endif

}


#endif /*PSORT_MERGE_H */
