#include <mpi.h>
#include <sys/time.h> 
#include <iostream>
#include <functional>
#include <algorithm>
#include <vector>
#include <sstream>
#include "../CombBLAS.h"

using namespace std;

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

	if(argc < 4)
	{
		if(myrank == 0)
		{
			cout << "Usage: ./ReduceTest <MatrixA> <SumColumns> <SumRows>" << endl;
			cout << "<Matrix>,<SumColumns>,<SumRows> are absolute addresses, and files should be in triples format" << endl;
		}
		MPI_Finalize(); 
		return -1;
	}				
	{
		string Aname(argv[1]);		
		string Bname(argv[2]);
		string Cname(argv[3]);

		ifstream inputB(Bname.c_str());
		ifstream inputC(Cname.c_str());
		MPI_Barrier(MPI_COMM_WORLD);
		
		PSpMat<double>::MPI_DCCols A;	
		FullyDistVec<int,double> colsums(A.getcommgrid());
		FullyDistVec<int,double> rowsums(A.getcommgrid());

		A.ReadDistribute(Aname, 0);
		colsums.ReadDistribute(inputB, 0);
		rowsums.ReadDistribute(inputC, 0);
		
		FullyDistVec< int, double > rowsums_control, colsums_control;
		A.Reduce(rowsums_control, Row, std::plus<double>() , 0.0);
		A.Reduce(colsums_control, Column, std::plus<double>() , 0.0);
		
		if (rowsums_control == rowsums && colsums_control == colsums)
		{
			SpParHelper::Print("Reduction via summation working correctly\n");	
		}
		else
		{
			SpParHelper::Print("ERROR in Reduce via summation, go fix it!\n");	
		}

		inputB.clear();
		inputB.close();
		inputC.clear();
		inputC.close();
	}
	MPI_Finalize();
	return 0;
}


