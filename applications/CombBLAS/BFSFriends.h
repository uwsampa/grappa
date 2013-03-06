#ifndef _BFS_FRIENDS_H_
#define _BFS_FRIENDS_H_

#include "mpi.h"
#include <iostream>
#include "SpParMat.h"	
#include "SpParHelper.h"
#include "MPIType.h"
#include "Friends.h"
#include "OptBuf.h"
#include "ParFriends.h"
#include "SpImplNoSR.h"
// AL: doesn't compile on OSX. #include <parallel/algorithm>

using namespace std;

template <class IT, class NT, class DER>
class SpParMat;

/*************************************************************************************************/
/*********************** FRIEND FUNCTIONS FOR BFS ONLY (NO SEMIRINGS) RUNS  **********************/
/***************************** BOTH PARALLEL AND SEQUENTIAL FUNCTIONS ****************************/
/*************************************************************************************************/

/** 
 * Multithreaded SpMV with sparse vector and preset buffers
 * the assembly of outgoing buffers sendindbuf/sendnumbuf are done here
 */
template <typename IT, typename VT>
void dcsc_gespmv_threaded_setbuffers (const SpDCCols<IT, bool> & A, const int32_t * indx, const VT * numx, int32_t nnzx, 
				 int32_t * sendindbuf, VT * sendnumbuf, int * cnts, int * dspls, int p_c)
{
	if(A.getnnz() > 0 && nnzx > 0)
	{
		int splits = A.getnsplit();
		if(splits > 0)
		{
			vector< vector<int32_t> > indy(splits);
			vector< vector< VT > > numy(splits);
			int32_t nlocrows = static_cast<int32_t>(A.getnrow());
			int32_t perpiece = nlocrows / splits;
			
			#ifdef _OPENMP
			#pragma omp parallel for 
			#endif
			for(int i=0; i<splits; ++i)
			{
				if(i != splits-1)
					SpMXSpV_ForThreading(*(A.GetDCSC(i)), perpiece, indx, numx, nnzx, indy[i], numy[i], i*perpiece);
				else
					SpMXSpV_ForThreading(*(A.GetDCSC(i)), nlocrows - perpiece*i, indx, numx, nnzx, indy[i], numy[i], i*perpiece);
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
						if( ( (*it) >= lastdata ) && cur_rec != last_rec)	
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

/**
  * Step 3 of the sparse SpMV algorithm, without the semiring (BFS only)
  * @param[in,out] optbuf {scratch space for all-to-all (fold) communication}
  * @param[in,out] indacc, numacc {index and values of the input vector, deleted upon exit}
  * @param[in,out] sendindbuf, sendnumbuf {index and values of the output vector, created}
 **/
template<typename VT, typename IT, typename UDER>
void LocalSpMV(const SpParMat<IT,bool,UDER> & A, int rowneighs, OptBuf<int32_t, VT > & optbuf, int32_t * & indacc, VT * & numacc, int * sendcnt, int accnz)
{	

#ifdef TIMING
	double t0=MPI_Wtime();
#endif
	if(optbuf.totmax > 0)	// graph500 optimization enabled
	{ 
		if(A.spSeq->getnsplit() > 0)
		{
			// optbuf.{inds/nums/dspls} and sendcnt are all pre-allocated and only filled by dcsc_gespmv_threaded
			dcsc_gespmv_threaded_setbuffers (*(A.spSeq), indacc, numacc, accnz, optbuf.inds, optbuf.nums, sendcnt, optbuf.dspls, rowneighs);	
		}
		else
		{
			// by-pass dcsc_gespmv call
			if(A.getlocalnnz() > 0 && accnz > 0)
			{
				SpMXSpV(*((A.spSeq)->GetDCSC()), (int32_t) A.getlocalrows(), indacc, numacc, 
					accnz, optbuf.inds, optbuf.nums, sendcnt, optbuf.dspls, rowneighs, optbuf.isthere);
			}
		}
		DeleteAll(indacc,numacc);
	}
	else
	{
		SpParHelper::Print("BFS only (no semiring) function only work with optimization buffers\n");
	}

#ifdef TIMING
	double t1=MPI_Wtime();
	cblas_localspmvtime += (t1-t0);
#endif
}


template <typename IU, typename VT>
void MergeContributions(FullyDistSpVec<IU,VT> & y, int * & recvcnt, int * & rdispls, int32_t * & recvindbuf, VT * & recvnumbuf, int rowneighs)
{
#ifdef TIMING
	double t0=MPI_Wtime();
#endif
	// free memory of y, in case it was aliased
	vector<IU>().swap(y.ind);
	vector<VT>().swap(y.num);
	
#ifndef HEAPMERGE
	IU ysize = y.MyLocLength();	// my local length is only O(n/p)
	bool * isthere = new bool[ysize];
	vector< pair<IU,VT> > ts_pairs;	
	fill_n(isthere, ysize, false);

	// We don't need to keep a "merger" because minimum will always come from the processor
	// with the smallest rank; so a linear sweep over the received buffer is enough	
	for(int i=0; i<rowneighs; ++i)
	{
		for(int j=0; j< recvcnt[i]; ++j) 
		{
			int32_t index = recvindbuf[rdispls[i] + j];
			if(!isthere[index])
				ts_pairs.push_back(make_pair(index, recvnumbuf[rdispls[i] + j]));
			
		}
	}
	DeleteAll(recvcnt, rdispls);
	DeleteAll(isthere, recvindbuf, recvnumbuf);
	__gnu_parallel::sort(ts_pairs.begin(), ts_pairs.end());
	int nnzy = ts_pairs.size();
	y.ind.resize(nnzy);
	y.num.resize(nnzy);
	for(int i=0; i< nnzy; ++i)
	{
		y.ind[i] = ts_pairs[i].first;
		y.num[i] = ts_pairs[i].second; 	
	}

#else
	// Alternative 2: Heap-merge
	int32_t hsize = 0;		
	int32_t inf = numeric_limits<int32_t>::min();
	int32_t sup = numeric_limits<int32_t>::max(); 
	KNHeap< int32_t, int32_t > sHeap(sup, inf); 
	int * processed = new int[rowneighs]();
	for(int32_t i=0; i<rowneighs; ++i)
	{
		if(recvcnt[i] > 0)
		{
			// key, proc_id
			sHeap.insert(recvindbuf[rdispls[i]], i);
			++hsize;
		}
	}	
	int32_t key, locv;
	if(hsize > 0)
	{
		sHeap.deleteMin(&key, &locv);
		y.ind.push_back( static_cast<IU>(key));
		y.num.push_back(recvnumbuf[rdispls[locv]]);	// nothing is processed yet
		
		if( (++(processed[locv])) < recvcnt[locv] )
			sHeap.insert(recvindbuf[rdispls[locv]+processed[locv]], locv);
		else
			--hsize;
	}

//	ofstream oput;
//	y.commGrid->OpenDebugFile("Merge", oput);
//	oput << "From displacements: "; copy(rdispls, rdispls+rowneighs, ostream_iterator<int>(oput, " ")); oput << endl;
//	oput << "From counts: "; copy(recvcnt, recvcnt+rowneighs, ostream_iterator<int>(oput, " ")); oput << endl;
	while(hsize > 0)
	{
		sHeap.deleteMin(&key, &locv);
		IU deref = rdispls[locv] + processed[locv];
		if(y.ind.back() != static_cast<IU>(key))	// y.ind is surely not empty
		{
			y.ind.push_back(static_cast<IU>(key));
			y.num.push_back(recvnumbuf[deref]);
		}
		
		if( (++(processed[locv])) < recvcnt[locv] )
			sHeap.insert(recvindbuf[rdispls[locv]+processed[locv]], locv);
		else
			--hsize;
	}
	DeleteAll(recvcnt, rdispls,processed);
	DeleteAll(recvindbuf, recvnumbuf);
#endif

#ifdef TIMING
	double t1=MPI_Wtime();
	cblas_mergeconttime += (t1-t0);
#endif
}	

/**
  * This is essentially a SpMV for BFS because it lacks the semiring.
  * It naturally justs selects columns of A (adjacencies of frontier) and 
  * merges with the minimum entry succeeding. SpParMat has to be boolean
  * input and output vectors are of type VT but their indices are IT
  */
template <typename VT, typename IT, typename UDER>
FullyDistSpVec<IT,VT>  SpMV (const SpParMat<IT,bool,UDER> & A, const FullyDistSpVec<IT,VT> & x, OptBuf<int32_t, VT > & optbuf)
{
	CheckSpMVCompliance(A,x);
	optbuf.MarkEmpty();
		
	MPI_Comm World = x.commGrid->GetWorld();
	MPI_Comm ColWorld = x.commGrid->GetColWorld();
	MPI_Comm RowWorld = x.commGrid->GetRowWorld();

	int accnz;
	int32_t trxlocnz;
	IT lenuntil;
	int32_t *trxinds, *indacc;
	VT *trxnums, *numacc;
	
#ifdef TIMING
	double t0=MPI_Wtime();
#endif
	TransposeVector(World, x, trxlocnz, lenuntil, trxinds, trxnums, true);			// trxinds (and potentially trxnums) is allocated
#ifdef TIMING
	double t1=MPI_Wtime();
	cblas_transvectime += (t1-t0);
#endif
	AllGatherVector(ColWorld, trxlocnz, lenuntil, trxinds, trxnums, indacc, numacc, accnz, true);	// trxinds (and potentially trxnums) is deallocated, indacc/numacc allocated
	
	FullyDistSpVec<IT, VT> y ( x.commGrid, A.getnrow());	// identity doesn't matter for sparse vectors
	int rowneighs; MPI_Comm_size(RowWorld,&rowneighs);
	int * sendcnt = new int[rowneighs]();	

	LocalSpMV(A, rowneighs, optbuf, indacc, numacc, sendcnt, accnz);	// indacc/numacc deallocated

	int * rdispls = new int[rowneighs];
	int * recvcnt = new int[rowneighs];
	MPI_Alltoall(sendcnt, 1, MPI_INT, recvcnt, 1, MPI_INT, RowWorld);	// share the request counts
	
	// receive displacements are exact whereas send displacements have slack
	rdispls[0] = 0;
	for(int i=0; i<rowneighs-1; ++i)
	{
		rdispls[i+1] = rdispls[i] + recvcnt[i];
	}
	int totrecv = accumulate(recvcnt,recvcnt+rowneighs,0);	
	int32_t * recvindbuf = new int32_t[totrecv];
	VT * recvnumbuf = new VT[totrecv];
	
#ifdef TIMING
	double t2=MPI_Wtime();
#endif
	if(optbuf.totmax > 0 )	// graph500 optimization enabled
	{
	        MPI_Alltoallv(optbuf.inds, sendcnt, optbuf.dspls, MPIType<int32_t>(), recvindbuf, recvcnt, rdispls, MPIType<int32_t>(), RowWorld);  
		MPI_Alltoallv(optbuf.nums, sendcnt, optbuf.dspls, MPIType<VT>(), recvnumbuf, recvcnt, rdispls, MPIType<VT>(), RowWorld);  
		delete [] sendcnt;
	}
	else
	{
		SpParHelper::Print("BFS only (no semiring) function only work with optimization buffers\n");
	}
#ifdef TIMING
	double t3=MPI_Wtime();
	cblas_alltoalltime += (t3-t2);
#endif

	MergeContributions(y,recvcnt, rdispls, recvindbuf, recvnumbuf, rowneighs);
	return y;	
}

#endif
