#ifndef _PAR_FRIENDS_H_
#define _PAR_FRIENDS_H_

#include "mpi.h"
#include <iostream>
#include <cstdarg>
#include "SpParMat.h"	
#include "SpParHelper.h"
#include "MPIType.h"
#include "Friends.h"
#include "OptBuf.h"


using namespace std;

template <class IT, class NT, class DER>
class SpParMat;

/*************************************************************************************************/
/**************************** FRIEND FUNCTIONS FOR PARALLEL CLASSES ******************************/
/*************************************************************************************************/


/**
 ** Concatenate all the FullyDistVec<IT,NT> objects into a single one
 **/
template <typename IT, typename NT>
FullyDistVec<IT,NT> Concatenate ( vector< FullyDistVec<IT,NT> > & vecs)
{
	if(vecs.size() < 1)
	{
		SpParHelper::Print("Warning: Nothing to concatenate, returning empty ");
		return FullyDistVec<IT,NT>();
	}
	else if (vecs.size() < 2)
	{
		return vecs[1];
	
	}
	else 
	{
		typename vector< FullyDistVec<IT,NT> >::iterator it = vecs.begin();
		shared_ptr<CommGrid> commGridPtr = it->getcommgrid();
		MPI_Comm World = commGridPtr->GetWorld();
		
		IT nglen = it->TotalLength();	// new global length
		IT cumloclen = it->MyLocLength();	// existing cumulative local lengths 
		++it;
		for(; it != vecs.end(); ++it)
		{
			if(*(commGridPtr) != *(it->getcommgrid()))
			{
				SpParHelper::Print("Grids are not comparable for FullyDistVec<IT,NT>::EWiseApply\n");
				MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
			}
			nglen += it->TotalLength();
			cumloclen += it->MyLocLength();
		}
		FullyDistVec<IT,NT> ConCat (commGridPtr, nglen, NT());	
		int nprocs = commGridPtr->GetSize();
		
		vector< vector< NT > > data(nprocs);
		vector< vector< IT > > inds(nprocs);
		IT gloffset = 0;
		for(it = vecs.begin(); it != vecs.end(); ++it)
		{
			IT loclen = it->LocArrSize();
			for(IT i=0; i < loclen; ++i)
			{
				IT locind;
				IT loffset = it->LengthUntil();
				int owner = ConCat.Owner(gloffset+loffset+i, locind);	
				data[owner].push_back(it->arr[i]);
				inds[owner].push_back(locind);
			}
			gloffset += it->TotalLength();
		}
		
		int * sendcnt = new int[nprocs];
		int * sdispls = new int[nprocs];
		for(int i=0; i<nprocs; ++i)
			sendcnt[i] = (int) data[i].size();
		
		int * rdispls = new int[nprocs];
		int * recvcnt = new int[nprocs];
		MPI_Alltoall(sendcnt, 1, MPI_INT, recvcnt, 1, MPI_INT, World);  // share the request counts
		sdispls[0] = 0;
		rdispls[0] = 0;
		for(int i=0; i<nprocs-1; ++i)
		{
			sdispls[i+1] = sdispls[i] + sendcnt[i];
			rdispls[i+1] = rdispls[i] + recvcnt[i];
		}
		IT totrecv = accumulate(recvcnt,recvcnt+nprocs,static_cast<IT>(0));
		NT * senddatabuf = new NT[cumloclen];
		for(int i=0; i<nprocs; ++i)
		{
			copy(data[i].begin(), data[i].end(), senddatabuf+sdispls[i]);
			vector<NT>().swap(data[i]);	// delete data vectors
		}
		NT * recvdatabuf = new NT[totrecv];
		MPI_Alltoallv(senddatabuf, sendcnt, sdispls, MPIType<NT>(), recvdatabuf, recvcnt, rdispls, MPIType<NT>(), World);  // send data
		delete [] senddatabuf;
		
		IT * sendindsbuf = new IT[cumloclen];
		for(int i=0; i<nprocs; ++i)
		{
			copy(inds[i].begin(), inds[i].end(), sendindsbuf+sdispls[i]);
			vector<IT>().swap(inds[i]);	// delete inds vectors
		}
		IT * recvindsbuf = new IT[totrecv];
		MPI_Alltoallv(sendindsbuf, sendcnt, sdispls, MPIType<IT>(), recvindsbuf, recvcnt, rdispls, MPIType<IT>(), World);  // send new inds
		DeleteAll(sendindsbuf, sendcnt, sdispls);

		for(int i=0; i<nprocs; ++i)
		{
			for(int j = rdispls[i]; j < rdispls[i] + recvcnt[i]; ++j)			
			{
				ConCat.arr[recvindsbuf[j]] = recvdatabuf[j];
			}
		}
		DeleteAll(recvindsbuf, recvcnt, rdispls);
		return ConCat;
	}
}

template <typename MATRIXA, typename MATRIXB>
bool CheckSpGEMMCompliance(const MATRIXA & A, const MATRIXB & B)
{
	if(A.getncol() != B.getnrow())
	{
		ostringstream outs;
		outs << "Can not multiply, dimensions does not match"<< endl;
		outs << A.getncol() << " != " << B.getnrow() << endl;
		SpParHelper::Print(outs.str());
		MPI_Abort(MPI_COMM_WORLD, DIMMISMATCH);
		return false;
	}	
	if((void*) &A == (void*) &B)
	{
		ostringstream outs;
		outs << "Can not multiply, inputs alias (make a temporary copy of one of them first)"<< endl;
		SpParHelper::Print(outs.str());
		MPI_Abort(MPI_COMM_WORLD, MATRIXALIAS);
		return false;
	}	
	return true;
}	


/**
 * Parallel C = A*B routine that uses a double buffered broadcasting scheme 
 * @pre { Input matrices, A and B, should not alias }
 * Most memory efficient version available. Total stages: 2*sqrt(p)
 * Memory requirement during first sqrt(p) stages: <= (3/2)*(nnz(A)+nnz(B))+(1/2)*nnz(C)
 * Memory requirement during second sqrt(p) stages: <= nnz(A)+nnz(B)+nnz(C)
 * Final memory requirement: nnz(C) if clearA and clearB are true 
 **/  
template <typename SR, typename NUO, typename UDERO, typename IU, typename NU1, typename NU2, typename UDERA, typename UDERB> 
SpParMat<IU,NUO,UDERO> Mult_AnXBn_DoubleBuff
		(SpParMat<IU,NU1,UDERA> & A, SpParMat<IU,NU2,UDERB> & B, bool clearA = false, bool clearB = false )

{
	if(!CheckSpGEMMCompliance(A,B) )
	{
		return SpParMat< IU,NUO,UDERO >();
	}

	int stages, dummy; 	// last two parameters of ProductGrid are ignored for Synch multiplication
	shared_ptr<CommGrid> GridC = ProductGrid((A.commGrid).get(), (B.commGrid).get(), stages, dummy, dummy);		
	IU C_m = A.spSeq->getnrow();
	IU C_n = B.spSeq->getncol();

	UDERA * A1seq = new UDERA();
	UDERA * A2seq = new UDERA(); 
	UDERB * B1seq = new UDERB();
	UDERB * B2seq = new UDERB();
	(A.spSeq)->Split( *A1seq, *A2seq); 
	const_cast< UDERB* >(B.spSeq)->Transpose();
	(B.spSeq)->Split( *B1seq, *B2seq);
	MPI_Barrier(GridC->GetWorld());

	IU ** ARecvSizes = SpHelper::allocate2D<IU>(UDERA::esscount, stages);
	IU ** BRecvSizes = SpHelper::allocate2D<IU>(UDERB::esscount, stages);

	SpParHelper::GetSetSizes( *A1seq, ARecvSizes, (A.commGrid)->GetRowWorld());
	SpParHelper::GetSetSizes( *B1seq, BRecvSizes, (B.commGrid)->GetColWorld());

	// Remotely fetched matrices are stored as pointers
	UDERA * ARecv; 
	UDERB * BRecv;
	vector< SpTuples<IU,NUO>  *> tomerge;

	int Aself = (A.commGrid)->GetRankInProcRow();
	int Bself = (B.commGrid)->GetRankInProcCol();	

	for(int i = 0; i < stages; ++i) 
	{
		vector<IU> ess;	
		if(i == Aself)
		{	
			ARecv = A1seq;	// shallow-copy 
		}
		else
		{
			ess.resize(UDERA::esscount);
			for(int j=0; j< UDERA::esscount; ++j)	
			{
				ess[j] = ARecvSizes[j][i];		// essentials of the ith matrix in this row	
			}
			ARecv = new UDERA();				// first, create the object
		}
		SpParHelper::BCastMatrix(GridC->GetRowWorld(), *ARecv, ess, i);	// then, receive its elements	
		ess.clear();	
		if(i == Bself)
		{
			BRecv = B1seq;	// shallow-copy
		}
		else
		{
			ess.resize(UDERB::esscount);		
			for(int j=0; j< UDERB::esscount; ++j)	
			{
				ess[j] = BRecvSizes[j][i];	
			}	
			BRecv = new UDERB();
		}
		SpParHelper::BCastMatrix(GridC->GetColWorld(), *BRecv, ess, i);	// then, receive its elements
		SpTuples<IU,NUO> * C_cont = MultiplyReturnTuples<SR, NUO>
						(*ARecv, *BRecv, // parameters themselves
						false, true,	// transpose information (B is transposed)
						i != Aself, 	// 'delete A' condition
						i != Bself);	// 'delete B' condition
		if(!C_cont->isZero()) 
			tomerge.push_back(C_cont);
		else
			delete C_cont;
	}
	if(clearA) delete A1seq;
	if(clearB) delete B1seq;
	
	// Set the new dimensions
	SpParHelper::GetSetSizes( *A2seq, ARecvSizes, (A.commGrid)->GetRowWorld());
	SpParHelper::GetSetSizes( *B2seq, BRecvSizes, (B.commGrid)->GetColWorld());

	// Start the second round
	for(int i = 0; i < stages; ++i) 
	{
		vector<IU> ess;	
		if(i == Aself)
		{	
			ARecv = A2seq;	// shallow-copy 
		}
		else
		{
			ess.resize(UDERA::esscount);
			for(int j=0; j< UDERA::esscount; ++j)	
			{
				ess[j] = ARecvSizes[j][i];		// essentials of the ith matrix in this row	
			}
			ARecv = new UDERA();				// first, create the object
		}

		SpParHelper::BCastMatrix(GridC->GetRowWorld(), *ARecv, ess, i);	// then, receive its elements	
		ess.clear();	
		
		if(i == Bself)
		{
			BRecv = B2seq;	// shallow-copy
		}
		else
		{
			ess.resize(UDERB::esscount);		
			for(int j=0; j< UDERB::esscount; ++j)	
			{
				ess[j] = BRecvSizes[j][i];	
			}	
			BRecv = new UDERB();
		}
		SpParHelper::BCastMatrix(GridC->GetColWorld(), *BRecv, ess, i);	// then, receive its elements

		SpTuples<IU,NUO> * C_cont = MultiplyReturnTuples<SR, NUO>
						(*ARecv, *BRecv, // parameters themselves
						false, true,	// transpose information (B is transposed)
						i != Aself, 	// 'delete A' condition
						i != Bself);	// 'delete B' condition
		if(!C_cont->isZero()) 
			tomerge.push_back(C_cont);
		else
			delete C_cont;
	}
	SpHelper::deallocate2D(ARecvSizes, UDERA::esscount);
	SpHelper::deallocate2D(BRecvSizes, UDERB::esscount);
	if(clearA) 
	{
		delete A2seq;
		delete A.spSeq;
		A.spSeq = NULL;
	}
	else
	{
		(A.spSeq)->Merge(*A1seq, *A2seq);
		delete A1seq;
		delete A2seq;
	}
	if(clearB) 
	{
		delete B2seq;
		delete B.spSeq;
		B.spSeq = NULL;	
	}
	else
	{
		(B.spSeq)->Merge(*B1seq, *B2seq);	
		delete B1seq;
		delete B2seq;
		const_cast< UDERB* >(B.spSeq)->Transpose();	// transpose back to original
	}
			
	UDERO * C = new UDERO(MergeAll<SR>(tomerge, C_m, C_n,true), false);	
	// First get the result in SpTuples, then convert to UDER
	return SpParMat<IU,NUO,UDERO> (C, GridC);		// return the result object
}


/**
 * Parallel A = B*C routine that uses only MPI-1 features
 * Relies on simple blocking broadcast
 * @pre { Input matrices, A and B, should not alias }
 **/  
template <typename SR, typename NUO, typename UDERO, typename IU, typename NU1, typename NU2, typename UDERA, typename UDERB> 
SpParMat<IU, NUO, UDERO> Mult_AnXBn_Synch 
		(SpParMat<IU,NU1,UDERA> & A, SpParMat<IU,NU2,UDERB> & B, bool clearA = false, bool clearB = false )

{
	if(!CheckSpGEMMCompliance(A,B) )
	{
		return SpParMat< IU,NUO,UDERO >();
	}
	int stages, dummy; 	// last two parameters of ProductGrid are ignored for Synch multiplication
	shared_ptr<CommGrid> GridC = ProductGrid((A.commGrid).get(), (B.commGrid).get(), stages, dummy, dummy);		
	IU C_m = A.spSeq->getnrow();
	IU C_n = B.spSeq->getncol();
	
	const_cast< UDERB* >(B.spSeq)->Transpose();	
	MPI_Barrier(GridC->GetWorld());

	IU ** ARecvSizes = SpHelper::allocate2D<IU>(UDERA::esscount, stages);
	IU ** BRecvSizes = SpHelper::allocate2D<IU>(UDERB::esscount, stages);
	
	SpParHelper::GetSetSizes( *(A.spSeq), ARecvSizes, (A.commGrid)->GetRowWorld());
	SpParHelper::GetSetSizes( *(B.spSeq), BRecvSizes, (B.commGrid)->GetColWorld());

	// Remotely fetched matrices are stored as pointers
	UDERA * ARecv; 
	UDERB * BRecv;
	vector< SpTuples<IU,NUO>  *> tomerge;

	int Aself = (A.commGrid)->GetRankInProcRow();
	int Bself = (B.commGrid)->GetRankInProcCol();	

	for(int i = 0; i < stages; ++i) 
	{
		vector<IU> ess;	
		if(i == Aself)
		{	
			ARecv = A.spSeq;	// shallow-copy 
		}
		else
		{
			ess.resize(UDERA::esscount);
			for(int j=0; j< UDERA::esscount; ++j)	
			{
				ess[j] = ARecvSizes[j][i];		// essentials of the ith matrix in this row	
			}
			ARecv = new UDERA();				// first, create the object
		}

		SpParHelper::BCastMatrix(GridC->GetRowWorld(), *ARecv, ess, i);	// then, receive its elements	
		ess.clear();	
		
		if(i == Bself)
		{
			BRecv = B.spSeq;	// shallow-copy
		}
		else
		{
			ess.resize(UDERB::esscount);		
			for(int j=0; j< UDERB::esscount; ++j)	
			{
				ess[j] = BRecvSizes[j][i];	
			}	
			BRecv = new UDERB();
		}
		SpParHelper::BCastMatrix(GridC->GetColWorld(), *BRecv, ess, i);	// then, receive its elements

		SpTuples<IU,NUO> * C_cont = MultiplyReturnTuples<SR, NUO>
						(*ARecv, *BRecv, // parameters themselves
						false, true,	// transpose information (B is transposed)
						i != Aself, 	// 'delete A' condition
						i != Bself);	// 'delete B' condition

		if(!C_cont->isZero()) 
			tomerge.push_back(C_cont);

		#ifndef NDEBUG
		ostringstream outs;
		outs << i << "th SUMMA iteration"<< endl;
		SpParHelper::Print(outs.str());
		#endif
	}
	if(clearA && A.spSeq != NULL) 
	{	
		delete A.spSeq;
		A.spSeq = NULL;
	}	
	if(clearB && B.spSeq != NULL) 
	{
		delete B.spSeq;
		B.spSeq = NULL;
	}

	SpHelper::deallocate2D(ARecvSizes, UDERA::esscount);
	SpHelper::deallocate2D(BRecvSizes, UDERB::esscount);
		
	UDERO * C = new UDERO(MergeAll<SR>(tomerge, C_m, C_n,true), false);	
	// First get the result in SpTuples, then convert to UDER
	// the last parameter to MergeAll deletes tomerge arrays

	if(!clearB)
		const_cast< UDERB* >(B.spSeq)->Transpose();	// transpose back to original

	return SpParMat<IU,NUO,UDERO> (C, GridC);		// return the result object
}


template <typename MATRIX, typename VECTOR>
void CheckSpMVCompliance(const MATRIX & A, const VECTOR & x)
{
	if(A.getncol() != x.TotalLength())
	{
		ostringstream outs;
		outs << "Can not multiply, dimensions does not match"<< endl;
		outs << A.getncol() << " != " << x.TotalLength() << endl;
		SpParHelper::Print(outs.str());
		MPI_Abort(MPI_COMM_WORLD, DIMMISMATCH);
	}
	if(! ( *(A.getcommgrid()) == *(x.getcommgrid())) ) 		
	{
		cout << "Grids are not comparable for SpMV" << endl; 
		MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
	}
}			


template <typename SR, typename IU, typename NUM, typename UDER> 
FullyDistSpVec<IU,typename promote_trait<NUM,IU>::T_promote>  SpMV 
	(const SpParMat<IU,NUM,UDER> & A, const FullyDistSpVec<IU,IU> & x, bool indexisvalue, OptBuf<int32_t, typename promote_trait<NUM,IU>::T_promote > & optbuf);

template <typename SR, typename IU, typename NUM, typename UDER> 
FullyDistSpVec<IU,typename promote_trait<NUM,IU>::T_promote>  SpMV 
	(const SpParMat<IU,NUM,UDER> & A, const FullyDistSpVec<IU,IU> & x, bool indexisvalue)
{
	typedef typename promote_trait<NUM,IU>::T_promote T_promote;
	OptBuf<int32_t, T_promote > optbuf = OptBuf<int32_t, T_promote >();
	return SpMV<SR>(A, x, indexisvalue, optbuf);
}

/**
 * Step 1 of the sparse SpMV algorithm 
 * @param[in,out]   trxlocnz, lenuntil,trxinds,trxnums  { set or allocated }
 * @param[in] 	indexisvalue	
 **/
template<typename IU, typename NV>
void TransposeVector(MPI_Comm & World, const FullyDistSpVec<IU,NV> & x, int32_t & trxlocnz, IU & lenuntil, int32_t * & trxinds, NV * & trxnums, bool indexisvalue)
{
	int32_t xlocnz = (int32_t) x.getlocnnz();	
	int32_t roffst = (int32_t) x.RowLenUntil();	// since trxinds is int32_t
	int32_t roffset;
	IU luntil = x.LengthUntil();
	int diagneigh = x.commGrid->GetComplementRank();

	MPI_Status status;
	MPI_Sendrecv(&roffst, 1, MPIType<int32_t>(), diagneigh, TROST, &roffset, 1, MPIType<int32_t>(), diagneigh, TROST, World, &status);
	MPI_Sendrecv(&xlocnz, 1, MPIType<int32_t>(), diagneigh, TRNNZ, &trxlocnz, 1, MPIType<int32_t>(), diagneigh, TRNNZ, World, &status);
	MPI_Sendrecv(&luntil, 1, MPIType<IU>(), diagneigh, TRLUT, &lenuntil, 1, MPIType<IU>(), diagneigh, TRLUT, World, &status);
	
	// ABAB: Important observation is that local indices (given by x.ind) is 32-bit addressible
	// Copy them to 32 bit integers and transfer that to save 50% of off-node bandwidth
	trxinds = new int32_t[trxlocnz];
	int32_t * temp_xind = new int32_t[xlocnz];
	for(int i=0; i< xlocnz; ++i)	temp_xind[i] = (int32_t) x.ind[i];
	MPI_Sendrecv(temp_xind, xlocnz, MPIType<int32_t>(), diagneigh, TRI, trxinds, trxlocnz, MPIType<int32_t>(), diagneigh, TRI, World, &status);
	delete [] temp_xind;
	if(!indexisvalue)
	{
		trxnums = new NV[trxlocnz];
		MPI_Sendrecv(const_cast<NV*>(SpHelper::p2a(x.num)), xlocnz, MPIType<NV>(), diagneigh, TRX, trxnums, trxlocnz, MPIType<NV>(), diagneigh, TRX, World, &status);
	}
	transform(trxinds, trxinds+trxlocnz, trxinds, bind2nd(plus<int32_t>(), roffset)); // fullydist indexing (p pieces) -> matrix indexing (sqrt(p) pieces)
}	

/**
 * Step 2 of the sparse SpMV algorithm 
 * @param[in,out]   trxinds, trxnums { deallocated }
 * @param[in,out]   indacc, numacc { allocated }
 * @param[in,out]	accnz { set }
 * @param[in] 		trxlocnz, lenuntil, indexisvalue
 **/
template<typename IU, typename NV>
void AllGatherVector(MPI_Comm & ColWorld, int trxlocnz, IU lenuntil, int32_t * & trxinds, NV * & trxnums, 
					 int32_t * & indacc, NV * & numacc, int & accnz, bool indexisvalue)
{
        int colneighs, colrank;
	MPI_Comm_size(ColWorld, &colneighs);
	MPI_Comm_rank(ColWorld, &colrank);
	int * colnz = new int[colneighs];
	colnz[colrank] = trxlocnz;
	MPI_Allgather(MPI_IN_PLACE, 1, MPI_INT, colnz, 1, MPI_INT, ColWorld);
	int * dpls = new int[colneighs]();	// displacements (zero initialized pid) 
	std::partial_sum(colnz, colnz+colneighs-1, dpls+1);
	accnz = std::accumulate(colnz, colnz+colneighs, 0);
	indacc = new int32_t[accnz];
	numacc = new NV[accnz];
	
	// ABAB: Future issues here, colnz is of type int (MPI limitation)
	// What if the aggregate vector size along the processor row/column is not 32-bit addressible?
	// This will happen when n/sqrt(p) > 2^31
	// Currently we can solve a small problem (scale 32) with 4096 processor
	// For a medium problem (scale 35), we'll need 32K processors which gives sqrt(p) ~ 180
	// 2^35 / 180 ~ 2^29 / 3 which is not an issue !
	
#ifdef TIMING
	double t0=MPI_Wtime();
#endif
	MPI_Allgatherv(trxinds, trxlocnz, MPIType<int32_t>(), indacc, colnz, dpls, MPIType<int32_t>(), ColWorld);
	
	delete [] trxinds;
	if(indexisvalue)
	{
		IU lenuntilcol;
		if(colrank == 0)  lenuntilcol = lenuntil;
		MPI_Bcast(&lenuntilcol, 1, MPIType<IU>(), 0, ColWorld);
		for(int i=0; i< accnz; ++i)	// fill numerical values from indices
		{
			numacc[i] = indacc[i] + lenuntilcol;
		}
	}
	else
	{
		MPI_Allgatherv(trxnums, trxlocnz, MPIType<NV>(), numacc, colnz, dpls, MPIType<NV>(), ColWorld);
		delete [] trxnums;
	}	
#ifdef TIMING
	double t1=MPI_Wtime();
	cblas_allgathertime += (t1-t0);
#endif
	DeleteAll(colnz,dpls);
}	



/**
  * Step 3 of the sparse SpMV algorithm, with the semiring 
  * @param[in,out] optbuf {scratch space for all-to-all (fold) communication}
  * @param[in,out] indacc, numacc {index and values of the input vector, deleted upon exit}
  * @param[in,out] sendindbuf, sendnumbuf {index and values of the output vector, created}
 **/
template<typename SR, typename IVT, typename OVT, typename IU, typename NUM, typename UDER>
void LocalSpMV(const SpParMat<IU,NUM,UDER> & A, int rowneighs, OptBuf<int32_t, OVT > & optbuf, int32_t * & indacc, IVT * & numacc, 
			   int32_t * & sendindbuf, OVT * & sendnumbuf, int * & sdispls, int * sendcnt, int accnz, bool indexisvalue)
{	
	if(optbuf.totmax > 0)	// graph500 optimization enabled
	{ 
		if(A.spSeq->getnsplit() > 0)
		{
			// optbuf.{inds/nums/dspls} and sendcnt are all pre-allocated and only filled by dcsc_gespmv_threaded
			dcsc_gespmv_threaded_setbuffers<SR> (*(A.spSeq), indacc, numacc, accnz, optbuf.inds, optbuf.nums, sendcnt, optbuf.dspls, rowneighs);	
		}
		else
		{
			dcsc_gespmv<SR> (*(A.spSeq), indacc, numacc, accnz, optbuf.inds, optbuf.nums, sendcnt, optbuf.dspls, rowneighs, indexisvalue);
		}
		DeleteAll(indacc,numacc);
	}
	else
	{
		if(A.spSeq->getnsplit() > 0)
		{
			// sendindbuf/sendnumbuf/sdispls are all allocated and filled by dcsc_gespmv_threaded
			int totalsent = dcsc_gespmv_threaded<SR> (*(A.spSeq), indacc, numacc, accnz, sendindbuf, sendnumbuf, sdispls, rowneighs);	
			
			DeleteAll(indacc, numacc);
			for(int i=0; i<rowneighs-1; ++i)
				sendcnt[i] = sdispls[i+1] - sdispls[i];
			sendcnt[rowneighs-1] = totalsent - sdispls[rowneighs-1];
		}
		else
		{
			// serial SpMV with sparse vector
			vector< int32_t > indy;
			vector< OVT >  numy;
			
			dcsc_gespmv<SR>(*(A.spSeq), indacc, numacc, accnz, indy, numy);	// actual multiplication
			DeleteAll(indacc, numacc);
			
			int32_t bufsize = indy.size();	// as compact as possible
			sendindbuf = new int32_t[bufsize];	
			sendnumbuf = new OVT[bufsize];
			int32_t perproc = A.getlocalrows() / rowneighs;	
			
			int k = 0;	// index to buffer
			for(int i=0; i<rowneighs; ++i)		
			{
				int32_t end_this = (i==rowneighs-1) ? A.getlocalrows(): (i+1)*perproc;
				while(k < bufsize && indy[k] < end_this) 
				{
					sendindbuf[k] = indy[k] - i*perproc;
					sendnumbuf[k] = numy[k];
					++sendcnt[i];
					++k; 
				}
			}
			sdispls = new int[rowneighs]();	
			partial_sum(sendcnt, sendcnt+rowneighs-1, sdispls+1); 
		}
	}
}

template <typename SR, typename IU, typename OVT>
void MergeContributions(FullyDistSpVec<IU,OVT> & y, int * & recvcnt, int * & rdispls, int32_t * & recvindbuf, OVT * & recvnumbuf, int rowneighs)
{
	// free memory of y, in case it was aliased
	vector<IU>().swap(y.ind);
	vector<OVT>().swap(y.num);
	
#ifndef HEAPMERGE
	IU ysize = y.MyLocLength();	// my local length is only O(n/p)
	bool * isthere = new bool[ysize];
	vector< pair<IU,OVT> > ts_pairs;	
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
	sort(ts_pairs.begin(), ts_pairs.end());
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
	for(int i=0; i<rowneighs; ++i)
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
	while(hsize > 0)
	{
		sHeap.deleteMin(&key, &locv);
		IU deref = rdispls[locv] + processed[locv];
		if(y.ind.back() == static_cast<IU>(key))	// y.ind is surely not empty
		{
			y.num.back() = SR::add(y.num.back(), recvnumbuf[deref]);
			// ABAB: Benchmark actually allows us to be non-deterministic in terms of parent selection
			// We can just skip this addition operator (if it's a max/min select)
		} 
		else
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

}

/** 
  * This version is the most flexible sparse matrix X sparse vector [Used in KDT]
  * It accepts different types for the matrix (NUM), the input vector (IVT) and the output vector (OVT)
  * without relying on automatic type promotion
  * Input (x) and output (y) vectors can be ALIASED because y is not written until the algorithm is done with x.
  */
template <typename SR, typename IVT, typename OVT, typename IU, typename NUM, typename UDER>
void SpMV (const SpParMat<IU,NUM,UDER> & A, const FullyDistSpVec<IU,IVT> & x, FullyDistSpVec<IU,OVT> & y, 
			bool indexisvalue, OptBuf<int32_t, OVT > & optbuf)
{
	CheckSpMVCompliance(A,x);
	optbuf.MarkEmpty();
	
	MPI_Comm World = x.commGrid->GetWorld();
	MPI_Comm ColWorld = x.commGrid->GetColWorld();
	MPI_Comm RowWorld = x.commGrid->GetRowWorld();
	
	int accnz;
	int32_t trxlocnz;
	IU lenuntil;
	int32_t *trxinds, *indacc;
	IVT *trxnums, *numacc;
	TransposeVector(World, x, trxlocnz, lenuntil, trxinds, trxnums, indexisvalue);
	AllGatherVector(ColWorld, trxlocnz, lenuntil, trxinds, trxnums, indacc, numacc, accnz, indexisvalue);
	
	int rowneighs;
	MPI_Comm_size(RowWorld, &rowneighs);
	int * sendcnt = new int[rowneighs]();	
	int32_t * sendindbuf;	
	OVT * sendnumbuf;
	int * sdispls;
	LocalSpMV<SR>(A, rowneighs, optbuf, indacc, numacc, sendindbuf, sendnumbuf, sdispls, sendcnt, accnz, indexisvalue);	// indacc/numacc deallocated, sendindbuf/sendnumbuf/sdispls allocated
	
	int * rdispls = new int[rowneighs];
	int * recvcnt = new int[rowneighs];
	MPI_Alltoall(sendcnt, 1, MPI_INT, recvcnt, 1, MPI_INT, RowWorld);       // share the request counts
	
	// receive displacements are exact whereas send displacements have slack
	rdispls[0] = 0;
	for(int i=0; i<rowneighs-1; ++i)
	{
		rdispls[i+1] = rdispls[i] + recvcnt[i];
	}
	int totrecv = accumulate(recvcnt,recvcnt+rowneighs,0);	
	int32_t * recvindbuf = new int32_t[totrecv];
	OVT * recvnumbuf = new OVT[totrecv];
	
#ifdef TIMING
	double t2=MPI_Wtime();
#endif
	if(optbuf.totmax > 0 )	// graph500 optimization enabled
	{
		MPI_Alltoallv(optbuf.inds, sendcnt, optbuf.dspls, MPIType<int32_t>(), recvindbuf, recvcnt, rdispls, MPIType<int32_t>(), RowWorld);
		MPI_Alltoallv(optbuf.nums, sendcnt, optbuf.dspls, MPIType<OVT>(), recvnumbuf, recvcnt, rdispls, MPIType<OVT>(), RowWorld);

		delete [] sendcnt;
	}
	else
	{
		 /*		ofstream oput;
		 x.commGrid->OpenDebugFile("Send", oput);
		 oput << "To displacements: "; copy(sdispls, sdispls+rowneighs, ostream_iterator<int>(oput, " ")); oput << endl;
		 oput << "To counts: "; copy(sendcnt, sendcnt+rowneighs, ostream_iterator<int>(oput, " ")); oput << endl;
		 for(int i=0; i< rowneighs; ++i)
		 {
		 oput << "To neighbor: " << i << endl; 
		 copy(sendindbuf+sdispls[i], sendindbuf+sdispls[i]+sendcnt[i], ostream_iterator<int32_t>(oput, " ")); oput << endl;
		 copy(sendnumbuf+sdispls[i], sendnumbuf+sdispls[i]+sendcnt[i], ostream_iterator<OVT>(oput, " ")); oput << endl;
		 }
		 oput.close(); */
		 
		MPI_Alltoallv(sendindbuf, sendcnt, sdispls, MPIType<int32_t>(), recvindbuf, recvcnt, rdispls, MPIType<int32_t>(), RowWorld);
		MPI_Alltoallv(sendnumbuf, sendcnt, sdispls, MPIType<OVT>(), recvnumbuf, recvcnt, rdispls, MPIType<OVT>(), RowWorld);

		DeleteAll(sendindbuf, sendnumbuf);
		DeleteAll(sendcnt, sdispls);
	}
#ifdef TIMING
	double t3=MPI_Wtime();
	cblas_alltoalltime += (t3-t2);
#endif
	
	//	ofstream output;
	//	A.commGrid->OpenDebugFile("Recv", output);
	//	copy(recvindbuf, recvindbuf+totrecv, ostream_iterator<IU>(output," ")); output << endl;
	//	output.close();
	
	MergeContributions<SR>(y,recvcnt, rdispls, recvindbuf, recvnumbuf, rowneighs);
}


template <typename SR, typename IVT, typename OVT, typename IU, typename NUM, typename UDER>
void SpMV (const SpParMat<IU,NUM,UDER> & A, const FullyDistSpVec<IU,IVT> & x, FullyDistSpVec<IU,OVT> & y, bool indexisvalue)
{
	OptBuf< int32_t, OVT > optbuf = OptBuf< int32_t,OVT >(); 
	SpMV<SR>(A, x, y, indexisvalue, optbuf);
}


/**
 * Automatic type promotion is ONLY done here, all the callee functions (in Friends.h and below) are initialized with the promoted type 
 * If indexisvalues = true, then we do not need to transfer values for x (happens for BFS iterations with boolean matrices and integer rhs vectors)
 **/
template <typename SR, typename IU, typename NUM, typename UDER>
FullyDistSpVec<IU,typename promote_trait<NUM,IU>::T_promote>  SpMV 
(const SpParMat<IU,NUM,UDER> & A, const FullyDistSpVec<IU,IU> & x, bool indexisvalue, OptBuf<int32_t, typename promote_trait<NUM,IU>::T_promote > & optbuf)
{		
	typedef typename promote_trait<NUM,IU>::T_promote T_promote;
	FullyDistSpVec<IU, T_promote> y ( x.getcommgrid(), A.getnrow());	// identity doesn't matter for sparse vectors
	SpMV<SR>(A, x, y, indexisvalue, optbuf);
	return y;
}

/**
 * Parallel dense SpMV
 **/ 
template <typename SR, typename IU, typename NUM, typename NUV, typename UDER> 
FullyDistVec<IU,typename promote_trait<NUM,NUV>::T_promote>  SpMV 
	(const SpParMat<IU,NUM,UDER> & A, const FullyDistVec<IU,NUV> & x )
{
	typedef typename promote_trait<NUM,NUV>::T_promote T_promote;
	CheckSpMVCompliance(A, x);

	MPI_Comm World = x.commGrid->GetWorld();
	MPI_Comm ColWorld = x.commGrid->GetColWorld();
	MPI_Comm RowWorld = x.commGrid->GetRowWorld();

	int xsize = (int) x.LocArrSize();
	int trxsize = 0;

	int diagneigh = x.commGrid->GetComplementRank();
	MPI_Status status;
	MPI_Sendrecv(&xsize, 1, MPI_INT, diagneigh, TRX, &trxsize, 1, MPI_INT, diagneigh, TRX, World, &status);
	
	NUV * trxnums = new NUV[trxsize];
	MPI_Sendrecv(const_cast<NUV*>(SpHelper::p2a(x.arr)), xsize, MPIType<NUV>(), diagneigh, TRX, trxnums, trxsize, MPIType<NUV>(), diagneigh, TRX, World, &status);

        int colneighs, colrank;
	MPI_Comm_size(ColWorld, &colneighs);
	MPI_Comm_rank(ColWorld, &colrank);
	int * colsize = new int[colneighs];
	colsize[colrank] = trxsize;
	MPI_Allgather(MPI_IN_PLACE, 1, MPI_INT, colsize, 1, MPI_INT, ColWorld);
	int * dpls = new int[colneighs]();	// displacements (zero initialized pid) 
	std::partial_sum(colsize, colsize+colneighs-1, dpls+1);
	int accsize = std::accumulate(colsize, colsize+colneighs, 0);
	NUV * numacc = new NUV[accsize];

	MPI_Allgatherv(trxnums, trxsize, MPIType<NUV>(), numacc, colsize, dpls, MPIType<NUV>(), ColWorld);
	delete [] trxnums;

	// serial SpMV with dense vector
	T_promote id = SR::id();
	IU ysize = A.getlocalrows();
	T_promote * localy = new T_promote[ysize];
	fill_n(localy, ysize, id);		
	dcsc_gespmv<SR>(*(A.spSeq), numacc, localy);	
	
	DeleteAll(numacc,colsize, dpls);

	// FullyDistVec<IT,NT>(shared_ptr<CommGrid> grid, IT globallen, NT initval, NT id)
	FullyDistVec<IU, T_promote> y ( x.commGrid, A.getnrow(), id);
	
	int rowneighs;
	MPI_Comm_size(RowWorld, &rowneighs);

	IU begptr, endptr;
	for(int i=0; i< rowneighs; ++i)
	{
		begptr = y.RowLenUntil(i);
		if(i == rowneighs-1)
		{
			endptr = ysize;
		}
		else
		{
			endptr = y.RowLenUntil(i+1);
		}
		MPI_Reduce(localy+begptr, SpHelper::p2a(y.arr), endptr-begptr, MPIType<T_promote>(), SR::mpi_op(), i, RowWorld);
	}
	delete [] localy;
	return y;
}

	
/**
 * Old version that is no longer considered optimal
 * Kept for legacy purposes
 * To be removed when other functionals are fully tested.
 **/ 
template <typename SR, typename IU, typename NUM, typename NUV, typename UDER> 
FullyDistSpVec<IU,typename promote_trait<NUM,NUV>::T_promote>  SpMV 
	(const SpParMat<IU,NUM,UDER> & A, const FullyDistSpVec<IU,NUV> & x)
{
	typedef typename promote_trait<NUM,NUV>::T_promote T_promote;
	CheckSpMVCompliance(A, x);

	MPI_Comm World = x.commGrid->GetWorld();
	MPI_Comm ColWorld = x.commGrid->GetColWorld();
	MPI_Comm RowWorld = x.commGrid->GetRowWorld();

	int xlocnz = (int) x.getlocnnz();
	int trxlocnz = 0;
	int roffst = x.RowLenUntil();
	int offset;

	int diagneigh = x.commGrid->GetComplementRank();
	MPI_Status status;
	MPI_Sendrecv(&xlocnz, 1, MPI_INT, diagneigh, TRX, &trxlocnz, 1, MPI_INT, diagneigh, TRX, World, &status);
	MPI_Sendrecv(&roffst, 1, MPI_INT, diagneigh, TROST, &offset, 1, MPI_INT, diagneigh, TROST, World, &status);
	
	IU * trxinds = new IU[trxlocnz];
	NUV * trxnums = new NUV[trxlocnz];
	MPI_Sendrecv(const_cast<IU*>(SpHelper::p2a(x.ind)), xlocnz, MPIType<IU>(), diagneigh, TRX, trxinds, trxlocnz, MPIType<IU>(), diagneigh, TRX, World, &status);
	MPI_Sendrecv(const_cast<NUV*>(SpHelper::p2a(x.num)), xlocnz, MPIType<NUV>(), diagneigh, TRX, trxnums, trxlocnz, MPIType<NUV>(), diagneigh, TRX, World, &status);
	transform(trxinds, trxinds+trxlocnz, trxinds, bind2nd(plus<IU>(), offset)); // fullydist indexing (n pieces) -> matrix indexing (sqrt(p) pieces)

        int colneighs, colrank;
	MPI_Comm_size(ColWorld, &colneighs);
	MPI_Comm_rank(ColWorld, &colrank);
	int * colnz = new int[colneighs];
	colnz[colrank] = trxlocnz;
	MPI_Allgather(MPI_IN_PLACE, 1, MPI_INT, colnz, 1, MPI_INT, ColWorld);
	int * dpls = new int[colneighs]();	// displacements (zero initialized pid) 
	std::partial_sum(colnz, colnz+colneighs-1, dpls+1);
	int accnz = std::accumulate(colnz, colnz+colneighs, 0);
	IU * indacc = new IU[accnz];
	NUV * numacc = new NUV[accnz];

	// ABAB: Future issues here, colnz is of type int (MPI limitation)
	// What if the aggregate vector size along the processor row/column is not 32-bit addressible?
	MPI_Allgatherv(trxinds, trxlocnz, MPIType<IU>(), indacc, colnz, dpls, MPIType<IU>(), ColWorld);
	MPI_Allgatherv(trxnums, trxlocnz, MPIType<NUV>(), numacc, colnz, dpls, MPIType<NUV>(), ColWorld);
	DeleteAll(trxinds, trxnums);

	// serial SpMV with sparse vector
	vector< int32_t > indy;
	vector< T_promote >  numy;
	
        int32_t * tmpindacc = new int32_t[accnz];
        for(int i=0; i< accnz; ++i) tmpindacc[i] = indacc[i];
	delete [] indacc;

	dcsc_gespmv<SR>(*(A.spSeq), tmpindacc, numacc, accnz, indy, numy);	// actual multiplication

	DeleteAll(tmpindacc, numacc);
	DeleteAll(colnz, dpls);

	FullyDistSpVec<IU, T_promote> y ( x.commGrid, A.getnrow());	// identity doesn't matter for sparse vectors
	IU yintlen = y.MyRowLength();

	int rowneighs;
	MPI_Comm_size(RowWorld,&rowneighs);
	vector< vector<IU> > sendind(rowneighs);
	vector< vector<T_promote> > sendnum(rowneighs);
	typename vector<int32_t>::size_type outnz = indy.size();
	for(typename vector<IU>::size_type i=0; i< outnz; ++i)
	{
		IU locind;
		int rown = y.OwnerWithinRow(yintlen, static_cast<IU>(indy[i]), locind);
		sendind[rown].push_back(locind);
		sendnum[rown].push_back(numy[i]);
	}

	IU * sendindbuf = new IU[outnz];
	T_promote * sendnumbuf = new T_promote[outnz];
	int * sendcnt = new int[rowneighs];
	int * sdispls = new int[rowneighs];
	for(int i=0; i<rowneighs; ++i)
		sendcnt[i] = sendind[i].size();

	int * rdispls = new int[rowneighs];
	int * recvcnt = new int[rowneighs];
	MPI_Alltoall(sendcnt, 1, MPI_INT, recvcnt, 1, MPI_INT, RowWorld);       // share the request counts

	sdispls[0] = 0;
	rdispls[0] = 0;
	for(int i=0; i<rowneighs-1; ++i)
	{
		sdispls[i+1] = sdispls[i] + sendcnt[i];
		rdispls[i+1] = rdispls[i] + recvcnt[i];
	}
	int totrecv = accumulate(recvcnt,recvcnt+rowneighs,0);
	IU * recvindbuf = new IU[totrecv];
	T_promote * recvnumbuf = new T_promote[totrecv];

	for(int i=0; i<rowneighs; ++i)
	{
		copy(sendind[i].begin(), sendind[i].end(), sendindbuf+sdispls[i]);
		vector<IU>().swap(sendind[i]);
	}
	for(int i=0; i<rowneighs; ++i)
	{
		copy(sendnum[i].begin(), sendnum[i].end(), sendnumbuf+sdispls[i]);
		vector<T_promote>().swap(sendnum[i]);
	}
	MPI_Alltoallv(sendindbuf, sendcnt, sdispls, MPIType<IU>(), recvindbuf, recvcnt, rdispls, MPIType<IU>(), RowWorld);
	MPI_Alltoallv(sendnumbuf, sendcnt, sdispls, MPIType<T_promote>(), recvnumbuf, recvcnt, rdispls, MPIType<T_promote>(), RowWorld);
	
	DeleteAll(sendindbuf, sendnumbuf);
	DeleteAll(sendcnt, recvcnt, sdispls, rdispls);
		
	// define a SPA-like data structure
	IU ysize = y.MyLocLength();
	T_promote * localy = new T_promote[ysize];
	bool * isthere = new bool[ysize];
	vector<IU> nzinds;	// nonzero indices		
	fill_n(isthere, ysize, false);
	
	for(int i=0; i< totrecv; ++i)
	{
		if(!isthere[recvindbuf[i]])
		{
			localy[recvindbuf[i]] = recvnumbuf[i];	// initial assignment
			nzinds.push_back(recvindbuf[i]);
			isthere[recvindbuf[i]] = true;
		} 
		else
		{
			localy[recvindbuf[i]] = SR::add(localy[recvindbuf[i]], recvnumbuf[i]);	
		}
	}
	DeleteAll(isthere, recvindbuf, recvnumbuf);
	sort(nzinds.begin(), nzinds.end());
	int nnzy = nzinds.size();
	y.ind.resize(nnzy);
	y.num.resize(nnzy);
	for(int i=0; i< nnzy; ++i)
	{
		y.ind[i] = nzinds[i];
		y.num[i] = localy[nzinds[i]]; 	
	}
	delete [] localy;
	return y;
}
	

template <typename IU, typename NU1, typename NU2, typename UDERA, typename UDERB> 
SpParMat<IU,typename promote_trait<NU1,NU2>::T_promote,typename promote_trait<UDERA,UDERB>::T_promote> EWiseMult 
	(const SpParMat<IU,NU1,UDERA> & A, const SpParMat<IU,NU2,UDERB> & B , bool exclude)
{
	typedef typename promote_trait<NU1,NU2>::T_promote N_promote;
	typedef typename promote_trait<UDERA,UDERB>::T_promote DER_promote;

	if(*(A.commGrid) == *(B.commGrid))	
	{
		DER_promote * result = new DER_promote( EWiseMult(*(A.spSeq),*(B.spSeq),exclude) );
		return SpParMat<IU, N_promote, DER_promote> (result, A.commGrid);
	}
	else
	{
		cout << "Grids are not comparable elementwise multiplication" << endl; 
		MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
		return SpParMat< IU,N_promote,DER_promote >();
	}
}
	
template <typename RETT, typename RETDER, typename IU, typename NU1, typename NU2, typename UDERA, typename UDERB, typename _BinaryOperation> 
SpParMat<IU,RETT,RETDER> EWiseApply 
	(const SpParMat<IU,NU1,UDERA> & A, const SpParMat<IU,NU2,UDERB> & B, _BinaryOperation __binary_op, bool notB, const NU2& defaultBVal)
{
	if(*(A.commGrid) == *(B.commGrid))	
	{
		RETDER * result = new RETDER( EWiseApply<RETT>(*(A.spSeq),*(B.spSeq), __binary_op, notB, defaultBVal) );
		return SpParMat<IU, RETT, RETDER> (result, A.commGrid);
	}
	else
	{
		cout << "Grids are not comparable elementwise apply" << endl; 
		MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
		return SpParMat< IU,RETT,RETDER >();
	}
}

template <typename RETT, typename RETDER, typename IU, typename NU1, typename NU2, typename UDERA, typename UDERB, typename _BinaryOperation, typename _BinaryPredicate> 
SpParMat<IU,RETT,RETDER> EWiseApply
	(const SpParMat<IU,NU1,UDERA> & A, const SpParMat<IU,NU2,UDERB> & B, _BinaryOperation __binary_op, _BinaryPredicate do_op, bool allowANulls, bool allowBNulls, const NU1& ANullVal, const NU2& BNullVal, const bool allowIntersect, const bool useExtendedBinOp)
{
	if(*(A.commGrid) == *(B.commGrid))	
	{
		RETDER * result = new RETDER( EWiseApply<RETT>(*(A.spSeq),*(B.spSeq), __binary_op, do_op, allowANulls, allowBNulls, ANullVal, BNullVal, allowIntersect) );
		return SpParMat<IU, RETT, RETDER> (result, A.commGrid);
	}
	else
	{
		cout << "Grids are not comparable elementwise apply" << endl; 
		MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
		return SpParMat< IU,RETT,RETDER >();
	}
}

// plain adapter
template <typename RETT, typename RETDER, typename IU, typename NU1, typename NU2, typename UDERA, typename UDERB, typename _BinaryOperation, typename _BinaryPredicate> 
SpParMat<IU,RETT,RETDER>
EWiseApply (const SpParMat<IU,NU1,UDERA> & A, const SpParMat<IU,NU2,UDERB> & B, _BinaryOperation __binary_op, _BinaryPredicate do_op, bool allowANulls, bool allowBNulls, const NU1& ANullVal, const NU2& BNullVal, const bool allowIntersect = true)
{
	return EWiseApply<RETT, RETDER>(A, B,
				EWiseExtToPlainAdapter<RETT, NU1, NU2, _BinaryOperation>(__binary_op),
				EWiseExtToPlainAdapter<bool, NU1, NU2, _BinaryPredicate>(do_op),
				allowANulls, allowBNulls, ANullVal, BNullVal, allowIntersect, true);
}
// end adapter


/**
 * if exclude is true, then we prune all entries W[i] != zero from V 
 * if exclude is false, then we perform a proper elementwise multiplication
**/
template <typename IU, typename NU1, typename NU2>
SpParVec<IU,typename promote_trait<NU1,NU2>::T_promote> EWiseMult 
	(const SpParVec<IU,NU1> & V, const DenseParVec<IU,NU2> & W , bool exclude, NU2 zero)
{
	typedef typename promote_trait<NU1,NU2>::T_promote T_promote;

	if(*(V.commGrid) == *(W.commGrid))	
	{
		SpParVec< IU, T_promote> Product(V.commGrid);
		Product.length = V.length;
		if(Product.diagonal)
		{
			if(exclude)
			{
				IU size= V.ind.size();
				for(IU i=0; i<size; ++i)
				{
					if(W.arr.size() <= V.ind[i] || W.arr[V.ind[i]] == zero) 	// keep only those
					{
						Product.ind.push_back(V.ind[i]);
						Product.num.push_back(V.num[i]);
					}
				}		
			}	
			else
			{
				IU size= V.ind.size();
				for(IU i=0; i<size; ++i)
				{
					if(W.arr.size() > V.ind[i] && W.arr[V.ind[i]] != zero) 	// keep only those
					{
						Product.ind.push_back(V.ind[i]);
						Product.num.push_back(V.num[i] * W.arr[V.ind[i]]);
					}
				}
			}
		}
		return Product;
	}
	else
	{
		cout << "Grids are not comparable elementwise multiplication" << endl; 
		MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
		return SpParVec< IU,T_promote>();
	}
}

/**
 * if exclude is true, then we prune all entries W[i] != zero from V 
 * if exclude is false, then we perform a proper elementwise multiplication
**/
template <typename IU, typename NU1, typename NU2>
FullyDistSpVec<IU,typename promote_trait<NU1,NU2>::T_promote> EWiseMult 
	(const FullyDistSpVec<IU,NU1> & V, const FullyDistVec<IU,NU2> & W , bool exclude, NU2 zero)
{
	typedef typename promote_trait<NU1,NU2>::T_promote T_promote;

	if(*(V.commGrid) == *(W.commGrid))	
	{
		FullyDistSpVec< IU, T_promote> Product(V.commGrid);
		if(V.glen != W.glen)
		{
			cerr << "Vector dimensions don't match for EWiseMult\n";
			MPI_Abort(MPI_COMM_WORLD, DIMMISMATCH);
		}
		else
		{
			Product.glen = V.glen;
			IU size= V.getlocnnz();
			if(exclude)
			{
				#if defined(_OPENMP) && defined(CBLAS_EXPERIMENTAL)	// not faster than serial
				int actual_splits = cblas_splits * 1;	// 1 is the parallel slackness
				vector <IU> tlosizes (actual_splits, 0);
				vector < vector<IU> > tlinds(actual_splits);
				vector < vector<T_promote> > tlnums(actual_splits);
				IU tlsize = size / actual_splits;
				#pragma omp parallel for //schedule(dynamic, 1)
				for(IU t = 0; t < actual_splits; ++t)
				{
					IU tlbegin = t*tlsize;
					IU tlend = (t==actual_splits-1)? size : (t+1)*tlsize;
					for(IU i=tlbegin; i<tlend; ++i)
					{
						if(W.arr[V.ind[i]] == zero) 	// keep only those
						{
							tlinds[t].push_back(V.ind[i]);
							tlnums[t].push_back(V.num[i]);
							tlosizes[t]++;
						}
					}
				}
				vector<IU> prefix_sum(actual_splits+1,0);
				partial_sum(tlosizes.begin(), tlosizes.end(), prefix_sum.begin()+1); 
				Product.ind.resize(prefix_sum[actual_splits]);
				Product.num.resize(prefix_sum[actual_splits]);
			
				#pragma omp parallel for //schedule(dynamic, 1)
				for(IU t=0; t< actual_splits; ++t)
				{
					copy(tlinds[t].begin(), tlinds[t].end(), Product.ind.begin()+prefix_sum[t]);
					copy(tlnums[t].begin(), tlnums[t].end(), Product.num.begin()+prefix_sum[t]);
				}
				#else
				for(IU i=0; i<size; ++i)
				{
					if(W.arr[V.ind[i]] == zero)     // keep only those
					{
                        	       		Product.ind.push_back(V.ind[i]);
                                		Product.num.push_back(V.num[i]);
                                      	}	
				}
				#endif
			}
			else
			{
				for(IU i=0; i<size; ++i)
				{
					if(W.arr[V.ind[i]] != zero) 	// keep only those
					{
						Product.ind.push_back(V.ind[i]);
						Product.num.push_back(V.num[i] * W.arr[V.ind[i]]);
					}
				}
			}
		}
		return Product;
	}
	else
	{
		cout << "Grids are not comparable elementwise multiplication" << endl; 
		MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
		return FullyDistSpVec< IU,T_promote>();
	}
}


/**
 * Performs an arbitrary binary operation _binary_op on the corresponding elements of two vectors with the result stored in a return vector ret. 
 * The binary operatiation is only performed if the binary predicate _doOp returns true for those elements. Otherwise the binary operation is not 
 * performed and ret does not contain an element at that position.
 * More formally the operation is defined as:
 * if (_doOp(V[i], W[i]))
 *    ret[i] = _binary_op(V[i], W[i])
 * else
 *    // ret[i] is not set
 * Hence _doOp can be used to implement a filter on either of the vectors.
 *
 * The above is only defined if both V[i] and W[i] exist (i.e. an intersection). To allow a union operation (ex. when V[i] doesn't exist but W[i] does) 
 * the allowVNulls flag is set to true and the Vzero argument is used as the missing V[i] value.
 *
 * The type of each element of ret must not necessarily be related to the types of V or W, so the return type must be explicitly specified as a template parameter:
 * FullyDistSpVec<int, double> r = EWiseApply<double>(V, W, plus, retTrue, false, 0)
**/
template <typename RET, typename IU, typename NU1, typename NU2, typename _BinaryOperation, typename _BinaryPredicate>
FullyDistSpVec<IU,RET> EWiseApply 
	(const FullyDistSpVec<IU,NU1> & V, const FullyDistVec<IU,NU2> & W , _BinaryOperation _binary_op, _BinaryPredicate _doOp, bool allowVNulls, NU1 Vzero, const bool useExtendedBinOp)
{
	typedef RET T_promote; //typedef typename promote_trait<NU1,NU2>::T_promote T_promote;
	if(*(V.commGrid) == *(W.commGrid))	
	{
		FullyDistSpVec< IU, T_promote> Product(V.commGrid);
		FullyDistVec< IU, NU1> DV (V);
		if(V.TotalLength() != W.TotalLength())
		{
			ostringstream outs;
			outs << "Vector dimensions don't match (" << V.TotalLength() << " vs " << W.TotalLength() << ") for EWiseApply (short version)\n";
			SpParHelper::Print(outs.str());
			MPI_Abort(MPI_COMM_WORLD, DIMMISMATCH);
		}
		else
		{
			Product.glen = V.glen;
			IU size= W.LocArrSize();
			IU spsize = V.getlocnnz();
			IU sp_iter = 0;
			if (allowVNulls)
			{
				// iterate over the dense vector
				for(IU i=0; i<size; ++i)
				{
					if(sp_iter < spsize && V.ind[sp_iter] == i)
					{
						if (_doOp(V.num[sp_iter], W.arr[i], false, false))
						{
							Product.ind.push_back(i);
							Product.num.push_back(_binary_op(V.num[sp_iter], W.arr[i], false, false));
						}
						sp_iter++;
					}
					else
					{
						if (_doOp(Vzero, W.arr[i], true, false))
						{
							Product.ind.push_back(i);
							Product.num.push_back(_binary_op(Vzero, W.arr[i], true, false));
						}
					}
				}
			}
			else
			{
				// iterate over the sparse vector
				for(sp_iter = 0; sp_iter < spsize; ++sp_iter)
				{
					if (_doOp(V.num[sp_iter], W.arr[V.ind[sp_iter]], false, false))
					{
						Product.ind.push_back(V.ind[sp_iter]);
						Product.num.push_back(_binary_op(V.num[sp_iter], W.arr[V.ind[sp_iter]], false, false));
					}
				}
			}
		}
		return Product;
	}
	else
	{
		cout << "Grids are not comparable for EWiseApply" << endl; 
		MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
		return FullyDistSpVec< IU,T_promote>();
	}
}

/**
 * Performs an arbitrary binary operation _binary_op on the corresponding elements of two vectors with the result stored in a return vector ret. 
 * The binary operatiation is only performed if the binary predicate _doOp returns true for those elements. Otherwise the binary operation is not 
 * performed and ret does not contain an element at that position.
 * More formally the operation is defined as:
 * if (_doOp(V[i], W[i]))
 *    ret[i] = _binary_op(V[i], W[i])
 * else
 *    // ret[i] is not set
 * Hence _doOp can be used to implement a filter on either of the vectors.
 *
 * The above is only defined if both V[i] and W[i] exist (i.e. an intersection). To allow a union operation (ex. when V[i] doesn't exist but W[i] does) 
 * the allowVNulls flag is set to true and the Vzero argument is used as the missing V[i] value.
 * !allowVNulls && !allowWNulls => intersection
 * !allowVNulls &&  allowWNulls => operate on all elements of V 
 *  allowVNulls && !allowWNulls => operate on all elements of W
 *  allowVNulls &&  allowWNulls => union
 *
 * The type of each element of ret must not necessarily be related to the types of V or W, so the return type must be explicitly specified as a template parameter:
 * FullyDistSpVec<int, double> r = EWiseApply<double>(V, W, plus, retTrue, false, 0, false, 0)
**/
template <typename RET, typename IU, typename NU1, typename NU2, typename _BinaryOperation, typename _BinaryPredicate>
FullyDistSpVec<IU,RET> EWiseApply 
	(const FullyDistSpVec<IU,NU1> & V, const FullyDistSpVec<IU,NU2> & W , _BinaryOperation _binary_op, _BinaryPredicate _doOp, bool allowVNulls, bool allowWNulls, NU1 Vzero, NU2 Wzero, const bool allowIntersect, const bool useExtendedBinOp)
{
	typedef RET T_promote; // typename promote_trait<NU1,NU2>::T_promote T_promote;
	if(*(V.commGrid) == *(W.commGrid))	
	{
		FullyDistSpVec< IU, T_promote> Product(V.commGrid);
		if(V.glen != W.glen)
		{
			ostringstream outs;
			outs << "Vector dimensions don't match (" << V.glen << " vs " << W.glen << ") for EWiseApply (full version)\n";
			SpParHelper::Print(outs.str());
			MPI_Abort(MPI_COMM_WORLD, DIMMISMATCH);
		}
		else
		{
			Product.glen = V.glen;
			typename vector< IU  >::const_iterator indV = V.ind.begin();
			typename vector< NU1 >::const_iterator numV = V.num.begin();
			typename vector< IU  >::const_iterator indW = W.ind.begin();
			typename vector< NU2 >::const_iterator numW = W.num.begin();
			
			while (indV < V.ind.end() && indW < W.ind.end())
			{
				if (*indV == *indW)
				{
					// overlap
					if (allowIntersect)
					{
						if (_doOp(*numV, *numW, false, false))
						{
							Product.ind.push_back(*indV);
							Product.num.push_back(_binary_op(*numV, *numW, false, false));
						}
					}
					indV++; numV++;
					indW++; numW++;
				}
				else if (*indV < *indW)
				{
					// V has value but W does not
					if (allowWNulls)
					{
						if (_doOp(*numV, Wzero, false, true))
						{
							Product.ind.push_back(*indV);
							Product.num.push_back(_binary_op(*numV, Wzero, false, true));
						}
					}
					indV++; numV++;
				}
				else //(*indV > *indW)
				{
					// W has value but V does not
					if (allowVNulls)
					{
						if (_doOp(Vzero, *numW, true, false))
						{
							Product.ind.push_back(*indW);
							Product.num.push_back(_binary_op(Vzero, *numW, true, false));
						}
					}
					indW++; numW++;
				}
			}
			// clean up
			while (allowWNulls && indV < V.ind.end())
			{
				if (_doOp(*numV, Wzero, false, true))
				{
					Product.ind.push_back(*indV);
					Product.num.push_back(_binary_op(*numV, Wzero, false, true));
				}
				indV++; numV++;
			}
			while (allowVNulls && indW < W.ind.end())
			{
				if (_doOp(Vzero, *numW, true, false))
				{
					Product.ind.push_back(*indW);
					Product.num.push_back(_binary_op(Vzero, *numW, true, false));
				}
				indW++; numW++;
			}
		}
		return Product;
	}
	else
	{
		cout << "Grids are not comparable for EWiseApply" << endl; 
		MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
		return FullyDistSpVec< IU,T_promote>();
	}
}

// plain callback versions
template <typename RET, typename IU, typename NU1, typename NU2, typename _BinaryOperation, typename _BinaryPredicate>
FullyDistSpVec<IU,RET> EWiseApply 
	(const FullyDistSpVec<IU,NU1> & V, const FullyDistVec<IU,NU2> & W , _BinaryOperation _binary_op, _BinaryPredicate _doOp, bool allowVNulls, NU1 Vzero)
{
	return EWiseApply<RET>(V, W,
					EWiseExtToPlainAdapter<RET, NU1, NU2, _BinaryOperation>(_binary_op),
					EWiseExtToPlainAdapter<bool, NU1, NU2, _BinaryPredicate>(_doOp),
					allowVNulls, Vzero, true);
}

template <typename RET, typename IU, typename NU1, typename NU2, typename _BinaryOperation, typename _BinaryPredicate>
FullyDistSpVec<IU,RET> EWiseApply 
	(const FullyDistSpVec<IU,NU1> & V, const FullyDistSpVec<IU,NU2> & W , _BinaryOperation _binary_op, _BinaryPredicate _doOp, bool allowVNulls, bool allowWNulls, NU1 Vzero, NU2 Wzero, const bool allowIntersect = true)
{
	return EWiseApply<RET>(V, W,
					EWiseExtToPlainAdapter<RET, NU1, NU2, _BinaryOperation>(_binary_op),
					EWiseExtToPlainAdapter<bool, NU1, NU2, _BinaryPredicate>(_doOp),
					allowVNulls, allowWNulls, Vzero, Wzero, allowIntersect, true);
}



#endif

