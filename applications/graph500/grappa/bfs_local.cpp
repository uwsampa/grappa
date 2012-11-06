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

//#include <boost/hash.hpp>
//#include <boost/unordered_set.hpp>
#include <boost/dynamic_bitset.hpp>

GRAPPA_DEFINE_EVENT_GROUP(bfs);


#define read      Grappa_delegate_read_word
#define write     Grappa_delegate_write_word
//#define cmp_swap  Grappa_delegate_compare_and_swap_word
#define fetch_add Grappa_delegate_fetch_and_add_word
#define allreduce_add Grappa_allreduce<int64_t,coll_add<int64_t>,0>

static PushBuffer<int64_t> vlist_buf;

static GlobalAddress<int64_t> vlist;
static GlobalAddress<int64_t> xoff;
static GlobalAddress<int64_t> xadj;
static GlobalAddress<int64_t> bfs_tree;
static GlobalAddress<int64_t> k2;
static int64_t nadj;
static int64_t nv;

#define XOFF(k) (xoff+2*(k))
#define XENDOFF(k) (xoff+2*(k)+1)

#define GA64(name) ((GlobalAddress<int64_t>,name))

// count number of neighbors visited
static uint64_t bfs_neighbors_visited = 0;
static bool bfs_counters_added = false;

// count number of vertex visited
static uint64_t bfs_vertex_visited = 0;

//template< typename T >
//std::size_t hash_value(GlobalAddress<T> const& a) {
  //boost::hash<int64_t> h;
  //return h((int64_t)a.raw_bits());
//}
//static boost::unordered_set< GlobalAddress<int64_t> > parent_set;

DEFINE_int64(cas_flattener_size, 20, "log2 of the number of unique elements in the hash set used to short-circuit compare and swaps");

int64_t cmp_swaps_total;
int64_t cmp_swaps_shorted;

class CmpSwapCombiner {
  size_t log2n;
  intptr_t * in_set;
public:
  CmpSwapCombiner() {
    log2n = FLAGS_cas_flattener_size;
    in_set = new intptr_t[1L << log2n];
    clear();
  }
  ~CmpSwapCombiner() { delete in_set; }

  void clear() {
    cmp_swaps_total = cmp_swaps_shorted = 0;
    memset(in_set, 0, sizeof(intptr_t)*(1L<<log2n));
  }

  bool flat_cas(GlobalAddress<int64_t> target, int64_t cmp_val, int64_t set_val) {
    cmp_swaps_total++;
    intptr_t t = target.raw_bits();
    uint64_t h = ((uint64_t)t) % (1L << log2n);

    if (in_set[h] == t) {
      cmp_swaps_shorted++;
      return false;
    } else {
      if (in_set[h] == 0) {
        in_set[h] = t;
      }
      return Grappa_delegate_compare_and_swap_word(target, cmp_val, set_val);
    }
  }
};

static CmpSwapCombiner * combiner = NULL;

//inline bool cmp_swap_short(GlobalAddress<int64_t> target, int64_t cmp_val, int64_t set_val) {
  //cmp_swaps_total++;
  //if (parent_set.count(target)) {
    //cmp_swaps_shorted++;
    //return false;
  //} else {
    //parent_set.insert(target);
    //return Grappa_delegate_compare_and_swap_word(target, cmp_val, set_val);
  //}
//}

void visit_neighbor(int64_t i, int64_t * neighbor_base, const int64_t& v) {
  ++bfs_neighbors_visited;

  CHECK( v < nv ) << "| v = " << v << ", nv = " << nv;
  CHECK( i < nv ) << "| i = " << i << " (nv = " << nv << ")";

  const int64_t j = neighbor_base[i];
  
  //if (v == 33707) { VLOG(1) << "neighbor_base = " << neighbor_base; }

  CHECK( j < nv ) << "| v[" << v << "].neighbors[" << i << "] = " << j << " (nv = " << nv << ") \n neighbor_base = " << neighbor_base;
  
  if (combiner->flat_cas(bfs_tree+j, -1, v)) {
    vlist_buf.push(j);
  }
}

/// for use with forall_local
/// @param v  vertex in frontier
void visit_frontier(int64_t * va) {
  ++bfs_vertex_visited;
  const int64_t v = *va;

  int64_t buf[2];
  Incoherent<int64_t>::RO cxoff(xoff+2*v, 2, buf);
  const int64_t vstart = cxoff[0], vend = cxoff[1];

  //if (v == 33707) {
    ////std::stringstream ss; ss << "neighbors: ";
    ////for (int64_t i=vstart; i<vend; i++) { ss << read(xadj+i) << " "; }
    ////VLOG(1) << ss.str();
    //VLOG(1) << "vstart: " << vstart << ", vend-vstart = " << vend-vstart;
    //VLOG(1) << "start node: " << (xadj+vstart).node();
  //}

  forall_local_async<int64_t,int64_t,visit_neighbor>(xadj+vstart, vend-vstart, make_linear(va));
}

static unsigned marker = -1;


LOOP_FUNCTOR(setup_bfs, nid, GA64(_vlist)GA64(_xoff)GA64(_xadj)GA64(_bfs_tree)GA64(_k2)((int64_t,_nadj)) ((int64_t,_nv))) {
  
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

  // initialize cmp_swap flat combiner
  //cmp_swaps_total = cmp_swaps_shorted = 0;
  //parent_set.clear();
  if (combiner == NULL) { combiner = new CmpSwapCombiner(); }
  combiner->clear();
}

LOOP_FUNCTION( bfs_finish_level, nid ) {  
  vlist_buf.flush();
  VLOG(2) << "phase complete";
}

LOOP_FUNCTION( bfs_finish, nid ) {
  bfs_neighbors_visited = allreduce_add(bfs_neighbors_visited);
  bfs_vertex_visited = allreduce_add(bfs_vertex_visited);
  cmp_swaps_shorted = allreduce_add(cmp_swaps_shorted);
  cmp_swaps_total = allreduce_add(cmp_swaps_total);
}

double make_bfs_tree(csr_graph * g, GlobalAddress<int64_t> bfs_tree, int64_t root) {
  int64_t NV = g->nv;
  GlobalAddress<int64_t> vlist = Grappa_typed_malloc<int64_t>(NV);
 
#ifdef VTRACE 
  if (marker == -1) marker = VT_MARKER_DEF("bfs_level", VT_MARKER_TYPE_HINT);
#endif

  double t;
  t = timer();
  
  // start with root as only thing in vlist
  write(vlist, root);
  
  int64_t k1 = 0, k2 = 1;
  GlobalAddress<int64_t> k2addr = make_global(&k2);
  
  // setup globals on all nodes
  { setup_bfs f(vlist, g->xoff, g->xadj, bfs_tree, k2addr, g->nadj, g->nv); fork_join_custom(&f); }
  
  // initialize bfs_tree to -1
  Grappa_memset(bfs_tree, (int64_t)-1,  NV);
  
  write(bfs_tree+root, root); // parent of root is self
  
  while (k1 != k2) {
    VLOG(2) << "k1=" << k1 << ", k2=" << k2;
    const int64_t oldk2 = k2;
    
    forall_local<int64_t,visit_frontier>(vlist+k1, k2-k1);

    { bfs_finish_level f; fork_join_custom(&f); }

    k1 = oldk2;
    k2 = k2;
  }
  t = timer() - t;
  
  { bfs_finish f; fork_join_custom(&f); }
  VLOG(1) << "bfs_vertex_visited = " << bfs_vertex_visited;
  VLOG(1) << "bfs_neighbors_visited = " << bfs_neighbors_visited;
  VLOG(1) << "cmp_swaps_shorted: " << cmp_swaps_shorted;
  VLOG(1) << "cmp_swaps_total: " << cmp_swaps_total;

  Grappa_free(vlist);
  
  return t;
}

