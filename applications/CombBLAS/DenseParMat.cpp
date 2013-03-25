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

#include <numeric>
#include "DenseParMat.h"
#include "MPIType.h"
#include "Operations.h"

using namespace std;

template <class IT, class NT>
template <typename _BinaryOperation>
DenseParVec< IT,NT > DenseParMat<IT,NT>::Reduce(Dim dim, _BinaryOperation __binary_op, NT identity) const
{
	DenseParVec<IT,NT> parvec(commGrid, identity);

	switch(dim)
	{
		case Column:	// pack along the columns, result is a vector of size n
		{
			NT * sendbuf = new NT[n];
			for(int j=0; j < n; ++j)
			{
				sendbuf[j] = identity;
				for(int i=0; i < m; ++i)
				{
					sendbuf[j] = __binary_op(array[i][j], sendbuf[j]); 
				}
			}
			NT * recvbuf = NULL;
			int root = commGrid->GetDiagOfProcCol();
			if(parvec.diagonal)
			{
				parvec.arr.resize(n);
				recvbuf = &parvec.arr[0];	
			}
			MPI_Reduce(sendbuf, recvbuf, n, MPIType<NT>(), MPIOp<_BinaryOperation, NT>::op(), root,commGrid->GetColWorld());
			delete sendbuf;
			break;
		}
		case Row:	// pack along the rows, result is a vector of size m
		{
			NT * sendbuf = new NT[m];
			for(int i=0; i < m; ++i)
			{
				sendbuf[i] = std::accumulate( array[i], array[i]+n, identity, __binary_op);
			}
			NT * recvbuf = NULL;
			int root = commGrid->GetDiagOfProcRow();
			if(parvec.diagonal)
			{
				parvec.arr.resize(m);
				recvbuf = &parvec.arr[0];	
			}
			MPI_Reduce(sendbuf, recvbuf, m, MPIType<NT>(), MPIOp<_BinaryOperation, NT>::op(), root,commGrid->GetRowWorld());
			delete [] sendbuf;
			break;
		}
		default:
		{
			cout << "Unknown reduction dimension, returning empty vector" << endl;
			break;
		}
	}
	return parvec;
}

template <class IT, class NT>
template <typename DER>
DenseParMat< IT,NT > & DenseParMat<IT,NT>::operator+=(const SpParMat< IT,NT,DER > & rhs)		// add a sparse matrix
{
	if(*commGrid == *rhs.commGrid)	
	{
		(rhs.spSeq)->UpdateDense(array, plus<double>());
	}
	else
	{
		cout << "Grids are not comparable elementwise addition" << endl; 
		MPI_Abort(MPI_COMM_WORLD,GRIDMISMATCH);
	}
	return *this;	
}


template <class IT, class NT>
DenseParMat< IT,NT > &  DenseParMat<IT,NT>::operator=(const DenseParMat< IT,NT > & rhs)		// assignment operator
{
	if(this != &rhs)		
	{
		if(array != NULL) 
			SpHelper::deallocate2D(array, m);

		m = rhs.m;
		n = rhs.n;
		if(rhs.array != NULL)	
		{
			array = SpHelper::allocate2D<NT>(m, n);
			for(int i=0; i< m; ++i)
				copy(array[i], array[i]+n, rhs.array[i]);
		}
		commGrid.reset(new CommGrid(*(rhs.commGrid)));		
	}
	return *this;
}


