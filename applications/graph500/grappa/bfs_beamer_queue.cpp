#include "common.h"
#include "timer.h"
#include <Delegate.hpp>
#include <ParallelLoop.hpp>
#include <Array.hpp>
#include <GlobalVector.hpp>
#include <utility>

using namespace Grappa;
namespace d = Grappa::delegate;

DECLARE_double(beamer_alpha);
DECLARE_double(beamer_beta);

GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, bfs_vertex_visited);
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, bfs_edge_visited);

// little helper for iterating over things numerous enough to need to be buffered
#define for_buffered(i, n, start, end, nbuf) \
  for (size_t i=start, n=nbuf; i<end && (n = MIN(nbuf, end-i)); i+=nbuf)

struct BFSParent {
  int64_t depth  : 16;
  int64_t parent : 48;
  
  BFSParent(): depth(-1), parent(-1) {}
  BFSParent(short depth, int64_t parent): depth(depth), parent(parent) {}
  
  bool operator==(const BFSParent& o) const { return o.depth == depth && o.parent == parent; }
};

static csr_graph g;
static GlobalAddress<range_t> eoff;
static GlobalAddress<BFSParent> bfs_tree;
static int64_t current_depth;
static int64_t delta_frontier_edges;

static const size_t NBUF = 64;

double make_bfs_tree(csr_graph * g_in, GlobalAddress<int64_t> _bfs_tree, int64_t root) {
  static_assert(sizeof(long) == sizeof(int64_t), "Can't use long as substitute for int64_t");
  LOG_FIRST_N(INFO,1) << "bfs_version: 'beamer_queue'";
  
  auto& _g = *g_in;
  call_on_all_cores([_g,_bfs_tree]{
    g = _g;
    eoff = static_cast<GlobalAddress<range_t>>(g.xoff);
    auto bt = _bfs_tree;
    bfs_tree = static_cast<GlobalAddress<BFSParent>>(bt);
    current_depth = 0;
  });

  auto frontier = GlobalVector<long>::create(g.nv);
  auto next     = GlobalVector<long>::create(g.nv);
  
  DVLOG(1) << "make_bfs_tree (" << root << ")";
  
  double t = timer();
  
  // start with root as only thing in frontier
  frontier->push(root);
  
  // initialize bfs_tree to -1
  Grappa::memset(bfs_tree, BFSParent(),  g.nv);
  // parent of root is self
  d::write(bfs_tree+root, BFSParent(0, root));
  
  bool top_down = true;
  size_t prev_nf = -1;
  int64_t frontier_edges = 0;
  int64_t remaining_edges = g.nadj;
  
  while (!frontier->empty()) {
    auto nf = frontier->size();
    VLOG(3) << "remaining_edges = " << remaining_edges << ", nf = " << nf << ", prev_nf = " << prev_nf;
    if (top_down && frontier_edges > remaining_edges/FLAGS_beamer_alpha && nf > prev_nf) {
      VLOG(2) << "switching to bottom-up";
      top_down = false;
    } else if (!top_down && frontier_edges < g.nv/FLAGS_beamer_beta && nf < prev_nf) {
      VLOG(2) << "switching to top-down";
      top_down = true;
    }
    
    if (top_down) {
      VLOG(2) << "top_down";
      // top-down level
      forall_localized(frontier->begin(), frontier->size(), [next](long si, long& sv) {
        ++bfs_vertex_visited;
        auto r = d::read(eoff+sv);
        forall_localized_async(g.xadj+r.start, r.end-r.start, [sv,next](long ei, long& ev) {
          ++bfs_edge_visited;
          if (d::compare_and_swap(bfs_tree+ev, BFSParent(), BFSParent(current_depth, sv))) {
            next->push(ev);
            delta_frontier_edges += d::call(eoff+ev,[](range_t* r){ return r->end-r->start; });
          }
        });
      });
    } else {
      VLOG(2) << "bottom_up";
      // bottom-up level
      forall_localized(bfs_tree, g.nv, [next](int64_t sv, BFSParent& p){
        if (p.depth != -1) return;
        ++bfs_vertex_visited;
        auto r = d::read(eoff+sv);
        for_buffered(j, nc, r.start, r.end, NBUF) {
          int64_t adj_buf[NBUF];
          Incoherent<int64_t>::RO cadj(g.xadj+j, nc, adj_buf);
          for (size_t k=0; k<nc; k++) {
            ++bfs_edge_visited;
            const int64_t ev = cadj[k];
            
            if (d::call(bfs_tree+ev,
              [](BFSParent* t){ return t->depth != -1 && t->depth < current_depth; }
            )) {
              p = {current_depth, ev};
              next->push(sv);
              return;
            }
          }
        }
      });
    }
    std::swap(frontier, next);
    next->clear();
    frontier_edges = reduce<int64_t,collective_add>(&delta_frontier_edges);
    remaining_edges -= frontier_edges;
    prev_nf = nf;
    call_on_all_cores([]{
      current_depth++;
      delta_frontier_edges = 0;
    });
  }
  
  t = timer() - t;
  
  frontier->destroy();
  next->destroy();
  
  // convert tree-entries back to just ints (drop depth)
  forall_localized(bfs_tree, g.nv, [](int64_t i, BFSParent& p){
    int64_t * t = reinterpret_cast<int64_t*>(&p);
    if (p.depth == -1) {
      *t = -1;
    } else {
      *t = p.parent;
    }
  });
  
  return t;
}

