#include <sys/time.h> 
#include <iostream>
#include <functional>
#include <algorithm>
#include <vector>
#include <sstream>
#include "../CombBLAS.h"

using namespace std;

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
			cout << "Usage: ./IteratorTest <BASEADDRESS> <Matrix>" << endl;
			cout << "Input file <Matrix> should be under <BASEADDRESS> in triples format" << endl;
		}
		MPI_Finalize(); 
		return -1;
	}				
	{
		string directory(argv[1]);		
		string name(argv[2]);
		name = directory+"/"+name;

		typedef SpParMat <int, double, SpDCCols<int,double> > PARMAT;

		PARMAT A;
		A.ReadDistribute(name, 0);	// read it from file

		int count = 0;	
		int total = 0;
	
		for(SpDCCols<int,double>::SpColIter colit = A.seq().begcol(); colit != A.seq().endcol(); ++colit)	// iterate over columns
		{
			for(SpDCCols<int,double>::SpColIter::NzIter nzit = A.seq().begnz(colit); nzit != A.seq().endnz(colit); ++nzit)
			{	
				// cout << nzit.rowid() << '\t' << colit.colid() << '\t' << nzit.value() << '\n';	
				count++;
			}
		}	
		MPI_Allreduce( &count, &total, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
		
		if(total == A.getnnz())
			SpParHelper::Print( "Iteration passed soft test\n");
		else
			SpParHelper::Print( "Iteration failed !!!\n") ;
		
	}
	MPI_Finalize();
	return 0;
}

