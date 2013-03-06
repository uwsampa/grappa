/****************************************************************/
/* Parallel Combinatorial BLAS Library (for Graph Computations) */
/* version 1.2 -------------------------------------------------*/
/* date: 10/06/2011 --------------------------------------------*/
/* authors: Aydin Buluc (abuluc@lbl.gov), Adam Lugowski --------*/
/****************************************************************/
/*
Copyright (c) 2011, Aydin Buluc

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

#ifndef _SP_HELPER_H_
#define _SP_HELPER_H_

#include <vector>
#include <limits>
#include "SpDefs.h"
#include "StackEntry.h"
#include "promote.h"
#include "Isect.h"
#include "HeapEntry.h"
#include "SpImpl.h"

using namespace std;

template <class IT, class NT>
class Dcsc;

class SpHelper
{
public:
	template <typename T>
	static const T * p2a (const std::vector<T> & v)   // pointer to array
	{
    		if(v.empty()) return NULL;
    		else return (&v[0]);
	}

	template <typename T>
	static T * p2a (std::vector<T> & v)   // pointer to array
	{
    		if(v.empty()) return NULL;
    		else return (&v[0]);
	}


	template<typename _ForwardIterator>
	static bool is_sorted(_ForwardIterator __first, _ForwardIterator __last)
	{
      		if (__first == __last)
        		return true;

      		_ForwardIterator __next = __first;
      		for (++__next; __next != __last; __first = __next, ++__next)
        		if (*__next < *__first)
          			return false;
      		return true;
    	}
  	template<typename _ForwardIterator, typename _StrictWeakOrdering>
    	static bool is_sorted(_ForwardIterator __first, _ForwardIterator __last,  _StrictWeakOrdering __comp)
    	{
      		if (__first == __last)
        		return true;

		_ForwardIterator __next = __first;
		for (++__next; __next != __last; __first = __next, ++__next)
			if (__comp(*__next, *__first))
          			return false;
      		return true;
	}
	template<typename _ForwardIter, typename T>
	static void iota(_ForwardIter __first, _ForwardIter __last, T __val)
	{
		while (__first != __last)
	     		*__first++ = __val++;
	}
	template<typename In, typename Out, typename UnPred>
	static Out copyIf(In first, In last, Out result, UnPred pred) 
	{
   		for ( ;first != last; ++first)
      			if (pred(*first))
         			*result++ = *first;
   		return(result);
	}
	
	template<typename T, typename I1, typename I2>
	static T ** allocate2D(I1 m, I2 n)
	{
		T ** array = new T*[m];
		for(I1 i = 0; i<m; ++i) 
			array[i] = new T[n];
		return array;
	}
	template<typename T, typename I>
	static void deallocate2D(T ** array, I m)
	{
		for(I i = 0; i<m; ++i) 
			delete [] array[i];
		delete [] array;
	}

	
	template <typename SR, typename NT1, typename NT2, typename IT, typename OVT>
	static IT Popping(NT1 * numA, NT2 * numB, StackEntry< OVT, pair<IT,IT> > * multstack,
		 	IT & cnz, KNHeap< pair<IT,IT> , IT > & sHeap, Isect<IT> * isect1, Isect<IT> * isect2);

	template <typename IT, typename NT1, typename NT2>
	static void SpIntersect(const Dcsc<IT,NT1> & Adcsc, const Dcsc<IT,NT2> & Bdcsc, Isect<IT>* & cols, Isect<IT>* & rows, 
			Isect<IT>* & isect1, Isect<IT>* & isect2, Isect<IT>* & itr1, Isect<IT>* & itr2);

	template <typename SR, typename IT, typename NT1, typename NT2, typename OVT>
	static IT SpCartesian(const Dcsc<IT,NT1> & Adcsc, const Dcsc<IT,NT2> & Bdcsc, IT kisect, Isect<IT> * isect1, 
			Isect<IT> * isect2, StackEntry< OVT, pair<IT,IT> > * & multstack);

	template <typename SR, typename IT, typename NT1, typename NT2, typename OVT>
	static IT SpColByCol(const Dcsc<IT,NT1> & Adcsc, const Dcsc<IT,NT2> & Bdcsc, IT nA,	 
			StackEntry< OVT, pair<IT,IT> > * & multstack);

	template <typename NT, typename IT>
	static void ShrinkArray(NT * & array, IT newsize)
	{
		NT * narray = new NT[newsize];
		memcpy(narray, array, newsize*sizeof(NT));	// copy only a portion of the old elements

		delete [] array;
		array = narray;		
	}

	template <typename NT, typename IT>
	static void DoubleStack(StackEntry<NT, pair<IT,IT> > * & multstack, IT & cnzmax, IT add)
	{
		StackEntry<NT, pair<IT,IT> > * tmpstack = multstack; 		
		multstack = new StackEntry<NT, pair<IT,IT> >[2* cnzmax + add];
		memcpy(multstack, tmpstack, sizeof(StackEntry<NT, pair<IT,IT> >) * cnzmax);
		
		cnzmax = 2*cnzmax + add;
		delete [] tmpstack;
	}

	template <typename IT>
	static bool first_compare(pair<IT, IT> pair1, pair<IT, IT> pair2) 
	{ return pair1.first < pair2.first; }

};



/**
 * Pop an element, do the numerical semiring multiplication & insert the result into multstack
 */
template <typename SR, typename NT1, typename NT2, typename IT, typename OVT>
IT SpHelper::Popping(NT1 * numA, NT2 * numB, StackEntry< OVT, pair<IT,IT> > * multstack, 
			IT & cnz, KNHeap< pair<IT,IT>,IT > & sHeap, Isect<IT> * isect1, Isect<IT> * isect2)
{
	pair<IT,IT> key;	
	IT inc;
	sHeap.deleteMin(&key, &inc);

	OVT value = SR::multiply(numA[isect1[inc].current], numB[isect2[inc].current]);
	if (!SR::returnedSAID())
	{
		if(cnz != 0)
		{
			if(multstack[cnz-1].key == key)	// already exists
			{
				multstack[cnz-1].value = SR::add(multstack[cnz-1].value, value);
			}
			else
			{
				multstack[cnz].value = value;
				multstack[cnz].key   = key;
				++cnz;
			}
		}
		else
		{
			multstack[cnz].value = value;
			multstack[cnz].key   = key;
			++cnz;
		}
	}
	return inc;
}

/**
  * Finds the intersecting row indices of Adcsc and col indices of Bdcsc  
  * @param[IT] Bdcsc {the transpose of the dcsc structure of matrix B}
  * @param[IT] Adcsc {the dcsc structure of matrix A}
  **/
template <typename IT, typename NT1, typename NT2>
void SpHelper::SpIntersect(const Dcsc<IT,NT1> & Adcsc, const Dcsc<IT,NT2> & Bdcsc, Isect<IT>* & cols, Isect<IT>* & rows, 
				Isect<IT>* & isect1, Isect<IT>* & isect2, Isect<IT>* & itr1, Isect<IT>* & itr2)
{
	cols = new Isect<IT>[Adcsc.nzc];
	rows = new Isect<IT>[Bdcsc.nzc];
	
	for(IT i=0; i < Adcsc.nzc; ++i)			
	{
		cols[i].index	= Adcsc.jc[i];		// column index
		cols[i].size	= Adcsc.cp[i+1] - Adcsc.cp[i];
		cols[i].start	= Adcsc.cp[i];		// pointer to row indices
		cols[i].current = Adcsc.cp[i];		// pointer to row indices
	}
	for(IT i=0; i < Bdcsc.nzc; ++i)			
	{
		rows[i].index	= Bdcsc.jc[i];		// column index
		rows[i].size	= Bdcsc.cp[i+1] - Bdcsc.cp[i];
		rows[i].start	= Bdcsc.cp[i];		// pointer to row indices
		rows[i].current = Bdcsc.cp[i];		// pointer to row indices
	}

	/* A single set_intersection would only return the elements of one sequence 
	 * But we also want random access to the other array's elements 
	 * Thus we do the intersection twice
	 */
	IT mink = min(Adcsc.nzc, Bdcsc.nzc);
	isect1 = new Isect<IT>[mink];	// at most
	isect2 = new Isect<IT>[mink];	// at most
	itr1 = set_intersection(cols, cols + Adcsc.nzc, rows, rows + Bdcsc.nzc, isect1);	
	itr2 = set_intersection(rows, rows + Bdcsc.nzc, cols, cols + Adcsc.nzc, isect2);	
	// itr1 & itr2 are now pointing to one past the end of output sequences
}

/**
 * Performs cartesian product on the dcsc structures. 
 * Indices to perform the product are given by isect1 and isect2 arrays
 * Returns the "actual" number of elements in the merged stack
 * Bdcsc is "already transposed" (i.e. Bdcsc->ir gives column indices, and Bdcsc->jc gives row indices)
 **/
template <typename SR, typename IT, typename NT1, typename NT2, typename OVT>
IT SpHelper::SpCartesian(const Dcsc<IT,NT1> & Adcsc, const Dcsc<IT,NT2> & Bdcsc, IT kisect, Isect<IT> * isect1, 
		Isect<IT> * isect2, StackEntry< OVT, pair<IT,IT> > * & multstack)
{	
	pair<IT,IT> supremum(numeric_limits<IT>::max(), numeric_limits<IT>::max());
	pair<IT,IT> infimum (numeric_limits<IT>::min(), numeric_limits<IT>::min());
 
	KNHeap< pair<IT,IT> , IT > sHeapDcsc(supremum, infimum);	

	// Create a sequence heap that will eventually construct DCSC of C
	// The key to sort is pair<col_ind, row_ind> so that output is in column-major order
	for(IT i=0; i< kisect; ++i)
	{
		pair<IT,IT> key(Bdcsc.ir[isect2[i].current], Adcsc.ir[isect1[i].current]);
		sHeapDcsc.insert(key, i);
	}

	IT cnz = 0;						
	IT cnzmax = Adcsc.nz + Bdcsc.nz;	// estimate on the size of resulting matrix C
	multstack = new StackEntry< OVT, pair<IT,IT> > [cnzmax];	

	bool finished = false;
	while(!finished)		// multiplication loop  (complexity O(flops * log (kisect))
	{
		finished = true;
		if (cnz + kisect > cnzmax)		// double the size of multstack
		{
			DoubleStack(multstack, cnzmax, kisect);
		} 

		// inc: the list to increment its pointer in the k-list merging
		IT inc = Popping< SR >(Adcsc.numx, Bdcsc.numx, multstack, cnz, sHeapDcsc, isect1, isect2);
		isect1[inc].current++;	
		
		if(isect1[inc].current < isect1[inc].size + isect1[inc].start)
		{
			pair<IT,IT> key(Bdcsc.ir[isect2[inc].current], Adcsc.ir[isect1[inc].current]);
			sHeapDcsc.insert(key, inc);	// push the same element with a different key [increasekey]
			finished = false;
		}
		// No room to go in isect1[], but there is still room to go in isect2[i]
		else if(isect2[inc].current + 1 < isect2[inc].size + isect2[inc].start)
		{
			isect1[inc].current = isect1[inc].start;	// wrap-around
			isect2[inc].current++;

			pair<IT,IT> key(Bdcsc.ir[isect2[inc].current], Adcsc.ir[isect1[inc].current]);
			sHeapDcsc.insert(key, inc);	// push the same element with a different key [increasekey]
			finished = false;
		}
		else // don't push, one of the lists has been deplated
		{
			kisect--;
			if(kisect != 0)
			{
				finished = false;
			}
		}
	}
	return cnz;
}


template <typename SR, typename IT, typename NT1, typename NT2, typename OVT>
IT SpHelper::SpColByCol(const Dcsc<IT,NT1> & Adcsc, const Dcsc<IT,NT2> & Bdcsc, IT nA, 
			StackEntry< OVT, pair<IT,IT> > * & multstack)
{
	IT cnz = 0;
	IT cnzmax = Adcsc.nz + Bdcsc.nz;	// estimate on the size of resulting matrix C
	multstack = new StackEntry<OVT, pair<IT,IT> >[cnzmax];	 

	float cf  = static_cast<float>(nA+1) / static_cast<float>(Adcsc.nzc);
	IT csize = static_cast<IT>(ceil(cf));   // chunk size
	IT * aux;
	//IT auxsize = Adcsc.ConstructAux(nA, aux);
	Adcsc.ConstructAux(nA, aux);

	for(IT i=0; i< Bdcsc.nzc; ++i)		// for all the columns of B
	{
		IT prevcnz = cnz;
		IT nnzcol = Bdcsc.cp[i+1] - Bdcsc.cp[i];
		HeapEntry<IT, NT1> * wset = new HeapEntry<IT, NT1>[nnzcol]; 
		// heap keys are just row indices (IT) 
		// heap values are <numvalue, runrank>  
		// heap size is nnz(B(:,i)

		// colnums vector keeps column numbers requested from A
		vector<IT> colnums(nnzcol);

		// colinds.first vector keeps indices to A.cp, i.e. it dereferences "colnums" vector (above),
		// colinds.second vector keeps the end indices (i.e. it gives the index to the last valid element of A.cpnack)
		vector< pair<IT,IT> > colinds(nnzcol);		
		copy(Bdcsc.ir + Bdcsc.cp[i], Bdcsc.ir + Bdcsc.cp[i+1], colnums.begin());
		
		Adcsc.FillColInds(&colnums[0], colnums.size(), colinds, aux, csize);
		IT maxnnz = 0;	// max number of nonzeros in C(:,i)	
		IT hsize = 0;
		
		for(IT j = 0; (unsigned)j < colnums.size(); ++j)		// create the initial heap 
		{
			if(colinds[j].first != colinds[j].second)	// current != end
			{
				wset[hsize++] = HeapEntry< IT,NT1 > (Adcsc.ir[colinds[j].first], j, Adcsc.numx[colinds[j].first]);
				maxnnz += colinds[j].second - colinds[j].first;
			} 
		}	
		make_heap(wset, wset+hsize);

		if (cnz + maxnnz > cnzmax)		// double the size of multstack
		{
			SpHelper::DoubleStack(multstack, cnzmax, maxnnz);
		} 

		// No need to keep redefining key and hentry with each iteration of the loop
		while(hsize > 0)
		{
			pop_heap(wset, wset + hsize);         // result is stored in wset[hsize-1]
			IT locb = wset[hsize-1].runr;	// relative location of the nonzero in B's current column 

			// type promotion done here: 
			// static T_promote multiply(const T1 & arg1, const T2 & arg2)
			//	return (static_cast<T_promote>(arg1) * static_cast<T_promote>(arg2) );
			OVT mrhs = SR::multiply(wset[hsize-1].num, Bdcsc.numx[Bdcsc.cp[i]+locb]);
			if (!SR::returnedSAID())
			{
				if(cnz != prevcnz && multstack[cnz-1].key.second == wset[hsize-1].key)	// if (cnz == prevcnz) => first nonzero for this column
				{
					multstack[cnz-1].value = SR::add(multstack[cnz-1].value, mrhs);
				}
				else
				{
					multstack[cnz].value = mrhs;
					multstack[cnz++].key = make_pair(Bdcsc.jc[i], wset[hsize-1].key);	
					// first entry is the column index, as it is in column-major order
				}
			}
			
			if( (++(colinds[locb].first)) != colinds[locb].second)	// current != end
			{
				// runr stays the same !
				wset[hsize-1].key = Adcsc.ir[colinds[locb].first];
				wset[hsize-1].num = Adcsc.numx[colinds[locb].first];  
				push_heap(wset, wset+hsize);
			}
			else
			{
				--hsize;
			}
		}
		delete [] wset;
	}
	delete [] aux;
	return cnz;
}


#endif
