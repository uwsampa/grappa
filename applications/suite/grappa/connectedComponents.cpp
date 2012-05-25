#include "defs.hpp"
#include <SoftXMT.hpp>
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
  SoftXMT_delegate_write_word(marks+k, (color_t)k);
  joiner.signal();
}

static void merge_marks(graphint k) {
  graphint i = SoftXMT_delegate_read_word(sV+k);
  graphint j = SoftXMT_delegate_read_word(eV+k);
  graphint di = SoftXMT_delegate_read_word(marks+i);
  graphint dj = SoftXMT_delegate_read_word(marks+j);
  if (di < dj) {
    graphint ddj = SoftXMT_delegate_read_word(marks+dj);
    if (dj == ddj) {
      SoftXMT_delegate_write_word(marks+dj, di);
      ++nchanged;
    }
  }
  joiner.signal();
}

template <bool final>
static void compact_marks(graphint i) {
  graphint _di;
  Incoherent<graphint>::RW di(marks+i, 1, &_di);
  graphint ddi = SoftXMT_delegate_read_word(marks+*di);
  while (*di != ddi) {
    *di = ddi;
    ddi = SoftXMT_delegate_read_word(marks+*di);
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
  
  range_t vr = blockDist(0, NV, SoftXMT_mynode(), SoftXMT_nodes());
  range_t er = blockDist(0, NE, SoftXMT_mynode(), SoftXMT_nodes());
  
  ncomponents = 0;
  sV = startVertex;
  eV = endVertex;
  marks = marks_;
  
  joiner.reset();
  
  VLOG(5) << "before init_marks";
  for (graphint i=vr.start; i < vr.end; i++) {
    joiner.registerTask();
    SoftXMT_privateTask(&init_marks, i);
  }
  joiner.wait();
//  SoftXMT_barrier_commsafe();
  SoftXMT_barrier_suspending();
  
  VLOG(5) << "here";
  
  while (1) {
    nchanged = 0;
    
    for (graphint i = er.start; i < er.end; i++) {
      joiner.registerTask();
      SoftXMT_privateTask(&merge_marks, i);
    }
    joiner.wait();
    VLOG(5) << "nchanged = " << nchanged;
    // global reduction to find out if anyone changed anything
    nchanged = SoftXMT_allreduce<graphint,coll_add<graphint>,0>(nchanged);
    VLOG(5) << "global_nchanged = " << nchanged;
    
    if (nchanged == 0) break;
    
    for (graphint i = vr.start; i < vr.end; i++) {
      joiner.registerTask();
      SoftXMT_privateTask(&compact_marks<false>, i);
    }
    joiner.wait();
    VLOG(5) << "ncomponents = " << ncomponents;
//    SoftXMT_barrier_commsafe();
    SoftXMT_barrier_suspending();
  }
  
  for (graphint i = vr.start; i < vr.end; i++) {
    joiner.registerTask();
    SoftXMT_privateTask(&compact_marks<true>, i);
  }
  joiner.wait();
  VLOG(5) << "ncomponents = " << ncomponents;
//  SoftXMT_barrier_commsafe();
  SoftXMT_barrier_suspending();
  
  ncomponents = SoftXMT_allreduce<graphint,coll_add<graphint>,0>(ncomponents);
}

graphint connectedComponents(graph * g) {
  
  connectedCompFunc f(g->numVertices, g->numEdges, g->marks, g->startVertex, g->endVertex);
  fork_join_custom(&f);
  
  return ncomponents;
}
