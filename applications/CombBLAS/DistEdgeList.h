/****************************************************************/
/* Parallel Combinatorial BLAS Library (for Graph Computations) */
/* version 1.2 -------------------------------------------------*/
/* date: 10/06/2011 --------------------------------------------*/
/* authors: Aydin Buluc (abuluc@lbl.gov), Adam Lugowski --------*/
/****************************************************************/
/*
Copyright (c) 2011, Aydin Buluc

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef _DIST_EDGE_LIST_H_
#define _DIST_EDGE_LIST_H_

#include <iostream>
#include <fstream>
#include <cmath>
#include <mpi.h>
#include <vector>
#include <iterator>
#include "CombBLAS.h"
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
#include "DenseParMat.h"
#include "FullyDistVec.h"
#include "Friends.h"
#include "Operations.h"


/** 
 * From Graph 500 reference implementation v2.1.1
**/
typedef struct packed_edge {
  uint32_t v0_low;
  uint32_t v1_low;
  uint32_t high; /* v1 in high half, v0 in low half */
} packed_edge;

static inline int64_t get_v0_from_edge(const packed_edge* p) {
  return (p->v0_low | ((int64_t)((int16_t)(p->high & 0xFFFF)) << 32));
}

static inline int64_t get_v1_from_edge(const packed_edge* p) {
  return (p->v1_low | ((int64_t)((int16_t)(p->high >> 16)) << 32));
}

static inline void write_edge(packed_edge* p, int64_t v0, int64_t v1) {
  p->v0_low = (uint32_t)v0;
  p->v1_low = (uint32_t)v1;
  p->high = ((v0 >> 32) & 0xFFFF) | (((v1 >> 32) & 0xFFFF) << 16);
}


template <typename IT>
class DistEdgeList
{
public:	
	// Constructors
	DistEdgeList ();
	DistEdgeList (const char * filename, IT globaln, IT globalm);	// read from binary in parallel
	~DistEdgeList ();

	void Dump64bit(string filename);
	void Dump32bit(string filename);
	void GenGraph500Data(double initiator[4], int log_numverts, int edgefactor, bool scramble =false, bool packed=false);
	void CleanupEmpties();
	
	int64_t getGlobalV() const { return globalV; }
	IT getNumLocalEdges() const { return nedges; }
	
private:
	shared_ptr<CommGrid> commGrid; 
	
	IT* edges; // edge list composed of pairs of edge endpoints.
	           // Edge i goes from edges[2*i+0] to edges[2*i+1]
	packed_edge * pedges;
	           
	IT nedges; 	// number of local edges
	IT memedges; 	// number of edges for which there is space. nedges <= memedges
	int64_t globalV;
	
	void SetMemSize(IT ne);
	
	template<typename IU>
	friend void PermEdges(DistEdgeList<IU> & DEL);
	
	template <typename IU>
	friend void RenameVertices(DistEdgeList<IU> & DEL);

	template <class IU, class NU, class UDER>
	friend class SpParMat;
};

template<typename IU>
void PermEdges(DistEdgeList<IU> & DEL);


#include "DistEdgeList.cpp"

#endif
