#include "common.h"
#include "timer.h"
#include <Grappa.hpp>
#include <ForkJoin.hpp>
#include <Cache.hpp>
#include <Delegate.hpp>
#include <PerformanceTools.hpp>
#include <GlobalAllocator.hpp>
#include <GlobalTaskJoiner.hpp>
#include <AsyncParallelFor.hpp>
#include <PushBuffer.hpp>
#include <Collective.hpp>
#include <Array.hpp>
#include <Collective.hpp>
#include <ParallelLoop.hpp>

using namespace Grappa;

GRAPPA_DEFINE_EVENT_GROUP(bfs);

static PushBuffer<int64_t> vlist_buf;

static GlobalAddress<int64_t> vlist;
static GlobalAddress<int64_t> xoff;
static GlobalAddress<int64_t> xadj;
static GlobalAddress<int64_t> bfs_tree;
static GlobalAddress<int64_t> k2;
static int64_t nadj;
static int64_t nv;
static int64_t current_root;

#define XOFF(k) (xoff+2*(k))
#define XENDOFF(k) (xoff+2*(k)+1)

// count number of neighbors visited
static uint64_t bfs_neighbors_visited = 0;
static bool bfs_counters_added = false;

// count number of vertex visited
static uint64_t bfs_vertex_visited = 0;

static unsigned marker = -1;

double make_bfs_tree(csr_graph * g_in, GlobalAddress<int64_t> _bfs_tree, int64_t root) {
  LOG_FIRST_N(INFO,1) << "bfs_version: 'localized'";
  
  int64_t NV = g_in->nv;
  GlobalAddress<int64_t> _vlist = Grappa_typed_malloc<int64_t>(NV);
  DVLOG(1) << "make_bfs_tree(" << root << ")";
#ifdef VTRACE 
  if (marker == -1) marker = VT_MARKER_DEF("bfs_level", VT_MARKER_TYPE_HINT);
#endif

  double t;
  t = timer();
  
  // start with root as only thing in vlist
  delegate::write(_vlist, root);
  
  int64_t k1 = 0, _k2 = 1;
  GlobalAddress<int64_t> k2addr = make_global(&_k2);
  
  // setup globals on all nodes
  
  auto& g = *g_in;
  
  on_all_cores([_vlist,g,_bfs_tree,k2addr,root]{
    if ( !bfs_counters_added ) {
      bfs_counters_added = true;
      Grappa_add_profiling_counter( &bfs_neighbors_visited, "bfs_neighbors_visited", "bfsneigh", true, 0 );
      Grappa_add_profiling_counter( &bfs_vertex_visited, "bfs_vertex_visited", "bfsverts", true, 0 );
    }
    
    // setup globals
    vlist = _vlist;
    xoff = g.xoff;
    xadj = g.xadj;
    bfs_tree = _bfs_tree;
    k2 = k2addr;
    nadj = g.nadj;
    nv = g.nv;
    current_root = root;
    
    // initialize push buffer (so it knows where to push to)
    vlist_buf.setup(vlist, k2);
  });
  
  // initialize bfs_tree to -1
  Grappa::memset(_bfs_tree, (int64_t)-1,  NV);
  
  delegate::write(_bfs_tree+root, root); // parent of root is self
  
  CHECK_EQ(delegate::read(_bfs_tree+root), root);
  
  while (k1 != _k2) {
    VLOG(2) << "k1=" << k1 << ", k2=" << _k2;
    const int64_t oldk2 = _k2;
    
    forall_localized(_vlist+k1, _k2-k1, [](int64_t index, int64_t& source_v) {
      DVLOG(4) << "[" << index << "] source_v = " << source_v;
      ++bfs_vertex_visited;
      
      int64_t buf[2];
      Incoherent<int64_t>::RO cxoff(xoff+2*source_v, 2, buf);
      const int64_t vstart = cxoff[0], vend = cxoff[1];
      
      forall_localized_async(xadj+vstart, vend-vstart, [source_v](int64_t index, int64_t& ev){
        ++bfs_neighbors_visited;
        
        DCHECK( source_v < nv ) << "| v = " << source_v << ", nv = " << nv;
        DCHECK( index < nv ) << "| i = " << index << " (nv = " << nv << ")";
        
        const int64_t j = ev;
        
        DCHECK( j < nv ) << "| v[" << source_v << "].neighbors[" << index << "] = " << j << " (nv = " << nv << ") \n &edge = " << &ev;
        
        if (delegate::compare_and_swap(bfs_tree+j, -1, source_v)) {
          DCHECK_NE(j, current_root);
          vlist_buf.push(j);
        }
      });
    });

    on_all_cores([]{
      vlist_buf.flush();
      VLOG(2) << "phase complete";
    });
    k1 = oldk2;
    _k2 = _k2;
  }
  t = timer() - t;
  
  bfs_neighbors_visited = Grappa::reduce<uint64_t,collective_add>(&bfs_neighbors_visited);
  bfs_vertex_visited    = Grappa::reduce<uint64_t,collective_add>(&bfs_vertex_visited);
  VLOG(1) << "bfs_vertex_visited: " << bfs_vertex_visited;
  VLOG(1) << "bfs_neighbors_visited: " << bfs_neighbors_visited;

  Grappa_free(_vlist);
  
  return t;
}

