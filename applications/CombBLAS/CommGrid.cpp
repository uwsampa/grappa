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

#include "CombBLAS.h"

CommGrid::CommGrid(MPI_Comm world, int nrowproc, int ncolproc): grrows(nrowproc), grcols(ncolproc)
{
	MPI_Comm_dup(world, &commWorld);
	MPI_Comm_rank(commWorld, &myrank);
	int nproc;
	MPI_Comm_size(commWorld,&nproc);

	if(grrows == 0 && grcols == 0)
	{
		grrows = (int)std::sqrt((float)nproc);
		grcols = grrows;

		if(grcols * grrows != nproc)
		{
			cerr << "This version of the Combinatorial BLAS only works on a square logical processor grid" << endl;
			MPI_Abort(MPI_COMM_WORLD,NOTSQUARE);
		}
	}
	assert((nproc == (grrows*grcols)));

	myproccol =  (int) (myrank % grcols);
	myprocrow =  (int) (myrank / grcols);
		
	/** 
	  * Create row and column communicators (must be collectively called)
	  * C syntax: int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm)
	  * C++ syntax: MPI::Intercomm MPI::Intercomm::Split(int color, int key) consts  
	  * Semantics: Processes with the same color are in the same new communicator 
	  */
	MPI_Comm_split(commWorld,myprocrow, myrank,&rowWorld);
	MPI_Comm_split(commWorld,myproccol, myrank,&colWorld);
	CreateDiagWorld();

	int rowRank, colRank;
	MPI_Comm_rank(rowWorld,&rowRank);
	MPI_Comm_rank(colWorld,&colRank);
	assert( (rowRank == myproccol) );
	assert( (colRank == myprocrow) );
}

void CommGrid::CreateDiagWorld()
{
	if(grrows != grcols)	
	{
		cout << "The grid is not square... !" << endl;
		cout << "Returning diagworld to everyone instead of the diagonal" << endl;
		diagWorld = commWorld;
		return;
	}
	int * process_ranks = new int[grcols];
	for(int i=0; i < grcols; ++i)
	{
		process_ranks[i] = i*grcols + i;
	}
	MPI_Group group;
	MPI_Comm_group(commWorld,&group);
	MPI_Group diag_group;
	MPI_Group_incl(group,grcols, process_ranks, &diag_group); // int MPI_Group_incl(MPI_Group group, int n, int *ranks, MPI_Group *newgroup)
	MPI_Group_free(&group);
	delete [] process_ranks;

	// The Create() function returns MPI_COMM_NULL to processes that are NOT in group	
	MPI_Comm_create(commWorld,diag_group,&diagWorld);
	MPI_Group_free(&diag_group);
}

bool CommGrid::OnSameProcCol( int rhsrank)
{
	return ( myproccol == ((int) (rhsrank % grcols)) );
} 

bool CommGrid::OnSameProcRow( int rhsrank)
{
	return ( myprocrow == ((int) (rhsrank / grcols)) );
} 

//! Return rank in the column world
int CommGrid::GetRankInProcCol( int wholerank)
{
	return ((int) (wholerank / grcols));
} 

//! Return rank in the row world
int CommGrid::GetRankInProcRow( int wholerank)
{
	return ((int) (wholerank % grcols));
}

//! Get the rank of the diagonal processor in that particular row 
//! In the ith processor row, the diagonal processor is the ith processor within that row 
int CommGrid::GetDiagOfProcRow( )
{
	return myprocrow;
}

//! Get the rank of the diagonal processor in that particular col 
//! In the ith processor col, the diagonal processor is the ith processor within that col
int CommGrid::GetDiagOfProcCol( )
{
	return myproccol;
}

bool CommGrid::operator== (const CommGrid & rhs) const
{
        int result;
	MPI_Comm_compare(commWorld, rhs.commWorld, &result);
	if ((result != MPI_IDENT) && (result != MPI_CONGRUENT))
	{
		// A call to MPI::Comm::Compare after MPI::Comm::Dup returns MPI_CONGRUENT
		// MPI::CONGRUENT means the communicators have the same group members, in the same order
    		return false;
	}
	return ( (grrows == rhs.grrows) && (grcols == rhs.grcols) && (myprocrow == rhs.myprocrow) && (myproccol == rhs.myproccol));
}	


void CommGrid::OpenDebugFile(string prefix, ofstream & output) const 
{
	stringstream ss;
	string rank;
	ss << myrank;
	ss >> rank;
	string ofilename = prefix;
	ofilename += rank;
	output.open(ofilename.c_str(), ios_base::app );
}

shared_ptr<CommGrid> ProductGrid(CommGrid * gridA, CommGrid * gridB, int & innerdim, int & Aoffset, int & Boffset)
{
	if(gridA->grcols != gridB->grrows)
	{
		cout << "Grids don't confirm for multiplication" << endl;
		MPI_Abort(MPI_COMM_WORLD,GRIDMISMATCH);
	}
	innerdim = gridA->grcols;
	Aoffset = (gridA->myprocrow + gridA->myproccol) % gridA->grcols;	// get sequences that avoids contention
	Boffset = (gridB->myprocrow + gridB->myproccol) % gridB->grrows;
		
	MPI_Comm world = MPI_COMM_WORLD;
	return shared_ptr<CommGrid>( new CommGrid(world, gridA->grrows, gridB->grcols) );
}

