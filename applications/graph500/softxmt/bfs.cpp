#include "common.h"
#include "SoftXMT.hpp"
#include "ForkJoin.hpp"
#include "Cache.hpp"
#include "Delegate.hpp"
#include "PerformanceTools.hpp"
#include "GlobalAllocator.hpp"
#include "timer.h"
#include "GlobalTaskJoiner.hpp"

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

static void bfs_visit_neighbor(uint64_t packed, GlobalAddress<GlobalTaskJoiner> rjoiner) {
  int64_t vo = packed & 0xFFFFFFFF;
  int64_t v = packed >> 32;
#ifdef DEBUG
  CHECK(vo < nadj) << "unpacking 'vo' unsuccessful (" << vo << " < " << nadj << ")";
  CHECK(v < nadj) << "unpacking 'v' unsuccessful (" << v << " < " << nadj << ")";  
  GRAPPA_EVENT(visit_neighbor_ev, "visit neighbor of vertex", 1, bfs, v);
#endif
  
  const int64_t j = read(xadj+vo);
  
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
  GlobalTaskJoiner::remoteSignal(rjoiner);
}

static void bfs_visit_vertex(int64_t k, GlobalAddress<GlobalTaskJoiner> rjoiner) {
  const int64_t v = read(vlist+k);
  
  int64_t buf[2];
  Incoherent<int64_t>::RO cr(xoff+2*v, 2, buf);
  const int64_t vstart = cr[0], vend = cr[1];
  
  for (int64_t vo = vstart; vo < vend; vo++) {
    uint64_t packed = (((uint64_t)v) << 32) | vo;
    global_joiner.registerTask(); // register these new tasks on *this task*'s joiner
    SoftXMT_publicTask(&bfs_visit_neighbor, packed, global_joiner.addr());
  }
  GlobalTaskJoiner::remoteSignal(rjoiner);
}

void bfs_level(Node nid, int64_t start, int64_t end) {
  range_t r = blockDist(start, end, nid, SoftXMT_nodes());

  kbuf = 0;
  
  global_joiner.reset();

  for (int64_t i = r.start; i < r.end; i++) {
    global_joiner.registerTask();
    SoftXMT_publicTask(&bfs_visit_vertex, i, global_joiner.addr());
  }
  global_joiner.wait();
}

void clear_buffers() {
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

LOOP_FUNCTOR(bfs_node, nid, GA64(_vlist)GA64(_xoff)GA64(_xadj)GA64(_bfs_tree)GA64(_k2)((int64_t,_nadj))) {
  // setup globals
  kbuf = 0;
  vlist = _vlist;
  xoff = _xoff;
  xadj = _xadj;
  bfs_tree = _bfs_tree;
  k2 = _k2;
  nadj = _nadj;

  int64_t k1 = 0, _k2 = 1;
  
  while (k1 != _k2) {
    VLOG(2) << "k1=" << k1 << ", k2=" << _k2;
    const int64_t oldk2 = _k2;
    
    bfs_level(SoftXMT_mynode(), k1, oldk2);

    SoftXMT_barrier_suspending();

    clear_buffers();

    SoftXMT_barrier_suspending();
    
    k1 = oldk2;
    _k2 = read(k2);
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
  
  { bfs_node f(vlist, g->xoff, g->xadj, bfs_tree, k2addr, g->nadj); fork_join_custom(&f); }
    
  t = timer() - t;
  
  SoftXMT_free(vlist);
  
  SoftXMT_merge_and_dump_stats();
  
  return t;
}

