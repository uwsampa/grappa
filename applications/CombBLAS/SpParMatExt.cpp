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

//! This file is a placeholder for some obsolete 
//! or lesser used functions that take diagonally distributed vectors as inputs


template <class IT, class NT, class DER>
void SpParMat<IT,NT,DER>::DimScale(const DenseParVec<IT,NT> & v, Dim dim)
{
	switch(dim)
	{
		case Column:	// scale each "Column", using a row vector
		{
			// Diagonal processor broadcast data so that everyone gets the scaling vector 
			NT * scaler = NULL;
			int root = commGrid->GetDiagOfProcCol();
			if(v.diagonal)
			{	
				scaler = const_cast<NT*>(SpHelper::p2a(v.arr));	
			}
			else
			{	
				scaler = new NT[getlocalcols()];	
			}
			MPI_Bcast(scaler, getlocalcols(), MPIType<NT>(), root, commGrid->GetColWorld());	

			for(typename DER::SpColIter colit = spSeq->begcol(); colit != spSeq->endcol(); ++colit)	// iterate over columns
			{
				for(typename DER::SpColIter::NzIter nzit = spSeq->begnz(colit); nzit != spSeq->endnz(colit); ++nzit)
				{
					nzit.value() *=  scaler[colit.colid()];
				}
			}
			if(!v.diagonal)	delete [] scaler;
			break;
		}
		case Row:
		{
			NT * scaler = NULL;
			int root = commGrid->GetDiagOfProcRow();
			if(v.diagonal)
			{	
				scaler = const_cast<NT*>(SpHelper::p2a(v.arr));	
			}
			else
			{	
				scaler = new NT[getlocalrows()];	
			}
			MPI_Bcast(scaler, getlocalrows(), MPIType<NT>(), root, commGrid->GetRowWorld());	

			for(typename DER::SpColIter colit = spSeq->begcol(); colit != spSeq->endcol(); ++colit)
			{
				for(typename DER::SpColIter::NzIter nzit = spSeq->begnz(colit); nzit != spSeq->endnz(colit); ++nzit)
				{
					nzit.value() *= scaler[nzit.rowid()];
				}
			}
			if(!v.diagonal)	delete [] scaler;			
			break;
		}
		default:
		{
			cout << "Unknown scaling dimension, returning..." << endl;
			break;
		}
	}
}


/** 
 * Generalized sparse matrix indexing (ri/ci are 0-based indexed)
 * Both the storage and the actual values in SpParVec should be IT
 * The index vectors are distributed on diagonal processors
 * We can use this function to apply a permutation like A(p,q) 
 * Sequential indexing subroutine (via multiplication) is general enough.
 */
template <class IT, class NT, class DER>
SpParMat<IT,NT,DER> SpParMat<IT,NT,DER>::operator() (const SpParVec<IT,IT> & ri, const SpParVec<IT,IT> & ci) const
{
	// We create two boolean matrices P and Q
	// Dimensions:  P is size(ri) x m
	//		Q is n x size(ci) 

	IT colneighs = commGrid->GetGridRows();	// number of neighbors along this processor column (including oneself)
	IT rowneighs = commGrid->GetGridCols();	// number of neighbors along this processor row (including oneself)
	IT totalm = getnrow();	// collective call
	IT totaln = getncol();
	IT m_perproc = totalm / rowneighs;	// these are CORRECT, as P's column dimension is m
	IT n_perproc = totaln / colneighs;	// and Q's row dimension is n
	IT p_nnz, q_nnz, rilen, cilen; 
	IT * pcnts;
	IT * qcnts;
	
	IT diaginrow = commGrid->GetDiagOfProcRow();
	IT diagincol = commGrid->GetDiagOfProcCol();

	// infer the concrete type SpMat<IT,IT>
	typedef typename create_trait<DER, IT, NT>::T_inferred DER_IT;
	DER_IT * PSeq;
	DER_IT * QSeq;

	IT diagneigh = commGrid->GetComplementRank();
	IT mylocalrows = getlocalrows();
	IT mylocalcols = getlocalcols();
	IT trlocalrows, trlocalcols;
	MPI_Status status;
	MPI_Sendrecv(&mylocalrows, 1, MPIType<IT>(), diagneigh, TRROWX, &trlocalrows, 1, MPIType<IT>(), diagneigh, TRROWX, commGrid->GetWorld(), &status);
	MPI_Sendrecv(&mylocalcols, 1, MPIType<IT>(), diagneigh, TRCOLX, &trlocalcols, 1, MPIType<IT>(), diagneigh, TRCOLX, commGrid->GetWorld(), &status);

	if(ri.diagonal)		// only the diagonal processors hold vectors
	{
		// broadcast the size 
		rilen = ri.ind.size();
		cilen = ci.ind.size();
		MPI_Bcast(&rilen, 1, MPIType<IT>(), diaginrow, commGrid->rowWorld);
		MPI_Bcast(&cilen, 1, MPIType<IT>(), diagincol, commGrid->colWorld);

		vector< vector<IT> > rowdata_rowid(rowneighs);
		vector< vector<IT> > rowdata_colid(rowneighs);
		vector< vector<IT> > coldata_rowid(colneighs);
		vector< vector<IT> > coldata_colid(colneighs);

		IT locvecr = ri.ind.size();	// nnz in local vector
		for(IT i=0; i < locvecr; ++i)
		{	
			// numerical values (permutation indices) are 0-based
			IT rowrec = std::min(ri.num[i] / m_perproc, rowneighs-1);	// recipient processor along the column

			// ri's numerical values give the colids and its indices give rowids
			// thus, the rowid's are already offset'd where colid's are not
			rowdata_rowid[rowrec].push_back(ri.ind[i]);
			rowdata_colid[rowrec].push_back(ri.num[i] - (rowrec * m_perproc));
		}
		pcnts = new IT[rowneighs];
		for(IT i=0; i<rowneighs; ++i)
			pcnts[i] = rowdata_rowid[i].size();

		// Now, do it for ci
		IT locvecc = ci.ind.size();	
		for(IT i=0; i < locvecc; ++i)
		{	
			// numerical values (permutation indices) are 0-based
			IT colrec = std::min(ci.num[i] / n_perproc, colneighs-1);	// recipient processor along the column

			// ci's numerical values give the rowids and its indices give colids
			// thus, the colid's are already offset'd where rowid's are not
			coldata_rowid[colrec].push_back(ci.num[i] - (colrec * n_perproc));
			coldata_colid[colrec].push_back(ci.ind[i]);
		}

		qcnts = new IT[colneighs];
		for(IT i=0; i<colneighs; ++i)
			qcnts[i] = coldata_rowid[i].size();

		// the second parameter, sendcount, is the number of elements sent to *each* processor
		MPI_Scatter(pcnts, 1, MPIType<IT>(), &p_nnz, 1, MPIType<IT>(), diaginrow, (commGrid->rowWorld));
		MPI_Scatter(qcnts, 1, MPIType<IT>(), &q_nnz, 1, MPIType<IT>(), diagincol, (commGrid->colWorld));

		for(IT i=0; i<rowneighs; ++i)
		{
			if(i != diaginrow)	// destination is not me	
			{
				MPI_Send(SpHelper::p2a(rowdata_rowid[i]), pcnts[i], MPIType<IT>(), i, RFROWIDS, commGrid->rowWorld); 
				MPI_Send(SpHelper::p2a(rowdata_colid[i]), pcnts[i], MPIType<IT>(), i, RFCOLIDS, commGrid->rowWorld);
			}
		}

		for(IT i=0; i<colneighs; ++i)
		{
			if(i != diagincol)	// destination is not me	
			{
				MPI_Send(SpHelper::p2a(coldata_rowid[i]), qcnts[i], MPIType<IT>(), i, RFROWIDS, commGrid->colWorld); 
				MPI_Send(SpHelper::p2a(coldata_colid[i]), qcnts[i], MPIType<IT>(), i, RFCOLIDS, commGrid->colWorld); 
			}
		}
		DeleteAll(pcnts, qcnts);

		tuple<IT,IT,bool> * p_tuples = new tuple<IT,IT,bool>[p_nnz]; 
		for(IT i=0; i< p_nnz; ++i)
		{
			p_tuples[i] = make_tuple(rowdata_rowid[diaginrow][i], rowdata_colid[diaginrow][i], 1);
		}

		tuple<IT,IT,bool> * q_tuples = new tuple<IT,IT,bool>[q_nnz]; 
		for(IT i=0; i< q_nnz; ++i)
		{
			q_tuples[i] = make_tuple(coldata_rowid[diagincol][i], coldata_colid[diagincol][i], 1);
		}

		PSeq = new DER_IT(); 
		PSeq->Create( p_nnz, rilen, trlocalrows, p_tuples);		// deletion of tuples[] is handled by SpMat::Create

		QSeq = new DER_IT();  
		QSeq->Create( q_nnz, trlocalcols, cilen, q_tuples);		// deletion of tuples[] is handled by SpMat::Create

	}
	else	// all others receive data from the diagonal
	{
		MPI_Bcast(&rilen, 1, MPIType<IT>(), diaginrow, commGrid->rowWorld);
		MPI_Bcast(&cilen, 1, MPIType<IT>(), diagincol, commGrid->colWorld);

		// receive the receive counts ...
		MPI_Scatter(pcnts, 1, MPIType<IT>(), &p_nnz, 1, MPIType<IT>(), diaginrow, commGrid->rowWorld);
		MPI_Scatter(qcnts, 1, MPIType<IT>(), &q_nnz, 1, MPIType<IT>(), diagincol, commGrid->colWorld);
		
		// create space for incoming data ... 
		IT * p_rows = new IT[p_nnz];
		IT * p_cols = new IT[p_nnz];
		IT * q_rows = new IT[q_nnz];
		IT * q_cols = new IT[q_nnz];
		
		// receive actual data ... 
		MPI_Recv(p_rows, p_nnz, MPIType<IT>(), diaginrow, RFROWIDS, commGrid->rowWorld);	
		MPI_Recv(p_cols, p_nnz, MPIType<IT>(), diaginrow, RFCOLIDS, commGrid->rowWorld);	
	
		MPI_Recv(q_rows, q_nnz, MPIType<IT>(), diagincol, RFROWIDS, commGrid->colWorld);	
		MPI_Recv(q_cols, q_nnz, MPIType<IT>(), diagincol, RFCOLIDS, commGrid->colWorld);	

		tuple<IT,IT,bool> * p_tuples = new tuple<IT,IT,bool>[p_nnz]; 
		for(IT i=0; i< p_nnz; ++i)
		{
			p_tuples[i] = make_tuple(p_rows[i], p_cols[i], 1);
		}

		tuple<IT,IT,bool> * q_tuples = new tuple<IT,IT,bool>[q_nnz]; 
		for(IT i=0; i< q_nnz; ++i)
		{
			q_tuples[i] = make_tuple(q_rows[i], q_cols[i], 1);
		}
		DeleteAll(p_rows, p_cols, q_rows, q_cols);

		PSeq = new DER_IT(); 
		PSeq->Create( p_nnz, rilen, trlocalrows, p_tuples);		// deletion of tuples[] is handled by SpMat::Create

		QSeq = new DER_IT();  
		QSeq->Create( q_nnz, trlocalcols, cilen, q_tuples);		// deletion of tuples[] is handled by SpMat::Create
	}
	
	// Distributed matrix generation (collective call)
	SpParMat<IT,bool,DER_IT> P (PSeq, commGrid);
	SpParMat<IT,bool,DER_IT> Q (QSeq, commGrid);

	// Do parallel matrix-matrix multiply
	typedef PlusTimesSRing<bool, NT> PTBOOLNT;
	typedef PlusTimesSRing<NT, bool> PTNTBOOL;

        return Mult_AnXBn_Synch<PTNTBOOL, NT, DER>(Mult_AnXBn_Synch<PTBOOLNT, NT, DER>(P, *this), Q);
}


