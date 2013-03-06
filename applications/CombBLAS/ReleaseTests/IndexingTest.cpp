#include <mpi.h>
#include <sys/time.h> 
#include <iostream>
#include <functional>
#include <algorithm>
#include <vector>
#include <sstream>
#include "../CombBLAS.h"

using namespace std;

template <typename IT, typename NT>
pair< FullyDistVec<IT,IT>, FullyDistVec<IT,NT> > TopK(FullyDistSpVec<IT,NT> & v, IT k)
{
	// FullyDistVec::FullyDistVec(IT glen, NT initval) 
	FullyDistVec<IT,IT> sel(k, 0);
	
	//void FullyDistVec::iota(IT globalsize, NT first)
	sel.iota(k, v.TotalLength() - k);

	FullyDistSpVec<IT,NT> sorted(v);
	FullyDistSpVec<IT,IT> perm = sorted.sort();	

	// FullyDistVec FullyDistSpVec::operator(FullyDistVec & v)
	FullyDistVec<IT,IT> topkind = perm(sel);   
	FullyDistVec<IT,NT> topkele = v(topkind);	
	return make_pair(topkind, topkele);
} 


int main(int argc, char* argv[])
{
	int nprocs, myrank;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);

	if(argc < 6)
	{
		if(myrank == 0)
		{
			cout << "Usage: ./IndexingTest <BASEADDRESS> <Matrix> <IndexedMatrix> <VectorOne> <VectorTwo>" << endl;
			cout << "Example: ./IndexingTest ../mfiles B_100x100.txt B_10x30_Indexed.txt rand10outta100.txt rand30outta100.txt" << endl;
			cout << "Input files should be under <BASEADDRESS> in tuples format" << endl;
		}
		MPI_Finalize(); 
		return -1;
	}				
	{
		string directory(argv[1]);		
		string normalname(argv[2]);
		string indexdname(argv[3]);
		string vec1name(argv[4]);
		string vec2name(argv[5]);
		normalname = directory+"/"+normalname;
		indexdname = directory+"/"+indexdname;
		vec1name = directory+"/"+vec1name;
		vec2name = directory+"/"+vec2name;

		ifstream inputvec1(vec1name.c_str());
		ifstream inputvec2(vec2name.c_str());

		if(myrank == 0)
		{	
			if(inputvec1.fail() || inputvec2.fail())
			{
				cout << "One of the input vector files do not exist, aborting" << endl;
				MPI_Abort(MPI_COMM_WORLD, NOFILE);
				return -1;
			}
		}
		MPI_Barrier(MPI_COMM_WORLD);
		typedef SpParMat <int, double , SpDCCols<int,double> > PARDBMAT;
		PARDBMAT A, AID, ACID;		// declare objects
		FullyDistVec<int,int> vec1, vec2;

		A.ReadDistribute(normalname, 0);	
		AID.ReadDistribute(indexdname, 0);	
		vec1.ReadDistribute(inputvec1, 0);
		vec2.ReadDistribute(inputvec2, 0);

		vec1.Apply(bind2nd(minus<int>(), 1));	// For 0-based indexing
		vec2.Apply(bind2nd(minus<int>(), 1));	
		ACID = A(vec1, vec2);

		if (ACID == AID)
		{
			SpParHelper::Print("Indexing working correctly\n");	
		}
		else
		{
			SpParHelper::Print("ERROR in indexing, go fix it!\n");	
		}

		FullyDistVec<int,int> crow, ccol;
		FullyDistVec<int,double> cval;
		A.Find(crow, ccol, cval);
		FullyDistSpVec<int, double> sval = cval;	
		sval.DebugPrint();

		pair< FullyDistVec<int,int> , FullyDistVec<int,double> > ptopk; 
		ptopk = TopK(sval, 3);
		//ptopk.first.DebugPrint();
		//ptopk.second.DebugPrint();

		// generate random permutations
		FullyDistVec<int,int> p, q;
		p.iota(A.getnrow(), 0);
		q.iota(A.getncol(), 0);
		p.RandPerm();	
		q.RandPerm();

		PARDBMAT B = A(p,q);
		A.PrintInfo();
		B.PrintInfo();

		float oldbalance = A.LoadImbalance();
		float newbalance = B.LoadImbalance();
		ostringstream outs;
		outs << "Old balance: " << oldbalance << endl;
		outs << "New balance: " << newbalance << endl;
		SpParHelper::Print(outs.str());
		SpParHelper::Print(outs.str());

		// B = P A Q
		// get the inverse permutations
		FullyDistVec<int, int> pinv = p.sort();
		FullyDistVec<int, int> qinv = q.sort();
		SpParHelper::Print("Sorts are done\n");
		PARDBMAT C = B(pinv,qinv);
		if(C == A)
		{
			SpParHelper::Print("Double permutation successfully restored the original\n");	
		}
		else
		{
			//A.PrintForPatoh("Original.patoh");
			//B.PrintForPatoh("Permuted.patoh");	
			//C.PrintForPatoh("Restored.patoh");
		}

		inputvec1.clear();
		inputvec1.close();
		inputvec2.clear();
		inputvec2.close();
	}
	MPI_Finalize();
	return 0;
}
