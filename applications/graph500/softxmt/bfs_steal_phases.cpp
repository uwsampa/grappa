#include "common.h"
#include "SoftXMT.hpp"
#include "ForkJoin.hpp"
#include "Cache.hpp"
#include "Delegate.hpp"
#include "PerformanceTools.hpp"
#include "GlobalAllocator.hpp"
#include "timer.h"

GRAPPA_DEFINE_EVENT_GROUP(bfs);

#define read      SoftXMT_delegate_read_word
#define write     SoftXMT_delegate_write_word
#define cmp_swap  SoftXMT_delegate_compare_and_swap_word
#define fetch_add SoftXMT_delegate_fetch_and_add_word

#define BUF_LEN 16384
static int64_t buf[BUF_LEN];
static int64_t kbuf;
static GlobalAddress<int64_t> vlist;
static GlobalAddress<int64_t> xoff;
static GlobalAddress<int64_t> xadj;
static GlobalAddress<int64_t> bfs_tree;
static GlobalAddress<int64_t> k2;
static int64_t nadj;

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

static void bfs_visit_neighbor(uint64_t packed, GlobalAddress<LocalTaskJoiner> rjoiner) {
  int64_t vo = packed & 0xFFFFFFFF;
  int64_t v = packed >> 32;
  CHECK(vo < nadj) << "unpacking 'vo' unsuccessful (" << vo << " < " << nadj << ")";
  CHECK(v < nadj) << "unpacking 'v' unsuccessful (" << v << " < " << nadj << ")";  
  
  GRAPPA_EVENT(visit_neighbor_ev, "visit neighbor of vertex", 1, bfs, v);
  
  GlobalAddress<int64_t> xv = xadj+vo;
  CHECK(xv.node() < SoftXMT_nodes()) << " [" << xv.node() << " < " << SoftXMT_nodes() << "]";
  
  const int64_t j = read(xv);
  if (cmp_swap(bfs_tree+j, -1, v)) {
    while (kbuf == -1) { SoftXMT_yield(); }
    if (kbuf < BUF_LEN) {
      buf[kbuf] = j;
      (kbuf)++;
    } else {
      kbuf = -1; // lock other threads out temporarily
      int64_t voff = fetch_add(k2, BUF_LEN);
      Incoherent<int64_t>::RW cvlist(vlist+voff, BUF_LEN);
      for (int64_t vk=0; vk < BUF_LEN; vk++) {
        cvlist[vk] = buf[vk];
      }
      buf[0] = j;
      kbuf = 1;
    }
  }
  LocalTaskJoiner::remoteSignal(rjoiner);
}

static void bfs_visit_vertex(int64_t k, GlobalAddress<LocalTaskJoiner> rjoiner) {
  GlobalAddress<int64_t> vk = vlist+k;
  CHECK(vk.node() < SoftXMT_nodes()) << " [" << vk.node() << " < " << SoftXMT_nodes() << "]";
  const int64_t v = read(vk);
  
  // TODO: do these two together (cache)
  const int64_t vstart = read(XOFF(v));
  const int64_t vend = read(XENDOFF(v));
  CHECK(vstart < nadj) << vstart << " < " << nadj;
  CHECK(vend < nadj) << vend << " < " << nadj;
  CHECK(v < (1L<<32)) << "can't pack 'v' into 32-bit value! have to get more creative";
  CHECK(vend < (1L<<32)) << "can't pack 'vo' into 32-bit value! have to get more creative";
  
  GRAPPA_EVENT(visit_vertex_ev, "visit vertex: num neighbors", 1, bfs, vend-vstart);

  LocalTaskJoiner myjoiner;
  GlobalAddress<LocalTaskJoiner> myjoiner_addr = make_global(&myjoiner);

  for (int64_t vo = vstart; vo < vend; vo++) {
    uint64_t packed = (((uint64_t)v) << 32) | vo;
    myjoiner.registerTask(); // register these new tasks on *this task*'s joiner
    SoftXMT_publicTask(&bfs_visit_neighbor, packed, myjoiner_addr);
  }
  myjoiner.wait();
  LocalTaskJoiner::remoteSignal(rjoiner);
}

LOOP_FUNCTOR(bfs_node, nid, ((int64_t,start)) ((int64_t,end))) {
  range_t r = blockDist(start, end, nid, SoftXMT_nodes());
  
  kbuf = 0;
  
  LocalTaskJoiner nodejoiner;
  GlobalAddress<LocalTaskJoiner> nodejoiner_addr = make_global(&nodejoiner);

  for (int64_t i = r.start; i < r.end; i++) {
    nodejoiner.registerTask();
    SoftXMT_publicTask(&bfs_visit_vertex, i, nodejoiner_addr);
  }
  
  VLOG(1) << "node joiner (" << nodejoiner.outstanding << ") before wait";
  nodejoiner.wait();
  VLOG(1) << "node joiner (" << nodejoiner.outstanding << ")";
  
}

LOOP_FUNCTION(clear_buffers, nid) {
  if (kbuf) {
    int64_t voff = fetch_add(k2, kbuf);
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
  GlobalAddress<int64_t> vlist = SoftXMT_typed_malloc<int64_t>(NV);
  
  double t;
  t = timer();
  
  // start with root as only thing in vlist
  write(vlist, root);
  
  int64_t k1 = 0, k2 = 1;
  GlobalAddress<int64_t> k2addr = make_global(&k2);
  
  // initialize bfs_tree to -1
  SoftXMT_memset(bfs_tree, (int64_t)-1,  NV);
  
  write(bfs_tree+root, root); // parent of root is self
  
  bfs_setup fsetup(vlist, g->xoff, g->xadj, bfs_tree, k2addr, g->nadj);
  fork_join_custom(&fsetup);
  
  while (k1 != k2) {
    VLOG(2) << "k1=" << k1 << ", k2=" << k2;
    const int64_t oldk2 = k2;
    
    bfs_node fbfs(k1, oldk2);
    fork_join_custom(&fbfs);

    { clear_buffers f; fork_join_custom(&f); }

    k1 = oldk2;
  }
  
  t = timer() - t;
  
  SoftXMT_free(vlist);
  
  SoftXMT_merge_and_dump_stats();
  
  return t;
}

