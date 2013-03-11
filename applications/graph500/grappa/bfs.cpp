#include "common.h"
#include "Grappa.hpp"
#include "ForkJoin.hpp"
#include "Cache.hpp"
#include "PerformanceTools.hpp"
#include "GlobalAllocator.hpp"
#include "timer.h"
#include "GlobalTaskJoiner.hpp"
#include "AsyncParallelFor.hpp"
#include "PushBuffer.hpp"
#include <Collective.hpp>
#include <Array.hpp>
#include <ParallelLoop.hpp>
#include <Delegate.hpp>

using namespace Grappa;

GRAPPA_DEFINE_EVENT_GROUP(bfs);

static PushBuffer<int64_t> vlist_buf;

static GlobalAddress<int64_t> vlist;
static GlobalAddress<int64_t> xoff;
static GlobalAddress<int64_t> xadj;
static GlobalAddress<int64_t> bfs_tree;
static GlobalAddress<int64_t> k2;
static int64_t nadj;

#define XOFF(k) (xoff+2*(k))
#define XENDOFF(k) (xoff+2*(k)+1)

// count number of vertex visited
static uint64_t bfs_vertex_visited = 0;

// count number of neighbors visited
static uint64_t bfs_neighbors_visited = 0;
static bool bfs_counters_added = false;

static unsigned marker = -1;

void bfs_level(int64_t start, int64_t end) {
#ifdef VTRACE
  VT_TRACER("bfs_level");
  if (mycore() == 0) {
    char s[256];
    sprintf(s, "<%ld>", end-start);
    VT_MARKER(marker, s);
  }
#endif

  vlist_buf.setup(vlist, k2);
  
  forall_global_public(start, end-start, [](int64_t kstart, int64_t kiters) {
    int64_t buf[kiters];
    Incoherent<int64_t>::RO cvlist(vlist+kstart, kiters, buf);

    for (int64_t i=0; i<kiters; i++) {
      ++bfs_vertex_visited;

      const int64_t v = cvlist[i];
      
      int64_t buf[2];
      Incoherent<int64_t>::RO cxoff(xoff+2*v, 2, buf);
      const int64_t vstart = cxoff[0], vend = cxoff[1]; // (xoff[2v], xoff[2v+1])
      
      forall_here_async_public(vstart, vend-vstart, [v](int64_t estart, int64_t eiters) {
        //const int64_t j = read(xadj+vo);
        //VLOG(1) << "estart: " << estart << ", eiters: " << eiters;

        int64_t cbuf[eiters];
        Incoherent<int64_t>::RO cadj(xadj+estart, eiters, cbuf);
        
        for (int64_t i = 0; i < eiters; i++) {
          ++bfs_neighbors_visited;

          const int64_t j = cadj[i];
          //VLOG(1) << "v = " << v << ", j = " << j << ", i = " << i << ", eiters = " << eiters;

          if (delegate::compare_and_swap(bfs_tree+j, -1, v)) {
            vlist_buf.push(j);
          }
        }
      });
    }
  });
  
  vlist_buf.flush();
    
}

double make_bfs_tree(csr_graph * g, GlobalAddress<int64_t> local_bfs_tree, int64_t root) {
  int64_t NV = g->nv;
  GlobalAddress<int64_t> local_vlist = Grappa_typed_malloc<int64_t>(NV);
 
#ifdef VTRACE 
  if (marker == -1) marker = VT_MARKER_DEF("bfs_level", VT_MARKER_TYPE_HINT);
#endif

  double t;
  t = timer();
  
  // start with root as only thing in vlist
  delegate::write(local_vlist, root);
  
  int64_t k1 = 0, local_k2 = 1;
  GlobalAddress<int64_t> k2addr = make_global(&local_k2);
  
  // initialize bfs_tree to -1
  Grappa::memset(local_bfs_tree, (int64_t)-1,  NV);
  
  delegate::write(local_bfs_tree+root, root); // parent of root is self
  
  csr_graph& graph = *g;
  on_all_cores([local_vlist, graph, local_bfs_tree, k2addr] {
    if ( !bfs_counters_added ) {
      bfs_counters_added = true;
      Grappa_add_profiling_counter( &bfs_neighbors_visited, "bfs_neighbors_visited", "bfsneigh", true, 0 );
      Grappa_add_profiling_counter( &bfs_vertex_visited, "bfs_vertex_visited", "bfsverts", true, 0 );
    }

    // setup globals
    vlist = local_vlist;
    xoff = graph.xoff;
    xadj = graph.xadj;
    bfs_tree = local_bfs_tree;
    k2 = k2addr;
    nadj = graph.nadj;

    int64_t k1 = 0, _k2 = 1;
    
    while (k1 != _k2) {
      VLOG(2) << "k1=" << k1 << ", k2=" << _k2;
      const int64_t oldk2 = _k2;
      
      bfs_level(k1, oldk2);
      
      barrier();
      VLOG(2) << "phase complete";
      
      k1 = oldk2;
      _k2 = delegate::read(k2);
    }
  });
  
  t = timer() - t;

  bfs_neighbors_visited = reduce<uint64_t,collective_add>(&bfs_neighbors_visited);
  bfs_vertex_visited = reduce<uint64_t,collective_add>(&bfs_vertex_visited);

  VLOG(1) << "bfs_vertex_visited = " << bfs_vertex_visited;
  VLOG(1) << "bfs_neighbors_visited = " << bfs_neighbors_visited;

  Grappa_free(local_vlist);
  
  return t;
}

