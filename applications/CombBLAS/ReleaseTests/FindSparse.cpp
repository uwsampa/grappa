/****************************************************************/
/* Parallel Combinatorial BLAS Library (for Graph Computations) */
/* version 1.3 -------------------------------------------------*/
/* date: 2/1/2013 ----------------------------------------------*/
/* authors: Aydin Buluc (abuluc@lbl.gov), Adam Lugowski --------*/
/****************************************************************/
/*
 Copyright (c) 2010-, Aydin Buluc
 
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

#include <mpi.h>
#include <sys/time.h> 
#include <iostream>
#include <functional>
#include <algorithm>
#include <vector>
#include <sstream>
#include "../CombBLAS.h"

using namespace std;

template <typename PARMAT>
void Symmetricize(PARMAT & A)
{
	// boolean addition is practically a "logical or"
	// therefore this doesn't destruct any links
	PARMAT AT = A;
	AT.Transpose();
	A += AT;
}

int main(int argc, char* argv[])
{
	int nprocs, myrank;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);

	if(argc < 3)
	{
		if(myrank == 0)
		{
			cout << "Usage: ./FindSparse <BASEADDRESS> <Matrix>" << endl;
			cout << "Input files should be under <BASEADDRESS> in appropriate format" << endl;
		}
		MPI_Finalize();
		return -1;
	}				
	{
		string directory(argv[1]);		
		string matrixname(argv[2]);
		matrixname = directory+"/"+matrixname;
	
		typedef SpParMat <int, double, SpDCCols<int,double> > PARDBMAT;
		PARDBMAT A;		// declare objects
		FullyDistVec<int,int> crow, ccol;
		FullyDistVec<int,double> cval;

		A.ReadDistribute(matrixname, 0);	

		A.Find(crow, ccol, cval);
		PARDBMAT B(A.getnrow(), A.getncol(), crow, ccol, cval); // Sparse()

		if (A ==  B)
		{
			SpParHelper::Print("Find and Sparse working correctly\n");	
		}
		else
		{
			SpParHelper::Print("ERROR in Find(), go fix it!\n");	

			SpParHelper::Print("Rows array: \n");
			crow.DebugPrint();

			SpParHelper::Print("Columns array: \n");
			ccol.DebugPrint();

			SpParHelper::Print("Values array: \n");
			cval.DebugPrint();
		}
		
		// Begin testing a Matlab like use case
		// Example provided by Arne De Coninck <arne.deconinck@ugent.be>
		// X = ones(size(A,2),1) 
		// M = [X' * X, X' * A; A'* X, A' * A]
		
		A.PrintInfo();
		Symmetricize(A);
		FullyDistVec<int,int> rowsym, colsym;
		FullyDistVec<int,double> valsym;
		A.Find(rowsym, colsym, valsym);
		
		FullyDistVec<int,double> colsums; 
		A.Reduce(colsums, Column, plus<double>(), 0.0);
		
		FullyDistVec<int,double> numcols(1, A.getncol());

	#if defined(COMBBLAS_TR1) || defined(COMBBLAS_BOOST) || defined(NOTGNU)
		vector< FullyDistVec<int,double> > vals2concat;
		vals2concat.push_back(numcols);
		vals2concat.push_back(colsums);
		vals2concat.push_back(colsums);
		vals2concat.push_back(valsym);
	#else
		vector< FullyDistVec<int,double> > vals2concat{numcols, colsums, colsums, valsym};
	#endif
		FullyDistVec<int,double> nval = Concatenate(vals2concat);
		nval.PrintInfo("Values:");
		
		// sparse() expects a zero based index		
		FullyDistVec<int,int> firstrowrids(A.getncol()+1, 0);	// M(1,:)
		FullyDistVec<int,int> firstcolrids(A.getncol(), 0);	// M(2:end,1)
		firstcolrids.iota(A.getncol(),1);	// fill M(2:end,1)'s row ids
		rowsym.Apply(bind2nd(plus<int>(), 1));

	#if defined(COMBBLAS_TR1) || defined(COMBBLAS_BOOST) || defined(NOTGNU)
		vector< FullyDistVec<int,int> > rows2concat;
		rows2concat.push_back(firstrowrids);
		rows2concat.push_back(firstcolrids);
		rows2concat.push_back(rowsym);
	#else	
		vector< FullyDistVec<int,int> > rows2concat{firstrowrids, firstcolrids, rowsym};	// C++11 style
	#endif
		FullyDistVec<int,int> nrow = Concatenate(rows2concat);
		nrow.PrintInfo("Row ids:");
		
		FullyDistVec<int,int> firstrowcids(A.getncol()+1, 0);	// M(1,:)
		firstrowcids.iota(A.getncol()+1,0);	// fill M(1,:)'s column ids
		FullyDistVec<int,int> firstcolcids(A.getncol(), 0);	// M(2:end,1)
		colsym.Apply(bind2nd(plus<int>(), 1));

	#if defined(COMBBLAS_TR1) || defined(COMBBLAS_BOOST) || defined(NOTGNU)
		vector< FullyDistVec<int,int> > cols2concat;
		cols2concat.push_back(firstrowcids);
		cols2concat.push_back(firstcolcids);
		cols2concat.push_back(colsym);
	#else
		vector< FullyDistVec<int,int> > cols2concat{firstrowcids, firstcolcids, colsym}; // C++11 style
	#endif
		FullyDistVec<int,int> ncol = Concatenate(cols2concat);
		ncol.PrintInfo("Column ids:");
		
		PARDBMAT M(A.getnrow()+1, A.getncol()+1, nrow, ncol, nval); // Sparse()
		M.PrintInfo();
		
		// End Arne's test case 
	}
	MPI_Finalize();
	return 0;
}
