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

//#include <boost/hash.hpp>
//#include <boost/unordered_set.hpp>
#include <boost/dynamic_bitset.hpp>

GRAPPA_DEFINE_EVENT_GROUP(bfs);

struct bfs_tree_entry {
  int64_t depth  : 16;
  int64_t parent : 48;

  bfs_tree_entry(): depth(-1), parent(0) {}
  bfs_tree_entry(short depth, int64_t parent): depth(depth), parent(parent) {}
};

static PushBuffer<int64_t> vlist_buf;

static GlobalAddress<int64_t> vlist;
static GlobalAddress<int64_t> xoff;
static GlobalAddress<int64_t> xadj;
static GlobalAddress<bfs_tree_entry> bfs_tree;
static GlobalAddress<int64_t> k2;
static int64_t nadj;
static int64_t nv;

static int64_t current_depth;
static int64_t nfrontier_edges; // (partitioned)
static int64_t delta_frontier_edges;
static int64_t nremaining_edges; // (partitioned)
static int64_t delta_remaining_edges;

void incr_frontier_edges(int64_t v) {
  int64_t xoff_buf[2];
  Incoherent<int64_t>::RO cxoff(xoff+2*v, 2, xoff_buf);
  const int64_t degree = cxoff[1] - cxoff[0];
  delta_frontier_edges += degree;
}

#define XOFF(k) (xoff+2*(k))
#define XENDOFF(k) (xoff+2*(k)+1)

#define GA64(name) ((GlobalAddress<int64_t>,name))

// little helper for iterating over things numerous enough to need to be buffered
#define for_buffered(i, n, start, end, nbuf) \
  for (size_t i=start, n=nbuf; i<end && (n = MIN(nbuf, end-i)); i+=nbuf)

static const size_t NBUF = (STACK_SIZE / 2) / sizeof(int64_t);


// count number of neighbors visited
static uint64_t bfs_neighbors_visited = 0;
static bool bfs_counters_added = false;

// count number of vertex visited
static uint64_t bfs_vertex_visited = 0;

//DEFINE_int64(cas_flattener_size, 20, "log2 of the number of unique elements in the hash set used to short-circuit compare and swaps");
DEFINE_double(beamer_alpha, 1.0, "Beamer BFS parameter for switching to bottom-up.");
DEFINE_double(beamer_beta, 1.0, "Beamer BFS parameter for switching back to top-down.");

//int64_t cmp_swaps_total;
//int64_t cmp_swaps_shorted;

inline bool claim_parenthood(GlobalAddress<bfs_tree_entry> target, int64_t parent) {
  cmp_swaps_total++;
  return delegate::call(target.node(), [target,parent]{
    bfs_tree_entry * t = target.pointer();
    if (t->depth == -1) {
      t->depth = current_depth;
      t->parent = p.parent;
      return true;
    } else {
      return false;
    }
  });
}

inline bool in_frontier(int64_t v) {
  GlobalAddress<bfs_tree_entry> target = bfs_tree+v;
  return delegate::call(target.node(), [target]{
    bfs_tree_entry * t = target.pointer();
    if (t->depth == -1) return false;
    return t->depth < current_depth;
  });
}

void up_visit_parents(bfs_tree_entry * parent) {
  bfs_tree_entry& p = *parent;
  
}

static unsigned mark_bfs_level = -1;
static unsigned mark_start_bottom_up = -1;
static unsigned mark_start_top_down = -1;


LOOP_FUNCTOR(setup_bfs, nid, GA64(_vlist)GA64(_xoff)GA64(_xadj)((GlobalAddress<bfs_tree_entry>,_bfs_tree))GA64(_k2)((int64_t,_nadj)) ((int64_t,_nv))) {

  if ( !bfs_counters_added ) {
    bfs_counters_added = true;
    Grappa_add_profiling_counter( &bfs_neighbors_visited, "bfs_neighbors_visited", "bfsneigh", true, 0 );
    Grappa_add_profiling_counter( &bfs_vertex_visited, "bfs_vertex_visited", "bfsverts", true, 0 );
  }

  // setup globals
  vlist = _vlist;
  xoff = _xoff;
  xadj = _xadj;
  bfs_tree = _bfs_tree;
  k2 = _k2;
  nadj = _nadj;
  nv = _nv;
  
  // initialize push buffer (so it knows where to push to)
  vlist_buf.setup(vlist, k2);

  current_depth = 0;
  nfrontier_edges = 0;
  nremaining_edges = nadj;
  delta_frontier_edges = delta_remaining_edges = 0;

  // initialize cmp_swap flat combiner
  //cmp_swaps_total = cmp_swaps_shorted = 0;
  //parent_set.clear();
  //if (combiner == NULL) { combiner = new CmpSwapCombiner(); }
  //combiner->clear();
}

LOOP_FUNCTION( bfs_finish_level, nid ) {  
  vlist_buf.flush();
  current_depth++;

  nfrontier_edges = allreduce_add(delta_frontier_edges);
  nremaining_edges -= nfrontier_edges; // allreduce_add(delta_remaining_edges);
  delta_frontier_edges = delta_remaining_edges = 0;

  //VLOG(2) << "phase complete";
}

LOOP_FUNCTION( bfs_finish, nid ) {
  bfs_neighbors_visited = allreduce_add(bfs_neighbors_visited);
  bfs_vertex_visited = allreduce_add(bfs_vertex_visited);
  cmp_swaps_shorted = allreduce_add(cmp_swaps_shorted);
  cmp_swaps_total = allreduce_add(cmp_swaps_total);
}

void convert_tree_entry(bfs_tree_entry * e) {
  int64_t * t = reinterpret_cast<int64_t*>(e);
  if (e->depth() == -1) {
    *t = -1;
  } else { 
    *t = e->parent();
  }
  CHECK( *t < nv) << "bfs_tree[" << make_linear(e)-bfs_tree << "] = " << e->parent() << " (depth=" << e->depth() << ")";
}

#ifdef VTRACE

#define GRAPPA_MARKER(name, marker, value) \
      do { char s[256]; sprintf(s, "<%ld>", value); VT_MARKER(marker, s); } while (0)
#define GRAPPA_TRACE_START(name) VT_USER_START(name)
#define GRAPPA_TRACE_END(name) VT_USER_END(name)

#else

#define GRAPPA_MARKER(name,marker,value)  do {} while (0)
#define GRAPPA_TRACE_START(name)          do {} while (0)
#define GRAPPA_TRACE_END(name)            do {} while (0)

#endif

double make_bfs_tree(csr_graph * g, GlobalAddress<int64_t> in_bfs_tree, int64_t root) {
  int64_t NV = g->nv;
  GlobalAddress<int64_t> vlist = Grappa_typed_malloc<int64_t>(NV);
  GlobalAddress<bfs_tree_entry> bfs_tree = (GlobalAddress<bfs_tree_entry>)in_bfs_tree;
 
#ifdef VTRACE 
  if (mark_bfs_level == -1) mark_bfs_level = VT_MARKER_DEF("bfs_level", VT_MARKER_TYPE_HINT);
  if (mark_start_bottom_up == -1) mark_start_bottom_up = VT_MARKER_DEF("bfs_start_bottom_up", VT_MARKER_TYPE_HINT);
  if (mark_start_top_down == -1) mark_start_top_down = VT_MARKER_DEF("bfs_start_top_down", VT_MARKER_TYPE_HINT);
#endif

  double t;
  t = timer();
  
  // start with root as only thing in vlist
  write(vlist, root);
  nfrontier_edges = 1;
  
  int64_t k1 = 0, k2 = 1;
  GlobalAddress<int64_t> k2addr = make_global(&k2);
  
  // setup globals on all nodes
  { setup_bfs f(vlist, g->xoff, g->xadj, bfs_tree, k2addr, g->nadj, g->nv); fork_join_custom(&f); }
  
  // initialize bfs_tree to -1
  //Grappa_memset_local(bfs_tree, bfs_tree_entry(),  NV);
  forall_local<bfs_tree_entry,init_bfs_tree>(bfs_tree, NV);
  
  write(bfs_tree+root, root); // parent of root is self
  
  bool top_down = true;
  size_t prev_nf = -1;
  
  GRAPPA_TRACE_START("top_down");
  while (k1 != k2) {
    const int64_t oldk2 = k2;
    const size_t nf = k2-k1; // num vertices in frontier
    VLOG(2) << "k1=" << k1 << ", k2=" << k2 << ", m_f=" << nfrontier_edges << ", m_u=" << nremaining_edges << ", nf="<<nf<<", alpha=" << FLAGS_beamer_alpha << ", beta=" << FLAGS_beamer_beta;
    GRAPPA_MARKER("bfs_level", mark_bfs_level, nf);

    if ( top_down && nfrontier_edges > nremaining_edges/FLAGS_beamer_alpha && nf > prev_nf) {
      VLOG(2) << "switching to bottom_up";
      GRAPPA_MARKER("bfs_bottom_up", mark_start_bottom_up, nf);
      GRAPPA_TRACE_END("top_down");
      GRAPPA_TRACE_START("bottom_up");
      top_down = false;
    } else if (!top_down && nf < NV/FLAGS_beamer_beta && nf < prev_nf) {
      VLOG(2) << "switching to top_down";
      GRAPPA_MARKER("bfs_top_down", mark_start_top_down, nf);
      GRAPPA_TRACE_END("bottom_up");
      GRAPPA_TRACE_START("top_down");
      top_down = true;
    }

    if (top_down) {
      VLOG(2) << "top_down";
      // top-down level
      // forall_local<int64_t,visit_frontier>(vlist+k1, k2-k1);
      forall_localized(vlist+k1, k2-k1, [](int64_t i, int64_t& v){
        ++bfs_vertex_visited;
        
        int64_t buf[2];
        Incoherent<int64_t>::RO cxoff(xoff+2*v, 2, buf);
        const int64_t vstart = cxoff[0], vend = cxoff[1];
        
        // forall_local_async<int64_t,int64_t,visit_neighbor>(xadj+vstart, vend-vstart, make_linear(va));
        forall_localized_async(xadj+vstart, vend-vstart, [v](int64_t ji, int64_t j) {
          ++bfs_neighbors_visited;
          
          // TODO: feed-forward-ize
          if (claim_parenthood(bfs_tree+j, v)) {
            vlist_buf.push(j);
            incr_frontier_edges(j);
          }
        });

      });
    } else {
      // bottom-up level
      // forall_local<bfs_tree_entry,up_visit_parents,1>(bfs_tree, NV);
      forall_localized(bfs_tree, NV, [](int64_t ti, bfs_tree_entry& p){
        if (p.depth() != -1) return;
        
        ++bfs_vertex_visited;
        
        const int64_t v = make_linear(&p) - bfs_tree;
        
        int64_t xoff_buf[2];
        Incoherent<int64_t>::RO cxoff(xoff+2*v, 2, xoff_buf);
        const int64_t vstart = cxoff[0], vend = cxoff[1];
        
        for_buffered(j, nc, vstart, vend, NBUF) {
          int64_t adj_buf[NBUF];
          Incoherent<int64_t>::RO cadj(xadj+j, nc, adj_buf);
          for (size_t k=0; k<nc; k++) {
            
            ++bfs_neighbors_visited;
            const int64_t n = cadj[k];
            
            if (in_frontier(n)) {
              p.set(current_depth, n);
              vlist_buf.push(v);
              //incr_frontier_edges(v); // not strictly necessary to keep count anymore
              return;
            }
            
          }
        }
      });
    }

    { bfs_finish_level f; fork_join_custom(&f); }
    
    prev_nf = nf;
    k1 = oldk2;
    k2 = k2;
  }
  t = timer() - t;
  GRAPPA_TRACE_END("top_down");
  
  { bfs_finish f; fork_join_custom(&f); }
  VLOG(1) << "bfs_vertex_visited = " << bfs_vertex_visited;
  VLOG(1) << "bfs_neighbors_visited = " << bfs_neighbors_visited;
  VLOG(1) << "cmp_swaps_shorted: " << cmp_swaps_shorted;
  VLOG(1) << "cmp_swaps_total: " << cmp_swaps_total;
  
  Grappa_free(vlist);
  
  // clean up bfs_tree depths
  forall_local<bfs_tree_entry,convert_tree_entry>(bfs_tree, NV);

  return t;
}

