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

	if(argc < 8)
	{
		if(myrank == 0)
		{
			cout << "Usage: ./SpAsgnTest <BASEADDRESS> <Matrix> <PrunedMatrix> <RHSMatrix> <AssignedMatrix> <VectorRowIndices> <VectorColIndices>" << endl;
			cout << "Example: ./SpAsgnTest ../mfiles A_100x100.txt A_with20x30hole.txt dense_20x30matrix.txt A_wdenseblocks.txt 20outta100.txt 30outta100.txt" << endl;
			cout << "Input files should be under <BASEADDRESS> in tuples format" << endl;
		}
		MPI_Finalize(); 
		return -1;
	}				
	{
		string directory(argv[1]);		
		string normalname(argv[2]);
		string prunedname(argv[3]);
		string rhsmatname(argv[4]);
		string assignname(argv[5]);
		string vec1name(argv[6]);
		string vec2name(argv[7]);
		normalname = directory+"/"+normalname;
		prunedname = directory+"/"+prunedname;
		rhsmatname = directory+"/"+rhsmatname;
		assignname = directory+"/"+assignname;
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
		PARDBMAT A, Apr, B, C;		// declare objects
		FullyDistVec<int,int> vec1, vec2;

		A.ReadDistribute(normalname, 0);	
		Apr.ReadDistribute(prunedname, 0);	
		B.ReadDistribute(rhsmatname, 0);	
		C.ReadDistribute(assignname, 0);	
		vec1.ReadDistribute(inputvec1, 0);
		vec2.ReadDistribute(inputvec2, 0);

		vec1.Apply(bind2nd(minus<int>(), 1));	// For 0-based indexing
		vec2.Apply(bind2nd(minus<int>(), 1));	

		PARDBMAT Atemp = A;
		Atemp.Prune(vec1, vec2);
			
		// We should get the original A back.
		if( Atemp  == Apr)
		{
			SpParHelper::Print("Pruning is working\n");
		}
		else
		{
			SpParHelper::Print("Error in pruning, go fix it\n");
		}
		
		A.SpAsgn(vec1, vec2, B);
		if (A == C)
		{
			SpParHelper::Print("SpAsgn working correctly\n");	
		}
		else
		{
			SpParHelper::Print("ERROR in SpAsgn, go fix it!\n");	
			A.SaveGathered("Erroneous_SpAsgnd.txt");
		}

		FullyDistVec<int,int> crow, ccol;
		FullyDistVec<int,double> cval;
		A.Find(crow, ccol, cval);
		FullyDistSpVec<int, double> sval = cval;	
		// sval.DebugPrint();

		pair< FullyDistVec<int,int> , FullyDistVec<int,double> > ptopk; 
		ptopk = TopK(sval, 3);
		//ptopk.first.DebugPrint();
		//ptopk.second.DebugPrint();

		
		inputvec1.clear();
		inputvec1.close();
		inputvec2.clear();
		inputvec2.close();
	}
	MPI_Finalize();
	return 0;
}
