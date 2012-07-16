#include "common.h"
#include "SoftXMT.hpp"
#include "ForkJoin.hpp"
#include "Cache.hpp"
#include "Delegate.hpp"
#include "PerformanceTools.hpp"
#include "GlobalAllocator.hpp"
#include "timer.h"
#include "GlobalTaskJoiner.hpp"
#include "AsyncParallelFor.hpp"
#include "PushBuffer.hpp"

GRAPPA_DEFINE_EVENT_GROUP(bfs);


#define read      SoftXMT_delegate_read_word
#define write     SoftXMT_delegate_write_word
#define cmp_swap  SoftXMT_delegate_compare_and_swap_word
#define fetch_add SoftXMT_delegate_fetch_and_add_word

static PushBuffer<int64_t> vlist_buf;

static GlobalAddress<int64_t> vlist;
static GlobalAddress<int64_t> xoff;
static GlobalAddress<int64_t> xadj;
static GlobalAddress<int64_t> bfs_tree;
static GlobalAddress<int64_t> k2;
static int64_t nadj;

#define XOFF(k) (xoff+2*(k))
#define XENDOFF(k) (xoff+2*(k)+1)

#define GA64(name) ((GlobalAddress<int64_t>,name))

// count number of neighbors visited
static uint64_t bfs_neighbors_visited = 0;
static bool bfs_counters_added = false;

void bfs_visit_neighbor(int64_t estart, int64_t eiters, GlobalAddress<void*> packed) {
  int64_t v = (int64_t)packed.pointer();
  
  //const int64_t j = read(xadj+vo);
  //VLOG(1) << "estart: " << estart << ", eiters: " << eiters;

  int64_t cbuf[eiters];
  Incoherent<int64_t>::RO cadj(xadj+estart, eiters, cbuf);
  
  for (int64_t i = 0; i < eiters; i++) {
    ++bfs_neighbors_visited;

    const int64_t j = cadj[i];
    //VLOG(1) << "v = " << v << ", j = " << j << ", i = " << i << ", eiters = " << eiters;

    if (cmp_swap(bfs_tree+j, -1, v)) {
      vlist_buf.push(j);
    }
  }
}

// count number of vertex visited
static uint64_t bfs_vertex_visited = 0;

void bfs_visit_vertex(int64_t kstart, int64_t kiters) {
  //VLOG(1) << "bfs_visit_vertex(" << kstart << ", " << kiters << ")";

  int64_t buf[kiters];
  Incoherent<int64_t>::RO cvlist(vlist+kstart, kiters, buf);

  for (int64_t i=0; i<kiters; i++) {
    ++bfs_vertex_visited;

    const int64_t v = cvlist[i];
    
    int64_t buf[2];
    Incoherent<int64_t>::RO cxoff(xoff+2*v, 2, buf);
    const int64_t vstart = cxoff[0], vend = cxoff[1];
    
    async_parallel_for_hack(bfs_visit_neighbor, vstart, vend-vstart, v);
  }
}

static unsigned marker = -1;

void bfs_level(Node nid, int64_t start, int64_t end) {
#ifdef VTRACE
  VT_TRACER("bfs_level");
  if (SoftXMT_mynode() == 0) {
    char s[256];
    sprintf(s, "<%ld>", end-start);
    VT_MARKER(marker, s);
  }
#endif

  vlist_buf.setup(vlist, k2);
  
  global_async_parallel_for_thresh(bfs_visit_vertex, start, end-start, 1);
  
  vlist_buf.flush();
    
  SoftXMT_barrier_suspending();

  VLOG(2) << "phase complete";
}

LOOP_FUNCTOR(bfs_node, nid, GA64(_vlist)GA64(_xoff)GA64(_xadj)GA64(_bfs_tree)GA64(_k2)((int64_t,_nadj))) {
  
  if ( !bfs_counters_added ) {
    bfs_counters_added = true;
    SoftXMT_add_profiling_counter( &bfs_neighbors_visited, "bfs_neighbors_visited", "bfsneigh", true, 0 );
    SoftXMT_add_profiling_counter( &bfs_vertex_visited, "bfs_vertex_visited", "bfsverts", true, 0 );
  }

  // setup globals
  vlist = _vlist;
  xoff = _xoff;
  xadj = _xadj;
  bfs_tree = _bfs_tree;
  k2 = _k2;
  nadj = _nadj;

  int64_t k1 = 0, _k2 = 1;
  
  while (k1 != _k2) {
    VLOG(2) << "k1=" << k1 << ", k2=" << _k2;
    const int64_t oldk2 = _k2;
    
    bfs_level(SoftXMT_mynode(), k1, oldk2);

    k1 = oldk2;
    _k2 = read(k2);
  }  
}

double make_bfs_tree(csr_graph * g, GlobalAddress<int64_t> bfs_tree, int64_t root) {
  int64_t NV = g->nv;
  GlobalAddress<int64_t> vlist = SoftXMT_typed_malloc<int64_t>(NV);
 
#ifdef VTRACE 
  if (marker == -1) marker = VT_MARKER_DEF("bfs_level", VT_MARKER_TYPE_HINT);
#endif

  double t;
  t = timer();
  
  // start with root as only thing in vlist
  write(vlist, root);
  
  int64_t k1 = 0, k2 = 1;
  GlobalAddress<int64_t> k2addr = make_global(&k2);
  
  // initialize bfs_tree to -1
  SoftXMT_memset(bfs_tree, (int64_t)-1,  NV);
  
  write(bfs_tree+root, root); // parent of root is self
  
  { bfs_node f(vlist, g->xoff, g->xadj, bfs_tree, k2addr, g->nadj); fork_join_custom(&f); }
    
  t = timer() - t;
  
  SoftXMT_free(vlist);
  
  return t;
}

