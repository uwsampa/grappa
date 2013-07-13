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

double make_bfs_tree(GlobalAddress<Graph> g_in, GlobalAddress<int64_t> _bfs_tree, int64_t root) {
  static_assert(sizeof(long) == sizeof(int64_t), "Can't use long as substitute for int64_t");
  LOG_FIRST_N(INFO,1) << "bfs_version: 'local_adj'";
  
  call_on_all_cores([g_in,_bfs_tree]{
    g = g_in;
    bfs_tree = _bfs_tree;
  });
  
  // auto frontier = GlobalVector<long>::create(g.nv);
  // auto next     = GlobalVector<long>::create(g.nv);
  // 
  // DVLOG(1) << "makebfs_tree(" << root << ")";
  // 
  double t = walltime();
  // 
  // // start with root as only thing in frontier
  // frontier->push(root);
  // 
  // // initialize bfs_tree to -1
  // Grappa::memset(bfs_tree, -1,  g.nv);
  // // parent of root is self
  // delegate::write(bfs_tree+root, root);
  // 
  // while (!frontier->empty()) {
  //   forall_localized(frontier->begin(), frontier->size(), [frontier,next](long si, long& sv) {
  //     ++bfs_vertex_visited;
  //     auto r = delegate::read(eoff+sv);
  //     forall_localized_async(g.xadj+r.start, r.end-r.start, [sv,next](long ei, long& ev) {
  //       ++bfs_edge_visited;
  //       if (delegate::compare_and_swap(bfs_tree+ev, -1, sv)) {
  //         next->push(ev);
  //       }
  //     });
  //   });
  //   std::swap(frontier, next);
  //   next->clear();
  // }
  // 
  t = walltime() - t;
  // 
  // frontier->destroy();
  // next->destroy();
  
  return t;
}

