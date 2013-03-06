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

template <class T>
bool from_string(T & t, const string& s, std::ios_base& (*f)(std::ios_base&))
{
        istringstream iss(s);
        return !(iss >> f >> t).fail();
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
			cout << "Usage: ./IndexingTiming Input/Force/Binary <Inputfile>/<Scale>/<BinaryFile>" << endl;
		}
		MPI_Finalize(); 
		return -1;
	}				
	{
		typedef SpParMat <int, double, SpDCCols<int,double> > PARDBMAT;
		PARDBMAT * A;		// declare objects
		if(string(argv[1]) == string("Input"))
		{
			A->ReadDistribute(argv[2], 0);	
		}
		else if(string(argv[1]) == string("Binary"))
		{
			uint64_t n, m;
			from_string(n,string(argv[3]),std::dec);
			from_string(m,string(argv[4]),std::dec);
			
			ostringstream outs;
			outs << "Reading " << argv[2] << " with " << n << " vertices and " << m << " edges" << endl;
			SpParHelper::Print(outs.str());
			DistEdgeList<int64_t> * DEL = new DistEdgeList<int64_t>(argv[2], n, m);
			SpParHelper::Print("Read binary input to distributed edge list\n");

			PermEdges(*DEL);
			SpParHelper::Print("Permuted Edges\n");

			RenameVertices(*DEL);	
			SpParHelper::Print("Renamed Vertices\n");

			A = new PARDBMAT(*DEL, false); 
			delete DEL;	// free memory before symmetricizing
			SpParHelper::Print("Created double Sparse Matrix\n");
		}
		else if(string(argv[1]) == string("Force"))
		{
 			double initiator[4] = {.6, .4/3, .4/3, .4/3};
			DistEdgeList<int64_t> * DEL = new DistEdgeList<int64_t>();

			int scale = static_cast<unsigned>(atoi(argv[2]));
			ostringstream outs;
			outs << "Forcing scale to : " << scale << endl;
			SpParHelper::Print(outs.str());
			DEL->GenGraph500Data(initiator, scale, 8 * ((int64_t) std::pow(2.0, (double) scale)) / nprocs );
			SpParHelper::Print("Generated local RMAT matrices\n");
		
			PermEdges(*DEL);
			SpParHelper::Print("Permuted Edges\n");

			RenameVertices(*DEL);	
			SpParHelper::Print("Renamed Vertices\n");

			// conversion from distributed edge list, keeps self-loops, sums duplicates
			A = new PARDBMAT(*DEL, false); 
			delete DEL;	// free memory before symmetricizing
			SpParHelper::Print("Created double Sparse Matrix\n");
		}
		

		A->PrintInfo();	
		FullyDistVec<int,int> p;
		p.iota(A->getnrow(), 0);
		p.RandPerm();	
		SpParHelper::Print("Permutation Generated\n");
		PARDBMAT B = (*A)(p,p);
		B.PrintInfo();

		float oldbalance = A->LoadImbalance();
		float newbalance = B.LoadImbalance();
		ostringstream outs;
		outs << "Running on " << nprocs << " cores" << endl;
		outs << "Old balance: " << oldbalance << endl;
		outs << "New balance: " << newbalance << endl;
		SpParHelper::Print(outs.str());

		MPI_Barrier(MPI_COMM_WORLD);
		double t1 = MPI_Wtime(); 	// initilize (wall-clock) timer
	
		for(int i=0; i<ITERATIONS; i++)
		{
			B = (*A)(p,p);
		}
		
		MPI_Barrier(MPI_COMM_WORLD);
		double t2 = MPI_Wtime(); 	

		if(myrank == 0)
		{
			cout<<"Indexing Iterations finished"<<endl;	
			printf("%.6lf seconds elapsed per iteration\n", (t2-t1)/(double)ITERATIONS);
		}

		//  Test #2
		int nclust = 10;
		vector< FullyDistVec<int,int> > clusters(nclust);
		int nperclus = A->getnrow() / nclust;

		for(int i = 0; i< nclust; i++)
		{
			int k = std::min(nperclus, A->getnrow() - nperclus * i);
			clusters[i].iota(k, nperclus * i);
			clusters[i] = p(clusters[i]);
		}

		for(int i=0; i< nclust; i++)
		{
			B = (*A)(clusters[i], clusters[i]);
			B.PrintInfo();
		} 

		MPI_Barrier(MPI_COMM_WORLD);
		t1 = MPI_Wtime(); 	// initilize (wall-clock) timer
		for(int i=0; i< nclust; i++)
		{
			for(int j=0; j < ITERATIONS; j++)
				B = (*A)(clusters[i], clusters[i]);
		} 

		MPI_Barrier(MPI_COMM_WORLD);
		t2 = MPI_Wtime(); 	

		if(myrank == 0)
		{
			cout<<"Indexing Iterations finished"<<endl;	
			printf("%.6lf seconds elapsed per iteration\n", (t2-t1)/(double)ITERATIONS);
		}

		// Test #3: Pruning for SpAsgn
		for(int i=0; i< nclust; i++)
		{
			PARDBMAT C = *A;
			C.Prune(clusters[i], clusters[i]);
			C.PrintInfo();

			B = (*A)(clusters[i], clusters[i]);
			C.SpAsgn(clusters[i], clusters[i], B);	
			
			// We should get the original A back.
			if( C == (*A))
			{
				SpParHelper::Print("Pruning and SpAsgn seem to be working\n");
			}
			else
			{
				SpParHelper::Print("A and C don't match, below is C's info followed by B's info\n");
				C.PrintInfo();
				B.PrintInfo();
			}
		}

		
		delete A;
	}
	MPI_Finalize();
	return 0;
}
