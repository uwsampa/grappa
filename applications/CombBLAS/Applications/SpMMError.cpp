#include <mpi.h>
#include <sys/time.h> 
#include <iostream>
#include <functional>
#include <algorithm>
#include <vector>
#include <string>
#include <sstream>
#include <stdint.h>
#include "../CombBLAS.h"

double cblas_alltoalltime;
double cblas_allgathertime;


typedef SelectMaxSRing<bool, int64_t> SR;
typedef SpParMat < int64_t, bool, SpDCCols<int64_t,bool> > PSpMat_Bool;
typedef SpParMat < int64_t, int, SpDCCols<int64_t,int> > PSpMat_Int;

#define ValueType int64_t
typedef SpDCCols<int64_t,ValueType> DCColsType;
typedef SpParMat < int64_t, ValueType, DCColsType > PSpMat_Int64;
typedef PSpMat_Int64 MatType;

DECLARE_PROMOTE(MatType, MatType, MatType)
DECLARE_PROMOTE(DCColsType, DCColsType, DCColsType)

int main(int argc, char* argv[])
{
	int torusi[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
	int torusj[] = {3,0,1,2,7,4,5,6,11,8,9,10,15,12,13,14,1,2,3,0,5,6,7,4,9,10,11,8,13,14,15,12,12,13,14,15,0,1,2,3,4,5,6,7,8,9,10,11,4,5,6,7,8,9,10,11,12,13,14,15,0,1,2,3};

	int nprocs, myrank;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
	{
		SpParHelper::Print("Usage: SpMMError [arg]\nIf arg is present then matrix will be read from torus.mtx (no error).\nIf arg is absent, matrix will be created from vectors (error present)\n\n");
		// Declare objects
		MatType G1;
		MatType G2;
		if (argc > 1)
		{
			SpParHelper::Print("Reading torus.mtx\n");
			ifstream input("torus.mtx");
			G1.ReadDistribute(input, 0);	// read it from file
			ifstream input2("torus.mtx");
			G2.ReadDistribute(input2, 0);
			SpParHelper::Print("Read input\n");

		}
		else
		{
			SpParHelper::Print("Creating matrices from built-in vectors\n");
			FullyDistVec<int64_t, ValueType> dpvi(64, 0, 0);
			FullyDistVec<int64_t, ValueType> dpvj(64, 0, 0);
			FullyDistVec<int64_t, ValueType> dpvv(64, 1, 0);
			for (int i = 0; i <64; i++)
			{
				dpvi.SetElement(i, torusi[i]);
				dpvj.SetElement(i, torusj[i]);
			}
			dpvi.DebugPrint();
			dpvj.DebugPrint();
			G1 = MatType(16, 16, dpvi, dpvj, dpvv);
			G2 = MatType(16, 16, dpvi, dpvj, dpvv);

			ofstream out1("G1.txt");
			ofstream out2("G2.txt");
			out1 << G1;
			out2 << G2;
		}

		MatType G3(G1);
		G1.PrintInfo();
		G2.PrintInfo();
		G3.PrintInfo();
		SpParHelper::Print("The nnz values should be 112, 112, 112:\n");


		MatType G12 = Mult_AnXBn_Synch<PlusTimesSRing<int64_t, ValueType> >(G1, G2);
		G12.PrintInfo();
		ofstream out12("G12.txt");
		out12 << G12;

		MatType G13 = Mult_AnXBn_Synch<PlusTimesSRing<int64_t, ValueType> >(G1, G3);
		G13.PrintInfo();

		MatType G23 = Mult_AnXBn_Synch<PlusTimesSRing<int64_t, ValueType> >(G2, G3);
		G23.PrintInfo();

	}
	MPI_Finalize();
	return 0;
}

