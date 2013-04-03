#include "Grappa.hpp"
#include "Addressing.hpp"
#include "Cache.hpp"
#include "Delegate.hpp"
#include "ForkJoin.hpp"
#include "GlobalAllocator.hpp"
#include "Collective.hpp"
#include "GlobalTaskJoiner.hpp"
#include "PerformanceTools.hpp"
#include <ParallelLoop.hpp>
#include <Array.hpp>

#include "verify.hpp"

#include "oned_csr.h"
#include "timer.h"
#include "options.h"

using namespace Grappa;

static int64_t nedge_traversed;

void compute_levels(GlobalAddress<int64_t> level, int64_t nv, GlobalAddress<int64_t> bfs_tree, int64_t root) {
  
  Grappa::memset(level, (int64_t)-1, nv);
  delegate::write(level+root, 0);
  
  forall_localized(level, nv, [bfs_tree, level, nv, root] (int64_t k, int64_t& level_k) {
    if (level_k >= 0) return;
  
    int64_t tree_k = delegate::read(bfs_tree+k);
    if (tree_k >= 0 && k != root) {
      int64_t parent = k;
      int64_t nhop = 0;
      int64_t next_parent;
    
      /* Run up the three until we encounter an already-leveled vertex. */
      while (parent >= 0 && delegate::read(level+parent) < 0 && nhop < nv) {
        next_parent = delegate::read(bfs_tree+parent);
        assert(parent != next_parent);
        parent = next_parent;
        ++nhop;
      }
      CHECK(nhop < nv) << "Error: root " << k << " had a cycle.";
      CHECK(parent >= 0) << "Ran off the end for root " << k << ".";
    
      // Now assign levels until we meet an already-leveled vertex
      // NOTE: This permits benign races if parallelized.
      nhop += delegate::read(level+parent);
      parent = k;
      while (delegate::read(level+parent) < 0) {
        CHECK_GT(nhop, 0);
        delegate::write(level+parent, nhop);
        nhop--;
        parent = delegate::read(bfs_tree+parent);
      }
      CHECK_EQ(nhop, delegate::read(level+parent));
    
      // Internal check to catch mistakes in races...
#if defined(DEBUG)
      nhop = 0;
      parent = k;
      int64_t lastlvl = delegate::read(level+k) + 1;
      while ((next_parent = delegate::read(level+parent)) > 0) {
        CHECK_EQ(lastlvl, (1+next_parent));
        lastlvl = next_parent;
        parent = delegate::read(bfs_tree+parent);
        nhop++;
      }
#endif
    }
  });  
}

static int64_t load_nedge(int64_t root, GlobalAddress<int64_t> bfs_tree) {
  char fname[256];
  sprintf(fname, "ckpts/graph500.%lld.%lld.%lld%s.nedge", SCALE, edgefactor, root, (use_RMAT)?".rmat":"");
  FILE * fin = fopen(fname, "r");
  if (!fin) {
    LOG(ERROR) << "Unable to open file: " << fname << ", will do verify manually and save checkpoint.";
    return -1;
  }
  
  int64_t nedge_traversed;
  fread(&nedge_traversed, sizeof(nedge_traversed), 1, fin);
  return nedge_traversed;
}

static void save_nedge(int64_t root, int64_t nedge_traversed, GlobalAddress<int64_t> bfs_tree) {
  char fname[256];
  sprintf(fname, "ckpts/graph500.%lld.%lld.%lld%s.nedge", SCALE, edgefactor, root, (use_RMAT)?".rmat":"");
  FILE * fout = fopen(fname, "w");
  if (!fout) {
    LOG(ERROR) << "Unable to open file for writing: " << fname;
    return;
  }
  
  fwrite(&nedge_traversed, sizeof(nedge_traversed), 1, fout);
  
  fclose(fout);
}

int64_t verify_bfs_tree(GlobalAddress<int64_t> bfs_tree, int64_t max_bfsvtx, int64_t root, tuple_graph * tg) {
  
  static int nverified = 0;
  if (nverified > 0) {
    LOG(INFO) << "warning: skipping verification!!";    
    return nedge_traversed;
  }
  nverified++;
  
  //TAU_PHASE("verify_bfs_tree", "int64_t (GlobalAddress<int64_t>,int64_t,int64_t,tuple_graph*)", TAU_USER);
  
  CHECK_EQ(delegate::read(bfs_tree+root), root);
  
  int64_t nv = max_bfsvtx+1;
  
  if (!verify) {
    LOG(INFO) << "warning: skipping verification!!";
    nedge_traversed = load_nedge(root, bfs_tree);

    if (nedge_traversed != -1) {
      return nedge_traversed;
    }
  }
  
  int64_t err = 0;
  
  GlobalAddress<int64_t> seen_edge = Grappa_typed_malloc<int64_t>(nv);
  GlobalAddress<int64_t> level = Grappa_typed_malloc<int64_t>(nv);
  
  double t;
  TIME(t, compute_levels(level, nv, bfs_tree, root));
  VLOG(1) << "compute_levels time: " << t;
  
  t = timer();
  Grappa::memset(seen_edge, 0, nv);
  t = timer() - t;
  VLOG(1) << "set_const time: " << t;
  
  t = timer();
  
  call_on_all_cores([]{ nedge_traversed = 0; });
    
  forall_localized(tg->edges, tg->nedge, [seen_edge, bfs_tree, level, max_bfsvtx]
      (int64_t e, packed_edge& cedge) {  
    const int64_t i = cedge.v0;
    const int64_t j = cedge.v1;
    int64_t lvldiff;

    if (i < 0 || j < 0) return;
    CHECK(!(i > max_bfsvtx && j <= max_bfsvtx)) << "Error!";
    CHECK(!(j > max_bfsvtx && i <= max_bfsvtx)) << "Error!";
    if (i > max_bfsvtx) // both i & j are on the same side of max_bfsvtx
      return;

    // All neighbors must be in the tree.
    int64_t ti = delegate::read(bfs_tree+i);
    int64_t tj = delegate::read(bfs_tree+j);

    CHECK(!(ti >= 0 && tj < 0)) << "Error! ti=" << ti << ", tj=" << tj;
    CHECK(!(tj >= 0 && ti < 0)) << "Error! ti=" << ti << ", tj=" << tj;
    if (ti < 0) // both i & j have the same sign
      return;

    /* Both i and j are in the tree, count as a traversed edge.
 
     NOTE: This counts self-edges and repeated edges.  They're
     part of the input data.
     */
    nedge_traversed++;

    // Mark seen tree edges.
    if (i != j) {
      if (ti == j)
        delegate::write(seen_edge+i, 1);
      if (tj == i)
        delegate::write(seen_edge+j, 1);
    }
    lvldiff = delegate::read(level+i) - delegate::read(level+j);
    /* Check that the levels differ by no more than one. */
    CHECK(!(lvldiff > 1 || lvldiff < -1)) << "Error, levels differ by more than one! "
      << "(k = " << e << ", lvl[" << i << "]=" << delegate::read(level+i) << ", lvl[" << j << "]=" << delegate::read(level+j) << ")";
  });
  nedge_traversed = Grappa::reduce<int64_t,collective_add>(&nedge_traversed);
  
  t = timer() - t;
  VLOG(1) << "verify_func time: " << t;

  /* Check that every BFS edge was seen and that there's only one root. */
  forall_localized(bfs_tree, nv, [root, seen_edge](int64_t k, int64_t& tk){
    if (k != root) {
      CHECK(!(tk >= 0 && !delegate::read(seen_edge+k))) << "Error!";
      CHECK_NE(tk, k) << "Error!";
    }
  });  
  VLOG(1) << "final_verify_func time: " << t;  
  
  Grappa_free(seen_edge);
  Grappa_free(level);
    
  // everything checked out...
  if (!verify) save_nedge(root, nedge_traversed, bfs_tree);
    
  return nedge_traversed;
}
