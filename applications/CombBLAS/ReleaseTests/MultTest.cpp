#include <mpi.h>
#include <sys/time.h> 
#include <iostream>
#include <functional>
#include <algorithm>
#include <vector>
#include <sstream>
#include "../CombBLAS.h"

using namespace std;
#ifdef TIMING
double cblas_alltoalltime;
double cblas_allgathertime;
#endif

#ifdef _OPENMP
int cblas_splits = omp_get_max_threads(); 
#else
int cblas_splits = 1;
#endif


// Simple helper class for declarations: Just the numerical type is templated 
// The index type and the sequential matrix type stays the same for the whole code
// In this case, they are "int" and "SpDCCols"
template <class NT>
class PSpMat 
{ 
public: 
	typedef SpDCCols < int64_t, NT > DCCols;
	typedef SpParMat < int64_t, NT, DCCols > MPI_DCCols;
};

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
			cout << "Usage: ./MultTest <MatrixA> <MatrixB> <MatrixC> <vecX> <vecY>" << endl;
			cout << "<MatrixA>,<MatrixB>,<MatrixC> are absolute addresses, and files should be in triples format" << endl;
		}
		MPI_Finalize(); 
		return -1;
	}				
	{
		string Aname(argv[1]);		
		string Bname(argv[2]);
		string Cname(argv[3]);
		string V1name(argv[4]);
		string V2name(argv[5]);

		ifstream vecinpx(V1name.c_str());
		ifstream vecinpy(V2name.c_str());

		MPI_Barrier(MPI_COMM_WORLD);	
		typedef PlusTimesSRing<double, double> PTDOUBLEDOUBLE;	
		typedef SelectMaxSRing<bool, int64_t> SR;	

		PSpMat<double>::MPI_DCCols A, B, C, CControl;	// construct objects
		FullyDistVec<int64_t, double> ycontrol, x;
		FullyDistSpVec<int64_t, double> spycontrol, spx;
		
		A.ReadDistribute(Aname, 0);
#ifndef NOGEMM
		B.ReadDistribute(Bname, 0);
		CControl.ReadDistribute(Cname, 0);
#endif
		x.ReadDistribute(vecinpx, 0);
		spx.ReadDistribute(vecinpx, 0);
		ycontrol.ReadDistribute(vecinpy,0);
		spycontrol.ReadDistribute(vecinpy,0);

		FullyDistVec<int64_t, double> y = SpMV<PTDOUBLEDOUBLE>(A, x);
		if (ycontrol == y)
		{
			SpParHelper::Print("Dense SpMV (fully dist) working correctly\n");	
		}
		else
		{
			SpParHelper::Print("ERROR in Dense SpMV, go fix it!\n");	
			ofstream ydense("ycontrol_dense.txt");
			y.SaveGathered(ydense,0);
			ydense.close();
		}

		FullyDistSpVec<int64_t, double> spy = SpMV<PTDOUBLEDOUBLE>(A, spx);
		if (spycontrol == spy)
		{
			SpParHelper::Print("Sparse SpMV (fully dist) working correctly\n");	
		}
		else
		{
			SpParHelper::Print("ERROR in Sparse SpMV, go fix it!\n");	
			ofstream ysparse("ycontrol_sparse.txt");
			spy.SaveGathered(ysparse,0);
			ysparse.close();
		}
#ifndef NOGEMM
		C = Mult_AnXBn_Synch<PTDOUBLEDOUBLE, double, PSpMat<double>::DCCols >(A,B);
		if (CControl == C)
		{
			SpParHelper::Print("Synchronous Multiplication working correctly\n");	
			// C.SaveGathered("CControl.txt");
		}
		else
		{
			SpParHelper::Print("ERROR in Synchronous Multiplication, go fix it!\n");	
		}

		C = Mult_AnXBn_DoubleBuff<PTDOUBLEDOUBLE, double, PSpMat<double>::DCCols >(A,B);
		if (CControl == C)
		{
			SpParHelper::Print("Double buffered multiplication working correctly\n");	
		}
		else
		{
			SpParHelper::Print("ERROR in double buffered multiplication, go fix it!\n");	
		}
#endif
		OptBuf<int32_t, int64_t> optbuf;
		PSpMat<bool>::MPI_DCCols ABool(A);

		spx.Apply(bind1st (multiplies<double>(), 100));
		FullyDistSpVec<int64_t, int64_t> spxint64 (spx);
		FullyDistSpVec<int64_t, int64_t> spyint64 = SpMV<SR>(ABool, spxint64, false);

		ABool.OptimizeForGraph500(optbuf);
		FullyDistSpVec<int64_t, int64_t> spyint64buf = SpMV<SR>(ABool, spxint64, false, optbuf);
		
		if (spyint64 == spyint64buf)
		{
			SpParHelper::Print("Graph500 Optimizations are correct\n");	
		}
		else
		{
			SpParHelper::Print("ERROR in graph500 optimizations, go fix it!\n");	
			ofstream of1("Original_SpMSV.txt");
			ofstream of2("Buffered_SpMSV.txt");
			spyint64.SaveGathered(of1,0);
			spyint64buf.SaveGathered(of2,0);
		}
		ABool.ActivateThreading(cblas_splits);
		FullyDistSpVec<int64_t, int64_t> spyint64_threaded = SpMV<SR>(ABool, spxint64, false);

		if (spyint64 == spyint64_threaded)
		{
			SpParHelper::Print("Multithreaded Sparse SpMV works\n");	
		}
		else
		{
			SpParHelper::Print("ERROR in multithreaded sparse SpMV, go fix it!\n");	
		}

		vecinpx.clear();
		vecinpx.close();
		vecinpy.clear();
		vecinpy.close();
	}
	MPI_Finalize();
	return 0;
}

