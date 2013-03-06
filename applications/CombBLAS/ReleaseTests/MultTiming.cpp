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

	if(argc < 3)
	{
		if(myrank == 0)
		{
			cout << "Usage: ./MultTest <MatrixA> <MatrixB>" << endl;
			cout << "<MatrixA>,<MatrixB> are absolute addresses, and files should be in triples format" << endl;
		}
		MPI_Finalize(); 
		return -1;
	}				
	{
		string Aname(argv[1]);		
		string Bname(argv[2]);
		typedef PlusTimesSRing<double, double> PTDOUBLEDOUBLE;	
		PSpMat<double>::MPI_DCCols A, B;	// construct objects
		
		A.ReadDistribute(Aname, 0);
		B.ReadDistribute(Bname, 0);
		SpParHelper::Print("Data read\n");

		{ // force the calling of C's destructor
			PSpMat<double>::MPI_DCCols C = Mult_AnXBn_DoubleBuff<PTDOUBLEDOUBLE, double, PSpMat<double>::DCCols >(A, B);
			int64_t cnnz = C.getnnz();
			ostringstream tinfo;
			tinfo << "C has a total of " << cnnz << " nonzeros" << endl;
			SpParHelper::Print(tinfo.str());
			SpParHelper::Print("Warmed up for DoubleBuff\n");
			
		}	
		MPI_Barrier(MPI_COMM_WORLD);
		MPI_Pcontrol(1,"SpGEMM_DoubleBuff");
		double t1 = MPI_Wtime(); 	// initilize (wall-clock) timer
		for(int i=0; i<ITERATIONS; i++)
		{
			PSpMat<double>::MPI_DCCols C = Mult_AnXBn_DoubleBuff<PTDOUBLEDOUBLE, double, PSpMat<double>::DCCols >(A, B);
		}
		MPI_Barrier(MPI_COMM_WORLD);
		double t2 = MPI_Wtime(); 	
		MPI_Pcontrol(-1,"SpGEMM_DoubleBuff");
		if(myrank == 0)
		{
			cout<<"Double buffered multiplications finished"<<endl;	
			printf("%.6lf seconds elapsed per iteration\n", (t2-t1)/(double)ITERATIONS);
		}

		{// force the calling of C's destructor
			PSpMat<double>::MPI_DCCols C = Mult_AnXBn_Synch<PTDOUBLEDOUBLE, double, PSpMat<double>::DCCols >(A, B);
		}
		SpParHelper::Print("Warmed up for Synch\n");
		MPI_Barrier(MPI_COMM_WORLD);
		MPI_Pcontrol(1,"SpGEMM_Synch");
		t1 = MPI_Wtime(); 	// initilize (wall-clock) timer
		for(int i=0; i<ITERATIONS; i++)
		{
			PSpMat<double>::MPI_DCCols C = Mult_AnXBn_Synch<PTDOUBLEDOUBLE, double, PSpMat<double>::DCCols >(A, B);
		}
		MPI_Barrier(MPI_COMM_WORLD);
		MPI_Pcontrol(-1,"SpGEMM_Synch");
		t2 = MPI_Wtime(); 	
		if(myrank == 0)
		{
			cout<<"Synchronous multiplications finished"<<endl;	
			printf("%.6lf seconds elapsed per iteration\n", (t2-t1)/(double)ITERATIONS);
		}

		/*
		C = Mult_AnXBn_ActiveTarget<PTDOUBLEDOUBLE, double, PSpMat<double>::DCCols >(A, B);
		SpParHelper::Print("Warmed up for ActiveTarget\n");
		MPI_Barrier(MPI_COMM_WORLD);
		MPI_Pcontrol(1,"SpGEMM_ActiveTarget");
		t1 = MPI_Wtime(); 	// initilize (wall-clock) timer
		for(int i=0; i<ITERATIONS; i++)
		{
			C = Mult_AnXBn_ActiveTarget<PTDOUBLEDOUBLE, double, PSpMat<double>::DCCols >(A, B);
		}
		MPI_Barrier(MPI_COMM_WORLD);
		MPI_Pcontrol(-1,"SpGEMM_ActiveTarget");
		t2 = MPI_Wtime(); 	
		if(myrank == 0)
		{
			cout<<"Active target multiplications finished"<<endl;	
			printf("%.6lf seconds elapsed per iteration\n", (t2-t1)/(double)ITERATIONS);
		}		

		C = Mult_AnXBn_PassiveTarget<PTDOUBLEDOUBLE, double, PSpMat<double>::DCCols >(A, B);
		SpParHelper::Print("Warmed up for PassiveTarget\n");
		MPI_Barrier(MPI_COMM_WORLD);
		MPI_Pcontrol(1,"SpGEMM_PassiveTarget");
		t1 = MPI_Wtime(); 	// initilize (wall-clock) timer
		for(int i=0; i<ITERATIONS; i++)
		{
			C = Mult_AnXBn_PassiveTarget<PTDOUBLEDOUBLE, double, PSpMat<double>::DCCols >(A, B);
		}
		MPI_Barrier(MPI_COMM_WORLD);
		MPI_Pcontrol(-1,"SpGEMM_PassiveTarget");
		t2 = MPI_Wtime(); 	
		if(myrank == 0)
		{
			cout<<"Passive target multiplications finished"<<endl;	
			printf("%.6lf seconds elapsed per iteration\n", (t2-t1)/(double)ITERATIONS);
		}		
		*/
	}
	MPI_Finalize();
	return 0;
}

