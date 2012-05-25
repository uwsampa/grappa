// Originally from GraphCT, version 0.6.0
// License: GeorgiaTech

#include "defs.hpp"
#include <SoftXMT.hpp>
#include <ForkJoin.hpp>
#include <GlobalAllocator.hpp>
#include <Delegate.hpp>
#include <common.hpp>

#define read      SoftXMT_delegate_read_word
#define write     SoftXMT_delegate_write_word
#define cmp_swap  SoftXMT_delegate_compare_and_swap_word
#define fetch_add SoftXMT_delegate_fetch_and_add_word

static inline double read_double(GlobalAddress<double> addr) {
  int64_t temp = SoftXMT_delegate_read_word(addr);
  return *reinterpret_cast<double*>(&temp);
}
static inline void write_double(GlobalAddress<double> addr, double val) {
  SoftXMT_delegate_write_word(addr, *reinterpret_cast<int64_t*>(&val));
}

struct CentralityScratch {
  GlobalAddress<double> delta;
  GlobalAddress<graphint> dist, Q, sigma, marks, QHead, child, child_count, explored,
                          Qnext;
};

static LocalTaskJoiner joiner;

static graph g;
static CentralityScratch c;
static GlobalAddress<double> bc;
static graphint d_phase;
static graphint local_Qnext;

//static inline void visit_adj(const graphint& v, const graphint& k, const graphint& sigmav, graphint * ccount) {
static void bfs_push_visit_neighbor(uint64_t packed) {
  graphint k = packed & 0xFFFFFFFF;
  graphint v = packed >> 32;
  graphint sigmav = read(c.sigma+v);
  graphint vStart = read(g.edgeStart+v);
//---------------------------
  graphint w = read(g.endVertex+k);
  graphint d = read(c.dist+w);
  
  /* If node has not been visited, set distance and push on Q (but only once) */
  if (d < 0) {
    if (cmp_swap(c.marks+w, 0, 1)) {
      write(c.dist+w, d_phase);
      write(c.Q + fetch_add(c.Qnext, 1), w);
    }
    fetch_add(c.sigma+w, sigmav);
    graphint ccount = fetch_add(c.child_count+v, 1);
    graphint l = vStart + ccount;
    write(c.child+l, w);
  } else if (d == d_phase) {
    fetch_add(c.sigma+w, sigmav);
    graphint ccount = fetch_add(c.child_count+v, 1);
    graphint l = vStart + ccount;
    write(c.child+l, w);
  }
//----------------------------
  joiner.signal();
}

static void bfs_push_visit_vertex(int64_t j) {
  const int64_t v = read(c.Q+j);
  CHECK(v < g.numVertices) << v << " < " << g.numVertices;
  graphint vrange_[2];
  Incoherent<graphint>::RO vrange(g.edgeStart+v, 2, vrange_);
  const graphint vstart = vrange[0];
  const graphint vend = vrange[1];
  CHECK(v < (1L<<32)) << "can't pack 'v' into 32-bit value! have to get more creative";
  CHECK(vend < (1L<<32)) << "can't pack 'vo' into 32-bit value! have to get more creative";
  
  for (int64_t k = vstart; k < vend; k++) {
    uint64_t packed = (((uint64_t)v) << 32) | k;
    joiner.registerTask();
    SoftXMT_privateTask(&bfs_push_visit_neighbor, packed);
  }
  joiner.signal();
}

LOOP_FUNCTOR(bfs_push, nid, ((graphint,d_phase_)) ((int64_t,start)) ((int64_t,end))) {
  d_phase = d_phase_;
  
  range_t r = blockDist(start, end, nid, SoftXMT_nodes());
  
  joiner.reset();
  for (int64_t i = r.start; i < r.end; i++) {
    joiner.registerTask();
    SoftXMT_privateTask(&bfs_push_visit_vertex, i);
  }
  joiner.wait();
}

FUNCTOR(AtomicAddDouble, ((GlobalAddress<double>,r)) ((double,v)) ) {
  CHECK(SoftXMT_mynode() == r.node());
  *r.pointer() += v;
}

static void bfs_pop_visit_vertex(graphint j) {
  graphint v = read(c.Q+j); 
  graphint myStart = read(g.edgeStart+v);
  graphint myEnd   = myStart + read(c.child_count+v);
  double sum = 0;
  double sigma_v = (double)read(c.sigma+v);
  
  for (graphint k = myStart; k < myEnd; k++) {
    graphint w = read(c.child+k);
    sum += sigma_v * (1.0 + read_double(c.delta+w)) / (double)read(c.sigma+w);
  }
  write_double(c.delta+v, sum);

  AtomicAddDouble aad(bc+v, sum);
  SoftXMT_delegate_func(&aad, aad.r.node());
  joiner.signal();
}

LOOP_FUNCTOR(bfs_pop, nid, ((graphint,start)) ((graphint,end)) ) {
  range_t r = blockDist(start, end, nid, SoftXMT_nodes());
  joiner.reset();
  for (int64_t i = r.start; i < r.end; i++) {
    joiner.registerTask();
    SoftXMT_privateTask(&bfs_pop_visit_vertex, i);
  }
  joiner.wait();
}

LOOP_FUNCTOR( initCentrality, nid, ((graph,g_)) ((CentralityScratch,s_)) ((GlobalAddress<double>,bc_)) ) {
  g = g_;
  c = s_;
  bc = bc_;
}

LOOP_FUNCTOR( totalCentrality, v, ((GlobalAddress<double>,total)) ) {
  AtomicAddDouble aad(total, read_double(bc+v));
  SoftXMT_delegate_func(&aad, total.node());
}

double centrality(graph *g, GlobalAddress<double> bc, graphint Vs) {
  graphint Qnext;

  bool computeAllVertices = (Vs == g->numVertices);
  
  c.delta       = SoftXMT_typed_malloc<double>  (g->numVertices);
  c.dist        = SoftXMT_typed_malloc<graphint>(g->numVertices);
  c.Q           = SoftXMT_typed_malloc<graphint>(g->numVertices);
  c.sigma       = SoftXMT_typed_malloc<graphint>(g->numVertices);
  c.marks       = SoftXMT_typed_malloc<graphint>(g->numVertices+2);
  c.QHead       = SoftXMT_typed_malloc<graphint>(100 * SCALE);
  c.child       = SoftXMT_typed_malloc<graphint>(g->numEdges);
  c.child_count = SoftXMT_typed_malloc<graphint>(g->numVertices);
  if (!computeAllVertices) c.explored = SoftXMT_typed_malloc<graphint>(g->numVertices);
  c.Qnext       = make_global(&Qnext);
  
  {
    initCentrality f(*g, c, bc); fork_join_custom(&f);
  }
  
  SoftXMT_memset(bc, 0.0, g->numVertices);
  if (!computeAllVertices) SoftXMT_memset(c.explored, (graphint)0L, g->numVertices);
  
  srand(12345);
  
  graphint nQ, d_phase, Qstart, Qend;
  
  for (graphint x = 0; (x < g->numVertices) && (Vs > 0); x++) {
    /// Choose vertex at random
    graphint s;
    if (computeAllVertices) {
      s = x;
    } else {
      do {
        s = rand() % g->numVertices;
      } while (!cmp_swap(c.explored+s, 0, 1));
    }
    
    graphint pair_[2];
    Incoherent<graphint>::RO pair(g->edgeStart+s, 2, pair_);
    
    if (pair[0] == pair[1]) {
      continue;
    } else {
      Vs--;
    }
    
    VLOG(3) << "s = " << s;
    
    SoftXMT_memset(c.dist, (graphint)-1, g->numVertices);
    SoftXMT_memset(c.sigma, (graphint)0, g->numVertices);
    SoftXMT_memset(c.marks, (graphint)0, g->numVertices);
    SoftXMT_memset(c.child_count, (graphint)0, g->numVertices);
    
    // Push node i onto Q and set bounds for first Q sublist
    write(c.Q+0, s);
    Qnext = 1;
    nQ = 1;
    write(c.QHead+0, 0);
    write(c.QHead+1, 1);
    write(c.dist+s, 0);
    write(c.marks+s, 1);
    write(c.sigma+s, 1);

  PushOnStack: // push nodes onto Q
    
    // Execute the nested loop for each node v on the Q AND
    // for each neighbor w of v whose edge weight is not divisible by 8
    d_phase = nQ;
    Qstart = read(c.QHead+nQ-1);
    Qend = read(c.QHead+nQ);
    {
      bfs_push f(d_phase, Qstart, Qend); fork_join_custom(&f);
    }
    
    // If new nodes pushed onto Q
    if (Qnext != read(c.QHead+nQ)) {
      nQ++;
      write(c.QHead+nQ, Qnext); 
      goto PushOnStack;
    }
    
    // Dependence accumulation phase
    nQ--;
    VLOG(3) << "nQ = " << nQ;
    
//    {
//      std::stringstream ss;
//      ss << "sigma = [";
//      for (graphint i=0; i<g->numVertices; i++) ss << " " << read(c.sigma+i);
//      ss << " ]";
//      VLOG(1) << ss.str();
//    } 
    SoftXMT_memset(c.delta, 0.0, g->numVertices);
    
    // Pop nodes off of Q in the reverse order they were pushed on
    for ( ; nQ > 1; nQ--) {
      Qstart = read(c.QHead+nQ-1);
      Qend = read(c.QHead+nQ);
      
      {
        bfs_pop f(Qstart, Qend); fork_join_custom(&f);
      }
    }
//    {
//      std::stringstream ss;
//      ss << "bc = [";
//      for (graphint i=0; i<g->numVertices; i++) ss << " " << read_double(bc+i);
//      ss << " ]";
//      VLOG(1) << ss.str();
//    }
//    VLOG(1) << "bc[" << 12 << "] = " << read_double(bc+12);
  } // end for(x=0; x<NV && Vs>0)
    
  SoftXMT_free(c.delta);
  SoftXMT_free(c.dist);
  SoftXMT_free(c.Q);
  SoftXMT_free(c.sigma);
  SoftXMT_free(c.marks);
  SoftXMT_free(c.QHead);
  SoftXMT_free(c.child);
  SoftXMT_free(c.child_count);
  SoftXMT_free(c.explored);
  
  double bc_total = 0;
  {
    totalCentrality f(make_global(&bc_total)); fork_join(&f, 0, g->numVertices);
  }
  return bc_total / g->numVertices;
}
