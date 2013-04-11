#include <mpi.h>
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

	if(argc < 4)
	{
		if(myrank == 0)
		{
			cout << "Usage: ./TransposeTest <BASEADDRESS> <Matrix> <MatrixTranspose>" << endl;
			cout << "Input file <Matrix> and <MatrixTranspose> should be under <BASEADDRESS> in triples format" << endl;
		}
		MPI_Finalize(); 
		return -1;
	}				
	{
		string directory(argv[1]);		
		string normalname(argv[2]);
		string transname(argv[3]);
		normalname = directory+"/"+normalname;
		transname = directory+"/"+transname;

		typedef SpParMat <int, bool, SpDCCols<int,bool> > PARBOOLMAT;

		PARBOOLMAT A, AT, ATControl;		// construct object
		A.ReadDistribute(normalname, 0);	// read it from file, note that we use the transpose of "input" data
		AT = A;
		AT.Transpose();

		ATControl.ReadDistribute(transname, 0);
		if (ATControl == AT)
		{
			SpParHelper::Print("Transpose working correctly\n");	
		}
		else
		{
			SpParHelper::Print("ERROR in transpose, go fix it!\n");	
		}
	}
	MPI_Finalize();
	return 0;
}
