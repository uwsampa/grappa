#include "common.h"
#include "timer.h"
#include <Delegate.hpp>
#include <ParallelLoop.hpp>
#include <Array.hpp>
#include <GlobalVector.hpp>
#include <utility>

using namespace Grappa;

csr_graph g;
GlobalAddress<range_t> eoff;
GlobalAddress<long> bfs_tree;

GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, bfs_vertex_visited);
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, bfs_edge_visited);

double make_bfs_tree(csr_graph * g_in, GlobalAddress<int64_t> _bfs_tree, int64_t root) {
  static_assert(sizeof(long) == sizeof(int64_t), "Can't use long as substitute for int64_t");
  LOG_FIRST_N(INFO,1) << "bfs_version: 'queue'";
  
  // setup graph data structure
  auto& _g = *g_in;
  call_on_all_cores([_g,_bfs_tree]{
    g = _g;
    eoff = static_cast<GlobalAddress<range_t>>(g.xoff);
    bfs_tree = _bfs_tree;
  });

  auto frontier = GlobalVector<long>::create(g.nv);
  auto next     = GlobalVector<long>::create(g.nv);

  double t = timer();
  
  // start with root as only thing in frontier
  frontier->push(root);
  
  // initialize bfs_tree to -1
  Grappa::memset(bfs_tree, -1,  g.nv);
  // parent of root is self
  delegate::write(bfs_tree+root, root);
  
  while (!frontier->empty()) {
    // for each vertex in frontier
    forall_localized(frontier->begin(), frontier->size(), [frontier,next](long si, long& sv) {
      // for each adjacency
      auto r = delegate::read(eoff+sv);
      forall_localized_async(g.xadj+r.start, r.end-r.start, [sv,next](long ei, long& ev) {
        // if doesn't have a parent yet... 
        if (delegate::compare_and_swap(bfs_tree+ev, -1, sv)) {
          // add to next phase
          next->push(ev);
        }
      });
    });
    std::swap(frontier, next);
    next->clear();
  }
  
  t = timer() - t;
  
  frontier->destroy();
  next->destroy();
  
  return t;
}

