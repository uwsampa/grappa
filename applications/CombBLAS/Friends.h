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

#ifndef _FRIENDS_H_
#define _FRIENDS_H_

#include <iostream>
#include "SpMat.h"	// Best to include the base class first
#include "SpHelper.h"
#include "StackEntry.h"
#include "Isect.h"
#include "Deleter.h"
#include "SpImpl.h"
#include "SpImplNoSR.h"
#include "SpParHelper.h"
#include "Compare.h"
#include "CombBLAS.h"
using namespace std;

template <class IU, class NU>	
class SpTuples;

template <class IU, class NU>	
class SpDCCols;

template <class IU, class NU>	
class Dcsc;

/*************************************************************************************************/
/**************************** SHARED ADDRESS SPACE FRIEND FUNCTIONS ******************************/
/****************************** MULTITHREADED LOGIC ALSO GOES HERE *******************************/
/*************************************************************************************************/

//! SpMV with dense vector
template <typename SR, typename IU, typename NU, typename RHS, typename LHS>
void dcsc_gespmv (const SpDCCols<IU, NU> & A, const RHS * x, LHS * y)
{
	if(A.nnz > 0)
	{	
		for(IU j =0; j<A.dcsc->nzc; ++j)	// for all nonzero columns
		{
			IU colid = A.dcsc->jc[j];
			for(IU i = A.dcsc->cp[j]; i< A.dcsc->cp[j+1]; ++i)
			{
				IU rowid = A.dcsc->ir[i];
				SR::axpy(A.dcsc->numx[i], x[colid], y[rowid]);
				if (SR::returnedSAID())
				{
					cout << "the semiring returned SAID but that is not implemented. results will be incorrect." << endl;
					throw string("the semiring returned SAID but that is not implemented. results will be incorrect.");
				}
			}
		}
	}
}

/** 
  * Multithreaded SpMV with sparse vector
  * the assembly of outgoing buffers sendindbuf/sendnumbuf are done here
  */
template <typename SR, typename IU, typename NUM, typename IVT, typename OVT>
int dcsc_gespmv_threaded (const SpDCCols<IU, NUM> & A, const int32_t * indx, const IVT * numx, int32_t nnzx, 
		int32_t * & sendindbuf, OVT * & sendnumbuf, int * & sdispls, int p_c)
{
	// FACTS: Split boundaries (for multithreaded execution) are independent of recipient boundaries
	// Two splits might create output to the same recipient (needs to be merged)
	// However, each split's output is distinct (no duplicate elimination is needed after merge) 

	sdispls = new int[p_c]();	// initialize to zero (as all indy might be empty)
	if(A.getnnz() > 0 && nnzx > 0)
	{
		int splits = A.getnsplit();
		if(splits > 0)
		{
			int32_t nlocrows = static_cast<int32_t>(A.getnrow());
			int32_t perpiece = nlocrows / splits;
			vector< vector< int32_t > > indy(splits);
			vector< vector< OVT > > numy(splits);

			// Parallelize with OpenMP
			#ifdef _OPENMP
			#pragma omp parallel for // num_threads(6)
			#endif
			for(int i=0; i<splits; ++i)
			{
				if(i != splits-1)
					SpMXSpV_ForThreading<SR>(*(A.GetDCSC(i)), perpiece, indx, numx, nnzx, indy[i], numy[i], i*perpiece);
				else
					SpMXSpV_ForThreading<SR>(*(A.GetDCSC(i)), nlocrows - perpiece*i, indx, numx, nnzx, indy[i], numy[i], i*perpiece);
			}

			vector<int> accum(splits+1, 0);
			for(int i=0; i<splits; ++i)
				accum[i+1] = accum[i] + indy[i].size();

			sendindbuf = new int32_t[accum[splits]];
			sendnumbuf = new OVT[accum[splits]];
			int32_t perproc = nlocrows / p_c;	
			int32_t last_rec = p_c-1;
			
			// keep recipients of last entries in each split (-1 for an empty split)
			// so that we can delete indy[] and numy[] contents as soon as they are processed		
			vector<int32_t> end_recs(splits);
			for(int i=0; i<splits; ++i)
			{
				if(indy[i].empty())
					end_recs[i] = -1;
				else
					end_recs[i] = min(indy[i].back() / perproc, last_rec);
			}
			#ifdef _OPENMP
			#pragma omp parallel for // num_threads(6)
			#endif	
			for(int i=0; i<splits; ++i)
			{
				if(!indy[i].empty())	// guarantee that .begin() and .end() are not null
				{
					// FACT: Data is sorted, so if the recipient of begin is the same as the owner of end, 
					// then the whole data is sent to the same processor
					int32_t beg_rec = min( indy[i].front() / perproc, last_rec); 			

					// We have to test the previous "split", to see if we are marking a "recipient head" 
					// set displacement markers for the completed (previous) buffers only
					if(i != 0)
					{
						int k = i-1;
						while (k >= 0 && end_recs[k] == -1) k--;	// loop backwards until seeing an non-empty split
						if(k >= 0)	// we found a non-empty split
						{
							fill(sdispls+end_recs[k]+1, sdispls+beg_rec+1, accum[i]);	// last entry to be set is sdispls[beg_rec]
						}
						// else fill sdispls[1...beg_rec] with zero (already done)
					}
					// else set sdispls[0] to zero (already done)
					if(beg_rec == end_recs[i])	// fast case
					{
						transform(indy[i].begin(), indy[i].end(), indy[i].begin(), bind2nd(minus<int32_t>(), perproc*beg_rec));
						copy(indy[i].begin(), indy[i].end(), sendindbuf+accum[i]);
						copy(numy[i].begin(), numy[i].end(), sendnumbuf+accum[i]);
					}
					else	// slow case
					{
						// FACT: No matter how many splits or threads, there will be only one "recipient head"
						// Therefore there are no race conditions for marking send displacements (sdispls)
						int end = indy[i].size();
						for(int cur=0; cur< end; ++cur)	
						{
							int32_t cur_rec = min( indy[i][cur] / perproc, last_rec); 			
							while(beg_rec != cur_rec)	
							{
								sdispls[++beg_rec] = accum[i] + cur;	// first entry to be set is sdispls[beg_rec+1]
							}
							sendindbuf[ accum[i] + cur ] = indy[i][cur] - perproc*beg_rec;	// convert to receiver's local index
							sendnumbuf[ accum[i] + cur ] = numy[i][cur];
						}
					}
					vector<int32_t>().swap(indy[i]);
					vector<OVT>().swap(numy[i]);
					bool lastnonzero = true;	// am I the last nonzero split?
					for(int k=i+1; k < splits; ++k)
					{
						if(end_recs[k] != -1)
							lastnonzero = false;
					} 
					if(lastnonzero)
						fill(sdispls+end_recs[i]+1, sdispls+p_c, accum[i+1]);
				}	// end_if(!indy[i].empty)
			}	// end parallel for	
			return accum[splits];
		}
		else
		{
			cout << "Something is wrong, splits should be nonzero for multithreaded execution" << endl;
			return 0;
		}
	}
	else
	{
		sendindbuf = NULL;
		sendnumbuf = NULL;
		return 0;
	}
}



/** 
 * Multithreaded SpMV with sparse vector and preset buffers
 * the assembly of outgoing buffers sendindbuf/sendnumbuf are done here
 * IVT: input vector numerical type
 * OVT: output vector numerical type
 */
template <typename SR, typename IU, typename NUM, typename IVT, typename OVT>
void dcsc_gespmv_threaded_setbuffers (const SpDCCols<IU, NUM> & A, const int32_t * indx, const IVT * numx, int32_t nnzx, 
				 int32_t * sendindbuf, OVT * sendnumbuf, int * cnts, int * dspls, int p_c)
{
	if(A.getnnz() > 0 && nnzx > 0)
	{
		int splits = A.getnsplit();
		if(splits > 0)
		{
			vector< vector<int32_t> > indy(splits);
			vector< vector< OVT > > numy(splits);
			int32_t nlocrows = static_cast<int32_t>(A.getnrow());
			int32_t perpiece = nlocrows / splits;
			
			#ifdef _OPENMP
			#pragma omp parallel for 
			#endif
			for(int i=0; i<splits; ++i)
			{
				if(i != splits-1)
					SpMXSpV_ForThreading<SR>(*(A.GetDCSC(i)), perpiece, indx, numx, nnzx, indy[i], numy[i], i*perpiece);
				else
					SpMXSpV_ForThreading<SR>(*(A.GetDCSC(i)), nlocrows - perpiece*i, indx, numx, nnzx, indy[i], numy[i], i*perpiece);
			}
			
			int32_t perproc = nlocrows / p_c;	
			int32_t last_rec = p_c-1;
			
			// keep recipients of last entries in each split (-1 for an empty split)
			// so that we can delete indy[] and numy[] contents as soon as they are processed		
			vector<int32_t> end_recs(splits);
			for(int i=0; i<splits; ++i)
			{
				if(indy[i].empty())
					end_recs[i] = -1;
				else
					end_recs[i] = min(indy[i].back() / perproc, last_rec);
			}
			
			int ** loc_rec_cnts = new int *[splits];	
			#ifdef _OPENMP	
			#pragma omp parallel for
			#endif	
			for(int i=0; i<splits; ++i)
			{
				loc_rec_cnts[i]  = new int[p_c](); // thread-local recipient data
				if(!indy[i].empty())	// guarantee that .begin() and .end() are not null
				{
					int32_t cur_rec = min( indy[i].front() / perproc, last_rec);
					int32_t lastdata = (cur_rec+1) * perproc;  // one past last entry that goes to this current recipient
					for(typename vector<int32_t>::iterator it = indy[i].begin(); it != indy[i].end(); ++it)
					{

						if( ( (*it) >= lastdata ) && cur_rec != last_rec )
						{
							cur_rec = min( (*it) / perproc, last_rec);
							lastdata = (cur_rec+1) * perproc;
						}
						++loc_rec_cnts[i][cur_rec];
					}
				}
			}
			#ifdef _OPENMP	
			#pragma omp parallel for 
			#endif
			for(int i=0; i<splits; ++i)
			{
				if(!indy[i].empty())	// guarantee that .begin() and .end() are not null
				{
					// FACT: Data is sorted, so if the recipient of begin is the same as the owner of end, 
					// then the whole data is sent to the same processor
					int32_t beg_rec = min( indy[i].front() / perproc, last_rec); 
					int32_t alreadysent = 0;	// already sent per recipient 
					for(int before = i-1; before >= 0; before--)
						 alreadysent += loc_rec_cnts[before][beg_rec];
						
					if(beg_rec == end_recs[i])	// fast case
					{
						transform(indy[i].begin(), indy[i].end(), indy[i].begin(), bind2nd(minus<int32_t>(), perproc*beg_rec));
						copy(indy[i].begin(), indy[i].end(), sendindbuf + dspls[beg_rec] + alreadysent);
						copy(numy[i].begin(), numy[i].end(), sendnumbuf + dspls[beg_rec] + alreadysent);
					}
					else	// slow case
					{
						int32_t cur_rec = beg_rec;
						int32_t lastdata = (cur_rec+1) * perproc;  // one past last entry that goes to this current recipient
						for(typename vector<int32_t>::iterator it = indy[i].begin(); it != indy[i].end(); ++it)
						{
							if( ( (*it) >= lastdata ) && cur_rec != last_rec )
							{
								cur_rec = min( (*it) / perproc, last_rec);
								lastdata = (cur_rec+1) * perproc;

								// if this split switches to a new recipient after sending some data
								// then it's sure that no data has been sent to that recipient yet
						 		alreadysent = 0;
							}
							sendindbuf[ dspls[cur_rec] + alreadysent ] = (*it) - perproc*cur_rec;	// convert to receiver's local index
							sendnumbuf[ dspls[cur_rec] + (alreadysent++) ] = *(numy[i].begin() + (it-indy[i].begin()));
						}
					}
				}
			}
			// Deallocated rec counts serially once all threads complete
			for(int i=0; i< splits; ++i)	
			{
				for(int j=0; j< p_c; ++j)
					cnts[j] += loc_rec_cnts[i][j];
				delete [] loc_rec_cnts[i];
			}
			delete [] loc_rec_cnts;
		}
		else
		{
			cout << "Something is wrong, splits should be nonzero for multithreaded execution" << endl;
		}
	}
}

//! SpMV with sparse vector
//! MIND: Matrix index type
//! VIND: Vector index type (optimized: int32_t, general: int64_t)
template <typename SR, typename MIND, typename VIND, typename NUM, typename IVT, typename OVT>
void dcsc_gespmv (const SpDCCols<MIND, NUM> & A, const VIND * indx, const IVT * numx, VIND nnzx, vector<VIND> & indy, vector<OVT>  & numy)
{
	if(A.getnnz() > 0 && nnzx > 0)
	{
		if(A.getnsplit() > 0)
		{
			cout << "Call dcsc_gespmv_threaded instead" << endl;
		}
		else
		{
			SpMXSpV<SR>(*(A.GetDCSC()), (VIND) A.getnrow(), indx, numx, nnzx, indy, numy);
		}
	}
}

/** SpMV with sparse vector
  * @param[in] indexisvalue is only used for BFS-like computations, if true then we can call the optimized version that skips SPA
  */
template <typename SR, typename IU, typename NUM, typename IVT, typename OVT>
void dcsc_gespmv (const SpDCCols<IU, NUM> & A, const int32_t * indx, const IVT * numx, int32_t nnzx, 
		int32_t * indy, OVT * numy, int * cnts, int * dspls, int p_c, bool indexisvalue)
{
	if(A.getnnz() > 0 && nnzx > 0)
	{
		if(A.getnsplit() > 0)
		{
			SpParHelper::Print("Call dcsc_gespmv_threaded instead\n");
		}
		else
		{
			if(indexisvalue)
			{
				int32_t localm = (int32_t) A.getnrow();
				BitMap * isthere = new BitMap(localm);
				SpMXSpV(*(A.GetDCSC()), localm, indx, numx, nnzx, indy, numy, cnts, dspls, p_c, isthere);
				delete isthere;
			}
			else
				SpMXSpV<SR>(*(A.GetDCSC()), (int32_t) A.getnrow(), indx, numx, nnzx, indy, numy, cnts, dspls, p_c);
		}
	}
}


template<typename IU>
void BooleanRowSplit(SpDCCols<IU, bool> & A, int numsplits)
{
	A.splits = numsplits;
	IU perpiece = A.m / A.splits;
	vector<IU> prevcolids(A.splits, -1);	// previous column id's are set to -1
	vector<IU> nzcs(A.splits, 0);
	vector<IU> nnzs(A.splits, 0);
	vector < vector < pair<IU,IU> > > colrowpairs(A.splits);
	if(A.nnz > 0 && A.dcsc != NULL)
	{
		for(IU i=0; i< A.dcsc->nzc; ++i)
		{
			for(IU j = A.dcsc->cp[i]; j< A.dcsc->cp[i+1]; ++j)
			{
				IU colid = A.dcsc->jc[i];
				IU rowid = A.dcsc->ir[j];
				IU owner = min(rowid / perpiece, static_cast<IU>(A.splits-1));
				colrowpairs[owner].push_back(make_pair(colid, rowid - owner*perpiece));

				if(prevcolids[owner] != colid)
				{
					prevcolids[owner] = colid;
					++nzcs[owner];
				}
				++nnzs[owner];
			}
		}
	}
	delete A.dcsc;	// claim memory
	//copy(nzcs.begin(), nzcs.end(), ostream_iterator<IU>(cout," " )); cout << endl;
	//copy(nnzs.begin(), nnzs.end(), ostream_iterator<IU>(cout," " )); cout << endl;	
	A.dcscarr = new Dcsc<IU,bool>*[A.splits];	
	
	// To be parallelized with OpenMP
	for(int i=0; i< A.splits; ++i)
	{
		sort(colrowpairs[i].begin(), colrowpairs[i].end());	// sort w.r.t. columns
		A.dcscarr[i] = new Dcsc<IU,bool>(nnzs[i],nzcs[i]);	
		fill(A.dcscarr[i]->numx, A.dcscarr[i]->numx+nnzs[i], static_cast<bool>(1));
		IU curnzc = 0;				// number of nonzero columns constructed so far
		IU cindex = colrowpairs[i][0].first;
		IU rindex = colrowpairs[i][0].second;

		A.dcscarr[i]->ir[0] = rindex;
		A.dcscarr[i]->jc[curnzc] = cindex;
		A.dcscarr[i]->cp[curnzc++] = 0; 

		for(IU j=1; j<nnzs[i]; ++j)
		{
			cindex = colrowpairs[i][j].first;
			rindex = colrowpairs[i][j].second;

			A.dcscarr[i]->ir[j] = rindex;
			if(cindex != A.dcscarr[i]->jc[curnzc-1])
			{
				A.dcscarr[i]->jc[curnzc] = cindex;
				A.dcscarr[i]->cp[curnzc++] = j;
			}
		}
		A.dcscarr[i]->cp[curnzc] = nnzs[i];
	}
}


/**
 * SpTuples(A*B') (Using OuterProduct Algorithm)
 * Returns the tuples for efficient merging later
 * Support mixed precision multiplication
 * The multiplication is on the specified semiring (passed as parameter)
 */
template<class SR, class NUO, class IU, class NU1, class NU2>
SpTuples<IU, NUO> * Tuples_AnXBt 
					(const SpDCCols<IU, NU1> & A, 
					 const SpDCCols<IU, NU2> & B,
					bool clearA = false, bool clearB = false)
{
	IU mdim = A.m;	
	IU ndim = B.m;	// B is already transposed

	if(A.isZero() || B.isZero())
	{
		if(clearA)	delete const_cast<SpDCCols<IU, NU1> *>(&A);
		if(clearB)	delete const_cast<SpDCCols<IU, NU2> *>(&B);
		return new SpTuples< IU, NUO >(0, mdim, ndim);	// just return an empty matrix
	}
	Isect<IU> *isect1, *isect2, *itr1, *itr2, *cols, *rows;
	SpHelper::SpIntersect(*(A.dcsc), *(B.dcsc), cols, rows, isect1, isect2, itr1, itr2);
	
	IU kisect = static_cast<IU>(itr1-isect1);		// size of the intersection ((itr1-isect1) == (itr2-isect2))
	if(kisect == 0)
	{
		if(clearA)	delete const_cast<SpDCCols<IU, NU1> *>(&A);
		if(clearB)	delete const_cast<SpDCCols<IU, NU2> *>(&B);
		DeleteAll(isect1, isect2, cols, rows);
		return new SpTuples< IU, NUO >(0, mdim, ndim);
	}
	
	StackEntry< NUO, pair<IU,IU> > * multstack;

	IU cnz = SpHelper::SpCartesian< SR > (*(A.dcsc), *(B.dcsc), kisect, isect1, isect2, multstack);  
	DeleteAll(isect1, isect2, cols, rows);

	if(clearA)	delete const_cast<SpDCCols<IU, NU1> *>(&A);
	if(clearB)	delete const_cast<SpDCCols<IU, NU2> *>(&B);
	return new SpTuples<IU, NUO> (cnz, mdim, ndim, multstack);
}

/**
 * SpTuples(A*B) (Using ColByCol Algorithm)
 * Returns the tuples for efficient merging later
 * Support mixed precision multiplication
 * The multiplication is on the specified semiring (passed as parameter)
 */
template<class SR, class NUO, class IU, class NU1, class NU2>
SpTuples<IU, NUO> * Tuples_AnXBn 
					(const SpDCCols<IU, NU1> & A, 
					 const SpDCCols<IU, NU2> & B,
					bool clearA = false, bool clearB = false)
{
	IU mdim = A.m;	
	IU ndim = B.n;	
	if(A.isZero() || B.isZero())
	{
		return new SpTuples<IU, NUO>(0, mdim, ndim);
	}
	StackEntry< NUO, pair<IU,IU> > * multstack;
	IU cnz = SpHelper::SpColByCol< SR > (*(A.dcsc), *(B.dcsc), A.n,  multstack);  
	
	if(clearA)	
		delete const_cast<SpDCCols<IU, NU1> *>(&A);
	if(clearB)
		delete const_cast<SpDCCols<IU, NU2> *>(&B);

	return new SpTuples<IU, NUO> (cnz, mdim, ndim, multstack);
}


template<class SR, class NUO, class IU, class NU1, class NU2>
SpTuples<IU, NUO> * Tuples_AtXBt 
					(const SpDCCols<IU, NU1> & A, 
					 const SpDCCols<IU, NU2> & B, 
					bool clearA = false, bool clearB = false)
{
	IU mdim = A.n;	
	IU ndim = B.m;	
	cout << "Tuples_AtXBt function has not been implemented yet !" << endl;
		
	return new SpTuples<IU, NUO> (0, mdim, ndim);
}

template<class SR, class NUO, class IU, class NU1, class NU2>
SpTuples<IU, NUO> * Tuples_AtXBn 
					(const SpDCCols<IU, NU1> & A, 
					 const SpDCCols<IU, NU2> & B,
					bool clearA = false, bool clearB = false)
{
	IU mdim = A.n;	
	IU ndim = B.n;	
	cout << "Tuples_AtXBn function has not been implemented yet !" << endl;
		
	return new SpTuples<IU, NUO> (0, mdim, ndim);
}

// Performs a balanced merge of the array of SpTuples
// Assumes the input parameters are already column sorted
template<class SR, class IU, class NU>
SpTuples<IU,NU> MergeAll( const vector<SpTuples<IU,NU> *> & ArrSpTups, IU mstar = 0, IU nstar = 0, bool delarrs = false )
{
	int hsize =  ArrSpTups.size();		
	if(hsize == 0)
	{
		return SpTuples<IU,NU>(0, mstar,nstar);
	}
	else
	{
		mstar = ArrSpTups[0]->m;
		nstar = ArrSpTups[0]->n;
	}
	for(int i=1; i< hsize; ++i)
	{
		if((mstar != ArrSpTups[i]->m) || nstar != ArrSpTups[i]->n)
		{
			cerr << "Dimensions do not match on MergeAll()" << endl;
			return SpTuples<IU,NU>(0,0,0);
		}
	}
	if(hsize > 1)
	{
		ColLexiCompare<IU,int> heapcomp;
		tuple<IU, IU, int> * heap = new tuple<IU, IU, int> [hsize];	// (rowindex, colindex, source-id)
		IU * curptr = new IU[hsize];
		fill_n(curptr, hsize, static_cast<IU>(0)); 
		IU estnnz = 0;

		for(int i=0; i< hsize; ++i)
		{
			estnnz += ArrSpTups[i]->getnnz();
			heap[i] = make_tuple(get<0>(ArrSpTups[i]->tuples[0]), get<1>(ArrSpTups[i]->tuples[0]), i);
		}	
		make_heap(heap, heap+hsize, not2(heapcomp));

		tuple<IU, IU, NU> * ntuples = new tuple<IU,IU,NU>[estnnz]; 
		IU cnz = 0;

		while(hsize > 0)
		{
			pop_heap(heap, heap + hsize, not2(heapcomp));         // result is stored in heap[hsize-1]
			int source = get<2>(heap[hsize-1]);

			if( (cnz != 0) && 
				((get<0>(ntuples[cnz-1]) == get<0>(heap[hsize-1])) && (get<1>(ntuples[cnz-1]) == get<1>(heap[hsize-1]))) )
			{
				get<2>(ntuples[cnz-1])  = SR::add(get<2>(ntuples[cnz-1]), ArrSpTups[source]->numvalue(curptr[source]++)); 
			}
			else
			{
				ntuples[cnz++] = ArrSpTups[source]->tuples[curptr[source]++];
			}
			
			if(curptr[source] != ArrSpTups[source]->getnnz())	// That array has not been depleted
			{
				heap[hsize-1] = make_tuple(get<0>(ArrSpTups[source]->tuples[curptr[source]]), 
								get<1>(ArrSpTups[source]->tuples[curptr[source]]), source);
				push_heap(heap, heap+hsize, not2(heapcomp));
			}
			else
			{
				--hsize;
			}
		}
		SpHelper::ShrinkArray(ntuples, cnz);
		DeleteAll(heap, curptr);
	
		if(delarrs)
		{	
			for(size_t i=0; i<ArrSpTups.size(); ++i)
				delete ArrSpTups[i];
		}
		return SpTuples<IU,NU> (cnz, mstar, nstar, ntuples);
	}
	else
	{
		SpTuples<IU,NU> ret = *ArrSpTups[0];
		if(delarrs)
			delete ArrSpTups[0];
		return ret;
	}
}

/**
 * @param[in]   exclude if false,
 *      \n              then operation is A = A .* B
 *      \n              else operation is A = A .* not(B) 
 **/
template <typename IU, typename NU1, typename NU2>
Dcsc<IU, typename promote_trait<NU1,NU2>::T_promote> EWiseMult(const Dcsc<IU,NU1> & A, const Dcsc<IU,NU2> * B, bool exclude)
{
	typedef typename promote_trait<NU1,NU2>::T_promote N_promote;
	IU estnzc, estnz;
	if(exclude)
	{	
		estnzc = A.nzc;
		estnz = A.nz; 
	} 
	else
	{
		estnzc = std::min(A.nzc, B->nzc);
		estnz  = std::min(A.nz, B->nz);
	}

	Dcsc<IU,N_promote> temp(estnz, estnzc);

	IU curnzc = 0;
	IU curnz = 0;
	IU i = 0;
	IU j = 0;
	temp.cp[0] = 0;
	
	if(!exclude)	// A = A .* B
	{
		while(i< A.nzc && B != NULL && j<B->nzc)
		{
			if(A.jc[i] > B->jc[j]) 		++j;
			else if(A.jc[i] < B->jc[j]) 	++i;
			else
			{
				IU ii = A.cp[i];
				IU jj = B->cp[j];
				IU prevnz = curnz;		
				while (ii < A.cp[i+1] && jj < B->cp[j+1])
				{
					if (A.ir[ii] < B->ir[jj])	++ii;
					else if (A.ir[ii] > B->ir[jj])	++jj;
					else
					{
						temp.ir[curnz] = A.ir[ii];
						temp.numx[curnz++] = A.numx[ii++] * B->numx[jj++];	
					}
				}
				if(prevnz < curnz)	// at least one nonzero exists in this column
				{
					temp.jc[curnzc++] = A.jc[i];	
					temp.cp[curnzc] = temp.cp[curnzc-1] + curnz-prevnz;
				}
				++i;
				++j;
			}
		}
	}
	else	// A = A .* not(B)
	{
		while(i< A.nzc && B != NULL && j< B->nzc)
		{
			if(A.jc[i] > B->jc[j])		++j;
			else if(A.jc[i] < B->jc[j])
			{
				temp.jc[curnzc++] = A.jc[i++];
				for(IU k = A.cp[i-1]; k< A.cp[i]; k++)	
				{
					temp.ir[curnz] 		= A.ir[k];
					temp.numx[curnz++] 	= A.numx[k];
				}
				temp.cp[curnzc] = temp.cp[curnzc-1] + (A.cp[i] - A.cp[i-1]);
			}
			else
			{
				IU ii = A.cp[i];
				IU jj = B->cp[j];
				IU prevnz = curnz;		
				while (ii < A.cp[i+1] && jj < B->cp[j+1])
				{
					if (A.ir[ii] > B->ir[jj])	++jj;
					else if (A.ir[ii] < B->ir[jj])
					{
						temp.ir[curnz] = A.ir[ii];
						temp.numx[curnz++] = A.numx[ii++];
					}
					else	// eliminate those existing nonzeros
					{
						++ii;	
						++jj;	
					}
				}
				while (ii < A.cp[i+1])
				{
					temp.ir[curnz] = A.ir[ii];
					temp.numx[curnz++] = A.numx[ii++];
				}

				if(prevnz < curnz)	// at least one nonzero exists in this column
				{
					temp.jc[curnzc++] = A.jc[i];	
					temp.cp[curnzc] = temp.cp[curnzc-1] + curnz-prevnz;
				}
				++i;
				++j;
			}
		}
		while(i< A.nzc)
		{
			temp.jc[curnzc++] = A.jc[i++];
			for(IU k = A.cp[i-1]; k< A.cp[i]; ++k)
			{
				temp.ir[curnz] 	= A.ir[k];
				temp.numx[curnz++] = A.numx[k];
			}
			temp.cp[curnzc] = temp.cp[curnzc-1] + (A.cp[i] - A.cp[i-1]);
		}
	}

	temp.Resize(curnzc, curnz);
	return temp;
}	

template <typename N_promote, typename IU, typename NU1, typename NU2, typename _BinaryOperation>
Dcsc<IU, N_promote> EWiseApply(const Dcsc<IU,NU1> & A, const Dcsc<IU,NU2> * B, _BinaryOperation __binary_op, bool notB, const NU2& defaultBVal)
{
	//typedef typename promote_trait<NU1,NU2>::T_promote N_promote;
	IU estnzc, estnz;
	if(notB)
	{	
		estnzc = A.nzc;
		estnz = A.nz; 
	} 
	else
	{
		estnzc = std::min(A.nzc, B->nzc);
		estnz  = std::min(A.nz, B->nz);
	}

	Dcsc<IU,N_promote> temp(estnz, estnzc);

	IU curnzc = 0;
	IU curnz = 0;
	IU i = 0;
	IU j = 0;
	temp.cp[0] = 0;
	
	if(!notB)	// A = A .* B
	{
		while(i< A.nzc && B != NULL && j<B->nzc)
		{
			if(A.jc[i] > B->jc[j]) 		++j;
			else if(A.jc[i] < B->jc[j]) 	++i;
			else
			{
				IU ii = A.cp[i];
				IU jj = B->cp[j];
				IU prevnz = curnz;		
				while (ii < A.cp[i+1] && jj < B->cp[j+1])
				{
					if (A.ir[ii] < B->ir[jj])	++ii;
					else if (A.ir[ii] > B->ir[jj])	++jj;
					else
					{
						temp.ir[curnz] = A.ir[ii];
						temp.numx[curnz++] = __binary_op(A.numx[ii++], B->numx[jj++]);	
					}
				}
				if(prevnz < curnz)	// at least one nonzero exists in this column
				{
					temp.jc[curnzc++] = A.jc[i];	
					temp.cp[curnzc] = temp.cp[curnzc-1] + curnz-prevnz;
				}
				++i;
				++j;
			}
		}
	}
	else	// A = A .* not(B)
	{
		while(i< A.nzc && B != NULL && j< B->nzc)
		{
			if(A.jc[i] > B->jc[j])		++j;
			else if(A.jc[i] < B->jc[j])
			{
				temp.jc[curnzc++] = A.jc[i++];
				for(IU k = A.cp[i-1]; k< A.cp[i]; k++)	
				{
					temp.ir[curnz] 		= A.ir[k];
					temp.numx[curnz++] 	= __binary_op(A.numx[k], defaultBVal);
				}
				temp.cp[curnzc] = temp.cp[curnzc-1] + (A.cp[i] - A.cp[i-1]);
			}
			else
			{
				IU ii = A.cp[i];
				IU jj = B->cp[j];
				IU prevnz = curnz;		
				while (ii < A.cp[i+1] && jj < B->cp[j+1])
				{
					if (A.ir[ii] > B->ir[jj])	++jj;
					else if (A.ir[ii] < B->ir[jj])
					{
						temp.ir[curnz] = A.ir[ii];
						temp.numx[curnz++] = __binary_op(A.numx[ii++], defaultBVal);
					}
					else	// eliminate those existing nonzeros
					{
						++ii;	
						++jj;	
					}
				}
				while (ii < A.cp[i+1])
				{
					temp.ir[curnz] = A.ir[ii];
					temp.numx[curnz++] = __binary_op(A.numx[ii++], defaultBVal);
				}

				if(prevnz < curnz)	// at least one nonzero exists in this column
				{
					temp.jc[curnzc++] = A.jc[i];	
					temp.cp[curnzc] = temp.cp[curnzc-1] + curnz-prevnz;
				}
				++i;
				++j;
			}
		}
		while(i< A.nzc)
		{
			temp.jc[curnzc++] = A.jc[i++];
			for(IU k = A.cp[i-1]; k< A.cp[i]; ++k)
			{
				temp.ir[curnz] 	= A.ir[k];
				temp.numx[curnz++] = __binary_op(A.numx[k], defaultBVal);
			}
			temp.cp[curnzc] = temp.cp[curnzc-1] + (A.cp[i] - A.cp[i-1]);
		}
	}

	temp.Resize(curnzc, curnz);
	return temp;
}


template<typename IU, typename NU1, typename NU2>
SpDCCols<IU, typename promote_trait<NU1,NU2>::T_promote > EWiseMult (const SpDCCols<IU,NU1> & A, const SpDCCols<IU,NU2> & B, bool exclude)
{
	typedef typename promote_trait<NU1,NU2>::T_promote N_promote; 
	assert(A.m == B.m);
	assert(A.n == B.n);

	Dcsc<IU, N_promote> * tdcsc = NULL;
	if(A.nnz > 0 && B.nnz > 0)
	{ 
		tdcsc = new Dcsc<IU, N_promote>(EWiseMult(*(A.dcsc), B.dcsc, exclude));
		return 	SpDCCols<IU, N_promote> (A.m , A.n, tdcsc);
	}
	else if (A.nnz > 0 && exclude) // && B.nnz == 0
	{
		tdcsc = new Dcsc<IU, N_promote>(EWiseMult(*(A.dcsc), (const Dcsc<IU,NU2>*)NULL, exclude));
		return 	SpDCCols<IU, N_promote> (A.m , A.n, tdcsc);
	}
	else
	{
		return 	SpDCCols<IU, N_promote> (A.m , A.n, tdcsc);
	}
}


template<typename N_promote, typename IU, typename NU1, typename NU2, typename _BinaryOperation>
SpDCCols<IU, N_promote> EWiseApply (const SpDCCols<IU,NU1> & A, const SpDCCols<IU,NU2> & B, _BinaryOperation __binary_op, bool notB, const NU2& defaultBVal)
{
	//typedef typename promote_trait<NU1,NU2>::T_promote N_promote; 
	assert(A.m == B.m);
	assert(A.n == B.n);

	Dcsc<IU, N_promote> * tdcsc = NULL;
	if(A.nnz > 0 && B.nnz > 0)
	{ 
		tdcsc = new Dcsc<IU, N_promote>(EWiseApply<N_promote>(*(A.dcsc), B.dcsc, __binary_op, notB, defaultBVal));
		return 	SpDCCols<IU, N_promote> (A.m , A.n, tdcsc);
	}
	else if (A.nnz > 0 && notB) // && B.nnz == 0
	{
		tdcsc = new Dcsc<IU, N_promote>(EWiseApply<N_promote>(*(A.dcsc), (const Dcsc<IU,NU2>*)NULL, __binary_op, notB, defaultBVal));
		return 	SpDCCols<IU, N_promote> (A.m , A.n, tdcsc);
	}
	else
	{
		return 	SpDCCols<IU, N_promote> (A.m , A.n, tdcsc);
	}
}

/** 
 * Implementation based on operator +=
 * Element wise apply with the following constraints
 * The operation to be performed is __binary_op
 * The operation `c = __binary_op(a, b)` is only performed if `do_op(a, b)` returns true
 * If allowANulls is true, then if A is missing an element that B has, then ANullVal is used
 * In that case the operation becomes c[i,j] = __binary_op(ANullVal, b[i,j])
 * If both allowANulls and allowBNulls is false then the function degenerates into intersection
 */
template <typename RETT, typename IU, typename NU1, typename NU2, typename _BinaryOperation, typename _BinaryPredicate>
Dcsc<IU, RETT> EWiseApply(const Dcsc<IU,NU1> * Ap, const Dcsc<IU,NU2> * Bp, _BinaryOperation __binary_op, _BinaryPredicate do_op, bool allowANulls, bool allowBNulls, const NU1& ANullVal, const NU2& BNullVal, const bool allowIntersect)
{
	if (Ap == NULL && Bp == NULL)
		return Dcsc<IU,RETT>(0, 0);
	
	if (Ap == NULL && Bp != NULL)
	{
		if (!allowANulls)
			return Dcsc<IU,RETT>(0, 0);
			
		const Dcsc<IU,NU2> & B = *Bp;
		IU estnzc = B.nzc;
		IU estnz  = B.nz;
		Dcsc<IU,RETT> temp(estnz, estnzc);
	
		IU curnzc = 0;
		IU curnz = 0;
		//IU i = 0;
		IU j = 0;
		temp.cp[0] = 0;
		while(j<B.nzc)
		{
			// Based on the if statement below which handles A null values.
			j++;
			IU prevnz = curnz;		
			temp.jc[curnzc++] = B.jc[j-1];
			for(IU k = B.cp[j-1]; k< B.cp[j]; ++k)
			{
				if (do_op(ANullVal, B.numx[k], true, false))
				{
					temp.ir[curnz] 		= B.ir[k];
					temp.numx[curnz++] 	= __binary_op(ANullVal, B.numx[k], true, false);
				}
			}
			//temp.cp[curnzc] = temp.cp[curnzc-1] + (B.cp[j] - B.cp[j-1]);
			temp.cp[curnzc] = temp.cp[curnzc-1] + curnz-prevnz;
		}
		temp.Resize(curnzc, curnz);
		return temp;
	}
	
	if (Ap != NULL && Bp == NULL)
	{
		if (!allowBNulls)
			return Dcsc<IU,RETT>(0, 0);

		const Dcsc<IU,NU1> & A = *Ap;
		IU estnzc = A.nzc;
		IU estnz  = A.nz;
		Dcsc<IU,RETT> temp(estnz, estnzc);
	
		IU curnzc = 0;
		IU curnz = 0;
		IU i = 0;
		//IU j = 0;
		temp.cp[0] = 0;
		while(i< A.nzc)
		{
			i++;
			IU prevnz = curnz;		
			temp.jc[curnzc++] = A.jc[i-1];
			for(IU k = A.cp[i-1]; k< A.cp[i]; k++)
			{
				if (do_op(A.numx[k], BNullVal, false, true))
				{
					temp.ir[curnz] 		= A.ir[k];
					temp.numx[curnz++] 	= __binary_op(A.numx[k], BNullVal, false, true);
				}
			}
			//temp.cp[curnzc] = temp.cp[curnzc-1] + (A.cp[i] - A.cp[i-1]);
			temp.cp[curnzc] = temp.cp[curnzc-1] + curnz-prevnz;
		}
		temp.Resize(curnzc, curnz);
		return temp;
	}
	
	// both A and B are non-NULL at this point
	const Dcsc<IU,NU1> & A = *Ap;
	const Dcsc<IU,NU2> & B = *Bp;
	
	IU estnzc = A.nzc + B.nzc;
	IU estnz  = A.nz + B.nz;
	Dcsc<IU,RETT> temp(estnz, estnzc);

	IU curnzc = 0;
	IU curnz = 0;
	IU i = 0;
	IU j = 0;
	temp.cp[0] = 0;
	while(i< A.nzc && j<B.nzc)
	{
		if(A.jc[i] > B.jc[j])
		{
			j++;
			if (allowANulls)
			{
				IU prevnz = curnz;		
				temp.jc[curnzc++] = B.jc[j-1];
				for(IU k = B.cp[j-1]; k< B.cp[j]; ++k)
				{
					if (do_op(ANullVal, B.numx[k], true, false))
					{
						temp.ir[curnz] 		= B.ir[k];
						temp.numx[curnz++] 	= __binary_op(ANullVal, B.numx[k], true, false);
					}
				}
				//temp.cp[curnzc] = temp.cp[curnzc-1] + (B.cp[j] - B.cp[j-1]);
				temp.cp[curnzc] = temp.cp[curnzc-1] + curnz-prevnz;
			}
		}
		else if(A.jc[i] < B.jc[j])
		{
			i++;
			if (allowBNulls)
			{
				IU prevnz = curnz;		
				temp.jc[curnzc++] = A.jc[i-1];
				for(IU k = A.cp[i-1]; k< A.cp[i]; k++)
				{
					if (do_op(A.numx[k], BNullVal, false, true))
					{
						temp.ir[curnz] 		= A.ir[k];
						temp.numx[curnz++] 	= __binary_op(A.numx[k], BNullVal, false, true);
					}
				}
				//temp.cp[curnzc] = temp.cp[curnzc-1] + (A.cp[i] - A.cp[i-1]);
				temp.cp[curnzc] = temp.cp[curnzc-1] + curnz-prevnz;
			}
		}
		else
		{
			temp.jc[curnzc++] = A.jc[i];
			IU ii = A.cp[i];
			IU jj = B.cp[j];
			IU prevnz = curnz;		
			while (ii < A.cp[i+1] && jj < B.cp[j+1])
			{
				if (A.ir[ii] < B.ir[jj])
				{
					if (allowBNulls && do_op(A.numx[ii], BNullVal, false, true))
					{
						temp.ir[curnz] = A.ir[ii];
						temp.numx[curnz++] = __binary_op(A.numx[ii++], BNullVal, false, true);
					}
					else
						ii++;
				}
				else if (A.ir[ii] > B.ir[jj])
				{
					if (allowANulls && do_op(ANullVal, B.numx[jj], true, false))
					{
						temp.ir[curnz] = B.ir[jj];
						temp.numx[curnz++] = __binary_op(ANullVal, B.numx[jj++], true, false);
					}
					else
						jj++;
				}
				else
				{
					if (allowIntersect && do_op(A.numx[ii], B.numx[jj], false, false))
					{
						temp.ir[curnz] = A.ir[ii];
						temp.numx[curnz++] = __binary_op(A.numx[ii++], B.numx[jj++], false, false);	// might include zeros
					}
					else
					{
						ii++;
						jj++;
					}
				}
			}
			while (ii < A.cp[i+1])
			{
				if (allowBNulls && do_op(A.numx[ii], BNullVal, false, true))
				{
					temp.ir[curnz] = A.ir[ii];
					temp.numx[curnz++] = __binary_op(A.numx[ii++], BNullVal, false, true);
				}
				else
					ii++;
			}
			while (jj < B.cp[j+1])
			{
				if (allowANulls && do_op(ANullVal, B.numx[jj], true, false))
				{
					temp.ir[curnz] = B.ir[jj];
					temp.numx[curnz++] = __binary_op(ANullVal, B.numx[jj++], true, false);
				}
				else
					jj++;
			}
			temp.cp[curnzc] = temp.cp[curnzc-1] + curnz-prevnz;
			++i;
			++j;
		}
	}
	while(allowBNulls && i< A.nzc) // remaining A elements after B ran out
	{
		IU prevnz = curnz;		
		temp.jc[curnzc++] = A.jc[i++];
		for(IU k = A.cp[i-1]; k< A.cp[i]; ++k)
		{
			if (do_op(A.numx[k], BNullVal, false, true))
			{
				temp.ir[curnz] 	= A.ir[k];
				temp.numx[curnz++] = __binary_op(A.numx[k], BNullVal, false, true);
			}
		}
		//temp.cp[curnzc] = temp.cp[curnzc-1] + (A.cp[i] - A.cp[i-1]);
		temp.cp[curnzc] = temp.cp[curnzc-1] + curnz-prevnz;
	}
	while(allowANulls && j < B.nzc) // remaining B elements after A ran out
	{
		IU prevnz = curnz;		
		temp.jc[curnzc++] = B.jc[j++];
		for(IU k = B.cp[j-1]; k< B.cp[j]; ++k)
		{
			if (do_op(ANullVal, B.numx[k], true, false))
			{
				temp.ir[curnz] 	= B.ir[k];
				temp.numx[curnz++] 	= __binary_op(ANullVal, B.numx[k], true, false);
			}
		}
		//temp.cp[curnzc] = temp.cp[curnzc-1] + (B.cp[j] - B.cp[j-1]);
		temp.cp[curnzc] = temp.cp[curnzc-1] + curnz-prevnz;
	}
	temp.Resize(curnzc, curnz);
	return temp;
}

template <typename RETT, typename IU, typename NU1, typename NU2, typename _BinaryOperation, typename _BinaryPredicate> 
SpDCCols<IU,RETT> EWiseApply (const SpDCCols<IU,NU1> & A, const SpDCCols<IU,NU2> & B, _BinaryOperation __binary_op, _BinaryPredicate do_op, bool allowANulls, bool allowBNulls, const NU1& ANullVal, const NU2& BNullVal, const bool allowIntersect)
{
	assert(A.m == B.m);
	assert(A.n == B.n);

	Dcsc<IU, RETT> * tdcsc = new Dcsc<IU, RETT>(EWiseApply<RETT>(A.dcsc, B.dcsc, __binary_op, do_op, allowANulls, allowBNulls, ANullVal, BNullVal, allowIntersect));
	return 	SpDCCols<IU, RETT> (A.m , A.n, tdcsc);
}


#endif
