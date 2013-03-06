/****************************************************************/
/* Parallel Combinatorial BLAS Library (for Graph Computations) */
/* version 1.1 -------------------------------------------------*/
/* date: 12/25/2010 --------------------------------------------*/
/* authors: Aydin Buluc (abuluc@lbl.gov), Adam Lugowski --------*/
/****************************************************************/

/**
 * Functions that are used by multiple parallel matrix classes, but don't need the "this" pointer
 **/

#ifndef _SP_PAR_HELPER_H_
#define _SP_PAR_HELPER_H_

#include <vector>
#include <mpi.h>
#include "LocArr.h"
#include "CommGrid.h"
#include "MPIType.h"
#include "psort-1.0/src/psort.h"

using namespace std;

class SpParHelper
{
public:
	template<typename KEY, typename VAL, typename IT>
	static void GlobalSelect(IT gl_rank, pair<KEY,VAL> * & low,  pair<KEY,VAL> * & upp, pair<KEY,VAL> * array, IT length, const MPI_Comm & comm);

	template<typename KEY, typename VAL, typename IT>
	static void BipartiteSwap(pair<KEY,VAL> * low, pair<KEY,VAL> * array, IT length, int nfirsthalf, int color, const MPI_Comm & comm);

	// Necessary because psort creates three 2D vectors of size p-by-p
	// One of those vector with 8 byte data uses 8*(4096)^2 = 128 MB space 
	// Per processor extra storage becomes:
	//	24 MB with 1K processors
	//	96 MB with 2K processors
	//	384 MB with 4K processors
	// 	1.5 GB with 8K processors
	template<typename KEY, typename VAL, typename IT>
	static void MemoryEfficientPSort(pair<KEY,VAL> * array, IT length, IT * dist, const MPI_Comm & comm);
	
	template<typename KEY, typename VAL, typename IT>
	static void DebugPrintKeys(pair<KEY,VAL> * array, IT length, IT * dist, MPI_Comm & World);

	template<typename IT, typename NT, typename DER>
	static void FetchMatrix(SpMat<IT,NT,DER> & MRecv, const vector<IT> & essentials, vector<MPI_Win> & arrwin, int ownind);

	template<typename IT, typename NT, typename DER>	
	static void BCastMatrix(MPI_Comm & comm1d, SpMat<IT,NT,DER> & Matrix, const vector<IT> & essentials, int root);

	template<typename IT, typename NT, typename DER>
	static void SetWindows(MPI_Comm & comm1d, const SpMat< IT,NT,DER > & Matrix, vector<MPI_Win> & arrwin);

	template <typename IT, typename NT, typename DER>
	static void GetSetSizes(const SpMat<IT,NT,DER> & Matrix, IT ** & sizes, MPI_Comm & comm1d);

	template <typename IT, typename DER>
	static void AccessNFetch(DER * & Matrix, int owner, vector<MPI_Win> & arrwin, MPI_Group & group, IT ** sizes);

	template <typename IT, typename DER>
	static void LockNFetch(DER * & Matrix, int owner, vector<MPI_Win> & arrwin, MPI_Group & group, IT ** sizes);

	static void StartAccessEpoch(int owner, vector<MPI_Win> & arrwin, MPI_Group & group);
	static void PostExposureEpoch(int self, vector<MPI_Win> & arrwin, MPI_Group & group);
	static void LockWindows(int ownind, vector<MPI_Win> & arrwin);
	static void UnlockWindows(int ownind, vector<MPI_Win> & arrwin);

	static void Print(const string & s);
	static void WaitNFree(vector<MPI_Win> & arrwin);
	static void FreeWindows(vector<MPI_Win> & arrwin);
};

#include "SpParHelper.cpp"
#endif
