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
  //TAU_PROFILE("bfs_visit_neighbor", "void (uint64_t)", TAU_USER1);

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
  //TAU_PROFILE("bfs_visit_vertex", "void (int64_t)", TAU_USER2);

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
  //VLOG(1) << "myjoiner (outstanding=" << myjoiner.outstanding << ")";
  LocalTaskJoiner::remoteSignal(rjoiner);
}

//LOOP_FUNCTOR(bfs_node, nid, ((int64_t,start)) ((int64_t,end))) {
void bfs_level(Node nid, int64_t start, int64_t end) {
  range_t r = blockDist(start, end, nid, SoftXMT_nodes());
  
  kbuf = 0;
  
  LocalTaskJoiner nodejoiner;
  GlobalAddress<LocalTaskJoiner> nodejoiner_addr = make_global(&nodejoiner);

  for (int64_t i = r.start; i < r.end; i++) {
    nodejoiner.registerTask();
    SoftXMT_publicTask(&bfs_visit_vertex, i, nodejoiner_addr);
  }
  nodejoiner.wait();
}

//LOOP_FUNCTION(clear_buffers, nid) {
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

LOOP_FUNCTOR(func_bfs_onelevel, k,
    (( GlobalAddress<int64_t>,vlist    ))
    (( GlobalAddress<int64_t>,xoff     ))
    (( GlobalAddress<int64_t>,xadj     ))
    (( GlobalAddress<int64_t>,bfs_tree ))
    (( GlobalAddress<int64_t>,k2       ))
    (( int64_t*,kbuf ))
    (( int64_t*,buf  ))
    (( int64_t, nadj )) )
{
  const int64_t v = read(vlist+k);
  
  // TODO: do these two together (cache)
  const int64_t vstart = read(XOFF(v));
  const int64_t vend = read(XENDOFF(v));
  CHECK(vstart < nadj) << vstart << " < " << nadj;
  CHECK(vend < nadj) << vend << " < " << nadj;

  for (int64_t vo = vstart; vo < vend; vo++) {
    const int64_t j = read(xadj+vo); // cadj[vo];
    if (cmp_swap(bfs_tree+j, -1, v)) {
      while (*kbuf == -1) { SoftXMT_yield(); }
      if (*kbuf < BUF_LEN) {
        buf[*kbuf] = j;
        (*kbuf)++;
      } else {
        *kbuf = -1; // lock other threads out temporarily
        int64_t voff = fetch_add(k2, BUF_LEN);
        Incoherent<int64_t>::RW cvlist(vlist+voff, BUF_LEN);
        for (int64_t vk=0; vk < BUF_LEN; vk++) {
          cvlist[vk] = buf[vk];
        }
        buf[0] = j;
        *kbuf = 1;
      }
    }
  }
}

LOOP_FUNCTOR(func_bfs_node, mynode,
  (( GlobalAddress<int64_t>,vlist    ))
  (( GlobalAddress<int64_t>,xoff     ))
  (( GlobalAddress<int64_t>,xadj     ))
  (( GlobalAddress<int64_t>,bfs_tree ))
  (( GlobalAddress<int64_t>,k2       ))
  (( int64_t,start ))
  (( int64_t,end  ))
  (( int64_t,nadj )) )
{
  int64_t kbuf = 0;
  int64_t buf[BUF_LEN];
  
  range_t r = blockDist(start, end, mynode, SoftXMT_nodes());
  
  func_bfs_onelevel f;
  f.vlist = vlist; f.xoff = xoff; f.xadj = xadj; f.bfs_tree = bfs_tree; f.k2 = k2;
  f.kbuf = &kbuf;
  f.buf = buf;
  f.nadj = nadj;
  
  fork_join_onenode(&f, r.start, r.end);
  
  // make sure to commit what's left in the buffer at the end
  if (kbuf) {
    int64_t voff = fetch_add(k2, kbuf);
    Incoherent<int64_t>::RW cvlist(vlist+voff, kbuf);
    for (int64_t vk=0; vk < kbuf; vk++) {
      cvlist[vk] = buf[vk];
    }
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
    
    //char phaseName[64];
    //sprintf(phaseName, "Level %lld -> %lld\n", k1, k2);
    //TAU_PHASE_CREATE_DYNAMIC(bfs_level, phaseName, "", TAU_USER);
    //TAU_PHASE_START(bfs_level);

    bfs_level(SoftXMT_mynode(), k1, oldk2);

    SoftXMT_barrier_suspending();

    clear_buffers();

    SoftXMT_barrier_suspending();
    //TAU_PHASE_STOP(bfs_level);
    
    k1 = oldk2;
    _k2 = read(k2);
  }  
}

double make_bfs_tree(csr_graph * g, GlobalAddress<int64_t> bfs_tree, int64_t root) {
  //TAU_PHASE("make_bfs_tree", "double (csr_graph*,GlobalAddress<int64_t>,int64_t)", TAU_DEFAULT);

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
  //SoftXMT_dump_stats_all_nodes();
  SoftXMT_merge_and_dump_stats();
  
  return t;
}

