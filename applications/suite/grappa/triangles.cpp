// Modified from original version by Preston Briggs
// License: (created for Cray?)

#include "defs.hpp"
#include <SoftXMT.hpp>
#include <ForkJoin.hpp>
#include <Collective.hpp>
#include <iomanip>

typedef GlobalAddress<graphint> Addr;
#define read SoftXMT_delegate_read_word
#define write SoftXMT_delegate_write_word

static Addr edge;
static Addr eV;
static graphint ntriangles;
static LocalTaskJoiner joiner;

#define BUFSIZE (1L<<12)

// debug only
//static graphint NV;
//static graphint NE;

static void trianglesTask(graphint v) {
  graphint bufA_[2];
  graphint bufB_[2];
  
  graphint A = v;
  graphint B;
  graphint _Astart;
  graphint _Aedge;
  graphint _Bedge;
  graphint temp;
  
  Incoherent<graphint>::RO cedgeA(edge+A, 2, bufA_);
  
  Addr Astart = eV + cedgeA[0];
  Addr Alimit = eV + cedgeA[1];
  
  while ((_Aedge = read(Astart)) <= A) ++Astart;
  
  while (Astart < Alimit) {
    Addr Aedge = Astart;
    B = _Aedge; //read(Aedge);
    
    Incoherent<graphint>::RO cedgeB(edge+B, 2, bufB_);
    Addr Bedge = eV + cedgeB[0];
    Addr Blimit = eV + cedgeB[1];
    
    while ((read(Bedge) <= B || read(Bedge) < A) && Bedge < Blimit) { ++Bedge; }
    _Bedge = read(Bedge);
    
    while (Aedge < Alimit && Bedge < Blimit) {
      if (_Aedge < _Bedge) {
        _Aedge = read(++Aedge);
      } else if (_Aedge > _Bedge) {
        _Bedge = read(++Bedge);
      } else {
        VLOG(3) << "tri  " << A << " " << B << " " << _Aedge << " ";
        ntriangles++;
        // advance to the end of any duplicate edges
        while (Aedge < Alimit && _Aedge == (temp = read(Aedge+1))) ++Aedge;
        _Aedge = temp;
        ++Aedge;
        while (Bedge < Blimit && _Bedge == (temp = read(Bedge+1))) ++Bedge;
        _Bedge = temp;
        ++Bedge;
      }
    }
    
    _Astart = read(Astart);
    while (Astart < Alimit && _Astart == (temp = read(Astart+1))) ++Astart;
    _Aedge = temp;
    ++Astart;
//    while (Astart < Alimit && read(Astart+1) == read(Astart)) ++Astart;
//    Astart++;
  }
  
  joiner.signal();
}

inline void checkAndMoveCache(graphint& index, graphint& currStart, graphint& currLimit, const graphint& globalLimit, Incoherent<graphint>::RO& cache) {
  index++;
  if (index >= currLimit) {
    GlobalAddress<graphint> newAddr = cache.address() + currLimit;
    currStart += currLimit;
    currLimit = MIN(BUFSIZE, globalLimit-currStart);
    if (currLimit > 0) {
      index = 0;
      cache.reset(newAddr, currLimit);
      cache.block_until_acquired();
    }
  }
}

static void search_children(uint64_t packed) {
  graphint A = packed & 0xFFFFFFFF;
  graphint Astart = packed >> 32;
  graphint temp;
  graphint bufRangeA[2];
  graphint bufRangeB[2];
  graphint bufA[BUFSIZE];
  graphint bufB[BUFSIZE];
  
//  CHECK(A < NV) << A << " < " << NV;
  
  Incoherent<graphint>::RO rangeA(edge+A, 2, bufRangeA);
  
  graphint Alimit = rangeA[1] - Astart;
  graphint astart = 0;
  graphint alim = MIN(BUFSIZE, Alimit); // A current limit
  
//  CHECK(Astart + alim-1 < NE) << Astart << " + " << alim << " < " << NE;
  
  Incoherent<graphint>::RO edgesA(eV+Astart, alim, bufA);
  
  graphint B = edgesA[0]; // read(Astart)
//  CHECK(B < NV) << B << " < " << NV;
  
  Incoherent<graphint>::RO rangeB(edge+B, 2, bufRangeB);
  graphint Blimit = rangeB[1] - rangeB[0];
  graphint bstart = 0;
  graphint blim = MIN(BUFSIZE, Blimit); // B current limit
  
//  CHECK(rangeB[0] + blim-1 < NE) << rangeB[0] << " + " << blim << " < " << NE;
  
  Incoherent<graphint>::RO edgesB(eV+rangeB[0], blim, bufB);
  
  graphint ai = 0;
  graphint bi = 0;
#define incrA() checkAndMoveCache(ai, astart, alim, Alimit, edgesA)
#define incrB() checkAndMoveCache(bi, bstart, blim, Blimit, edgesB)

  while (bstart+bi < Blimit && edgesB[bi] <= B) incrB();
    
  while (astart+ai < Alimit && bstart+bi < Blimit) {
    if (edgesA[ai] < edgesB[bi]) {
      incrA();
    } else if (edgesA[ai] > edgesB[bi]) {
      incrB();
    } else {
//      char tmsg[64];
//      sprintf(tmsg, "tri  %04llu %04llu %04llu \n", A, B, edgesA[ai]);
//      VLOG(4) << tmsg;
      ntriangles++;
      // advance past any duplicates
      temp = edgesA[ai];
//      incrA();
      while (astart+ai < Alimit && temp == edgesA[ai]) incrA();
      temp = edgesB[bi];
//      incrB();
      while (bstart+bi < Blimit && temp == edgesB[bi]) incrB();
    }
  }
  joiner.signal();
}
#undef incrA
#undef incrB

static void gen_tasks(graphint A) {
  graphint bufA_[2];  
  graphint temp;
  
  Incoherent<graphint>::RO cedgeA(edge+A, 2, bufA_);
  
  size_t Astart = cedgeA[0];
  size_t Alimit = cedgeA[1];
  
  while ((temp = read(eV+Astart)) <= A) ++Astart;
  
  while (Astart < Alimit) {
    uint64_t packed = (Astart << 32) + A;
    joiner.registerTask();
    SoftXMT_privateTask(&search_children, packed);
    
    temp = read(eV+Astart);
    Astart++;
    while (Astart < Alimit && temp == read(eV+Astart)) Astart++;
  }
  
  joiner.signal();
}

LOOP_FUNCTOR( trianglesFunc, nid, ((graphint,NV)) ((Addr,edge_)) ((Addr,eV_)) ) {
  edge = edge_;
  eV = eV_;
  ntriangles = 0;
  
//  NV = NV_;
//  NE = NE_;
  
  range_t r = blockDist(0, NV, SoftXMT_mynode(), SoftXMT_nodes());
  joiner.reset();
  
  for (graphint i=r.start; i<r.end; i++) {
    joiner.registerTask();
    SoftXMT_privateTask(&gen_tasks, i);
//    SoftXMT_privateTask(&trianglesTask, i);
  }
  joiner.wait();
  VLOG(3) << "ntriangles (local) = " << ntriangles;
  
//  ntriangles = SoftXMT_collective_reduce(&collective_add, 0, ntriangles, 0);
  ntriangles = SoftXMT_allreduce<graphint,coll_add<graphint>,0>(ntriangles);
}

/*	Finds the number of _unique_ triangles in an undirected graph.
 Assumes that each vertex's outgoing edge list is sorted by endVertex.
 
 A triangle is 3 vertices that are connected to each other without going 
 through any other vertices. That is:
 
   A              A
  / \   not...   / \
 B---C          B-D-C
 
 It is worth noting that this count excludes duplicate edges and self-edges 
 that are actually present in the graph.
 */
graphint triangles(graph * g) {
  
  trianglesFunc f(g->numVertices, g->edgeStart, g->endVertex);
  fork_join_custom(&f);
  
  return ntriangles;
}
