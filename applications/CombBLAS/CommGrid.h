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

#ifndef _COMM_GRID_H_
#define _COMM_GRID_H_

#include <iostream>
#include <cmath>
#include <cassert>
#include <mpi.h>
#include <sstream>
#include <string>
#include <fstream>
#include <stdint.h>
#include "MPIType.h"
#include "CombBLAS.h"

using namespace std;

class CommGrid
{
public:
	CommGrid(MPI_Comm world, int nrowproc, int ncolproc);

	~CommGrid()
	{
		MPI_Comm_free(&commWorld);
		MPI_Comm_free(&rowWorld);
		MPI_Comm_free(&colWorld);
		if(diagWorld != MPI_COMM_NULL) MPI_Comm_free(&diagWorld);
	}
	CommGrid (const CommGrid & rhs): grrows(rhs.grrows), grcols(rhs.grcols),
			myprocrow(rhs.myprocrow), myproccol(rhs.myproccol), myrank(rhs.myrank) // copy constructor
	{
		MPI_Comm_dup(rhs.commWorld, &commWorld);
		MPI_Comm_dup(rhs.rowWorld, &rowWorld);
		MPI_Comm_dup(rhs.colWorld, &colWorld);

		// don't use the shortcut ternary ? operator, C++ syntax fails as
		// mpich implements MPI::COMM_NULL of different type than MPI::IntraComm
		if(rhs.diagWorld == MPI_COMM_NULL)
			diagWorld = MPI_COMM_NULL;
		else
			MPI_Comm_dup(rhs.diagWorld,&diagWorld);
	}
	
	CommGrid & operator=(const CommGrid & rhs)	// assignment operator
	{
		if(this != &rhs)		
		{
			MPI_Comm_free(&commWorld);
			MPI_Comm_free(&rowWorld);
			MPI_Comm_free(&colWorld);

			grrows = rhs.grrows;
			grcols = rhs.grcols;
			myrank = rhs.myrank;
			myprocrow = rhs.myprocrow;
			myproccol = rhs.myproccol;

			MPI_Comm_dup(rhs.commWorld, &commWorld);
			MPI_Comm_dup(rhs.rowWorld, &rowWorld);
			MPI_Comm_dup(rhs.colWorld, &colWorld);
			
			if(rhs.diagWorld == MPI_COMM_NULL)	diagWorld = MPI_COMM_NULL;
			else	MPI_Comm_dup(rhs.diagWorld,&diagWorld);
		}
		return *this;
	}
	void CreateDiagWorld();
	
	bool operator== (const CommGrid & rhs) const;
	bool operator!= (const CommGrid & rhs) const
	{
		return (! (*this == rhs));
	}
	bool OnSameProcCol( int rhsrank );
	bool OnSameProcRow( int rhsrank );

	int GetRank(int rowrank, int colrank) { return rowrank * grcols + colrank; }	
	int GetRank(int diagrank) { return diagrank * grcols + diagrank; }
	int GetRank() { return myrank; }
	int GetRankInProcRow() { return myproccol; }
	int GetRankInProcCol() { return myprocrow; }
	int GetDiagRank()
	{
		int rank;
		MPI_Comm_rank(diagWorld, &rank);
		return rank;
	}

	int GetRankInProcRow(int wholerank);
	int GetRankInProcCol(int wholerank);

	int GetDiagOfProcRow();
	int GetDiagOfProcCol();

	int GetComplementRank()	// For P(i,j), get rank of P(j,i)
	{
		return ((grcols * myproccol) + myprocrow);
	}
	
	MPI_Comm & GetWorld() { return commWorld; }
	MPI_Comm & GetRowWorld() { return rowWorld; }
	MPI_Comm & GetColWorld() { return colWorld; }
	MPI_Comm & GetDiagWorld() { return diagWorld; }
	MPI_Comm GetWorld() const { return commWorld; }
	MPI_Comm GetRowWorld() const { return rowWorld; }
	MPI_Comm GetColWorld() const { return colWorld; }
	MPI_Comm GetDiagWorld() const { return diagWorld; }

	int GetGridRows() { return grrows; }
	int GetGridCols() { return grcols; }
	int GetSize() { return grrows * grcols; }
	int GetDiagSize() 
	{ 
		int size;
		MPI_Comm_size(diagWorld, &size);
		return size;
	}

	void OpenDebugFile(string prefix, ofstream & output) const; 

	friend shared_ptr<CommGrid> ProductGrid(CommGrid * gridA, CommGrid * gridB, int & innerdim, int & Aoffset, int & Boffset);
private:
	// A "normal" MPI-1 communicator is an intracommunicator; MPI::COMM_WORLD is also an MPI::Intracomm object
	MPI_Comm commWorld, rowWorld, colWorld, diagWorld;

	// Processor grid is (grrow X grcol)
	int grrows, grcols;
	int myprocrow;
	int myproccol;
	int myrank;
	
	template <class IT, class NT, class DER>
	friend class SpParMat;
};

#endif
