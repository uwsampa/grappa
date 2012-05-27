// Inspired by Ullman (1976) Subgraph-isomorphism algorithm
// Brandon Holt
#include "defs.hpp"
#include <SoftXMT.hpp>
#include <ForkJoin.hpp>
#include <Collective.hpp>

//static GlobalAddress<graphint> eV;
//static GlobalAddress<graphint> edges;
//static GlobalAddress<graphint> weights;
//static GlobalAddress<color_t> marks;
static color_t minc, maxc;

static graph g;

static const color_t * pattern;
static size_t npattern;

static graphint nmatches;

static LocalTaskJoiner joiner;

static void checkEdgesRecursiveTask(uint64_t packed) {
  graphint v = packed & 0xFFFFFFFF;
  size_t poff = packed >> 32; // current pattern offset

  graphint edgeRange_buf_[2];

  Incoherent<graphint>::RO edgeRange(g.edgeStart+v, 2, edgeRange_buf_);
  graphint nedges = edgeRange[1] - edgeRange[0];
  graphint eVbuf_[nedges];
  Incoherent<graphint>::RO ceV(g.endVertex+edgeRange[0], nedges, eVbuf_);
  
  for (graphint j = 0; j < nedges; j++) {
    if (SoftXMT_delegate_read_word(g.marks+ceV[j]) == pattern[poff]) {
      VLOG(5) << v << " -> " << ceV[j] << " " << poff << "(" << pattern[poff] << ")";

      if (poff == npattern-1) {
        nmatches++;
      } else {
        CHECK(ceV[j] < (1L<<32)) << "v = " << ceV[j];
        packed = (((uint64_t)poff+1) << 32) | ceV[j];
        joiner.registerTask();
        SoftXMT_privateTask(&checkEdgesRecursiveTask, packed);
      }
    }
  }
  joiner.signal();
}

static void pathIsoTask(graphint v) {
  color_t cmark_buf_;
  
  Incoherent<color_t>::RO cmark(g.marks+v, 1, &cmark_buf_);
  
  if (*cmark == *pattern) {
    CHECK(v < (1L<<32)) << "v = " << v;
    uint64_t packed = (((uint64_t)1) << 32) | v;
    checkEdgesRecursiveTask(packed);
  } else {
    joiner.signal();
  }
}

LOOP_FUNCTOR( pathIsomorphismFunc, nid, ((graph,g_)) ((GlobalAddress<color_t>,pattern_)) ((size_t,npattern_)) ) {
  g = g_;
  npattern = npattern_;
  color_t buf_[npattern_];
  Incoherent<color_t>::RO cpattern(pattern_, npattern_, buf_);
  cpattern.start_acquire();
  pattern = &(*cpattern);
  
  joiner.reset();
  range_t vr = blockDist(0, g.numVertices, SoftXMT_mynode(), SoftXMT_nodes());
  for (graphint i=vr.start; i<vr.end; i++) {
    joiner.registerTask();
    SoftXMT_privateTask(&pathIsoTask, i);
  }
  joiner.wait();
  
  VLOG(5) << "nmatches = " << nmatches;
  
  nmatches = SoftXMT_collective_reduce(&collective_add, 0, nmatches, 0);
}

/* @param g: (directed) graph to search in
 @param vertices: pattern of vertex colors to search for (in order)
 @return matches: list of vertices that mark the beginning of the pattern */
graphint pathIsomorphism(graph* g, color_t* pattern, size_t npattern) {
  pathIsomorphismFunc f(*g, make_global(pattern), npattern);
  fork_join_custom(&f);
  return nmatches;
}

static void markColorsTask(graphint v) {
  graphint buf_[2];
  Incoherent<graphint>::RO edgeRange(g.edgeStart+v, 2, buf_);
  size_t nedges = edgeRange[1] - edgeRange[0];
  graphint wbuf_[nedges];
  Incoherent<graphint>::RO cw(g.intWeight+edgeRange[0], nedges, wbuf_);
  color_t mbuf_;
  {
    Incoherent<color_t>::WO cmark(g.marks+v, 1, &mbuf_);
    
    int total = 0;
    for (graphint j = 0; j < nedges; j++) {
      total += cw[j];
    }
    *cmark = (total % (maxc-minc)) + minc;
  }
  joiner.signal();
}

LOOP_FUNCTOR( markColorsFunc, nid, ((graph,g_)) ((color_t,minc_)) ((color_t,maxc_)) ) {
  g = g_;
  minc = minc_;
  maxc = maxc_;
  
  range_t vr = blockDist(0, g.numVertices, SoftXMT_mynode(), SoftXMT_nodes());
  
  joiner.reset();
  
  for (graphint i=vr.start; i<vr.end; i++) {
    joiner.registerTask();
    SoftXMT_privateTask(&markColorsTask, i);
  }
  joiner.wait();
}

/// In order to get reproducible results for pathIsomorphism, mark for 
/// each vertex should be based on something that will be consistent 
/// with the structure of the graph rather than the ordering or implicit 
/// numbering of vertices.
/// Arbitrarily choosing: sum(outgoing weights) % (maxc-minc) + minc 
void markColors(graph * g, color_t minc, color_t maxc) {
  markColorsFunc f(*g, minc, maxc);
  fork_join_custom(&f);
}

//void print_match(graph *dirg, color_t *pattern, graphint startVertex) {
//  graphint v = startVertex;
//  color_t *c = pattern;
//  printf("match %d: %"DFMT"(%"DFMT")", 0, v, *c);
//  c++;
//  while (*c != END) {
//    graphint nextV = -1;
//    for (graphint i=dirg->edgeStart[v]; i<dirg->edgeStart[v+1]; i++) {
//      if (dirg->marks[dirg->endVertex[i]] == *c) {
//        nextV = dirg->endVertex[i];
//        break;
//      }
//    }
//    assert(nextV != -1);
//    printf(" -> %"DFMT"(%"DFMT")", nextV, *c);
//    v = nextV;
//    c++;
//  }
//  printf("\n");
//}
