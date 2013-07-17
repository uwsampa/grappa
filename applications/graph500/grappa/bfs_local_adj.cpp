#include "common.h"
#include "timer.h"
#include "graph.hpp"
#include <Delegate.hpp>
#include <ParallelLoop.hpp>
#include <Array.hpp>
#include <GlobalVector.hpp>
#include <utility>

using namespace Grappa;

GlobalAddress<Graph> g;
GlobalAddress<long> bfs_tree;

GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, bfs_vertex_visited);
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, bfs_edge_visited);

DEFINE_bool(cas_flatten, false, "Flatten compare-and-swaps.");

//DEFINE_int64(cas_flattener_size, 20, "log2 of the number of unique elements in the hash set used to short-circuit compare and swaps");

GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, cmp_swaps_shorted, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, cmp_swaps_total, 0);

class CmpSwapCombiner {
  size_t log2n;
  intptr_t * in_set;
public:
  CmpSwapCombiner() {
    log2n = 20;
    in_set = new intptr_t[1L << log2n];
    clear();
  }
  ~CmpSwapCombiner() { delete in_set; }

  void clear() {
    cmp_swaps_total = cmp_swaps_shorted = 0;
    memset(in_set, 0, sizeof(intptr_t)*(1L<<log2n));
  }

  template< typename T >
  bool not_done_before(GlobalAddress<T> target) {
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
      return true;
    }
  }
};

static CmpSwapCombiner * combiner = NULL;


int64_t * frontier;
int64_t frontier_sz;
int64_t frontier_head;
int64_t frontier_tail;
int64_t frontier_level_mark;

void frontier_push(int64_t v) { frontier[frontier_tail++] = v; CHECK_LT(frontier_tail, frontier_sz); }
int64_t frontier_pop() { return frontier[frontier_head++]; }
int64_t frontier_next_level_size() { return frontier_tail - frontier_level_mark; }

GlobalCompletionEvent joiner;

// MessagePool * pool;
static unsigned marker = -1;

double make_bfs_tree(GlobalAddress<Graph> g_in, GlobalAddress<int64_t> _bfs_tree, int64_t root) {
  static_assert(sizeof(long) == sizeof(int64_t), "Can't use long as substitute for int64_t");
  if (FLAGS_cas_flatten) {
    LOG_FIRST_N(INFO,1) << "bfs_version: 'grappa_visited_cache'";    
  } else {
    LOG_FIRST_N(INFO,1) << "bfs_version: 'grappa_adj'";
  }
  GRAPPA_TRACER("make_bfs_tree()");
  
  call_on_all_cores([g_in,_bfs_tree]{
    g = g_in;
    bfs_tree = _bfs_tree;
    
    frontier_sz = g->nv/cores()*2;
    frontier = locale_alloc<int64_t>(frontier_sz);
    frontier_head = 0;
    frontier_tail = 0;
    frontier_level_mark = 0;
  });
  
  double t = walltime();
  
  // initialize bfs_tree to -1, parent of root is root, and init frontier with root
  forall_localized(g->vs, g->nv, [root](int64_t i, Graph::Vertex& v){
    if (i == root) {
      v.parent = root;
      frontier_push(root);
    } else {
      v.parent = -1;
    }
  });
  
  on_all_cores([root]{
    if (FLAGS_cas_flatten) combiner = new CmpSwapCombiner();
    
    int64_t next_level_total;
    int64_t yield_ct = 0;
    do {
      frontier_level_mark = frontier_tail;
      joiner.enroll();
      barrier();
      
      while (frontier_head < frontier_level_mark) {
        int64_t sv = frontier_pop();
        CHECK( (g->vs+sv).core() == mycore() || sv == root )
          << "sv = " << sv << ", core = " << (g->vs+sv).core();
        
        auto& src_v = *(g->vs+sv).pointer();
        for (auto& ev : src_v.adj_iter()) {
          if (FLAGS_cas_flatten == false || combiner->not_done_before(g->vs+ev)) {
            GRAPPA_TRACER("call_async<>(visit_parent)");
            delegate::call_async<&joiner>(*shared_pool, (g->vs+ev).core(), [sv,ev]{
              auto& end_v = *(g->vs+ev).pointer();
              if (end_v.parent == -1) {
                end_v.parent = sv;       // set as parent
                frontier_push(ev); // visit child in next level 
              }
            });
          }
        }
      }
      joiner.complete();
      joiner.wait();
      next_level_total = allreduce<int64_t,collective_add>( frontier_tail - frontier_level_mark );      
    } while (next_level_total > 0);
  });
  
  double bfs_time = walltime() - t;
  
  VLOG(0) << "after bfs";
  
  t = walltime();
  forall_localized(g->vs, g->nv, [](int64_t i, Graph::Vertex& v){
    // delegate::write_async(*pool, bfs_tree+i, v.parent);
    delegate::write(bfs_tree+i, v.parent);
  });
  VLOG(0) << "bfs_tree_write_time: " << walltime() - t;
  
  VLOG(2) << "tree(" << root << ")" << util::array_str("", bfs_tree, g->nv, 40);
  
  call_on_all_cores([]{
    // delete pool;
    locale_free(frontier);
    if (FLAGS_cas_flatten) delete combiner;
  });
  
  return bfs_time;
}

