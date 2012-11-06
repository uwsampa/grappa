#include "common.h"
#include "Grappa.hpp"
#include "ForkJoin.hpp"
#include "Cache.hpp"
#include "Delegate.hpp"
#include "PerformanceTools.hpp"
#include "GlobalAllocator.hpp"
#include "timer.h"

GRAPPA_DEFINE_EVENT_GROUP(bfs);

#define read      Grappa_delegate_read_word
#define write     Grappa_delegate_write_word
#define cmp_swap  Grappa_delegate_compare_and_swap_word
#define fetch_add Grappa_delegate_fetch_and_add_word

#define BUF_LEN 16384
static int64_t buf[BUF_LEN];
static int64_t kbuf;
static GlobalAddress<int64_t> vlist;
static GlobalAddress<int64_t> xoff;
static GlobalAddress<int64_t> xadj;
static GlobalAddress<int64_t> bfs_tree;
static GlobalAddress<int64_t> k2;
static int64_t nadj;

static LocalTaskJoiner joiner;

#define XOFF(k) (xoff+2*(k))
#define XENDOFF(k) (xoff+2*(k)+1)

#define GA64(name) ((GlobalAddress<int64_t>,name))

LOOP_FUNCTOR(bfs_setup, nid, GA64(_vlist)GA64(_xoff)GA64(_xadj)GA64(_bfs_tree)GA64(_k2)((int64_t,_nadj))) {
  kbuf = 0;
  vlist = _vlist;
  xoff = _xoff;
  xadj = _xadj;
  bfs_tree = _bfs_tree;
  k2 = _k2;
  nadj = _nadj;
}

static void bfs_visit_neighbor(uint64_t packed) {
  int64_t vo = packed & 0xFFFFFFFF;
  int64_t v = packed >> 32;
  CHECK(vo < nadj) << "unpacking 'vo' unsuccessful (" << vo << " < " << nadj << ")";
  CHECK(v < nadj) << "unpacking 'v' unsuccessful (" << v << " < " << nadj << ")";  
  
  GRAPPA_EVENT(visit_neighbor_ev, "visit neighbor of vertex", 1, bfs, v);
  
  GlobalAddress<int64_t> xv = xadj+vo;
  CHECK(xv.node() < Grappa_nodes()) << " [" << xv.node() << " < " << Grappa_nodes() << "]";
  
  const int64_t j = Grappa_delegate_read_word(xv);
  if (Grappa_delegate_compare_and_swap_word(bfs_tree+j, -1, v)) {
    while (kbuf == -1) { Grappa_yield(); }
    if (kbuf < BUF_LEN) {
      buf[kbuf] = j;
      (kbuf)++;
    } else {
      kbuf = -1; // lock other threads out temporarily
      int64_t voff = Grappa_delegate_fetch_and_add_word(k2, BUF_LEN);
      Incoherent<int64_t>::RW cvlist(vlist+voff, BUF_LEN);
      for (int64_t vk=0; vk < BUF_LEN; vk++) {
        cvlist[vk] = buf[vk];
      }
      buf[0] = j;
      kbuf = 1;
    }
  }
  joiner.signal();
}

static void bfs_visit_vertex(int64_t k) {
  GlobalAddress<int64_t> vk = vlist+k;
  CHECK(vk.node() < Grappa_nodes()) << " [" << vk.node() << " < " << Grappa_nodes() << "]";
  const int64_t v = Grappa_delegate_read_word(vk);
  
  // TODO: do these two together (cache)
  const int64_t vstart = Grappa_delegate_read_word(XOFF(v));
  const int64_t vend = Grappa_delegate_read_word(XENDOFF(v));
  CHECK(vstart < nadj) << vstart << " < " << nadj;
  CHECK(vend < nadj) << vend << " < " << nadj;
  CHECK(v < (1L<<32)) << "can't pack 'v' into 32-bit value! have to get more creative";
  CHECK(vend < (1L<<32)) << "can't pack 'vo' into 32-bit value! have to get more creative";
  
  GRAPPA_EVENT(visit_vertex_ev, "visit vertex: num neighbors", 1, bfs, vend-vstart);

  for (int64_t vo = vstart; vo < vend; vo++) {
    uint64_t packed = (((uint64_t)v) << 32) | vo;
    joiner.registerTask();
    Grappa_privateTask(&bfs_visit_neighbor, packed);
  }
  joiner.signal();
}

LOOP_FUNCTOR(bfs_node, nid, ((int64_t,start)) ((int64_t,end))) {
  range_t r = blockDist(start, end, nid, Grappa_nodes());
  
  kbuf = 0;
  
  for (int64_t i = r.start; i < r.end; i++) {
    joiner.registerTask();
    Grappa_privateTask(&bfs_visit_vertex, i);
  }

  joiner.wait();
  
  // make sure to commit what's left in the buffer at the end
  if (kbuf) {
    int64_t voff = Grappa_delegate_fetch_and_add_word(k2, kbuf);
    VLOG(2) << "flushing vlist buffer (kbuf=" << kbuf << ", k2=" << voff << ")";
    Incoherent<int64_t>::RW cvlist(vlist+voff, kbuf);
    for (int64_t vk=0; vk < kbuf; vk++) {
      cvlist[vk] = buf[vk];
    }
    kbuf = 0;
  }
}

double make_bfs_tree(csr_graph * g, GlobalAddress<int64_t> bfs_tree, int64_t root) {
  int64_t NV = g->nv;
  GlobalAddress<int64_t> vlist = Grappa_typed_malloc<int64_t>(NV);
  
  double t;
  t = timer();
  
  // start with root as only thing in vlist
  Grappa_delegate_write_word(vlist, root);
  
  int64_t k1 = 0, k2 = 1;
  GlobalAddress<int64_t> k2addr = make_global(&k2);
  
  // initialize bfs_tree to -1
  Grappa_memset(bfs_tree, (int64_t)-1,  NV);
  
  Grappa_delegate_write_word(bfs_tree+root, root); // parent of root is self
  
  bfs_setup fsetup(vlist, g->xoff, g->xadj, bfs_tree, k2addr, g->nadj);
  fork_join_custom(&fsetup);
  
  while (k1 != k2) {
    VLOG(2) << "k1=" << k1 << ", k2=" << k2;
    const int64_t oldk2 = k2;
    
    bfs_node fbfs(k1, oldk2);
    fork_join_custom(&fbfs);

    k1 = oldk2;
  }
  
  t = timer() - t;
  
  Grappa_free(vlist);

  Grappa_merge_and_dump_stats();
  
  return t;
}

