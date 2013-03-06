/****************************************************************/
/* Parallel Combinatorial BLAS Library (for Graph Computations) */
/* version 1.2 -------------------------------------------------*/
/* date: 09/25/2011 --------------------------------------------*/
/* authors: Aydin Buluc (abuluc@lbl.gov), Adam Lugowski --------*/
/****************************************************************/


#ifndef _SP_SEMANTIC_GRAPH_H_
#define _SP_SEMANTIC_GRAPH_H_

#include <iostream>
#include <fstream>
#include <cmath>
#include <mpi.h>
#include <vector>
#include <iterator>
// TR1 includes belong in CombBLAS.h

#include "SpMat.h"
#include "SpTuples.h"
#include "SpDCCols.h"
#include "CommGrid.h"
#include "MPIType.h"
#include "LocArr.h"
#include "SpDefs.h"
#include "Deleter.h"
#include "SpHelper.h"
#include "SpParHelper.h"
#include "FullyDistVec.h"
#include "Friends.h"
#include "Operations.h"

using namespace std;
using namespace std::tr1;

template <class IT, class NT, class DER>
class SemanticGraph
{
public:
	SemanticGraph(IT total_m, IT total_n, const FullyDistVec<IT,IT> & , const FullyDistVec<IT,IT> & , const FullyDistVec<IT,NT> & );    // matlab sparse
	typename typedef SpParMat < IT, NT, SpDCCols<IT,NT> > PSpMat;	// TODO: Convert to 32-bit local indices
	typename typedef FullyDistVec<IT,NT> PVec;
	
private:
	PSpMat SemMat;
	PVec SemVec; 
}


#endif
