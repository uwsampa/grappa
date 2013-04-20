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

#include "SpParMat.h"
#include "ParFriends.h"
#include "Operations.h"
#include "FileHeader.h"

#include <fstream>
#include <algorithm>
#include <stdexcept>
using namespace std;

/**
  * If every processor has a distinct triples file such as {A_0, A_1, A_2,... A_p} for p processors
 **/
template <class IT, class NT, class DER>
SpParMat< IT,NT,DER >::SpParMat (ifstream & input, MPI_Comm & world)
{
	assert( (sizeof(IT) >= sizeof(typename DER::LocalIT)) );
	if(!input.is_open())
	{
		perror("Input file doesn't exist\n");
		exit(-1);
	}
	commGrid.reset(new CommGrid(world, 0, 0));
	input >> (*spSeq);
}

template <class IT, class NT, class DER>
SpParMat< IT,NT,DER >::SpParMat (DER * myseq, MPI_Comm & world): spSeq(myseq)
{
	assert( (sizeof(IT) >= sizeof(typename DER::LocalIT)) );
	commGrid.reset(new CommGrid(world, 0, 0));
}

template <class IT, class NT, class DER>
SpParMat< IT,NT,DER >::SpParMat (DER * myseq, shared_ptr<CommGrid> grid): spSeq(myseq)
{
	assert( (sizeof(IT) >= sizeof(typename DER::LocalIT)) );
	commGrid = grid;
}	

template <class IT, class NT, class DER>
SpParMat< IT,NT,DER >::SpParMat (shared_ptr<CommGrid> grid)
{
	assert( (sizeof(IT) >= sizeof(typename DER::LocalIT)) );
	spSeq = new DER();
	commGrid = grid;
}

/**
  * If there is a single file read by the master process only, use this and then call ReadDistribute()
  * Since this is the default constructor, you don't need to explicitly call it, just a declaration will call it
 **/
template <class IT, class NT, class DER>
SpParMat< IT,NT,DER >::SpParMat ()
{
	
	assert( (sizeof(IT) >= sizeof(typename DER::LocalIT)) );
	spSeq = new DER();
	commGrid.reset(new CommGrid(MPI_COMM_WORLD, 0, 0));
}

template <class IT, class NT, class DER>
SpParMat< IT,NT,DER >::~SpParMat ()
{
	if(spSeq != NULL) delete spSeq;
}

template <class IT, class NT, class DER>
void SpParMat< IT,NT,DER >::FreeMemory ()
{
	if(spSeq != NULL) delete spSeq;
	spSeq = NULL;
}


template <class IT, class NT, class DER>
void SpParMat< IT,NT,DER >::Dump(string filename) const
{
	MPI_Comm World = commGrid->GetWorld();
    	int rank = commGrid->GetRank;
    	int nprocs = commGrid->GetSize();
		
	MPI_File thefile;
	MPI_File_open(World, filename.c_str(), MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &thefile);

	int rankinrow = commGrid->GetRankInProcRow();
	int rankincol = commGrid->GetRankInProcCol();
	int rowneighs = commGrid->GetGridCols();	// get # of processors on the row
	int colneighs = commGrid->GetGridRows();

	IT * colcnts = new IT[rowneighs];
	IT * rowcnts = new IT[colneighs];
	rowcnts[rankincol] = getlocalrows();
	colcnts[rankinrow] = getlocalcols();

	MPI_Allgather(MPI_IN_PLACE, 0, MPIType<IT>(), colcnts, 1, MPIType<IT>(), commGrid->GetRowWorld());
	IT coloffset = accumulate(colcnts, colcnts+rankinrow, static_cast<IT>(0));

	MPI_Allgather(MPI_IN_PLACE, 0, MPIType<IT>(), rowcnts, 1, MPIType<IT>(), commGrid->GetColWorld());	
	IT rowoffset = accumulate(rowcnts, rowcnts+rankincol, static_cast<IT>(0));
	DeleteAll(colcnts, rowcnts);

	IT * prelens = new IT[nprocs];
	prelens[rank] = 2*getlocalnnz();
	MPI_Allgather(MPI_IN_PLACE, 0, MPIType<IT>(), prelens, 1, MPIType<IT>(), commGrid->GetWorld());
	IT lengthuntil = accumulate(prelens, prelens+rank, static_cast<IT>(0));

	// The disp displacement argument specifies the position 
	// (absolute offset in bytes from the beginning of the file) 
	MPI_Offset disp = lengthuntil * sizeof(uint32_t);
	MPI_File_set_view(thefile, disp, MPI_UNSIGNED, MPI_UNSIGNED, "native", MPI_INFO_NULL);
	uint32_t * gen_edges = new uint32_t[prelens[rank]];
	
	IT k = 0;
	for(typename DER::SpColIter colit = spSeq->begcol(); colit != spSeq->endcol(); ++colit)	// iterate over columns
	{
		for(typename DER::SpColIter::NzIter nzit = spSeq->begnz(colit); nzit != spSeq->endnz(colit); ++nzit)
		{
			gen_edges[k++] = (uint32_t) (nzit.rowid() + rowoffset);
			gen_edges[k++] = (uint32_t) (colit.colid() +  coloffset);
		}
	}
	assert(k == prelens[rank]);
	MPI_File_write(thefile, gen_edges, prelens[rank], MPI_UNSIGNED, NULL);	
	MPI_File_close(&thefile);

	delete [] prelens;
	delete [] gen_edges;
}


template <class IT, class NT, class DER>
SpParMat< IT,NT,DER >::SpParMat (const SpParMat< IT,NT,DER > & rhs)
{
	if(rhs.spSeq != NULL)	
		spSeq = new DER(*(rhs.spSeq));  	// Deep copy of local block

	commGrid =  rhs.commGrid;
}

template <class IT, class NT, class DER>
SpParMat< IT,NT,DER > & SpParMat< IT,NT,DER >::operator=(const SpParMat< IT,NT,DER > & rhs)
{
	if(this != &rhs)		
	{
		//! Check agains NULL is probably unneccessary, delete won't fail on NULL
		//! But useful in the presence of a user defined "operator delete" which fails to check NULL
		if(spSeq != NULL) delete spSeq;
		if(rhs.spSeq != NULL)	
			spSeq = new DER(*(rhs.spSeq));  // Deep copy of local block
	
		commGrid = rhs.commGrid;
	}
	return *this;
}

template <class IT, class NT, class DER>
SpParMat< IT,NT,DER > & SpParMat< IT,NT,DER >::operator+=(const SpParMat< IT,NT,DER > & rhs)
{
	if(this != &rhs)		
	{
		if(*commGrid == *rhs.commGrid)	
		{
			(*spSeq) += (*(rhs.spSeq));
		}
		else
		{
			cout << "Grids are not comparable for parallel addition (A+B)" << endl; 
		}
	}
	else
	{
		cout<< "Missing feature (A+A): Use multiply with 2 instead !"<<endl;	
	}
	return *this;	
}

template <class IT, class NT, class DER>
float SpParMat< IT,NT,DER >::LoadImbalance() const
{
	IT totnnz = getnnz();	// collective call
	IT maxnnz = 0;    
	IT localnnz = (IT) spSeq->getnnz();
	MPI_Allreduce( &localnnz, &maxnnz, 1, MPIType<IT>(), MPI_MAX, commGrid->GetWorld());
	if(totnnz == 0) return 1;
 	return static_cast<float>((commGrid->GetSize() * maxnnz)) / static_cast<float>(totnnz);  
}

template <class IT, class NT, class DER>
IT SpParMat< IT,NT,DER >::getnnz() const
{
	IT totalnnz = 0;    
	IT localnnz = spSeq->getnnz();
	MPI_Allreduce( &localnnz, &totalnnz, 1, MPIType<IT>(), MPI_SUM, commGrid->GetWorld());
 	return totalnnz;  
}

template <class IT, class NT, class DER>
IT SpParMat< IT,NT,DER >::getnrow() const
{
	IT totalrows = 0;
	IT localrows = spSeq->getnrow();    
	MPI_Allreduce( &localrows, &totalrows, 1, MPIType<IT>(), MPI_SUM, commGrid->GetColWorld());
 	return totalrows;  
}

template <class IT, class NT, class DER>
IT SpParMat< IT,NT,DER >::getncol() const
{
	IT totalcols = 0;
	IT localcols = spSeq->getncol();    
	MPI_Allreduce( &localcols, &totalcols, 1, MPIType<IT>(), MPI_SUM, commGrid->GetRowWorld());
 	return totalcols;  
}

template <class IT, class NT, class DER>
template <typename _BinaryOperation>	
void SpParMat<IT,NT,DER>::DimApply(Dim dim, const FullyDistVec<IT, NT>& x, _BinaryOperation __binary_op)
{

	if(!(*commGrid == *(x.commGrid))) 		
	{
		cout << "Grids are not comparable for SpParMat::DimApply" << endl; 
		MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
	}

	MPI_Comm World = x.commGrid->GetWorld();
	MPI_Comm ColWorld = x.commGrid->GetColWorld();
	MPI_Comm RowWorld = x.commGrid->GetRowWorld();
	switch(dim)
	{
		case Column:	// scale each column
		{
			int xsize = (int) x.LocArrSize();
			int trxsize = 0;
			int diagneigh = x.commGrid->GetComplementRank();
			MPI_Status status;
			MPI_Sendrecv(&xsize, 1, MPI_INT, diagneigh, TRX, &trxsize, 1, MPI_INT, diagneigh, TRX, World, &status);
	
			NT * trxnums = new NT[trxsize];
			MPI_Sendrecv(const_cast<NT*>(SpHelper::p2a(x.arr)), xsize, MPIType<NT>(), diagneigh, TRX, trxnums, trxsize, MPIType<NT>(), diagneigh, TRX, World, &status);

			int colneighs, colrank;
			MPI_Comm_size(ColWorld, &colneighs);
			MPI_Comm_rank(ColWorld, &colrank);
			int * colsize = new int[colneighs];
			colsize[colrank] = trxsize;
		
			MPI_Allgather(MPI_IN_PLACE, 1, MPI_INT, colsize, 1, MPI_INT, ColWorld);	
			int * dpls = new int[colneighs]();	// displacements (zero initialized pid) 
			std::partial_sum(colsize, colsize+colneighs-1, dpls+1);
			int accsize = std::accumulate(colsize, colsize+colneighs, 0);
			NT * scaler = new NT[accsize];

			MPI_Allgatherv(trxnums, trxsize, MPIType<NT>(), scaler, colsize, dpls, MPIType<NT>(), ColWorld);
			DeleteAll(trxnums,colsize, dpls);

			for(typename DER::SpColIter colit = spSeq->begcol(); colit != spSeq->endcol(); ++colit)	// iterate over columns
			{
				for(typename DER::SpColIter::NzIter nzit = spSeq->begnz(colit); nzit != spSeq->endnz(colit); ++nzit)
				{
					nzit.value() = __binary_op(nzit.value(), scaler[colit.colid()]);
				}
			}
			delete [] scaler;
			break;
		}
		case Row:
		{
			int xsize = (int) x.LocArrSize();
			int rowneighs, rowrank;
			MPI_Comm_size(RowWorld, &rowneighs);
			MPI_Comm_rank(RowWorld, &rowrank);
			int * rowsize = new int[rowneighs];
			rowsize[rowrank] = xsize;
			MPI_Allgather(MPI_IN_PLACE, 1, MPI_INT, rowsize, 1, MPI_INT, RowWorld);
			int * dpls = new int[rowneighs]();	// displacements (zero initialized pid) 
			std::partial_sum(rowsize, rowsize+rowneighs-1, dpls+1);
			int accsize = std::accumulate(rowsize, rowsize+rowneighs, 0);
			NT * scaler = new NT[accsize];

			MPI_Allgatherv(const_cast<NT*>(SpHelper::p2a(x.arr)), xsize, MPIType<NT>(), scaler, rowsize, dpls, MPIType<NT>(), RowWorld);
			DeleteAll(rowsize, dpls);

			for(typename DER::SpColIter colit = spSeq->begcol(); colit != spSeq->endcol(); ++colit)
			{
				for(typename DER::SpColIter::NzIter nzit = spSeq->begnz(colit); nzit != spSeq->endnz(colit); ++nzit)
				{
					nzit.value() = __binary_op(nzit.value(), scaler[nzit.rowid()]);
				}
			}
			delete [] scaler;			
			break;
		}
		default:
		{
			cout << "Unknown scaling dimension, returning..." << endl;
			break;
		}
	}
}

template <class IT, class NT, class DER>
template <typename _BinaryOperation, typename _UnaryOperation >	
DenseParVec<IT,NT> SpParMat<IT,NT,DER>::Reduce(Dim dim, _BinaryOperation __binary_op, NT id, _UnaryOperation __unary_op) const
{
	DenseParVec<IT,NT> parvec(commGrid, id);
	Reduce(parvec, dim, __binary_op, id, __unary_op);			
	return parvec;
}

// default template arguments don't work with function templates
template <class IT, class NT, class DER>
template <typename _BinaryOperation>	
DenseParVec<IT,NT> SpParMat<IT,NT,DER>::Reduce(Dim dim, _BinaryOperation __binary_op, NT id) const
{
	DenseParVec<IT,NT> parvec(commGrid, id);
	Reduce(parvec, dim, __binary_op, id, myidentity<NT>() );			
	return parvec;
}

template <class IT, class NT, class DER>
template <typename VT, typename _BinaryOperation>	
void SpParMat<IT,NT,DER>::Reduce(DenseParVec<IT,VT> & rvec, Dim dim, _BinaryOperation __binary_op, VT id) const
{
	Reduce(rvec, dim, __binary_op, id, myidentity<NT>() );			
}

template <class IT, class NT, class DER>
template <typename VT, typename GIT, typename _BinaryOperation>	
void SpParMat<IT,NT,DER>::Reduce(FullyDistVec<GIT,VT> & rvec, Dim dim, _BinaryOperation __binary_op, VT id) const
{
	Reduce(rvec, dim, __binary_op, id, myidentity<NT>() );				
}

template <class IT, class NT, class DER>
template <typename VT, typename GIT, typename _BinaryOperation, typename _UnaryOperation>	// GIT: global index type of vector	
void SpParMat<IT,NT,DER>::Reduce(FullyDistVec<GIT,VT> & rvec, Dim dim, _BinaryOperation __binary_op, VT id, _UnaryOperation __unary_op) const
{
	if(*rvec.commGrid != *commGrid)
	{
		SpParHelper::Print("Grids are not comparable, SpParMat::Reduce() fails !"); 
		MPI_Abort(MPI_COMM_WORLD,GRIDMISMATCH);
	}
	switch(dim)
	{
		case Column:	// pack along the columns, result is a vector of size n
		{
			// We can't use rvec's distribution (rows first, columns later) here
        		IT n_thiscol = getlocalcols();   // length assigned to this processor column
			int colneighs = commGrid->GetGridRows();	// including oneself
        		int colrank = commGrid->GetRankInProcCol();

			GIT * loclens = new GIT[colneighs];
			GIT * lensums = new GIT[colneighs+1]();	// begin/end points of local lengths

        		GIT n_perproc = n_thiscol / colneighs;    // length on a typical processor
        		if(colrank == colneighs-1)
                		loclens[colrank] = n_thiscol - (n_perproc*colrank);
        		else
                		loclens[colrank] = n_perproc;

			MPI_Allgather(MPI_IN_PLACE, 0, MPIType<GIT>(), loclens, 1, MPIType<GIT>(), commGrid->GetColWorld());
			partial_sum(loclens, loclens+colneighs, lensums+1);	// loclens and lensums are different, but both would fit in 32-bits

			vector<VT> trarr;
			typename DER::SpColIter colit = spSeq->begcol();
			for(int i=0; i< colneighs; ++i)
			{
				VT * sendbuf = new VT[loclens[i]];
				fill(sendbuf, sendbuf+loclens[i], id);	// fill with identity

				for(; colit != spSeq->endcol() && colit.colid() < lensums[i+1]; ++colit)	// iterate over a portion of columns
				{
					for(typename DER::SpColIter::NzIter nzit = spSeq->begnz(colit); nzit != spSeq->endnz(colit); ++nzit)	// all nonzeros in this column
					{
						sendbuf[colit.colid()-lensums[i]] = __binary_op(static_cast<VT>(__unary_op(nzit.value())), sendbuf[colit.colid()-lensums[i]]);
					}
				}
				VT * recvbuf = NULL;
				if(colrank == i)
				{
					trarr.resize(loclens[i]);
					recvbuf = SpHelper::p2a(trarr);	
				}
				MPI_Reduce(sendbuf, recvbuf, loclens[i], MPIType<VT>(), MPIOp<_BinaryOperation, VT>::op(), i, commGrid->GetColWorld()); // root  = i
				delete [] sendbuf;
			}
			DeleteAll(loclens, lensums);

			GIT reallen;	// Now we have to transpose the vector
			GIT trlen = trarr.size();
			int diagneigh = commGrid->GetComplementRank();
			MPI_Status status;
			MPI_Sendrecv(&trlen, 1, MPIType<IT>(), diagneigh, TRNNZ, &reallen, 1, MPIType<IT>(), diagneigh, TRNNZ, commGrid->GetWorld(), &status);
	
			rvec.arr.resize(reallen);
			MPI_Sendrecv(SpHelper::p2a(trarr), trlen, MPIType<VT>(), diagneigh, TRX, SpHelper::p2a(rvec.arr), reallen, MPIType<VT>(), diagneigh, TRX, commGrid->GetWorld(), &status);
			rvec.glen = getncol();	// ABAB: Put a sanity check here
			break;

		}
		case Row:	// pack along the rows, result is a vector of size m
		{
			rvec.glen = getnrow();
			int rowneighs = commGrid->GetGridCols();
			int rowrank = commGrid->GetRankInProcRow();
			GIT * loclens = new GIT[rowneighs];
			GIT * lensums = new GIT[rowneighs+1]();	// begin/end points of local lengths
			loclens[rowrank] = rvec.MyLocLength();
			MPI_Allgather(MPI_IN_PLACE, 0, MPIType<GIT>(), loclens, 1, MPIType<GIT>(), commGrid->GetRowWorld());
			partial_sum(loclens, loclens+rowneighs, lensums+1);
			try
			{
				rvec.arr.resize(loclens[rowrank], id);

				// keeping track of all nonzero iterators within columns at once is unscalable w.r.t. memory (due to sqrt(p) scaling)
				// thus we'll do batches of column as opposed to all columns at once. 5 million columns take 80MB (two pointers per column)
				#define MAXCOLUMNBATCH 5 * 1024 * 1024
				typename DER::SpColIter begfinger = spSeq->begcol();	// beginning finger to columns
				
				// Each processor on the same processor row should execute the SAME number of reduce calls
				int numreducecalls = (int) ceil(static_cast<float>(spSeq->getnzc()) / static_cast<float>(MAXCOLUMNBATCH));
				int maxreducecalls;
				MPI_Allreduce( &numreducecalls, &maxreducecalls, 1, MPI_INT, MPI_MAX, commGrid->GetRowWorld());
				
				for(int k=0; k< maxreducecalls; ++k)
				{
					vector<typename DER::SpColIter::NzIter> nziters;
					typename DER::SpColIter curfinger = begfinger; 
					for(; curfinger != spSeq->endcol() && nziters.size() < MAXCOLUMNBATCH ; ++curfinger)	
					{
						nziters.push_back(spSeq->begnz(curfinger));
					}
					for(int i=0; i< rowneighs; ++i)		// step by step to save memory
					{
						VT * sendbuf = new VT[loclens[i]];
						fill(sendbuf, sendbuf+loclens[i], id);	// fill with identity
		
						typename DER::SpColIter colit = begfinger;		
						IT colcnt = 0;	// "processed column" counter
						for(; colit != curfinger; ++colit, ++colcnt)	// iterate over this batch of columns until curfinger
						{
							typename DER::SpColIter::NzIter nzit = nziters[colcnt];
							for(; nzit != spSeq->endnz(colit) && nzit.rowid() < lensums[i+1]; ++nzit)	// a portion of nonzeros in this column
							{
								sendbuf[nzit.rowid()-lensums[i]] = __binary_op(static_cast<VT>(__unary_op(nzit.value())), sendbuf[nzit.rowid()-lensums[i]]);
							}
							nziters[colcnt] = nzit;	// set the new finger
						}

						VT * recvbuf = NULL;
						if(rowrank == i)
						{
							for(int j=0; j< loclens[i]; ++j)
							{
								sendbuf[j] = __binary_op(rvec.arr[j], sendbuf[j]);	// rvec.arr will be overriden with MPI_Reduce, save its contents
							}
							recvbuf = SpHelper::p2a(rvec.arr);	
						}
						MPI_Reduce(sendbuf, recvbuf, loclens[i], MPIType<VT>(), MPIOp<_BinaryOperation, VT>::op(), i, commGrid->GetRowWorld()); // root = i
						delete [] sendbuf;
					}
					begfinger = curfinger;	// set the next begfilter
				}
				DeleteAll(loclens, lensums);	
			}
			catch (length_error& le) 
			{
	 			 cerr << "Length error: " << le.what() << endl;
  			}
			break;
		}
		default:
		{
			cout << "Unknown reduction dimension, returning empty vector" << endl;
			break;
		}
	}
}

/**
  * Reduce along the column/row into a vector
  * @param[in] __binary_op {the operation used for reduction; examples: max, min, plus, multiply, and, or. Its parameters and return type are all VT}
  * @param[in] id {scalar that is used as the identity for __binary_op; examples: zero, infinity}
  * @param[in] __unary_op {optional unary operation applied to nonzeros *before* the __binary_op; examples: 1/x, x^2}
  * @param[out] rvec {the return vector, specified as an output parameter to allow arbitrary return types via VT}
 **/ 
template <class IT, class NT, class DER>
template <typename VT, typename _BinaryOperation, typename _UnaryOperation>	
void SpParMat<IT,NT,DER>::Reduce(DenseParVec<IT,VT> & rvec, Dim dim, _BinaryOperation __binary_op, VT id, _UnaryOperation __unary_op) const
{
	if(rvec.zero != id)
	{
		ostringstream outs;
		outs << "SpParMat::Reduce(): Return vector's zero is different than set id"  << endl;
		outs << "Setting rvec.zero to id (" << id << ") instead" << endl;
		SpParHelper::Print(outs.str());
		rvec.zero = id;
	}
	if(*rvec.commGrid != *commGrid)
	{
		SpParHelper::Print("Grids are not comparable, SpParMat::Reduce() fails !"); 
		MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
	}
	switch(dim)
	{
		case Column:	// pack along the columns, result is a vector of size n
		{
			VT * sendbuf = new VT[getlocalcols()];
			fill(sendbuf, sendbuf+getlocalcols(), id);	// fill with identity

			for(typename DER::SpColIter colit = spSeq->begcol(); colit != spSeq->endcol(); ++colit)	// iterate over columns
			{
				for(typename DER::SpColIter::NzIter nzit = spSeq->begnz(colit); nzit != spSeq->endnz(colit); ++nzit)
				{
					sendbuf[colit.colid()] = __binary_op(static_cast<VT>(__unary_op(nzit.value())), sendbuf[colit.colid()]);
				}
			}
			VT * recvbuf = NULL;
			int root = commGrid->GetDiagOfProcCol();
			if(rvec.diagonal)
			{
				rvec.arr.resize(getlocalcols());
				recvbuf = SpHelper::p2a(rvec.arr);	
			}
			MPI_Reduce(sendbuf, recvbuf, getlocalcols(), MPIType<VT>(), MPIOp<_BinaryOperation, VT>::op(), root, commGrid->GetColWorld());
			delete [] sendbuf;
			break;
		}
		case Row:	// pack along the rows, result is a vector of size m
		{
			VT * sendbuf = new VT[getlocalrows()];
			fill(sendbuf, sendbuf+getlocalrows(), id);	// fill with identity
			
			for(typename DER::SpColIter colit = spSeq->begcol(); colit != spSeq->endcol(); ++colit)	// iterate over columns
			{
				for(typename DER::SpColIter::NzIter nzit = spSeq->begnz(colit); nzit != spSeq->endnz(colit); ++nzit)
				{
					sendbuf[nzit.rowid()] = __binary_op(static_cast<VT>(__unary_op(nzit.value())), sendbuf[nzit.rowid()]);
				}
			}
			VT * recvbuf = NULL;
			int root = commGrid->GetDiagOfProcRow();
			if(rvec.diagonal)
			{
				rvec.arr.resize(getlocalrows());
				recvbuf = SpHelper::p2a(rvec.arr);	
			}
			MPI_Reduce(sendbuf, recvbuf, getlocalrows(), MPIType<VT>(), MPIOp<_BinaryOperation, VT>::op(), root, commGrid->GetRowWorld());
			delete [] sendbuf;
			break;
		}
		default:
		{
			cout << "Unknown reduction dimension, returning empty vector" << endl;
			break;
		}
	}
}

template <class IT, class NT, class DER>
template <typename NNT,typename NDER>	 
SpParMat<IT,NT,DER>::operator SpParMat<IT,NNT,NDER> () const
{
	NDER * convert = new NDER(*spSeq);
	return SpParMat<IT,NNT,NDER> (convert, commGrid);
}

//! Change index type as well
template <class IT, class NT, class DER>
template <typename NIT, typename NNT,typename NDER>	 
SpParMat<IT,NT,DER>::operator SpParMat<NIT,NNT,NDER> () const
{
	NDER * convert = new NDER(*spSeq);
	return SpParMat<NIT,NNT,NDER> (convert, commGrid);
}

/** 
 * Create a submatrix of size m x (size(ci) * s) on a r x s processor grid
 * Essentially fetches the columns ci[0], ci[1],... ci[size(ci)] from every submatrix
 */
template <class IT, class NT, class DER>
SpParMat<IT,NT,DER> SpParMat<IT,NT,DER>::SubsRefCol (const vector<IT> & ci) const
{
	vector<IT> ri;
	DER * tempseq = new DER((*spSeq)(ri, ci)); 
	return SpParMat<IT,NT,DER> (tempseq, commGrid);	
} 

/** 
 * Generalized sparse matrix indexing (ri/ci are 0-based indexed)
 * Both the storage and the actual values in FullyDistVec should be IT
 * The index vectors are dense and FULLY distributed on all processors
 * We can use this function to apply a permutation like A(p,q) 
 * Sequential indexing subroutine (via multiplication) is general enough.
 */
template <class IT, class NT, class DER>
template <typename PTNTBOOL, typename PTBOOLNT>
SpParMat<IT,NT,DER> SpParMat<IT,NT,DER>::SubsRef_SR (const FullyDistVec<IT,IT> & ri, const FullyDistVec<IT,IT> & ci, bool inplace)
{
	// infer the concrete type SpMat<IT,IT>
	typedef typename create_trait<DER, IT, bool>::T_inferred DER_IT;

	if((*(ri.commGrid) != *(commGrid)) || (*(ci.commGrid) != *(commGrid)))
	{
		SpParHelper::Print("Grids are not comparable, SpRef fails !"); 
		MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
	}

	// Safety check
	IT locmax_ri = 0;
	IT locmax_ci = 0;
	if(!ri.arr.empty())
		locmax_ri = *max_element(ri.arr.begin(), ri.arr.end());
	if(!ci.arr.empty())
		locmax_ci = *max_element(ci.arr.begin(), ci.arr.end());

	IT total_m = getnrow();
	IT total_n = getncol();
	if(locmax_ri > total_m || locmax_ci > total_n)	
	{
		throw outofrangeexception();
	}

	// The indices for FullyDistVec are offset'd to 1/p pieces
	// The matrix indices are offset'd to 1/sqrt(p) pieces
	// Add the corresponding offset before sending the data 
	IT roffset = ri.RowLenUntil();
	IT rrowlen = ri.MyRowLength();
	IT coffset = ci.RowLenUntil();
	IT crowlen = ci.MyRowLength();

	// We create two boolean matrices P and Q
	// Dimensions:  P is size(ri) x m
	//		Q is n x size(ci) 
	// Range(ri) = {0,...,m-1}
	// Range(ci) = {0,...,n-1}

	IT rowneighs = commGrid->GetGridCols();	// number of neighbors along this processor row (including oneself)
	IT totalm = getnrow();	// collective call
	IT totaln = getncol();
	IT m_perproccol = totalm / rowneighs;
	IT n_perproccol = totaln / rowneighs;

	// Get the right local dimensions
	IT diagneigh = commGrid->GetComplementRank();
	IT mylocalrows = getlocalrows();
	IT mylocalcols = getlocalcols();
	IT trlocalrows;
	MPI_Status status;
	MPI_Sendrecv(&mylocalrows, 1, MPIType<IT>(), diagneigh, TRROWX, &trlocalrows, 1, MPIType<IT>(), diagneigh, TRROWX, commGrid->GetWorld(), &status);
	// we don't need trlocalcols because Q.Transpose() will take care of it

	vector< vector<IT> > rowid(rowneighs);	// reuse for P and Q 
	vector< vector<IT> > colid(rowneighs);

	// Step 1: Create P
	IT locvec = ri.arr.size();	// nnz in local vector
	for(typename vector<IT>::size_type i=0; i< (unsigned)locvec; ++i)
	{
		// numerical values (permutation indices) are 0-based
		// recipient alone progessor row
		IT rowrec = (m_perproccol!=0) ? std::min(ri.arr[i] / m_perproccol, rowneighs-1) : (rowneighs-1);	

		// ri's numerical values give the colids and its local indices give rowids
		rowid[rowrec].push_back( i + roffset);	
		colid[rowrec].push_back(ri.arr[i] - (rowrec * m_perproccol));
	}

	int * sendcnt = new int[rowneighs];	// reuse in Q as well
	int * recvcnt = new int[rowneighs];
	for(IT i=0; i<rowneighs; ++i)
		sendcnt[i] = rowid[i].size();

	MPI_Alltoall(sendcnt, 1, MPI_INT, recvcnt, 1, MPI_INT, commGrid->GetRowWorld()); // share the counts
	int * sdispls = new int[rowneighs]();
	int * rdispls = new int[rowneighs]();
	partial_sum(sendcnt, sendcnt+rowneighs-1, sdispls+1);
	partial_sum(recvcnt, recvcnt+rowneighs-1, rdispls+1);
	IT p_nnz = accumulate(recvcnt,recvcnt+rowneighs, static_cast<IT>(0));	

	// create space for incoming data ... 
	IT * p_rows = new IT[p_nnz];
	IT * p_cols = new IT[p_nnz];
  	IT * senddata = new IT[locvec];	// re-used for both rows and columns
	for(int i=0; i<rowneighs; ++i)
	{
		copy(rowid[i].begin(), rowid[i].end(), senddata+sdispls[i]);
		vector<IT>().swap(rowid[i]);	// clear memory of rowid
	}
	MPI_Alltoallv(senddata, sendcnt, sdispls, MPIType<IT>(), p_rows, recvcnt, rdispls, MPIType<IT>(), commGrid->GetRowWorld());

	for(int i=0; i<rowneighs; ++i)
	{
		copy(colid[i].begin(), colid[i].end(), senddata+sdispls[i]);
		vector<IT>().swap(colid[i]);	// clear memory of colid
	}
	MPI_Alltoallv(senddata, sendcnt, sdispls, MPIType<IT>(), p_cols, recvcnt, rdispls, MPIType<IT>(), commGrid->GetRowWorld());
	delete [] senddata;

	tuple<IT,IT,bool> * p_tuples = new tuple<IT,IT,bool>[p_nnz]; 
	for(IT i=0; i< p_nnz; ++i)
	{
		p_tuples[i] = make_tuple(p_rows[i], p_cols[i], 1);
	}
	DeleteAll(p_rows, p_cols);

	DER_IT * PSeq = new DER_IT(); 
	PSeq->Create( p_nnz, rrowlen, trlocalrows, p_tuples);		// deletion of tuples[] is handled by SpMat::Create

	SpParMat<IT,NT,DER> PA;
	if(&ri == &ci)	// Symmetric permutation
	{
		DeleteAll(sendcnt, recvcnt, sdispls, rdispls);
		#ifdef SPREFDEBUG
		SpParHelper::Print("Symmetric permutation\n");
		#endif
		SpParMat<IT,bool,DER_IT> P (PSeq, commGrid);
		if(inplace) 
		{
			#ifdef SPREFDEBUG	
			SpParHelper::Print("In place multiplication\n");
			#endif
        		*this = Mult_AnXBn_DoubleBuff<PTBOOLNT, NT, DER>(P, *this, false, true);	// clear the memory of *this

			//ostringstream outb;
			//outb << "P_after_" << commGrid->myrank;
			//ofstream ofb(outb.str().c_str());
			//P.put(ofb);

			P.Transpose();	
       	 		*this = Mult_AnXBn_DoubleBuff<PTNTBOOL, NT, DER>(*this, P, true, true);	// clear the memory of both *this and P
			return SpParMat<IT,NT,DER>();	// dummy return to match signature
		}
		else
		{
			PA = Mult_AnXBn_DoubleBuff<PTBOOLNT, NT, DER>(P,*this);
			P.Transpose();
			return Mult_AnXBn_DoubleBuff<PTNTBOOL, NT, DER>(PA, P);
		}
	}
	else
	{
		// Intermediate step (to save memory): Form PA and store it in P
		// Distributed matrix generation (collective call)
		SpParMat<IT,bool,DER_IT> P (PSeq, commGrid);

		// Do parallel matrix-matrix multiply
        	PA = Mult_AnXBn_DoubleBuff<PTBOOLNT, NT, DER>(P, *this);
	}	// P is destructed here
#ifndef NDEBUG
	PA.PrintInfo();
#endif
	// Step 2: Create Q  (use the same row-wise communication and transpose at the end)
	// This temporary to-be-transposed Q is size(ci) x n 
	locvec = ci.arr.size();	// nnz in local vector (reset variable)
	for(typename vector<IT>::size_type i=0; i< (unsigned)locvec; ++i)
	{
		// numerical values (permutation indices) are 0-based
		IT rowrec = (n_perproccol!=0) ? std::min(ci.arr[i] / n_perproccol, rowneighs-1) : (rowneighs-1);	

		// ri's numerical values give the colids and its local indices give rowids
		rowid[rowrec].push_back( i + coffset);	
		colid[rowrec].push_back(ci.arr[i] - (rowrec * n_perproccol));
	}

	for(IT i=0; i<rowneighs; ++i)
		sendcnt[i] = rowid[i].size();	// update with new sizes

	MPI_Alltoall(sendcnt, 1, MPI_INT, recvcnt, 1, MPI_INT, commGrid->GetRowWorld()); // share the counts
	fill(sdispls, sdispls+rowneighs, 0);	// reset
	fill(rdispls, rdispls+rowneighs, 0);
	partial_sum(sendcnt, sendcnt+rowneighs-1, sdispls+1);
	partial_sum(recvcnt, recvcnt+rowneighs-1, rdispls+1);
	IT q_nnz = accumulate(recvcnt,recvcnt+rowneighs, static_cast<IT>(0));	

	// create space for incoming data ... 
	IT * q_rows = new IT[q_nnz];
	IT * q_cols = new IT[q_nnz];
  	senddata = new IT[locvec];	
	for(int i=0; i<rowneighs; ++i)
	{
		copy(rowid[i].begin(), rowid[i].end(), senddata+sdispls[i]);
		vector<IT>().swap(rowid[i]);	// clear memory of rowid
	}
	MPI_Alltoallv(senddata, sendcnt, sdispls, MPIType<IT>(), q_rows, recvcnt, rdispls, MPIType<IT>(), commGrid->GetRowWorld());

	for(int i=0; i<rowneighs; ++i)
	{
		copy(colid[i].begin(), colid[i].end(), senddata+sdispls[i]);
		vector<IT>().swap(colid[i]);	// clear memory of colid
	}
	MPI_Alltoallv(senddata, sendcnt, sdispls, MPIType<IT>(), q_cols, recvcnt, rdispls, MPIType<IT>(), commGrid->GetRowWorld());
	DeleteAll(senddata, sendcnt, recvcnt, sdispls, rdispls);

	tuple<IT,IT,bool> * q_tuples = new tuple<IT,IT,bool>[q_nnz]; 
	for(IT i=0; i< q_nnz; ++i)
	{
		q_tuples[i] = make_tuple(q_rows[i], q_cols[i], 1);
	}
	DeleteAll(q_rows, q_cols);
	DER_IT * QSeq = new DER_IT(); 
	QSeq->Create( q_nnz, crowlen, mylocalcols, q_tuples);		// Creating Q' instead

	// Step 3: Form PAQ
	// Distributed matrix generation (collective call)
	SpParMat<IT,bool,DER_IT> Q (QSeq, commGrid);
	Q.Transpose();	
	if(inplace)
	{
       		*this = Mult_AnXBn_DoubleBuff<PTNTBOOL, NT, DER>(PA, Q, true, true);	// clear the memory of both PA and P
		return SpParMat<IT,NT,DER>();	// dummy return to match signature
	}
	else
	{
        	return Mult_AnXBn_DoubleBuff<PTNTBOOL, NT, DER>(PA, Q);
	}
}


template <class IT, class NT, class DER>
void SpParMat<IT,NT,DER>::SpAsgn(const FullyDistVec<IT,IT> & ri, const FullyDistVec<IT,IT> & ci, SpParMat<IT,NT,DER> & B)
{
	typedef PlusTimesSRing<NT, NT> PTRing;
	
	if((*(ri.commGrid) != *(B.commGrid)) || (*(ci.commGrid) != *(B.commGrid)))
	{
		SpParHelper::Print("Grids are not comparable, SpAsgn fails !"); 
		MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
	}
	IT total_m_A = getnrow();
	IT total_n_A = getncol();
	IT total_m_B = B.getnrow();
	IT total_n_B = B.getncol();
	
	if(total_m_B != ri.TotalLength())
	{
		SpParHelper::Print("First dimension of B does NOT match the length of ri, SpAsgn fails !"); 
		MPI_Abort(MPI_COMM_WORLD, DIMMISMATCH);
	}
	if(total_n_B != ci.TotalLength())
	{
		SpParHelper::Print("Second dimension of B does NOT match the length of ci, SpAsgn fails !"); 
		MPI_Abort(MPI_COMM_WORLD, DIMMISMATCH);
	}
	Prune(ri, ci);	// make a hole	
	
	// embed B to the size of A
	FullyDistVec<IT,IT> * rvec = new FullyDistVec<IT,IT>();
	rvec->iota(total_m_B, 0);	// sparse() expects a zero based index
	
	SpParMat<IT,NT,DER> R(total_m_A, total_m_B, ri, *rvec, 1);
	delete rvec;	// free memory
	SpParMat<IT,NT,DER> RB = Mult_AnXBn_DoubleBuff<PTRing, NT, DER>(R, B, true, false); // clear memory of R but not B
	
	FullyDistVec<IT,IT> * qvec = new FullyDistVec<IT,IT>();
	qvec->iota(total_n_B, 0);
	SpParMat<IT,NT,DER> Q(total_n_B, total_n_A, *qvec, ci, 1);
	delete qvec;	// free memory
	SpParMat<IT,NT,DER> RBQ = Mult_AnXBn_DoubleBuff<PTRing, NT, DER>(RB, Q, true, true); // clear memory of RB and Q
	*this += RBQ;	// extend-add
}

template <class IT, class NT, class DER>
void SpParMat<IT,NT,DER>::Prune(const FullyDistVec<IT,IT> & ri, const FullyDistVec<IT,IT> & ci)
{
	typedef PlusTimesSRing<NT, NT> PTRing;

	if((*(ri.commGrid) != *(commGrid)) || (*(ci.commGrid) != *(commGrid)))
	{
		SpParHelper::Print("Grids are not comparable, Prune fails !"); 
		MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
	}

	// Safety check
	IT locmax_ri = 0;
	IT locmax_ci = 0;
	if(!ri.arr.empty())
		locmax_ri = *max_element(ri.arr.begin(), ri.arr.end());
	if(!ci.arr.empty())
		locmax_ci = *max_element(ci.arr.begin(), ci.arr.end());

	IT total_m = getnrow();
	IT total_n = getncol();
	if(locmax_ri > total_m || locmax_ci > total_n)	
	{
		throw outofrangeexception();
	}

	SpParMat<IT,NT,DER> S(total_m, total_m, ri, ri, 1);
	SpParMat<IT,NT,DER> SA = Mult_AnXBn_DoubleBuff<PTRing, NT, DER>(S, *this, true, false); // clear memory of S but not *this

	SpParMat<IT,NT,DER> T(total_n, total_n, ci, ci, 1);
	SpParMat<IT,NT,DER> SAT = Mult_AnXBn_DoubleBuff<PTRing, NT, DER>(SA, T, true, true); // clear memory of SA and T
	EWiseMult(SAT, true);	// In-place EWiseMult with not(SAT)
}


// In-place version where rhs type is the same (no need for type promotion)
template <class IT, class NT, class DER>
void SpParMat<IT,NT,DER>::EWiseMult (const SpParMat< IT,NT,DER >  & rhs, bool exclude)
{
	if(*commGrid == *rhs.commGrid)	
	{
		spSeq->EWiseMult(*(rhs.spSeq), exclude);		// Dimension compatibility check performed by sequential function
	}
	else
	{
		cout << "Grids are not comparable, EWiseMult() fails !" << endl; 
		MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
	}	
}


template <class IT, class NT, class DER>
void SpParMat<IT,NT,DER>::EWiseScale(const DenseParMat<IT, NT> & rhs)
{
	if(*commGrid == *rhs.commGrid)	
	{
		spSeq->EWiseScale(rhs.array, rhs.m, rhs.n);	// Dimension compatibility check performed by sequential function
	}
	else
	{
		cout << "Grids are not comparable, EWiseScale() fails !" << endl; 
		MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
	}
}

template <class IT, class NT, class DER>
template <typename _BinaryOperation>
void SpParMat<IT,NT,DER>::UpdateDense(DenseParMat<IT, NT> & rhs, _BinaryOperation __binary_op) const
{
	if(*commGrid == *rhs.commGrid)	
	{
		if(getlocalrows() == rhs.m  && getlocalcols() == rhs.n)
		{
			spSeq->UpdateDense(rhs.array, __binary_op);
		}
		else
		{
			cout << "Matrices have different dimensions, UpdateDense() fails !" << endl;
			MPI_Abort(MPI_COMM_WORLD, DIMMISMATCH);
		}
	}
	else
	{
		cout << "Grids are not comparable, UpdateDense() fails !" << endl; 
		MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
	}
}

template <class IT, class NT, class DER>
void SpParMat<IT,NT,DER>::PrintInfo() const
{
	IT mm = getnrow(); 
	IT nn = getncol();
	IT nznz = getnnz();
	
	if (commGrid->myrank == 0)	
		cout << "As a whole: " << mm << " rows and "<< nn <<" columns and "<<  nznz << " nonzeros" << endl; 

#ifdef DEBUG
	IT allprocs = commGrid->grrows * commGrid->grcols;
	for(IT i=0; i< allprocs; ++i)
	{
		if (commGrid->myrank == i)
		{
			cout << "Processor (" << commGrid->GetRankInProcRow() << "," << commGrid->GetRankInProcCol() << ")'s data: " << endl;
			spSeq->PrintInfo();
		}
		MPI_Barrier(commGrid->GetWorld());
	}
#endif
}

template <class IT, class NT, class DER>
bool SpParMat<IT,NT,DER>::operator== (const SpParMat<IT,NT,DER> & rhs) const
{
	int local = static_cast<int>((*spSeq) == (*(rhs.spSeq)));
	int whole = 1;
	MPI_Allreduce( &local, &whole, 1, MPI_INT, MPI_BAND, commGrid->GetWorld());
	return static_cast<bool>(whole);	
}


/**
 ** Private function that carries code common to different sparse() constructors
 ** Before this call, commGrid is already set
 **/
template <class IT, class NT, class DER>
void SpParMat< IT,NT,DER >::SparseCommon(vector< vector < tuple<IT,IT,NT> > > & data, IT locsize, IT total_m, IT total_n)
{
	int nprocs = commGrid->GetSize();
	int * sendcnt = new int[nprocs];
	int * recvcnt = new int[nprocs];
	for(int i=0; i<nprocs; ++i)
		sendcnt[i] = data[i].size();	// sizes are all the same

	MPI_Alltoall(sendcnt, 1, MPI_INT, recvcnt, 1, MPI_INT, commGrid->GetWorld()); // share the counts
	int * sdispls = new int[nprocs]();
	int * rdispls = new int[nprocs]();
	partial_sum(sendcnt, sendcnt+nprocs-1, sdispls+1);
	partial_sum(recvcnt, recvcnt+nprocs-1, rdispls+1);

  	tuple<IT,IT,NT> * senddata = new tuple<IT,IT,NT>[locsize];	// re-used for both rows and columns
	for(int i=0; i<nprocs; ++i)
	{
		copy(data[i].begin(), data[i].end(), senddata+sdispls[i]);
		vector< tuple<IT,IT,NT> >().swap(data[i]);	// clear memory
	}
	MPI_Datatype MPI_triple;
	MPI_Type_contiguous(sizeof(tuple<IT,IT,NT>), MPI_CHAR, &MPI_triple);
	MPI_Type_commit(&MPI_triple);

	IT totrecv = accumulate(recvcnt,recvcnt+nprocs, static_cast<IT>(0));	
	tuple<IT,IT,NT> * recvdata = new tuple<IT,IT,NT>[totrecv];	
	MPI_Alltoallv(senddata, sendcnt, sdispls, MPI_triple, recvdata, recvcnt, rdispls, MPI_triple, commGrid->GetWorld());

	DeleteAll(senddata, sendcnt, recvcnt, sdispls, rdispls);
	MPI_Type_free(&MPI_triple);

	int r = commGrid->GetGridRows();
	int s = commGrid->GetGridCols();
	IT m_perproc = total_m / r;
	IT n_perproc = total_n / s;
	int myprocrow = commGrid->GetRankInProcCol();
	int myproccol = commGrid->GetRankInProcRow();
	IT locrows, loccols; 
	if(myprocrow != r-1)	locrows = m_perproc;
	else 	locrows = total_m - myprocrow * m_perproc;
	if(myproccol != s-1)	loccols = n_perproc;
	else	loccols = total_n - myproccol * n_perproc;

	SpTuples<IT,NT> A(totrecv, locrows, loccols, recvdata);	// It is ~SpTuples's job to deallocate
  	spSeq = new DER(A,false);        // Convert SpTuples to DER
}


//! All vectors are zero-based indexed (as usual)
template <class IT, class NT, class DER>
SpParMat< IT,NT,DER >::SpParMat (IT total_m, IT total_n, const FullyDistVec<IT,IT> & distrows, 
				const FullyDistVec<IT,IT> & distcols, const FullyDistVec<IT,NT> & distvals)
{
	if((*(distrows.commGrid) != *(distcols.commGrid)) || (*(distcols.commGrid) != *(distvals.commGrid)))
	{
		SpParHelper::Print("Grids are not comparable, Sparse() fails !"); 
		MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
	}
	if((distrows.TotalLength() != distcols.TotalLength()) || (distcols.TotalLength() != distvals.TotalLength()))
	{
		SpParHelper::Print("Vectors have different sizes, Sparse() fails !");
		MPI_Abort(MPI_COMM_WORLD, DIMMISMATCH);
	}

	commGrid = distrows.commGrid;	
	int nprocs = commGrid->GetSize();
	vector< vector < tuple<IT,IT,NT> > > data(nprocs);

	IT locsize = distrows.LocArrSize();
	for(IT i=0; i<locsize; ++i)
	{
		IT lrow, lcol; 
		int owner = Owner(total_m, total_n, distrows.arr[i], distcols.arr[i], lrow, lcol);
		data[owner].push_back(make_tuple(lrow,lcol,distvals.arr[i]));	
	}
	SparseCommon(data, locsize, total_m, total_n);
}



template <class IT, class NT, class DER>
SpParMat< IT,NT,DER >::SpParMat (IT total_m, IT total_n, const FullyDistVec<IT,IT> & distrows, 
				const FullyDistVec<IT,IT> & distcols, const NT & val)
{
	if((*(distrows.commGrid) != *(distcols.commGrid)) )
	{
		SpParHelper::Print("Grids are not comparable, Sparse() fails !"); 
		MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
	}
	if((distrows.TotalLength() != distcols.TotalLength()) )
	{
		SpParHelper::Print("Vectors have different sizes, Sparse() fails !");
		MPI_Abort(MPI_COMM_WORLD, DIMMISMATCH);
	}
	commGrid = distrows.commGrid;
	int nprocs = commGrid->GetSize();
	vector< vector < tuple<IT,IT,NT> > > data(nprocs);

	IT locsize = distrows.LocArrSize();
	for(IT i=0; i<locsize; ++i)
	{
		IT lrow, lcol; 
		int owner = Owner(total_m, total_n, distrows.arr[i], distcols.arr[i], lrow, lcol);
		data[owner].push_back(make_tuple(lrow,lcol,val));	
	}
	SparseCommon(data, locsize, total_m, total_n);
}

template <class IT, class NT, class DER>
template <class DELIT>
SpParMat< IT,NT,DER >::SpParMat (const DistEdgeList<DELIT> & DEL, bool removeloops)
{
	commGrid = DEL.commGrid;	
	typedef typename DER::LocalIT LIT;

	int nprocs = commGrid->GetSize();
	int r = commGrid->GetGridRows();
	int s = commGrid->GetGridCols();
	vector< vector<LIT> > data(nprocs);	// enties are pre-converted to local indices before getting pushed into "data"

	LIT m_perproc = DEL.getGlobalV() / r;
	LIT n_perproc = DEL.getGlobalV() / s;

	if(sizeof(LIT) < sizeof(DELIT))
	{
		ostringstream outs;
		outs << "Warning: Using smaller indices for the matrix than DistEdgeList\n";
		outs << "Local matrices are " << m_perproc << "-by-" << n_perproc << endl;
		SpParHelper::Print(outs.str());
	}	
	
	// to lower memory consumption, form sparse matrix in stages
	LIT stages = MEM_EFFICIENT_STAGES;	
	
	// even if local indices (LIT) are 32-bits, we should work with 64-bits for global info
	int64_t perstage = DEL.nedges / stages;
	LIT totrecv = 0;
	vector<LIT> alledges;

	int maxr = r-1;
	int maxs = s-1;	
	for(LIT s=0; s< stages; ++s)
	{
		int64_t n_befor = s*perstage;
		int64_t n_after= ((s==(stages-1))? DEL.nedges : ((s+1)*perstage));

		// clear the source vertex by setting it to -1
		int realedges = 0;	// these are "local" realedges

		if(DEL.pedges)	
		{
			for (int64_t i = n_befor; i < n_after; i++)
			{
				int64_t fr = get_v0_from_edge(&(DEL.pedges[i]));
				int64_t to = get_v1_from_edge(&(DEL.pedges[i]));

				if(fr >= 0 && to >= 0)	// otherwise skip
				{
					int rowowner = min(static_cast<int>(fr / m_perproc), maxr);
					int colowner = min(static_cast<int>(to / n_perproc), maxs); 
					int owner = commGrid->GetRank(rowowner, colowner);
					LIT rowid = fr - (rowowner * m_perproc);	
					LIT colid = to - (colowner * n_perproc);
					data[owner].push_back(rowid);	// row_id
					data[owner].push_back(colid);	// col_id
					++realedges;
				}
			}
		}
		else
		{
			for (int64_t i = n_befor; i < n_after; i++)
			{
				if(DEL.edges[2*i+0] >= 0 && DEL.edges[2*i+1] >= 0)	// otherwise skip
				{
					int rowowner = min(static_cast<int>(DEL.edges[2*i+0] / m_perproc), maxr);
					int colowner = min(static_cast<int>(DEL.edges[2*i+1] / n_perproc), maxs); 
					int owner = commGrid->GetRank(rowowner, colowner);
					LIT rowid = DEL.edges[2*i+0]- (rowowner * m_perproc);
					LIT colid = DEL.edges[2*i+1]- (colowner * n_perproc);
					data[owner].push_back(rowid);	
					data[owner].push_back(colid);
					++realedges;
				}
			}
		}

  		LIT * sendbuf = new LIT[2*realedges];
		int * sendcnt = new int[nprocs];
		int * sdispls = new int[nprocs];
		for(int i=0; i<nprocs; ++i)
			sendcnt[i] = data[i].size();

		int * rdispls = new int[nprocs];
		int * recvcnt = new int[nprocs];
		MPI_Alltoall(sendcnt, 1, MPI_INT, recvcnt, 1, MPI_INT,commGrid->GetWorld()); // share the counts

		sdispls[0] = 0;
		rdispls[0] = 0;
		for(int i=0; i<nprocs-1; ++i)
		{
			sdispls[i+1] = sdispls[i] + sendcnt[i];
			rdispls[i+1] = rdispls[i] + recvcnt[i];
		}
		for(int i=0; i<nprocs; ++i)
			copy(data[i].begin(), data[i].end(), sendbuf+sdispls[i]);
		
		// clear memory
		for(int i=0; i<nprocs; ++i)
			vector<LIT>().swap(data[i]);

		// ABAB: Total number of edges received might not be LIT-addressible
		// However, each edge_id is LIT-addressible
		IT thisrecv = accumulate(recvcnt,recvcnt+nprocs, static_cast<IT>(0));	// thisrecv = 2*locedges
		LIT * recvbuf = new LIT[thisrecv];
		totrecv += thisrecv;
			
		MPI_Alltoallv(sendbuf, sendcnt, sdispls, MPIType<LIT>(), recvbuf, recvcnt, rdispls, MPIType<LIT>(), commGrid->GetWorld());
		DeleteAll(sendcnt, recvcnt, sdispls, rdispls,sendbuf);
		copy (recvbuf,recvbuf+thisrecv,back_inserter(alledges));	// copy to all edges
		delete [] recvbuf;
	}

	int myprocrow = commGrid->GetRankInProcCol();
	int myproccol = commGrid->GetRankInProcRow();
	LIT locrows, loccols; 
	if(myprocrow != r-1)	locrows = m_perproc;
	else 	locrows = DEL.getGlobalV() - myprocrow * m_perproc;
	if(myproccol != s-1)	loccols = n_perproc;
	else	loccols = DEL.getGlobalV() - myproccol * n_perproc;

  	SpTuples<LIT,NT> A(totrecv/2, locrows, loccols, alledges, removeloops);  	// alledges is empty upon return
  	spSeq = new DER(A,false);        // Convert SpTuples to DER
}

template <class IT, class NT, class DER>
IT SpParMat<IT,NT,DER>::RemoveLoops()
{
	MPI_Comm DiagWorld = commGrid->GetDiagWorld();
	IT totrem;
	IT removed = 0;
	if(DiagWorld != MPI_COMM_NULL) // Diagonal processors only
	{
		SpTuples<IT,NT> tuples(*spSeq);
		delete spSeq;
		removed  = tuples.RemoveLoops();
		spSeq = new DER(tuples, false);	// Convert to DER
	}
	MPI_Allreduce( &removed, & totrem, 1, MPIType<IT>(), MPI_SUM, commGrid->GetWorld());
	return totrem;
}		

template <class IT, class NT, class DER>
template <typename LIT, typename OT>
void SpParMat<IT,NT,DER>::OptimizeForGraph500(OptBuf<LIT,OT> & optbuf)
{
	if(spSeq->getnsplit() > 0)
	{
		SpParHelper::Print("Can not declare preallocated buffers for multithreaded execution");
		return;
	}

	// Set up communication buffers, one for all
	typename DER::LocalIT mA = spSeq->getnrow();
	typename DER::LocalIT p_c = commGrid->GetGridCols();
	vector<bool> isthere(mA, false); // perhaps the only appropriate use of this crippled data structure
	typename DER::LocalIT perproc = mA / p_c; 
	vector<int> maxlens(p_c,0);	// maximum data size to be sent to any neighbor along the processor row

	for(typename DER::SpColIter colit = spSeq->begcol(); colit != spSeq->endcol(); ++colit)
	{
		for(typename DER::SpColIter::NzIter nzit = spSeq->begnz(colit); nzit != spSeq->endnz(colit); ++nzit)
		{
			typename DER::LocalIT rowid = nzit.rowid();
			if(!isthere[rowid])
			{
				typename DER::LocalIT owner = min(nzit.rowid() / perproc, p_c-1); 			
				maxlens[owner]++;
				isthere[rowid] = true;
			}
		}
	}
	SpParHelper::Print("Optimization buffers set\n");
	optbuf.Set(maxlens,mA);
}

template <class IT, class NT, class DER>
void SpParMat<IT,NT,DER>::ActivateThreading(int numsplits)
{
	spSeq->RowSplit(numsplits);
}


/**
 * Parallel routine that returns A*A on the semiring SR
 * Uses only MPI-1 features (relies on simple blocking broadcast)
 **/  
template <class IT, class NT, class DER>
template <typename SR>
void SpParMat<IT,NT,DER>::Square ()
{
	int stages, dummy; 	// last two parameters of productgrid are ignored for synchronous multiplication
	shared_ptr<CommGrid> Grid = ProductGrid(commGrid.get(), commGrid.get(), stages, dummy, dummy);		
	
	IT AA_m = spSeq->getnrow();
	IT AA_n = spSeq->getncol();
	
	DER seqTrn = spSeq->TransposeConst();	// will be automatically discarded after going out of scope		

	MPI_Barrier(commGrid->GetWorld());

	IT ** NRecvSizes = SpHelper::allocate2D<IT>(DER::esscount, stages);
	IT ** TRecvSizes = SpHelper::allocate2D<IT>(DER::esscount, stages);
	
	SpParHelper::GetSetSizes( *spSeq, NRecvSizes, commGrid->GetRowWorld());
	SpParHelper::GetSetSizes( seqTrn, TRecvSizes, commGrid->GetColWorld());

	// Remotely fetched matrices are stored as pointers
	DER * NRecv; 
	DER * TRecv;
	vector< SpTuples<IT,NT>  *> tomerge;

	int Nself = commGrid->GetRankInProcRow();
	int Tself = commGrid->GetRankInProcCol();	

	for(int i = 0; i < stages; ++i) 
	{
		vector<IT> ess;	
		if(i == Nself)
		{	
			NRecv = spSeq;	// shallow-copy 
		}
		else
		{
			ess.resize(DER::esscount);
			for(int j=0; j< DER::esscount; ++j)	
			{
				ess[j] = NRecvSizes[j][i];		// essentials of the ith matrix in this row	
			}
			NRecv = new DER();				// first, create the object
		}

		SpParHelper::BCastMatrix(Grid->GetRowWorld(), *NRecv, ess, i);	// then, broadcast its elements	
		ess.clear();	
		
		if(i == Tself)
		{
			TRecv = &seqTrn;	// shallow-copy
		}
		else
		{
			ess.resize(DER::esscount);		
			for(int j=0; j< DER::esscount; ++j)	
			{
				ess[j] = TRecvSizes[j][i];	
			}	
			TRecv = new DER();
		}
		SpParHelper::BCastMatrix(Grid->GetColWorld(), *TRecv, ess, i);	

		SpTuples<IT,NT> * AA_cont = MultiplyReturnTuples<SR, NT>(*NRecv, *TRecv, false, true);
		if(!AA_cont->isZero()) 
			tomerge.push_back(AA_cont);

		if(i != Nself)	
		{
			delete NRecv;		
		}
		if(i != Tself)	
		{
			delete TRecv;
		}
	}

	SpHelper::deallocate2D(NRecvSizes, DER::esscount);
	SpHelper::deallocate2D(TRecvSizes, DER::esscount);
	
	delete spSeq;		
	spSeq = new DER(MergeAll<SR>(tomerge, AA_m, AA_n), false);	// First get the result in SpTuples, then convert to UDER
	for(unsigned int i=0; i<tomerge.size(); ++i)
	{
		delete tomerge[i];
	}
}


template <class IT, class NT, class DER>
void SpParMat<IT,NT,DER>::Transpose()
{
	if(commGrid->myproccol == commGrid->myprocrow)	// Diagonal
	{
		spSeq->Transpose();			
	}
	else
	{
		typedef typename DER::LocalIT LIT;
		SpTuples<LIT,NT> Atuples(*spSeq);
		LIT locnnz = Atuples.getnnz();
		LIT * rows = new LIT[locnnz];
		LIT * cols = new LIT[locnnz];
		NT * vals = new NT[locnnz];
		for(LIT i=0; i < locnnz; ++i)
		{
			rows[i] = Atuples.colindex(i);	// swap (i,j) here
			cols[i] = Atuples.rowindex(i);
			vals[i] = Atuples.numvalue(i);
		}
		LIT locm = getlocalcols();
		LIT locn = getlocalrows();
		delete spSeq;

		LIT remotem, remoten, remotennz;
		swap(locm,locn);
		int diagneigh = commGrid->GetComplementRank();

		MPI_Status status;
		MPI_Sendrecv(&locnnz, 1, MPIType<LIT>(), diagneigh, TRTAGNZ, &remotennz, 1, MPIType<LIT>(), diagneigh, TRTAGNZ, commGrid->GetWorld(), &status);
		MPI_Sendrecv(&locn, 1, MPIType<LIT>(), diagneigh, TRTAGM, &remotem, 1, MPIType<LIT>(), diagneigh, TRTAGM, commGrid->GetWorld(), &status);
		MPI_Sendrecv(&locm, 1, MPIType<LIT>(), diagneigh, TRTAGN, &remoten, 1, MPIType<LIT>(), diagneigh, TRTAGN, commGrid->GetWorld(), &status);

		LIT * rowsrecv = new LIT[remotennz];
		MPI_Sendrecv(rows, locnnz, MPIType<LIT>(), diagneigh, TRTAGROWS, rowsrecv, remotennz, MPIType<LIT>(), diagneigh, TRTAGROWS, commGrid->GetWorld(), &status);
		delete [] rows;

		LIT * colsrecv = new LIT[remotennz];
		MPI_Sendrecv(cols, locnnz, MPIType<LIT>(), diagneigh, TRTAGCOLS, colsrecv, remotennz, MPIType<LIT>(), diagneigh, TRTAGCOLS, commGrid->GetWorld(), &status);
		delete [] cols;

		NT * valsrecv = new NT[remotennz];
		MPI_Sendrecv(vals, locnnz, MPIType<NT>(), diagneigh, TRTAGVALS, valsrecv, remotennz, MPIType<NT>(), diagneigh, TRTAGVALS, commGrid->GetWorld(), &status);
		delete [] vals;

		tuple<LIT,LIT,NT> * arrtuples = new tuple<LIT,LIT,NT>[remotennz];
		for(LIT i=0; i< remotennz; ++i)
		{
			arrtuples[i] = make_tuple(rowsrecv[i], colsrecv[i], valsrecv[i]);
		}	
		DeleteAll(rowsrecv, colsrecv, valsrecv);
		ColLexiCompare<LIT,NT> collexicogcmp;
		sort(arrtuples , arrtuples+remotennz, collexicogcmp );	// sort w.r.t columns here

		spSeq = new DER();
		spSeq->Create( remotennz, remotem, remoten, arrtuples);		// the deletion of arrtuples[] is handled by SpMat::Create
	}	
}		


template <class IT, class NT, class DER>
template <class HANDLER>
void SpParMat< IT,NT,DER >::SaveGathered(string filename, HANDLER handler, bool transpose) const
{
	int proccols = commGrid->GetGridCols();
	int procrows = commGrid->GetGridRows();
	IT totalm = getnrow();
	IT totaln = getncol();
	IT totnnz = getnnz();
	int flinelen = 0;
	ofstream out;
	if(commGrid->GetRank() == 0)
	{
		std::string s;
		std::stringstream strm;
		strm << totalm << " " << totaln << " " << totnnz << endl;
		s = strm.str();
		out.open(filename.c_str(),ios_base::trunc);
		flinelen = s.length();
		out.write(s.c_str(), flinelen);
		out.close();
	}
	int colrank = commGrid->GetRankInProcCol(); 
	int colneighs = commGrid->GetGridRows();
	IT * locnrows = new IT[colneighs];	// number of rows is calculated by a reduction among the processor column
	locnrows[colrank] = (IT) getlocalrows();
	MPI_Allgather(MPI_IN_PLACE, 0, MPIType<IT>(),locnrows, 1, MPIType<IT>(), commGrid->GetColWorld());
	IT roffset = accumulate(locnrows, locnrows+colrank, 0);
	delete [] locnrows;	

	MPI_Datatype datatype;
	MPI_Type_contiguous(sizeof(pair<IT,NT>), MPI_CHAR, &datatype);
	MPI_Type_commit(&datatype);

	for(int i = 0; i < procrows; i++)	// for all processor row (in order)
	{
		if(commGrid->GetRankInProcCol() == i)	// only the ith processor row
		{ 
			IT localrows = spSeq->getnrow();    // same along the processor row
			vector< vector< pair<IT,NT> > > csr(localrows);
			if(commGrid->GetRankInProcRow() == 0)	// get the head of processor row 
			{
				IT localcols = spSeq->getncol();    // might be different on the last processor on this processor row
				MPI_Bcast(&localcols, 1, MPIType<IT>(), 0, commGrid->GetRowWorld());
				for(typename DER::SpColIter colit = spSeq->begcol(); colit != spSeq->endcol(); ++colit)	// iterate over nonempty subcolumns
				{
					for(typename DER::SpColIter::NzIter nzit = spSeq->begnz(colit); nzit != spSeq->endnz(colit); ++nzit)
					{
						csr[nzit.rowid()].push_back( make_pair(colit.colid(), nzit.value()) );
					}
				}
			}
			else	// get the rest of the processors
			{
				IT n_perproc;
				MPI_Bcast(&n_perproc, 1, MPIType<IT>(), 0, commGrid->GetRowWorld());
				IT noffset = commGrid->GetRankInProcRow() * n_perproc; 
				for(typename DER::SpColIter colit = spSeq->begcol(); colit != spSeq->endcol(); ++colit)	// iterate over nonempty subcolumns
				{
					for(typename DER::SpColIter::NzIter nzit = spSeq->begnz(colit); nzit != spSeq->endnz(colit); ++nzit)
					{
						csr[nzit.rowid()].push_back( make_pair(colit.colid() + noffset, nzit.value()) );
					}
				}
			}
			pair<IT,NT> * ents = NULL;
			int * gsizes = NULL, * dpls = NULL;
			if(commGrid->GetRankInProcRow() == 0)	// only the head of processor row 
			{
				out.open(filename.c_str(),std::ios_base::app);
				gsizes = new int[proccols];
				dpls = new int[proccols]();	// displacements (zero initialized pid) 
			}
			for(int j = 0; j < localrows; ++j)	
			{
				IT rowcnt = 0;
				sort(csr[j].begin(), csr[j].end());
				int mysize = csr[j].size();
				MPI_Gather(&mysize, 1, MPI_INT, gsizes, 1, MPI_INT, 0, commGrid->GetRowWorld());
				if(commGrid->GetRankInProcRow() == 0)	
				{
					rowcnt = std::accumulate(gsizes, gsizes+proccols, static_cast<IT>(0));
					std::partial_sum(gsizes, gsizes+proccols-1, dpls+1);
					ents = new pair<IT,NT>[rowcnt];	// nonzero entries in the j'th local row
				}

				// int MPI_Gatherv (void* sbuf, int scount, MPI_Datatype stype, 
				// 		    void* rbuf, int *rcount, int* displs, MPI_Datatype rtype, int root, MPI_Comm comm)	
				MPI_Gatherv(SpHelper::p2a(csr[j]), mysize, datatype, ents, gsizes, dpls, datatype, 0, commGrid->GetRowWorld());
				if(commGrid->GetRankInProcRow() == 0)	
				{
					for(int k=0; k< rowcnt; ++k)
					{
						//out << j + roffset + 1 << "\t" << ents[k].first + 1 <<"\t" << ents[k].second << endl;
						if (!transpose)
							// regular
							out << j + roffset + 1 << "\t" << ents[k].first + 1 << "\t";
						else
							// transpose row/column
							out << ents[k].first + 1 << "\t" << j + roffset + 1 << "\t";
						handler.save(out, ents[k].second, j + roffset, ents[k].first);
						out << endl;
					}
					delete [] ents;
				}
			}
			if(commGrid->GetRankInProcRow() == 0)
			{
				DeleteAll(gsizes, dpls);
				out.close();
			}
		} // end_if the ith processor row 
		MPI_Barrier(commGrid->GetWorld());		// signal the end of ith processor row iteration (so that all processors block)
	}
}


// Prints in the following format suitable for I/O with PaToH
// 1st line: <index_base(0 or 1)> <|V|> <|N|> <pins>
// For row-wise sparse matrix partitioning (our case): 1 <num_rows> <num_cols> <nnz>
// For the rest of the file, (i+1)th line is a list of vertices that are members of the ith net
// Treats the matrix as binary (for now)
template <class IT, class NT, class DER>
void SpParMat< IT,NT,DER >::PrintForPatoh(string filename) const
{
	int proccols = commGrid->GetGridCols();
	int procrows = commGrid->GetGridRows();
	IT * gsizes;

	// collective calls
	IT totalm = getnrow();
	IT totaln = getncol();
	IT totnnz = getnnz();
	int flinelen = 0;
	if(commGrid->GetRank() == 0)
	{
		std::string s;
		std::stringstream strm;
		strm << 0 << " " << totalm << " " << totaln << " " << totnnz << endl;
		s = strm.str();
		std::ofstream out(filename.c_str(),std::ios_base::app);
		flinelen = s.length();
		out.write(s.c_str(), flinelen);
		out.close();
	}

	IT nzc = 0;	// nonempty column counts per processor column
	for(int i = 0; i < proccols; i++)	// for all processor columns (in order)
	{
		if(commGrid->GetRankInProcRow() == i)	// only the ith processor column
		{ 
			if(commGrid->GetRankInProcCol() == 0)	// get the head of column
			{
				std::ofstream out(filename.c_str(),std::ios_base::app);
				std::ostream_iterator<IT> dest(out, " ");

				gsizes = new IT[procrows];
				IT localrows = spSeq->getnrow();    
				MPI_Bcast(&localrows, 1, MPIType<IT>(), 0, commGrid->GetColWorld());
				IT netid = 0;
				for(typename DER::SpColIter colit = spSeq->begcol(); colit != spSeq->endcol(); ++colit)	// iterate over nonempty subcolumns
				{
					IT mysize;
					IT * vertices;
					while(netid <= colit.colid())
					{
						if(netid < colit.colid())	// empty subcolumns
						{
							mysize = 0;
						}
						else
						{
							mysize = colit.nnz();
							vertices = new IT[mysize];
							IT j = 0;
							for(typename DER::SpColIter::NzIter nzit = spSeq->begnz(colit); nzit != spSeq->endnz(colit); ++nzit)
							{
								vertices[j] = nzit.rowid();
								++j;
							}
						}
							
						MPI_Gather(&mysize, 1, MPIType<IT>(), gsizes, 1, MPIType<IT>(), 0, commGrid->GetColWorld());	
						IT colcnt = std::accumulate(gsizes, gsizes+procrows, 0);
						IT * ents = new IT[colcnt];	// nonzero entries in the netid'th column
						IT * dpls = new IT[procrows]();	// displacements (zero initialized pid) 
						std::partial_sum(gsizes, gsizes+procrows-1, dpls+1);

						// int MPI_Gatherv (void* sbuf, int scount, MPI_Datatype stype, 
						// 		    void* rbuf, int *rcount, int* displs, MPI_Datatype rtype, int root, MPI_Comm comm)	
						MPI_Gatherv(vertices, mysize, MPIType<IT>(), ents, gsizes, dpls, MPIType<IT>(), 0, commGrid->GetColWorld());
						if(colcnt != 0)
						{
							std::copy(ents, ents+colcnt, dest);	
							out << endl;
							++nzc;
						}
						delete [] ents; delete [] dpls;
						
						if(netid == colit.colid())	delete [] vertices;
						++netid;
					} 
				}
				while(netid < spSeq->getncol())
				{
					IT mysize = 0; 	
					MPI_Gather(&mysize, 1, MPIType<IT>(), gsizes, 1, MPIType<IT>(), 0, commGrid->GetColWorld());
					IT colcnt = std::accumulate(gsizes, gsizes+procrows, 0);
					IT * ents = new IT[colcnt];	// nonzero entries in the netid'th column
					IT * dpls = new IT[procrows]();	// displacements (zero initialized pid) 
					std::partial_sum(gsizes, gsizes+procrows-1, dpls+1);

					MPI_Gatherv(NULL, mysize, MPIType<IT>(), ents, gsizes, dpls, MPIType<IT>(), 0, commGrid->GetColWorld());
					if(colcnt != 0)
					{
						std::copy(ents, ents+colcnt, dest);	
						out << endl;
						++nzc;
					}
					delete [] ents; delete [] dpls;
					++netid;
				} 
				delete [] gsizes;
				out.close();
			}
			else	// get the rest of the processors
			{
				IT m_perproc;
				MPI_Bcast(&m_perproc, 1, MPIType<IT>(), 0, commGrid->GetColWorld());
				IT moffset = commGrid->GetRankInProcCol() * m_perproc; 
				IT netid = 0;
				for(typename DER::SpColIter colit = spSeq->begcol(); colit != spSeq->endcol(); ++colit)	// iterate over columns
				{
					IT mysize;
					IT * vertices;	
					while(netid <= colit.colid())
					{
						if(netid < colit.colid())	// empty subcolumns
						{
							mysize = 0;
						}
						else
						{
							mysize = colit.nnz();
							vertices = new IT[mysize];
							IT j = 0;
							for(typename DER::SpColIter::NzIter nzit = spSeq->begnz(colit); nzit != spSeq->endnz(colit); ++nzit)
							{
								vertices[j] = nzit.rowid() + moffset; 
								++j;
							}
						} 
						MPI_Gather(&mysize, 1, MPIType<IT>(), gsizes, 1, MPIType<IT>(), 0, commGrid->GetColWorld());
					
						// rbuf, rcount, displs, rtype are only significant at the root
						MPI_Gatherv(vertices, mysize, MPIType<IT>(), NULL, NULL, NULL, MPIType<IT>(), 0, commGrid->GetColWorld());
						
						if(netid == colit.colid())	delete [] vertices;
						++netid;
					}
				}
				while(netid < spSeq->getncol())
				{
					IT mysize = 0; 	
					MPI_Gather(&mysize, 1, MPIType<IT>(), gsizes, 1, MPIType<IT>(), 0, commGrid->GetColWorld());
					MPI_Gatherv(NULL, mysize, MPIType<IT>(), NULL, NULL, NULL, MPIType<IT>(), 0, commGrid->GetColWorld());
					++netid;
				} 
			}
		} // end_if the ith processor column

		MPI_Barrier(commGrid->GetWorld());		// signal the end of ith processor column iteration (so that all processors block)
		if((i == proccols-1) && (commGrid->GetRankInProcCol() == 0))	// if that was the last iteration and we are the column heads
		{
			IT totalnzc = 0;
			MPI_Reduce(&nzc, &totalnzc, 1, MPIType<IT>(), MPI_SUM, 0, commGrid->GetRowWorld());

			if(commGrid->GetRank() == 0)	// I am the master, hence I'll change the first line
			{
				// Don't just open with std::ios_base::app here
				// The std::ios::app flag causes fstream to call seekp(0, ios::end); at the start of every operator<<() inserter
				std::ofstream out(filename.c_str(),std::ios_base::in | std::ios_base::out | std::ios_base::binary);
				out.seekp(0, std::ios_base::beg);
				
				string s;
				std::stringstream strm;
				strm << 0 << " " << totalm << " " << totalnzc << " " << totnnz;
				s = strm.str();
				s.resize(flinelen-1, ' ');	// in case totalnzc has fewer digits than totaln

				out.write(s.c_str(), flinelen-1);
				out.close();
			}
		}
	} // end_for all processor columns
}

//! Handles all sorts of orderings as long as there are no duplicates
//! May perform better when the data is already reverse column-sorted (i.e. in decreasing order)
//! if nonum is true, then numerics are not supplied and they are assumed to be all 1's
template <class IT, class NT, class DER>
template <class HANDLER>
void SpParMat< IT,NT,DER >::ReadDistribute (const string & filename, int master, bool nonum, HANDLER handler, bool transpose, bool pario)
{
#ifdef TAU_PROFILE
   	TAU_PROFILE_TIMER(rdtimer, "ReadDistribute", "void SpParMat::ReadDistribute (const string & , int, bool, HANDLER, bool)", TAU_DEFAULT);
   	TAU_PROFILE_START(rdtimer);
#endif

	ifstream infile;
	FILE * binfile;	// points to "past header" if the file is binary
	int seeklength = 0;
	HeaderInfo hfile;
	if(commGrid->GetRank() == master)	// 1 processor
	{
		hfile = ParseHeader(filename, binfile, seeklength);
	}
	MPI_Bcast(&seeklength, 1, MPI_INT, master, commGrid->commWorld);

	IT total_m, total_n, total_nnz;
	IT m_perproc = 0, n_perproc = 0;

	int colneighs = commGrid->GetGridRows();	// number of neighbors along this processor column (including oneself)
	int rowneighs = commGrid->GetGridCols();	// number of neighbors along this processor row (including oneself)

	IT buffpercolneigh = MEMORYINBYTES / (colneighs * (2 * sizeof(IT) + sizeof(NT)));
	IT buffperrowneigh = MEMORYINBYTES / (rowneighs * (2 * sizeof(IT) + sizeof(NT)));
	if(pario)
	{
		// since all colneighs will be reading the data at the same time
		// chances are they might all read the data that should go to one
		// in that case buffperrowneigh > colneighs * buffpercolneigh 
		// in order not to overflow
		buffpercolneigh /= colneighs; 
		if(seeklength == 0)
			SpParHelper::Print("COMBBLAS: Parallel I/O requested but binary header is corrupted\n");
	}

	// make sure that buffperrowneigh >= buffpercolneigh to cover for this patological case:
	//   	-- all data received by a given column head (by vertical communication) are headed to a single processor along the row
	//   	-- then making sure buffperrowneigh >= buffpercolneigh guarantees that the horizontal buffer will never overflow
	buffperrowneigh = std::max(buffperrowneigh, buffpercolneigh);
	if(std::max(buffpercolneigh * colneighs, buffperrowneigh * rowneighs) > numeric_limits<int>::max())
	{  
		SpParHelper::Print("COMBBLAS: MPI doesn't support sending int64_t send/recv counts or displacements\n");
	}
 
	int * cdispls = new int[colneighs];
	for (IT i=0; i<colneighs; ++i)  cdispls[i] = i*buffpercolneigh;
	int * rdispls = new int[rowneighs];
	for (IT i=0; i<rowneighs; ++i)  rdispls[i] = i*buffperrowneigh;		

	int *ccurptrs = NULL, *rcurptrs = NULL;	
	int recvcount = 0;
	IT * rows = NULL; 
	IT * cols = NULL;
	NT * vals = NULL;

	// Note: all other column heads that initiate the horizontal communication has the same "rankinrow" with the master
	int rankincol = commGrid->GetRankInProcCol(master);	// get master's rank in its processor column
	int rankinrow = commGrid->GetRankInProcRow(master);	
	vector< tuple<IT, IT, NT> > localtuples;

	if(commGrid->GetRank() == master)	// 1 processor
	{		
		if( !hfile.fileexists )
		{
			SpParHelper::Print( "COMBBLAS: Input file doesn't exist\n");
			total_n = 0; total_m = 0;	
			BcastEssentials(commGrid->commWorld, total_m, total_n, total_nnz, master);
			return;
		}
		if (hfile.headerexists && hfile.format == 1) 
		{
			SpParHelper::Print("COMBBLAS: Ascii input with binary headers is not supported");
			total_n = 0; total_m = 0;	
			BcastEssentials(commGrid->commWorld, total_m, total_n, total_nnz, master);
			return;
		}
		if ( !hfile.headerexists )	// no header - ascii file (at this point, file exists)
		{
			infile.open(filename.c_str());
			char comment[256];
			infile.getline(comment,256);
			while(comment[0] == '%')
			{
				infile.getline(comment,256);
			}
			stringstream ss;
			ss << string(comment);
			ss >> total_m >> total_n >> total_nnz;
			if(pario)
			{
				SpParHelper::Print("COMBBLAS: Trying to read binary headerless file in parallel, aborting\n");
				total_n = 0; total_m = 0;	
				BcastEssentials(commGrid->commWorld, total_m, total_n, total_nnz, master);
				return;				
			}
		}
		else // hfile.headerexists && hfile.format == 0
		{
			total_m = hfile.m;
			total_n = hfile.n;
			total_nnz = hfile.nnz;
		}
		m_perproc = total_m / colneighs;
		n_perproc = total_n / rowneighs;
		BcastEssentials(commGrid->commWorld, total_m, total_n, total_nnz, master);
		AllocateSetBuffers(rows, cols, vals,  rcurptrs, ccurptrs, rowneighs, colneighs, buffpercolneigh);

		if(seeklength > 0 && pario)   // sqrt(p) processors also do parallel binary i/o
		{
			IT entriestoread =  total_nnz / colneighs;
			#ifdef IODEBUG
			ofstream oput;
			commGrid->OpenDebugFile("Read", oput);
			oput << "Total nnz: " << total_nnz << " entries to read: " << entriestoread << endl;
			oput.close();
			#endif
			ReadAllMine(binfile, rows, cols, vals, localtuples, rcurptrs, ccurptrs, rdispls, cdispls, m_perproc, n_perproc, 
				rowneighs, colneighs, buffperrowneigh, buffpercolneigh, entriestoread, handler, rankinrow, transpose);
		}
		else	// only this (master) is doing I/O (text or binary)
		{
			IT temprow, tempcol;
			NT tempval;	
			IT ntrow = 0, ntcol = 0; // not transposed row and column index
			char line[1024];
			bool nonumline = nonum;
			IT cnz = 0;
			for(; cnz < total_nnz; ++cnz)
			{	
				int colrec;
				size_t commonindex;
				stringstream linestream;
				if( (!hfile.headerexists) && (!infile.eof()))
				{
					// read one line at a time so that missing numerical values can be detected
					infile.getline(line, 1024);
					linestream << line;
					linestream >> temprow >> tempcol;
					if (!nonum)
					{
						// see if this line has a value
						linestream >> skipws;
						nonumline = linestream.eof();
					}
					--temprow;	// file is 1-based where C-arrays are 0-based
					--tempcol;
					ntrow = temprow;
					ntcol = tempcol;
				}
				else if(hfile.headerexists && (!feof(binfile)) ) 
				{
					handler.binaryfill(binfile, temprow , tempcol, tempval);
				}
				if (transpose)
				{
					IT swap = temprow;
					temprow = tempcol;
					tempcol = swap;
				}
				colrec = std::min(static_cast<int>(temprow / m_perproc), colneighs-1);	// precipient processor along the column
				commonindex = colrec * buffpercolneigh + ccurptrs[colrec];
					
				rows[ commonindex ] = temprow;
				cols[ commonindex ] = tempcol;
				if( (!hfile.headerexists) && (!infile.eof()))
				{
					vals[ commonindex ] = nonumline ? handler.getNoNum(ntrow, ntcol) : handler.read(linestream, ntrow, ntcol); //tempval;
				}
				else if(hfile.headerexists && (!feof(binfile)) ) 
				{
					vals[ commonindex ] = tempval;
				}
				++ (ccurptrs[colrec]);				
				if(ccurptrs[colrec] == buffpercolneigh || (cnz == (total_nnz-1)) )		// one buffer is full, or file is done !
				{
					MPI_Scatter(ccurptrs, 1, MPI_INT, &recvcount, 1, MPI_INT, rankincol, commGrid->colWorld); // first, send the receive counts

					// generate space for own recv data ... (use arrays because vector<bool> is cripled, if NT=bool)
					IT * temprows = new IT[recvcount];
					IT * tempcols = new IT[recvcount];
					NT * tempvals = new NT[recvcount];
					
					// then, send all buffers that to their recipients ...
					MPI_Scatterv(rows, ccurptrs, cdispls, MPIType<IT>(), temprows, recvcount,  MPIType<IT>(), rankincol, commGrid->colWorld);
					MPI_Scatterv(cols, ccurptrs, cdispls, MPIType<IT>(), tempcols, recvcount,  MPIType<IT>(), rankincol, commGrid->colWorld);
					MPI_Scatterv(vals, ccurptrs, cdispls, MPIType<NT>(), tempvals, recvcount,  MPIType<NT>(), rankincol, commGrid->colWorld);

					fill_n(ccurptrs, colneighs, 0);  				// finally, reset current pointers !
					DeleteAll(rows, cols, vals);
					
					HorizontalSend(rows, cols, vals,temprows, tempcols, tempvals, localtuples, rcurptrs, rdispls, 
							buffperrowneigh, rowneighs, recvcount, m_perproc, n_perproc, rankinrow);
					
					if( cnz != (total_nnz-1) )	// otherwise the loop will exit with noone to claim memory back
					{
						// reuse these buffers for the next vertical communication								
						rows = new IT [ buffpercolneigh * colneighs ];
						cols = new IT [ buffpercolneigh * colneighs ];
						vals = new NT [ buffpercolneigh * colneighs ];
					}
				} // end_if for "send buffer is full" case 
			} // end_for for "cnz < entriestoread" case
			assert (cnz == total_nnz);
			
			// Signal the end of file to other processors along the column
			fill_n(ccurptrs, colneighs, numeric_limits<int>::max());	
			MPI_Scatter(ccurptrs, 1, MPI_INT, &recvcount, 1, MPI_INT, rankincol, commGrid->colWorld);

			// And along the row ...
			fill_n(rcurptrs, rowneighs, numeric_limits<int>::max());				
			MPI_Scatter(rcurptrs, 1, MPI_INT, &recvcount, 1, MPI_INT, rankinrow, commGrid->rowWorld);
		}	// end of "else" (only one processor reads) block
	}	// end_if for "master processor" case
	else if( commGrid->OnSameProcCol(master) ) 	// (r-1) processors
	{
		BcastEssentials(commGrid->commWorld, total_m, total_n, total_nnz, master);
		m_perproc = total_m / colneighs;
		n_perproc = total_n / rowneighs;

		if(seeklength > 0 && pario)   // these processors also do parallel binary i/o
		{
			binfile = fopen(filename.c_str(), "rb");
			IT entrysize = handler.entrylength();
			int myrankincol = commGrid->GetRankInProcCol();
			IT perreader = total_nnz / colneighs;
			IT read_offset = entrysize * static_cast<IT>(myrankincol) * perreader + seeklength;
			IT entriestoread = perreader;
			if (myrankincol == colneighs-1) 
				entriestoread = total_nnz - static_cast<IT>(myrankincol) * perreader;
			fseek(binfile, read_offset, SEEK_SET);

			#ifdef IODEBUG
			ofstream oput;
			commGrid->OpenDebugFile("Read", oput);
			oput << "Total nnz: " << total_nnz << " OFFSET : " << read_offset << " entries to read: " << entriestoread << endl;
			oput.close();
			#endif
			
			AllocateSetBuffers(rows, cols, vals,  rcurptrs, ccurptrs, rowneighs, colneighs, buffpercolneigh);
			ReadAllMine(binfile, rows, cols, vals, localtuples, rcurptrs, ccurptrs, rdispls, cdispls, m_perproc, n_perproc, 
				rowneighs, colneighs, buffperrowneigh, buffpercolneigh, entriestoread, handler, rankinrow, transpose);
		}
		else // only master does the I/O
		{
			while(total_n > 0 || total_m > 0)	// otherwise input file does not exist !
			{
				// void MPI::Comm::Scatterv(const void* sendbuf, const int sendcounts[], const int displs[], const MPI::Datatype& sendtype,
				//				void* recvbuf, int recvcount, const MPI::Datatype & recvtype, int root) const
				// The outcome is as if the root executed n send operations,
				//	MPI_Send(sendbuf + displs[i] * extent(sendtype), sendcounts[i], sendtype, i, ...)
				// and each process executed a receive,
				// 	MPI_Recv(recvbuf, recvcount, recvtype, root, ...)
				// The send buffer is ignored for all nonroot processes.
				
				MPI_Scatter(ccurptrs, 1, MPI_INT, &recvcount, 1, MPI_INT, rankincol, commGrid->colWorld);                       // first receive the receive counts ...
				if( recvcount == numeric_limits<int>::max()) break;
				
				// create space for incoming data ... 
				IT * temprows = new IT[recvcount];
				IT * tempcols = new IT[recvcount];
				NT * tempvals = new NT[recvcount];
				
				// receive actual data ... (first 4 arguments are ignored in the receiver side)
				MPI_Scatterv(rows, ccurptrs, cdispls, MPIType<IT>(), temprows, recvcount,  MPIType<IT>(), rankincol, commGrid->colWorld);
				MPI_Scatterv(cols, ccurptrs, cdispls, MPIType<IT>(), tempcols, recvcount,  MPIType<IT>(), rankincol, commGrid->colWorld);
				MPI_Scatterv(vals, ccurptrs, cdispls, MPIType<NT>(), tempvals, recvcount,  MPIType<NT>(), rankincol, commGrid->colWorld);

				// now, send the data along the horizontal
				rcurptrs = new int[rowneighs];
				fill_n(rcurptrs, rowneighs, 0);	
				
				// HorizontalSend frees the memory of temp_xxx arrays and then creates and frees memory of all the six arrays itself
				HorizontalSend(rows, cols, vals,temprows, tempcols, tempvals, localtuples, rcurptrs, rdispls, 
					buffperrowneigh, rowneighs, recvcount, m_perproc, n_perproc, rankinrow);
			}
		}
		
		// Signal the end of file to other processors along the row
		fill_n(rcurptrs, rowneighs, numeric_limits<int>::max());				
		MPI_Scatter(rcurptrs, 1, MPI_INT, &recvcount, 1, MPI_INT, rankinrow, commGrid->rowWorld);
		delete [] rcurptrs;	
	}
	else		// r * (s-1) processors that only participate in the horizontal communication step
	{
		BcastEssentials(commGrid->commWorld, total_m, total_n, total_nnz, master);
		m_perproc = total_m / colneighs;
		n_perproc = total_n / rowneighs;
		while(total_n > 0 || total_m > 0)	// otherwise input file does not exist !
		{
			// receive the receive count
			MPI_Scatter(rcurptrs, 1, MPI_INT, &recvcount, 1, MPI_INT, rankinrow, commGrid->rowWorld);
			if( recvcount == numeric_limits<int>::max())
				break;

			// create space for incoming data ... 
			IT * temprows = new IT[recvcount];
			IT * tempcols = new IT[recvcount];
			NT * tempvals = new NT[recvcount];

			MPI_Scatterv(rows, rcurptrs, rdispls, MPIType<IT>(), temprows, recvcount,  MPIType<IT>(), rankinrow, commGrid->rowWorld);
			MPI_Scatterv(cols, rcurptrs, rdispls, MPIType<IT>(), tempcols, recvcount,  MPIType<IT>(), rankinrow, commGrid->rowWorld);
			MPI_Scatterv(vals, rcurptrs, rdispls, MPIType<NT>(), tempvals, recvcount,  MPIType<NT>(), rankinrow, commGrid->rowWorld);

			// now push what is ours to tuples
			IT moffset = commGrid->myprocrow * m_perproc; 
			IT noffset = commGrid->myproccol * n_perproc;
			
			for(IT i=0; i< recvcount; ++i)
			{					
				localtuples.push_back( 	make_tuple(temprows[i]-moffset, tempcols[i]-noffset, tempvals[i]) );
			}
			DeleteAll(temprows,tempcols,tempvals);
		}
	}
	DeleteAll(cdispls, rdispls);
	tuple<IT,IT,NT> * arrtuples = new tuple<IT,IT,NT>[localtuples.size()];  // the vector will go out of scope, make it stick !
	copy(localtuples.begin(), localtuples.end(), arrtuples);

 	IT localm = (commGrid->myprocrow != (commGrid->grrows-1))? m_perproc: (total_m - (m_perproc * (commGrid->grrows-1)));
 	IT localn = (commGrid->myproccol != (commGrid->grcols-1))? n_perproc: (total_n - (n_perproc * (commGrid->grcols-1)));
	spSeq->Create( localtuples.size(), localm, localn, arrtuples);		// the deletion of arrtuples[] is handled by SpMat::Create

#ifdef TAU_PROFILE
   	TAU_PROFILE_STOP(rdtimer);
#endif
	return;
}

template <class IT, class NT, class DER>
void SpParMat<IT,NT,DER>::AllocateSetBuffers(IT * & rows, IT * & cols, NT * & vals,  int * & rcurptrs, int * & ccurptrs, int rowneighs, int colneighs, IT buffpercolneigh)
{
	// allocate buffers on the heap as stack space is usually limited
	rows = new IT [ buffpercolneigh * colneighs ];
	cols = new IT [ buffpercolneigh * colneighs ];
	vals = new NT [ buffpercolneigh * colneighs ];
	
	ccurptrs = new int[colneighs];
	rcurptrs = new int[rowneighs];
	fill_n(ccurptrs, colneighs, 0);	// fill with zero
	fill_n(rcurptrs, rowneighs, 0);	
}

template <class IT, class NT, class DER>
void SpParMat<IT,NT,DER>::BcastEssentials(MPI_Comm & world, IT & total_m, IT & total_n, IT & total_nnz, int master)
{
	MPI_Bcast(&total_m, 1, MPIType<IT>(), master, world);
	MPI_Bcast(&total_n, 1, MPIType<IT>(), master, world);
	MPI_Bcast(&total_nnz, 1, MPIType<IT>(), master, world);
}
	
/*
 * @post {rows, cols, vals are pre-allocated on the heap after this call} 
 * @post {ccurptrs are set to zero; so that if another call is made to this function without modifying ccurptrs, no data will be send from this procesor}
 */
template <class IT, class NT, class DER>
void SpParMat<IT,NT,DER>::VerticalSend(IT * & rows, IT * & cols, NT * & vals, vector< tuple<IT,IT,NT> > & localtuples, int * rcurptrs, int * ccurptrs, int * rdispls, int * cdispls, 
				  IT m_perproc, IT n_perproc, int rowneighs, int colneighs, IT buffperrowneigh, IT buffpercolneigh, int rankinrow)
{
	// first, send/recv the counts ...
	int * colrecvdispls = new int[colneighs];
	int * colrecvcounts = new int[colneighs];
	MPI_Alltoall(ccurptrs, 1, MPI_INT, colrecvcounts, 1, MPI_INT, commGrid->colWorld);      // share the request counts
	int totrecv = accumulate(colrecvcounts,colrecvcounts+colneighs,0);	
	colrecvdispls[0] = 0; 		// receive displacements are exact whereas send displacements have slack
	for(int i=0; i<colneighs-1; ++i)
		colrecvdispls[i+1] = colrecvdispls[i] + colrecvcounts[i];
	
	// generate space for own recv data ... (use arrays because vector<bool> is cripled, if NT=bool)
	IT * temprows = new IT[totrecv];
	IT * tempcols = new IT[totrecv];
	NT * tempvals = new NT[totrecv];
	
	// then, exchange all buffers that to their recipients ...
	MPI_Alltoallv(rows, ccurptrs, cdispls, MPIType<IT>(), temprows, colrecvcounts, colrecvdispls, MPIType<IT>(), commGrid->colWorld);
	MPI_Alltoallv(cols, ccurptrs, cdispls, MPIType<IT>(), tempcols, colrecvcounts, colrecvdispls, MPIType<IT>(), commGrid->colWorld);
	MPI_Alltoallv(vals, ccurptrs, cdispls, MPIType<NT>(), tempvals, colrecvcounts, colrecvdispls, MPIType<NT>(), commGrid->colWorld);

	// finally, reset current pointers !
	fill_n(ccurptrs, colneighs, 0);
	DeleteAll(colrecvdispls, colrecvcounts);
	DeleteAll(rows, cols, vals);
	
	// rcurptrs/rdispls are zero initialized scratch space
	HorizontalSend(rows, cols, vals,temprows, tempcols, tempvals, localtuples, rcurptrs, rdispls, buffperrowneigh, rowneighs, totrecv, m_perproc, n_perproc, rankinrow);
	
	// reuse these buffers for the next vertical communication								
	rows = new IT [ buffpercolneigh * colneighs ];
	cols = new IT [ buffpercolneigh * colneighs ];
	vals = new NT [ buffpercolneigh * colneighs ];
}


/**
 * Private subroutine of ReadDistribute. 
 * Executed by p_r processors on the first processor column. 
 * @pre {rows, cols, vals are pre-allocated on the heap before this call} 
 * @param[in] rankinrow {row head's rank in its processor row - determines the scatter person} 
 */
template <class IT, class NT, class DER>
template <class HANDLER>
void SpParMat<IT,NT,DER>::ReadAllMine(FILE * binfile, IT * & rows, IT * & cols, NT * & vals, vector< tuple<IT,IT,NT> > & localtuples, int * rcurptrs, int * ccurptrs, int * rdispls, int * cdispls, 
		IT m_perproc, IT n_perproc, int rowneighs, int colneighs, IT buffperrowneigh, IT buffpercolneigh, IT entriestoread, HANDLER handler, int rankinrow, bool transpose)
{
	assert(entriestoread != 0);
	IT cnz = 0;
	IT temprow, tempcol;
	NT tempval;
	int finishedglobal = 1;
	while(cnz < entriestoread && !feof(binfile))	// this loop will execute at least once
	{
		handler.binaryfill(binfile, temprow , tempcol, tempval);
		if (transpose)
		{
			IT swap = temprow;
			temprow = tempcol;
			tempcol = swap;
		}
		int colrec = std::min(static_cast<int>(temprow / m_perproc), colneighs-1);	// precipient processor along the column
		size_t commonindex = colrec * buffpercolneigh + ccurptrs[colrec];
		rows[ commonindex ] = temprow;
		cols[ commonindex ] = tempcol;
		vals[ commonindex ] = tempval;
		++ (ccurptrs[colrec]);	
		if(ccurptrs[colrec] == buffpercolneigh || (cnz == (entriestoread-1)) )		// one buffer is full, or this processor's share is done !
		{			
			#ifdef IODEBUG
			ofstream oput;
			commGrid->OpenDebugFile("Read", oput);
			oput << "To column neighbors: ";
			copy(ccurptrs, ccurptrs+colneighs, ostream_iterator<int>(oput, " ")); oput << endl;
			oput.close();
			#endif

			VerticalSend(rows, cols, vals, localtuples, rcurptrs, ccurptrs, rdispls, cdispls, m_perproc, n_perproc, 
					rowneighs, colneighs, buffperrowneigh, buffpercolneigh, rankinrow);

			if(cnz == (entriestoread-1))	// last execution of the outer loop
			{
				int finishedlocal = 1;	// I am done, but let me check others 
				MPI_Allreduce( &finishedlocal, &finishedglobal, 1, MPI_INT, MPI_BAND, commGrid->colWorld);
				while(!finishedglobal)
				{
					#ifdef DEBUG
					ofstream oput;
					commGrid->OpenDebugFile("Read", oput);
					oput << "To column neighbors: ";
					copy(ccurptrs, ccurptrs+colneighs, ostream_iterator<int>(oput, " ")); oput << endl;
					oput.close();
					#endif

					// postcondition of VerticalSend: ccurptrs are set to zero
					// if another call is made to this function without modifying ccurptrs, no data will be send from this procesor
					VerticalSend(rows, cols, vals, localtuples, rcurptrs, ccurptrs, rdispls, cdispls, m_perproc, n_perproc, 
						rowneighs, colneighs, buffperrowneigh, buffpercolneigh, rankinrow);

					MPI_Allreduce( &finishedlocal, &finishedglobal, 1, MPI_INT, MPI_BAND, commGrid->colWorld);
				}
			}
			else // the other loop will continue executing
			{
				int finishedlocal = 0;
				MPI_Allreduce( &finishedlocal, &finishedglobal, 1, MPI_INT, MPI_BAND, commGrid->colWorld);
			}
		} // end_if for "send buffer is full" case 
		++cnz;
	}

	// signal the end to row neighbors
	fill_n(rcurptrs, rowneighs, numeric_limits<int>::max());				
	int recvcount;
	MPI_Scatter(rcurptrs, 1, MPI_INT, &recvcount, 1, MPI_INT, rankinrow, commGrid->rowWorld);
}


/**
 * Private subroutine of ReadDistribute
 * @param[in] rankinrow {Row head's rank in its processor row}
 * Initially temp_xxx arrays carry data received along the proc. column AND needs to be sent along the proc. row
 * After usage, function frees the memory of temp_xxx arrays and then creates and frees memory of all the six arrays itself
 */
template <class IT, class NT, class DER>
void SpParMat<IT,NT,DER>::HorizontalSend(IT * & rows, IT * & cols, NT * & vals, IT * & temprows, IT * & tempcols, NT * & tempvals, vector < tuple <IT,IT,NT> > & localtuples, 
					 int * rcurptrs, int * rdispls, IT buffperrowneigh, int rowneighs, int recvcount, IT m_perproc, IT n_perproc, int rankinrow)
{	
	rows = new IT [ buffperrowneigh * rowneighs ];
	cols = new IT [ buffperrowneigh * rowneighs ];
	vals = new NT [ buffperrowneigh * rowneighs ];
	
	// prepare to send the data along the horizontal
	for(int i=0; i< recvcount; ++i)
	{
		int rowrec = std::min(static_cast<int>(tempcols[i] / n_perproc), rowneighs-1);
		rows[ rowrec * buffperrowneigh + rcurptrs[rowrec] ] = temprows[i];
		cols[ rowrec * buffperrowneigh + rcurptrs[rowrec] ] = tempcols[i];
		vals[ rowrec * buffperrowneigh + rcurptrs[rowrec] ] = tempvals[i];
		++ (rcurptrs[rowrec]);	
	}

	#ifdef IODEBUG
	ofstream oput;
	commGrid->OpenDebugFile("Read", oput);
	oput << "To row neighbors: ";
	copy(rcurptrs, rcurptrs+rowneighs, ostream_iterator<int>(oput, " ")); oput << endl;
	oput << "Row displacements were: ";
	copy(rdispls, rdispls+rowneighs, ostream_iterator<int>(oput, " ")); oput << endl;
	oput.close();
	#endif

	MPI_Scatter(rcurptrs, 1, MPI_INT, &recvcount, 1, MPI_INT, rankinrow, commGrid->rowWorld); // Send the receive counts for horizontal communication	

	// the data is now stored in rows/cols/vals, can reset temporaries
	// sets size and capacity to new recvcount
	DeleteAll(temprows, tempcols, tempvals);
	temprows = new IT[recvcount];
	tempcols = new IT[recvcount];
	tempvals = new NT[recvcount];
	
	// then, send all buffers that to their recipients ...
	MPI_Scatterv(rows, rcurptrs, rdispls, MPIType<IT>(), temprows, recvcount,  MPIType<IT>(), rankinrow, commGrid->rowWorld);
	MPI_Scatterv(cols, rcurptrs, rdispls, MPIType<IT>(), tempcols, recvcount,  MPIType<IT>(), rankinrow, commGrid->rowWorld);
	MPI_Scatterv(vals, rcurptrs, rdispls, MPIType<NT>(), tempvals, recvcount,  MPIType<NT>(), rankinrow, commGrid->rowWorld);

	// now push what is ours to tuples
	IT moffset = commGrid->myprocrow * m_perproc; 
	IT noffset = commGrid->myproccol * n_perproc; 
	
	for(int i=0; i< recvcount; ++i)
	{					
		localtuples.push_back( 	make_tuple(temprows[i]-moffset, tempcols[i]-noffset, tempvals[i]) );
	}
	
	fill_n(rcurptrs, rowneighs, 0);
	DeleteAll(rows, cols, vals, temprows, tempcols, tempvals);		
}


//! The input parameters' identity (zero) elements as well as 
//! their communication grid is preserved while outputting
template <class IT, class NT, class DER>
void SpParMat<IT,NT,DER>::Find (FullyDistVec<IT,IT> & distrows, FullyDistVec<IT,IT> & distcols, FullyDistVec<IT,NT> & distvals) const
{
	if((*(distrows.commGrid) != *(distcols.commGrid)) || (*(distcols.commGrid) != *(distvals.commGrid)))
	{
		SpParHelper::Print("Grids are not comparable, Find() fails !"); 
		MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
	}
	IT globallen = getnnz();
	SpTuples<IT,NT> Atuples(*spSeq);
	
	FullyDistVec<IT,IT> nrows ( distrows.commGrid, globallen, 0); 
	FullyDistVec<IT,IT> ncols ( distcols.commGrid, globallen, 0); 
	FullyDistVec<IT,NT> nvals ( distvals.commGrid, globallen, NT()); 
	
	IT prelen = Atuples.getnnz();
	//IT postlen = nrows.MyLocLength();

	int rank = commGrid->GetRank();
	int nprocs = commGrid->GetSize();
	IT * prelens = new IT[nprocs];
	prelens[rank] = prelen;
	MPI_Allgather(MPI_IN_PLACE, 0, MPIType<IT>(), prelens, 1, MPIType<IT>(), commGrid->GetWorld());
	IT prelenuntil = accumulate(prelens, prelens+rank, static_cast<IT>(0));

	int * sendcnt = new int[nprocs]();	// zero initialize
	IT * rows = new IT[prelen];
	IT * cols = new IT[prelen];
	NT * vals = new NT[prelen];

	int rowrank = commGrid->GetRankInProcRow();
	int colrank = commGrid->GetRankInProcCol(); 
	int rowneighs = commGrid->GetGridCols();
	int colneighs = commGrid->GetGridRows();
	IT * locnrows = new IT[colneighs];	// number of rows is calculated by a reduction among the processor column
	IT * locncols = new IT[rowneighs];
	locnrows[colrank] = getlocalrows();
	locncols[rowrank] = getlocalcols();

	MPI_Allgather(MPI_IN_PLACE, 0, MPIType<IT>(),locnrows, 1, MPIType<IT>(), commGrid->GetColWorld());
	MPI_Allgather(MPI_IN_PLACE, 0, MPIType<IT>(),locncols, 1, MPIType<IT>(), commGrid->GetRowWorld());

	IT roffset = accumulate(locnrows, locnrows+colrank, static_cast<IT>(0));
	IT coffset = accumulate(locncols, locncols+rowrank, static_cast<IT>(0));
	
	DeleteAll(locnrows, locncols);
	for(int i=0; i< prelen; ++i)
	{
		IT locid;	// ignore local id, data will come in order
		int owner = nrows.Owner(prelenuntil+i, locid);
		sendcnt[owner]++;

		rows[i] = Atuples.rowindex(i) + roffset;	// need the global row index
		cols[i] = Atuples.colindex(i) + coffset;	// need the global col index
		vals[i] = Atuples.numvalue(i);
	}

	int * recvcnt = new int[nprocs];
	MPI_Alltoall(sendcnt, 1, MPI_INT, recvcnt, 1, MPI_INT, commGrid->GetWorld());   // get the recv counts

	int * sdpls = new int[nprocs]();	// displacements (zero initialized pid) 
	int * rdpls = new int[nprocs](); 
	partial_sum(sendcnt, sendcnt+nprocs-1, sdpls+1);
	partial_sum(recvcnt, recvcnt+nprocs-1, rdpls+1);

	MPI_Alltoallv(rows, sendcnt, sdpls, MPIType<IT>(), SpHelper::p2a(nrows.arr), recvcnt, rdpls, MPIType<IT>(), commGrid->GetWorld());
	MPI_Alltoallv(cols, sendcnt, sdpls, MPIType<IT>(), SpHelper::p2a(ncols.arr), recvcnt, rdpls, MPIType<IT>(), commGrid->GetWorld());
	MPI_Alltoallv(vals, sendcnt, sdpls, MPIType<NT>(), SpHelper::p2a(nvals.arr), recvcnt, rdpls, MPIType<NT>(), commGrid->GetWorld());

	DeleteAll(sendcnt, recvcnt, sdpls, rdpls);
	DeleteAll(prelens, rows, cols, vals);
	distrows = nrows;
	distcols = ncols;
	distvals = nvals;
}

//! The input parameters' identity (zero) elements as well as 
//! their communication grid is preserved while outputting
template <class IT, class NT, class DER>
void SpParMat<IT,NT,DER>::Find (FullyDistVec<IT,IT> & distrows, FullyDistVec<IT,IT> & distcols) const
{
	if((*(distrows.commGrid) != *(distcols.commGrid)) )
	{
		SpParHelper::Print("Grids are not comparable, Find() fails !"); 
		MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
	}
	IT globallen = getnnz();
	SpTuples<IT,NT> Atuples(*spSeq);
	
	FullyDistVec<IT,IT> nrows ( distrows.commGrid, globallen, 0); 
	FullyDistVec<IT,IT> ncols ( distcols.commGrid, globallen, 0); 
	
	IT prelen = Atuples.getnnz();

	int rank = commGrid->GetRank();
	int nprocs = commGrid->GetSize();
	IT * prelens = new IT[nprocs];
	prelens[rank] = prelen;
	MPI_Allgather(MPI_IN_PLACE, 0, MPIType<IT>(), prelens, 1, MPIType<IT>(), commGrid->GetWorld());
	IT prelenuntil = accumulate(prelens, prelens+rank, static_cast<IT>(0));

	int * sendcnt = new int[nprocs]();	// zero initialize
	IT * rows = new IT[prelen];
	IT * cols = new IT[prelen];
	NT * vals = new NT[prelen];

	int rowrank = commGrid->GetRankInProcRow();
	int colrank = commGrid->GetRankInProcCol(); 
	int rowneighs = commGrid->GetGridCols();
	int colneighs = commGrid->GetGridRows();
	IT * locnrows = new IT[colneighs];	// number of rows is calculated by a reduction among the processor column
	IT * locncols = new IT[rowneighs];
	locnrows[colrank] = getlocalrows();
	locncols[rowrank] = getlocalcols();

	MPI_Allgather(MPI_IN_PLACE, 0, MPIType<IT>(),locnrows, 1, MPIType<IT>(), commGrid->GetColWorld());
	MPI_Allgather(MPI_IN_PLACE, 0, MPIType<IT>(),locncols, 1, MPIType<IT>(), commGrid->GetColWorld());
	IT roffset = accumulate(locnrows, locnrows+colrank, static_cast<IT>(0));
	IT coffset = accumulate(locncols, locncols+rowrank, static_cast<IT>(0));
	
	DeleteAll(locnrows, locncols);
	for(int i=0; i< prelen; ++i)
	{
		IT locid;	// ignore local id, data will come in order
		int owner = nrows.Owner(prelenuntil+i, locid);
		sendcnt[owner]++;

		rows[i] = Atuples.rowindex(i) + roffset;	// need the global row index
		cols[i] = Atuples.colindex(i) + coffset;	// need the global col index
	}

	int * recvcnt = new int[nprocs];
	MPI_Alltoall(sendcnt, 1, MPI_INT, recvcnt, 1, MPI_INT, commGrid->GetWorld());   // get the recv counts

	int * sdpls = new int[nprocs]();	// displacements (zero initialized pid) 
	int * rdpls = new int[nprocs](); 
	partial_sum(sendcnt, sendcnt+nprocs-1, sdpls+1);
	partial_sum(recvcnt, recvcnt+nprocs-1, rdpls+1);

	MPI_Alltoallv(rows, sendcnt, sdpls, MPIType<IT>(), SpHelper::p2a(nrows.arr), recvcnt, rdpls, MPIType<IT>(), commGrid->GetWorld());
	MPI_Alltoallv(cols, sendcnt, sdpls, MPIType<IT>(), SpHelper::p2a(ncols.arr), recvcnt, rdpls, MPIType<IT>(), commGrid->GetWorld());

	DeleteAll(sendcnt, recvcnt, sdpls, rdpls);
	DeleteAll(prelens, rows, cols, vals);
	distrows = nrows;
	distcols = ncols;
}

template <class IT, class NT, class DER>
ofstream& SpParMat<IT,NT,DER>::put(ofstream& outfile) const
{
	outfile << (*spSeq) << endl;
	return outfile;
}

template <class IU, class NU, class UDER>
ofstream& operator<<(ofstream& outfile, const SpParMat<IU, NU, UDER> & s)
{
	return s.put(outfile) ;	// use the right put() function

}

/**
  * @param[in] grow {global row index}
  * @param[in] gcol {global column index}
  * @param[out] lrow {row index local to the owner}
  * @param[out] lcol {col index local to the owner}
  * @returns {owner processor id}
 **/
template <class IT, class NT,class DER>
int SpParMat<IT,NT,DER>::Owner(IT total_m, IT total_n, IT grow, IT gcol, IT & lrow, IT & lcol) const
{
	int procrows = commGrid->GetGridRows();
	int proccols = commGrid->GetGridCols();
	IT m_perproc = total_m / procrows;
	IT n_perproc = total_n / proccols;

	int own_procrow;	// owner's processor row
	if(m_perproc != 0)
	{
		own_procrow = std::min(static_cast<int>(grow / m_perproc), procrows-1);	// owner's processor row
	}
	else	// all owned by the last processor row
	{
		own_procrow = procrows -1;
	}
	int own_proccol;
	if(n_perproc != 0)
	{
		own_proccol = std::min(static_cast<int>(gcol / n_perproc), proccols-1);
	}
	else
	{
		own_proccol = proccols-1;
	}
	lrow = grow - (own_procrow * m_perproc);
	lcol = gcol - (own_proccol * n_perproc);
	return commGrid->GetRank(own_procrow, own_proccol);
}
