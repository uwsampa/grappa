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
#define ITERATIONS 10

// Simple helper class for declarations: Just the numerical type is templated 
// The index type and the sequential matrix type stays the same for the whole code
// In this case, they are "int" and "SpDCCols"
template <class NT>
class PSpMat 
{ 
public: 
	typedef SpDCCols < int, NT > DCCols;
	typedef SpParMat < int, NT, DCCols > MPI_DCCols;
};


int main(int argc, char* argv[])
{
	int nprocs, myrank;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);

	if(argc < 5)
	{
		if(myrank == 0)
		{
			cout << "Usage: ./GalerkinNew <Matrix> <OffDiagonal> <Diagonal> <T(right hand side restriction matrix)>" << endl;
			cout << "<Matrix> <OffDiagonal> <Diagonal> <T> are absolute addresses, and files should be in triples format" << endl;
			cout << "Example: ./GalerkinNew TESTDATA/grid3d_k5.txt TESTDATA/offdiag_grid3d_k5.txt TESTDATA/diag_grid3d_k5.txt TESTDATA/restrict_T_grid3d_k5.txt" << endl;
		}
		MPI_Finalize(); 
		return -1;
	}				
	{
		string Aname(argv[1]);		
		string Aoffd(argv[2]);
		string Adiag(argv[3]);
		string Tname(argv[4]);		

		// A = L+D
		// A*T = L*T + D*T;
		// S*(A*T) = S*L*T + S*D*T;
		ifstream inputD(Adiag.c_str());

		MPI_Barrier(MPI_COMM_WORLD);
		typedef PlusTimesSRing<double, double> PTDD;	

		PSpMat<double>::MPI_DCCols A, L, T;	// construct objects
		FullyDistVec<int,double> dvec;
		
		// For matrices, passing the file names as opposed to fstream objects
		A.ReadDistribute(Aname, 0);
		L.ReadDistribute(Aoffd, 0);
		T.ReadDistribute(Tname, 0);
		dvec.ReadDistribute(inputD,0);
		SpParHelper::Print("Data read\n");

		PSpMat<double>::MPI_DCCols S = T;
		S.Transpose();

		// force the calling of C's destructor; warm up instruction cache - also check correctness
		{
			PSpMat<double>::MPI_DCCols AT = PSpGEMM<PTDD>(A, T);
			PSpMat<double>::MPI_DCCols SAT = PSpGEMM<PTDD>(S, AT);

			PSpMat<double>::MPI_DCCols LT = PSpGEMM<PTDD>(L, T); 
			PSpMat<double>::MPI_DCCols SLT = PSpGEMM<PTDD>(S, LT);
			PSpMat<double>::MPI_DCCols SD = S;
			SD.DimApply(Column, dvec, multiplies<double>());	// scale columns of S to get SD
			PSpMat<double>::MPI_DCCols SDT = PSpGEMM<PTDD>(SD, T);
			SLT += SDT;	// now this is SAT

			if(SLT == SAT)
			{
				SpParHelper::Print("Splitting approach is correct\n");
			}
			else
			{
				SpParHelper::Print("Error in splitting, go fix it\n");
				SLT.PrintInfo();	
				SAT.PrintInfo();
				//SLT.SaveGathered("SLT.txt");
				//SAT.SaveGathered("SAT.txt");
			}
		}	
		MPI_Barrier(MPI_COMM_WORLD);
		double t1 = MPI_Wtime(); 	// initilize (wall-clock) timer
		for(int i=0; i<ITERATIONS; i++)
		{
			PSpMat<double>::MPI_DCCols AT = PSpGEMM<PTDD>(A, T);
			PSpMat<double>::MPI_DCCols SAT = PSpGEMM<PTDD>(S, AT);
		}
		MPI_Barrier(MPI_COMM_WORLD);
		double t2 = MPI_Wtime(); 	
		if(myrank == 0)
		{
			cout<<"Full restriction (without splitting) finished"<<endl;	
			printf("%.6lf seconds elapsed per iteration\n", (t2-t1)/(double)ITERATIONS);
		}

		MPI_Barrier(MPI_COMM_WORLD);
		t1 = MPI_Wtime(); 	// initilize (wall-clock) timer
		for(int i=0; i<ITERATIONS; i++)
		{

			PSpMat<double>::MPI_DCCols LT = PSpGEMM<PTDD>(L, T); 
			PSpMat<double>::MPI_DCCols SLT = PSpGEMM<PTDD>(S, LT);
			PSpMat<double>::MPI_DCCols SD = S;
			SD.DimApply(Column, dvec, multiplies<double>());	// scale columns of S to get SD
			PSpMat<double>::MPI_DCCols SDT = PSpGEMM<PTDD>(SD, T);
			SLT += SDT;
		}
		MPI_Barrier(MPI_COMM_WORLD);
		t2 = MPI_Wtime(); 	
		if(myrank == 0)
		{
			cout<<"Full restriction (with splitting) finished"<<endl;	
			printf("%.6lf seconds elapsed per iteration\n", (t2-t1)/(double)ITERATIONS);
		}
		inputD.clear();inputD.close();
	}
	MPI_Finalize();
	return 0;
}

