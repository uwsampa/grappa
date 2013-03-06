#include <sys/time.h> 
#include <iostream>
#include <functional>
#include <algorithm>
#include <vector>
#include <sstream>
#include "../FullyDistSpVec.h"
#include "../FullyDistVec.h"

using namespace std;

template <class T>
struct IsOdd : public unary_function<T,bool> {
  bool operator() (T number) {return (number%2==1);}
};

int main(int argc, char* argv[])
{
	int nprocs, myrank;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);

	try
	{
		FullyDistSpVec<int64_t, int64_t> SPV_A(1024);
		SPV_A.SetElement(2,2);
		SPV_A.SetElement(83,-83);
		SPV_A.SetElement(284,284);
		SpParHelper::Print("Printing SPV_A\n");
		SPV_A.DebugPrint();
		
		FullyDistSpVec<int64_t, int64_t> SPV_B(1024);
		SPV_B.SetElement(2,4);
		SPV_B.SetElement(184,368);
		SPV_B.SetElement(83,-1);
		SpParHelper::Print("Printing SPV_B\n");
		SPV_B.DebugPrint();

		FullyDistVec<int64_t, int64_t> FDV(-1);
		FDV.iota(64,0);
		SpParHelper::Print("Printing FPV\n");
		FDV.DebugPrint();

		FullyDistSpVec<int64_t, int64_t> FDSV = FDV.Find(IsOdd<int64_t>());
		SpParHelper::Print("Printing FPSV\n");
		FDSV.DebugPrint();

		FullyDistSpVec<int64_t, int64_t> SPV_C(12);
		SPV_C.SetElement(2,2);	
		SPV_C.SetElement(4,4);	
		SPV_C.SetElement(5,-5);	
		SPV_C.SetElement(6,6);	

		SpParHelper::Print("Printing SPV_C\n");
		SPV_C.DebugPrint();

		FullyDistSpVec<int64_t, int64_t> SPV_D(12);
		SPV_D.SetElement(2,4);	
		SPV_D.SetElement(3,9);	
		SPV_D.SetElement(5,-25);	
		SPV_D.SetElement(7,-49);	

		SpParHelper::Print("Printing SPV_D\n");
		SPV_D.DebugPrint();
		
		SPV_C += SPV_D;
		SPV_D += SPV_D;

		SpParHelper::Print("Printing SPV_C + SPV_D\n");
		SPV_C.DebugPrint();

		SpParHelper::Print("Printing SPV_D + SPV_D\n");
		SPV_D.DebugPrint();

		FullyDistSpVec<int64_t, int64_t> SPV_E(3);
		SPV_E.SetElement(0,3);
		SPV_E.SetElement(1,7);
		SPV_E.SetElement(2,10);

		SpParHelper::Print("Printing SPV_E\n");
		SPV_E.DebugPrint();

		FullyDistSpVec<int64_t, int64_t> SPV_F = SPV_C(SPV_E);

		SpParHelper::Print("Printing SPV_F = SPV_C(SPV_E)\n");
		SPV_F.DebugPrint();
		FullyDistSpVec<int64_t, int64_t> SPV_H = SPV_C;
		FullyDistSpVec<int64_t, int64_t> SPV_J = SPV_H(SPV_F);
		int64_t val = SPV_J[8];
		stringstream tss;
		string ss;
		if(val == SPV_J.NOT_FOUND)
		{	
			ss = "NOT_FOUND";
		}
		else
		{
			tss << val;
			ss = tss.str();
		}
		if(myrank == 0)
			cout << ss << endl;	
		SPV_J.SetElement(8, 777);

		val = SPV_J[8];
		if(val == SPV_J.NOT_FOUND)
		{	
			ss = "NOT_FOUND";
		}
		else
		{
			tss << val;
			ss = tss.str();
		}
		if(myrank == 0)
			cout << ss << endl;	

	}
	catch (exception& e)
  	{
    		cout << e.what() << endl;
  	}
	MPI_Finalize();
	return 0;
}

