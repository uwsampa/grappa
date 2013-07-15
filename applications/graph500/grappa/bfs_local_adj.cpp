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

double make_bfs_tree(GlobalAddress<Graph> g_in, GlobalAddress<int64_t> _bfs_tree, int64_t root) {
  static_assert(sizeof(long) == sizeof(int64_t), "Can't use long as substitute for int64_t");
  LOG_FIRST_N(INFO,1) << "bfs_version: 'local_adj'";
  
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
  
  // start with root as only thing in frontier
  delegate::call((g->vs+root).core(), [root]{ frontier_push(root); });
  
  // initialize bfs_tree to -1
  // Grappa::memset(bfs_tree, -1,  g->nv);
  forall_localized(g->vs, g->nv, [](Graph::Vertex& v){ v.parent = -1; });
  
  // parent of root is self
  // delegate::write(bfs_tree+root, root);
  delegate::call(g->vs+root, [root](Graph::Vertex * v){ v->parent = root; });
  
  on_all_cores([root]{
    // pool = new MessagePool(1L<<26);
    
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
          delegate::call_async<&joiner>(*shared_pool, (g->vs+ev).core(), [sv,ev]{
            auto& end_v = *(g->vs+ev).pointer();
            if (end_v.parent == -1) {
              end_v.parent = sv;       // set as parent
              frontier_push(ev); // visit child in next level 
            }
          });
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
  });
  
  return bfs_time;
}

