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
#define EDGEFACTOR 8

int main(int argc, char* argv[])
{
	int nprocs, myrank;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);

	if(argc < 2)
	{
		if(myrank == 0)
		{
			cout << "Usage: ./IndexingTiming <Scale>" << endl;
		}
		MPI_Finalize(); 
		return -1;
	}				
	{
		typedef SpParMat <int, double, SpDCCols<int,double> > PARDBMAT;
		PARDBMAT *A, *B;		// declare objects
 		double initiator[4] = {.6, .4/3, .4/3, .4/3};
		DistEdgeList<int64_t> * DEL = new DistEdgeList<int64_t>();

		int scale = static_cast<unsigned>(atoi(argv[1]));
		ostringstream outs, outs2, outs3;
		outs << "Forcing scale to : " << scale << endl;
		SpParHelper::Print(outs.str());
		DEL->GenGraph500Data(initiator, scale, EDGEFACTOR, true, true );        // generate packed edges
		SpParHelper::Print("Generated renamed edge lists\n");
		
		// conversion from distributed edge list, keeps self-loops, sums duplicates
		A = new PARDBMAT(*DEL, false); 	// already creates renumbered vertices (hence balanced)
		delete DEL;	// free memory before symmetricizing
		SpParHelper::Print("Created double Sparse Matrix\n");

		float balance = A->LoadImbalance();
		outs2 << "Load balance: " << balance << endl;
		SpParHelper::Print(outs2.str());
		A->PrintInfo();

		for(unsigned i=1; i<4; i++)
		{
			DEL = new DistEdgeList<int64_t>();
			DEL->GenGraph500Data(initiator, scale-i, ((double) EDGEFACTOR) / pow(2.0,i) , true, true );        // "i" scale smaller
			B = new PARDBMAT(*DEL, false);
			delete DEL;
			SpParHelper::Print("Created RHS Matrix\n");
			B->PrintInfo();
			FullyDistVec<int,int> perm;	// get a different permutation
			perm.iota(A->getnrow(), 0);
			perm.RandPerm();

			//void FullyDistVec::iota(IT globalsize, NT first)
			FullyDistVec<int,int> sel;
			sel.iota(B->getnrow(), 0);
			perm = perm(sel);  // just get the first B->getnrow() entries of the permutation
			perm.PrintInfo("Index vector");
		
			A->SpAsgn(perm,perm,*B);	// overriding A with a structurally similar piece. 
			A->PrintInfo();
		
			double t1 = MPI_Wtime();
			for(int j=0; j< ITERATIONS; ++j)
			{
				A->SpAsgn(perm,perm,*B);
			}	
			double t2 = MPI_Wtime();

			if(myrank == 0)
			{
				cout<< "Scale " << scale-i << " assignment iterations finished"<<endl;	
				printf("%.6lf seconds elapsed per iteration\n", (t2-t1)/(double)ITERATIONS);
			
			}	
			delete B;
		}
	}
	MPI_Finalize();
	return 0;
}
