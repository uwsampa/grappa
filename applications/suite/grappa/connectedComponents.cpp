#include "defs.hpp"
#include <Grappa.hpp>
#include <ForkJoin.hpp>
#include <Collective.hpp>
#include <Delegate.hpp>

#include <deque>

static GlobalAddress<graphint> sV;
static GlobalAddress<graphint> eV;
static GlobalAddress<color_t> marks;
static graphint nchanged;
static graphint ncomponents;

static LocalTaskJoiner joiner;

static void init_marks(graphint k) {
  Grappa_delegate_write_word(marks+k, (color_t)k);
  joiner.signal();
}

static void merge_marks(graphint k) {
  graphint i = Grappa_delegate_read_word(sV+k);
  graphint j = Grappa_delegate_read_word(eV+k);
  graphint di = Grappa_delegate_read_word(marks+i);
  graphint dj = Grappa_delegate_read_word(marks+j);
  if (di < dj) {
    graphint ddj = Grappa_delegate_read_word(marks+dj);
    if (dj == ddj) {
      Grappa_delegate_write_word(marks+dj, di);
      ++nchanged;
    }
  }
  joiner.signal();
}

template <bool final>
static void compact_marks(graphint i) {
  graphint _di;
  Incoherent<graphint>::RW di(marks+i, 1, &_di);
  graphint ddi = Grappa_delegate_read_word(marks+*di);
  while (*di != ddi) {
    *di = ddi;
    ddi = Grappa_delegate_read_word(marks+*di);
  }
  if (final) {
    if (*di == i) {
      ncomponents++;
      VLOG(5) << "component " << i;
    }
  }
  di.block_until_released();
  joiner.signal();
}

LOOP_FUNCTOR( connectedCompFunc, nid,
    ((graphint,NV)) ((graphint,NE))
    ((GlobalAddress<color_t>,marks_))
    ((GlobalAddress<graphint>,startVertex))
    ((GlobalAddress<graphint>,endVertex)) ) {
  
  range_t vr = blockDist(0, NV, Grappa_mynode(), Grappa_nodes());
  range_t er = blockDist(0, NE, Grappa_mynode(), Grappa_nodes());
  
  ncomponents = 0;
  sV = startVertex;
  eV = endVertex;
  marks = marks_;
  
  joiner.reset();
  
  VLOG(5) << "before init_marks";
  for (graphint i=vr.start; i < vr.end; i++) {
    joiner.registerTask();
    Grappa_privateTask(&init_marks, i);
  }
  joiner.wait();
//  Grappa_barrier_commsafe();
  Grappa_barrier_suspending();
  
  VLOG(5) << "here";
  
  while (1) {
    nchanged = 0;
    
    for (graphint i = er.start; i < er.end; i++) {
      joiner.registerTask();
      Grappa_privateTask(&merge_marks, i);
    }
    joiner.wait();
    VLOG(5) << "nchanged = " << nchanged;
    // global reduction to find out if anyone changed anything
    nchanged = Grappa_allreduce<graphint,coll_add<graphint>,0>(nchanged);
    VLOG(5) << "global_nchanged = " << nchanged;
    
    if (nchanged == 0) break;
    
    for (graphint i = vr.start; i < vr.end; i++) {
      joiner.registerTask();
      Grappa_privateTask(&compact_marks<false>, i);
    }
    joiner.wait();
    VLOG(5) << "ncomponents = " << ncomponents;
//    Grappa_barrier_commsafe();
    Grappa_barrier_suspending();
  }
  
  for (graphint i = vr.start; i < vr.end; i++) {
    joiner.registerTask();
    Grappa_privateTask(&compact_marks<true>, i);
  }
  joiner.wait();
  VLOG(5) << "ncomponents = " << ncomponents;
//  Grappa_barrier_commsafe();
  Grappa_barrier_suspending();
  
  ncomponents = Grappa_allreduce<graphint,coll_add<graphint>,0>(ncomponents);
}

/// Takes a graph as input and an array with length NV.  The array D will store
/// the coloring of each component.  The coloring will be using vertex IDs and
/// therefore will be an integer between 0 and NV-1.  The function returns the
/// total number of components.
graphint connectedComponents(graph * g) {
  
  connectedCompFunc f(g->numVertices, g->numEdges, g->marks, g->startVertex, g->endVertex);
  fork_join_custom(&f);
  
  return ncomponents;
}
