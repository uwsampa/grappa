#include "SoftXMT.hpp"
#include "Addressing.hpp"
#include "Cache.hpp"
#include "Delegate.hpp"
#include "ForkJoin.hpp"
#include "GlobalAllocator.hpp"

#include "verify.hpp"

#include "oned_csr.h"
#include "timer.h"
#include "options.h"

#include <TAU.h>

#define read SoftXMT_delegate_read_word
#define write SoftXMT_delegate_write_word

LOOP_FUNCTOR(compute_levels_func, k, ((GlobalAddress<int64_t>,bfs_tree))((GlobalAddress<int64_t>,level))((int64_t,nv))((int64_t,root)) ) {
  int64_t level_k = read(level+k);
  if (level_k >= 0) return;
  
  int64_t tree_k = read(bfs_tree+k);
  if (tree_k >= 0 && k != root) {
    int64_t parent = k;
    int64_t nhop = 0;
    int64_t next_parent;
    
    /* Run up the three until we encounter an already-leveled vertex. */
    while (parent >= 0 && read(level+parent) < 0 && nhop < nv) {
      next_parent = read(bfs_tree+parent);
      assert(parent != next_parent);
      parent = next_parent;
      ++nhop;
    }
    assert(nhop < nv); // no cycles
    if (nhop >= nv) { LOG(ERROR) << "Error: root " << k << " had a cycle."; }
    assert(parent >= 0); // did not run off the end
    if (parent < 0) { LOG(ERROR) << "Error: ran off the end for root " << k << "."; }
    
    // Now assign levels until we meet an already-leveled vertex
    // NOTE: This permits benign races if parallelized.
    nhop += read(level+parent);
    parent = k;
    while (read(level+parent) < 0) {
      assert(nhop > 0);
      write(level+parent, nhop);
      nhop--;
      parent = read(bfs_tree+parent);
    }
    assert(nhop == read(level+parent));
    
    // Internal check to catch mistakes in races...
#if defined(DEBUG)
    nhop = 0;
    parent = k;
    int64_t lastlvl = read(level+k) + 1;
    while ((next_parent = read(level+parent)) > 0) {
      assert(lastlvl == (1+next_parent));
      lastlvl = next_parent;
      parent = read(bfs_tree+parent);
      nhop++;
    }
#endif
  }
}

void compute_levels(GlobalAddress<int64_t> level, int64_t nv, GlobalAddress<int64_t> bfs_tree, int64_t root) {
  
  SoftXMT_memset(level, (int64_t)-1, nv);

  write(level+root, 0);
  
  compute_levels_func fl;
  fl.bfs_tree = bfs_tree;
  fl.level = level;
  fl.nv = nv;
  fl.root = root;
  fork_join(&fl, 0, nv);
}

LOOP_FUNCTOR(verify_func, k, ((GlobalAddress<int64_t>,bfs_tree)) ((GlobalAddress<packed_edge>,edges)) ((GlobalAddress<int64_t>,seen_edge)) ((GlobalAddress<int64_t>,level)) ((GlobalAddress<int64_t>,nedge_traversed)) ((GlobalAddress<int64_t>,err)) ((int64_t,max_bfsvtx)) ) {
  int terr;
  Incoherent<packed_edge>::RO cedge(edges+k, 1);
  const int64_t i = cedge[0].v0;
  const int64_t j = cedge[0].v1;
  int64_t lvldiff;
  
  if (i < 0 || j < 0) return;
  if (i > max_bfsvtx && j <= max_bfsvtx) {
    terr = -10;
    LOG(ERROR) << "Error!";
    return;
  }
  if (j > max_bfsvtx && i <= max_bfsvtx) {
    terr = -11;
    LOG(ERROR) << "Error!";
    return;
  }
  if (i > max_bfsvtx) // both i & j are on the same side of max_bfsvtx
    return;
  
  // All neighbors must be in the tree.
  int64_t ti = read(bfs_tree+i);
  int64_t tj = read(bfs_tree+j);
  
  if (ti >= 0 && tj < 0) { terr = -12; LOG(ERROR) << "Error! ti=" << ti << ", tj=" << tj; return; }
  if (tj >= 0 && ti < 0) { terr = -13; LOG(ERROR) << "Error! ti=" << ti << ", tj=" << tj; return; }
  if (ti < 0) // both i & j have the same sign
    return;
  
  /* Both i and j are in the tree, count as a traversed edge.
   
   NOTE: This counts self-edges and repeated edges.  They're
   part of the input data.
   */
  SoftXMT_delegate_fetch_and_add_word(nedge_traversed, 1);
  // Mark seen tree edges.
  if (i != j) {
    if (ti == j)
      write(seen_edge+i, 1);
    if (tj == i)
      write(seen_edge+j, 1);
  }
  lvldiff = read(level+i) - read(level+j);
  /* Check that the levels differ by no more than one. */
  if (lvldiff > 1 || lvldiff < -1) {
    terr = -14;
    LOG(ERROR) << "Error, levels differ by more than one! (k = " << k << ", lvl[" << i << "]=" << read(level+i) << ", lvl[" << j << "]=" << read(level+j) << ")";
    exit(1);
  }
}

LOOP_FUNCTOR(final_verify_func, k, ((GlobalAddress<int64_t>,bfs_tree)) ((GlobalAddress<int64_t>,seen_edge)) ((GlobalAddress<int64_t>,err)) ((int64_t,root))) {
  if (k != root) {
    int64_t tk = read(bfs_tree+k);
    if (tk >= 0 && !read(seen_edge+k)) {
      write(err, -15);
      LOG(ERROR) << "Error!";
    }
    if (tk == k) {
      write(err, -16);
      LOG(ERROR) << "Error!";
    }
  }
}

static int64_t load_nedge(int64_t root, GlobalAddress<int64_t> bfs_tree) {
  char fname[256];
  sprintf(fname, "ckpts/graph500.%lld.%lld.%lld.nedge", SCALE, edgefactor, root);
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
  sprintf(fname, "ckpts/graph500.%lld.%lld.%lld.nedge", SCALE, edgefactor, root);
  FILE * fout = fopen(fname, "w");
  if (!fout) {
    LOG(ERROR) << "Unable to open file for writing: " << fname;
    exit(1);
  }
  
  fwrite(&nedge_traversed, sizeof(nedge_traversed), 1, fout);
  
  fclose(fout);
}

int64_t verify_bfs_tree(GlobalAddress<int64_t> bfs_tree, int64_t max_bfsvtx, int64_t root, tuple_graph * tg) {
  TAU_PHASE("verify_bfs_tree", "int64_t (GlobalAddress<int64_t>,int64_t,int64_t,tuple_graph*)", TAU_USER);
  
  assert(read(bfs_tree+root) == root);
  
  int64_t nedge_traversed = 0;
  int64_t nv = max_bfsvtx+1;
  
  if (!verify) {
    LOG(INFO) << "warning: skipping verification!!";
    nedge_traversed = load_nedge(root, bfs_tree);

    if (nedge_traversed != -1) {
      return nedge_traversed;
    }
  }
  
  int64_t err = 0;
  
  GlobalAddress<int64_t> seen_edge = SoftXMT_typed_malloc<int64_t>(nv);
  GlobalAddress<int64_t> level = SoftXMT_typed_malloc<int64_t>(nv);
  
  double t;
  TIME(t, compute_levels(level, nv, bfs_tree, root));
  VLOG(1) << "compute_levels time: " << t;
  
  t = timer();
  SoftXMT_memset(seen_edge, (int64_t)0, nv);
  t = timer() - t;
  VLOG(1) << "set_const time: " << t;
  
  t = timer();
  verify_func fv;
  fv.bfs_tree = bfs_tree;
  fv.edges = tg->edges;
  fv.seen_edge = seen_edge;
  fv.level = level;
  fv.nedge_traversed = make_global(&nedge_traversed);
  fv.max_bfsvtx = max_bfsvtx;
  fv.err = make_global(&err);
  fork_join(&fv, 0, tg->nedge);
  t = timer() - t;
  VLOG(1) << "verify_func time: " << t;
  
  if (!err) {
    /* Check that every BFS edge was seen and that there's only one root. */
    t = timer();
    final_verify_func ff;
    ff.bfs_tree = bfs_tree;
    ff.seen_edge = seen_edge;
    ff.err = make_global(&err);
    ff.root = root;
    fork_join(&ff, 0, nv);
    t = timer() - t;
    VLOG(1) << "final_verify_func time: " << t;
  }
  
  SoftXMT_free(seen_edge);
  SoftXMT_free(level);
  
  if (err) {
    return err;
  } else {
    // everything checked out...
    if (!verify) save_nedge(root, nedge_traversed, bfs_tree);
    
    return nedge_traversed;
  }
}
