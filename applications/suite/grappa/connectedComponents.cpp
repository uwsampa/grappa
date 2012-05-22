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

static LocalDynamicBarrier barrier;

static void init_marks(graphint k) {
  SoftXMT_delegate_write_word(marks+k, (color_t)k);
  barrier.signal();
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
  barrier.signal();
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
  barrier.signal();
}

LOOP_FUNCTOR( connectedCompFunc, nid,
    ((graphint,NV)) ((graphint,NE))
    ((GlobalAddress<color_t>,marks_))
    ((GlobalAddress<graphint>,startVertex))
    ((GlobalAddress<graphint>,endVertex))
//    ((GlobalAddress<graphint>,global_nchanged))
    ((GlobalAddress<graphint>,global_ncomponents)) ) {
  
  range_t vr = blockDist(0, NV, SoftXMT_mynode(), SoftXMT_nodes());
  range_t er = blockDist(0, NE, SoftXMT_mynode(), SoftXMT_nodes());
  
  ncomponents = 0;
  sV = startVertex;
  eV = endVertex;
  marks = marks_;
  
  barrier.reset();
  
  VLOG(5) << "before init_marks";
  for (graphint i=vr.start; i < vr.end; i++) {
    barrier.registerTask();
    SoftXMT_privateTask(&init_marks, i);
  }
  barrier.wait();
  SoftXMT_barrier_commsafe();
  
  VLOG(5) << "here";
  
  while (1) {
    nchanged = 0;
//    if (global_nchanged.node() == nid) *global_nchanged.pointer() = 0;
    
    for (graphint i = er.start; i < er.end; i++) {
      barrier.registerTask();
      SoftXMT_privateTask(&merge_marks, i);
    }
    barrier.wait();
    VLOG(5) << "nchanged = " << nchanged;
    // global reduction to find out if anyone changed anything
    nchanged = SoftXMT_allreduce<graphint,coll_add<graphint>,0>(nchanged);
    VLOG(5) << "global_nchanged = " << nchanged;
    
    if (nchanged == 0) break;
    
    for (graphint i = vr.start; i < vr.end; i++) {
      barrier.registerTask();
      SoftXMT_privateTask(&compact_marks<false>, i);
    }
    barrier.wait();
    VLOG(5) << "ncomponents = " << ncomponents;
    SoftXMT_barrier_commsafe();
  }
  
  for (graphint i = vr.start; i < vr.end; i++) {
    barrier.registerTask();
    SoftXMT_privateTask(&compact_marks<true>, i);
  }
  barrier.wait();
  VLOG(5) << "ncomponents = " << ncomponents;
  SoftXMT_barrier_commsafe();
  
  SoftXMT_delegate_fetch_and_add_word(global_ncomponents, ncomponents);
}

graphint connectedComponents(graph * g) {
  graphint global_ncomponents = 0;
  
  connectedCompFunc f(g->numVertices, g->numEdges, g->marks, g->startVertex, g->endVertex, make_global(&global_ncomponents));
  fork_join_custom(&f);
  
  return global_ncomponents;
}
